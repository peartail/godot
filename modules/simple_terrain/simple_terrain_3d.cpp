/**************************************************************************/
/*  simple_terrain_3d.cpp                                                        */
/**************************************************************************/

#include "simple_terrain_3d.h"

#include "core/math/geometry_3d.h"
#include "core/math/random_pcg.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/resources/3d/world_3d.h"
#include "scene/resources/mesh.h"
#include "scene/resources/shader.h"
#include "servers/rendering/rendering_server.h"

void SimpleTerrain3D::_simple_terrain_data_changed() {
	if (syncing_data) {
		return;
	}
	rebuild_mesh();
}

void SimpleTerrain3D::_ensure_data() {
	if (simple_terrain_data.is_valid()) {
		simple_terrain_data->ensure_height_data_size(false);
		return;
	}

	// Lazily create terrain data after the node enters the world or is edited.
	// Creating default resources in the constructor makes Godot's default
	// property probing instantiate them, which is noisy for Mono/editor users.
	simple_terrain_data.instantiate();
	simple_terrain_data->fill_flat(flat_height);
	simple_terrain_data->connect_changed(callable_mp(this, &SimpleTerrain3D::_simple_terrain_data_changed));
}

Vector3 SimpleTerrain3D::_get_vertex_position(int p_x, int p_z) const {
	ERR_FAIL_COND_V(simple_terrain_data.is_null(), Vector3());
	const real_t half_size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size() * 0.5;
	// SimpleTerrainData stores only height values; X/Z are derived from grid indices.
	// Keeping this conversion centralized avoids subtle mismatches between mesh
	// building, picking, debug gizmos, and bounds calculation.
	return Vector3(
			(real_t)p_x * simple_terrain_data->get_cell_size() - half_size,
			simple_terrain_data->get_height(p_x, p_z),
			(real_t)p_z * simple_terrain_data->get_cell_size() - half_size);
}

real_t SimpleTerrain3D::_sample_nearest_height(real_t p_center_x, real_t p_center_z) const {
	// Flatten uses the closest existing vertex height as its target. This makes
	// the operation predictable when painting on slopes because it preserves the
	// clicked height instead of using a global flat_height setting.
	const int x = CLAMP(Math::round(p_center_x), 0, simple_terrain_data->get_vertex_count() - 1);
	const int z = CLAMP(Math::round(p_center_z), 0, simple_terrain_data->get_vertex_count() - 1);
	return simple_terrain_data->get_height(x, z);
}

real_t SimpleTerrain3D::_get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_z) const {
	// Smooth reads from a snapshot taken before the brush pass. Without this,
	// earlier edits in the same brush step would bias later vertices and create
	// directional smearing.
	const int vertex_count = simple_terrain_data->get_vertex_count();
	real_t total = 0.0;
	int count = 0;
	for (int z = MAX(0, p_z - 1); z <= MIN(vertex_count - 1, p_z + 1); z++) {
		for (int x = MAX(0, p_x - 1); x <= MIN(vertex_count - 1, p_x + 1); x++) {
			total += p_source_heights[simple_terrain_data->get_height_index(x, z)];
			count++;
		}
	}
	return total / (real_t)count;
}

real_t SimpleTerrain3D::_sample_value_noise(real_t p_x, real_t p_z, int p_seed) const {
	// Lightweight deterministic value noise used only for the "Random" button.
	// The initial terrain stays flat; random terrain is an explicit action.
	const real_t value = Math::sin(p_x * 12.9898 + p_z * 78.233 + (real_t)p_seed * 37.719) * 43758.5453;
	return Math::fposmod(value, (real_t)1.0) * 2.0 - 1.0;
}

Ref<ArrayMesh> SimpleTerrain3D::_build_chunk_mesh(int p_origin_x, int p_origin_z, int p_quad_width, int p_quad_depth) const {
	ERR_FAIL_COND_V(simple_terrain_data.is_null(), Ref<ArrayMesh>());

	const int grid_size = simple_terrain_data->get_grid_size();
	const int vertex_width = p_quad_width + 1;
	const int vertex_depth = p_quad_depth + 1;
	const int vertex_total = vertex_width * vertex_depth;

	Vector<Vector3> vertices;
	Vector<Vector3> normals;
	Vector<Vector2> uvs;
	Vector<int> indices;
	Vector<Vector3> normal_accumulator;
	vertices.resize(vertex_total);
	normals.resize(vertex_total);
	uvs.resize(vertex_total);
	normal_accumulator.resize(vertex_total);

	// Chunks intentionally share edge vertices by sampling the same SimpleTerrainData
	// coordinates. This prevents cracks; normals are local to each chunk for now
	// and may be smoothed across chunk borders in a later pass.
	Vector3 *vertices_w = vertices.ptrw();
	Vector2 *uvs_w = uvs.ptrw();
	Vector3 *normal_accumulator_w = normal_accumulator.ptrw();

	const real_t half_size = (real_t)grid_size * simple_terrain_data->get_cell_size() * 0.5;
	for (int local_z = 0; local_z < vertex_depth; local_z++) {
		for (int local_x = 0; local_x < vertex_width; local_x++) {
			const int terrain_x = p_origin_x + local_x;
			const int terrain_z = p_origin_z + local_z;
			const int local_index = local_z * vertex_width + local_x;
			vertices_w[local_index] = Vector3(
					(real_t)terrain_x * simple_terrain_data->get_cell_size() - half_size,
					simple_terrain_data->get_height(terrain_x, terrain_z),
					(real_t)terrain_z * simple_terrain_data->get_cell_size() - half_size);
			uvs_w[local_index] = Vector2((real_t)terrain_x / (real_t)grid_size, (real_t)terrain_z / (real_t)grid_size);
			normal_accumulator_w[local_index] = Vector3();
		}
	}

	auto get_local_index = [&](int p_x, int p_z) {
		return p_z * vertex_width + p_x;
	};

	auto add_triangle = [&](int p_a, int p_b, int p_c) {
		indices.push_back(p_a);
		indices.push_back(p_b);
		indices.push_back(p_c);
		// Accumulate face normals first and normalize once after all triangles are
		// emitted. This produces smooth vertex normals within each chunk without
		// requiring a separate SurfaceTool dependency.
		const Vector3 normal = (vertices[p_b] - vertices[p_a]).cross(vertices[p_c] - vertices[p_a]);
		normal_accumulator_w[p_a] += normal;
		normal_accumulator_w[p_b] += normal;
		normal_accumulator_w[p_c] += normal;
	};

	for (int z = 0; z < p_quad_depth; z++) {
		for (int x = 0; x < p_quad_width; x++) {
			const int top_left = get_local_index(x, z);
			const int top_right = get_local_index(x + 1, z);
			const int bottom_left = get_local_index(x, z + 1);
			const int bottom_right = get_local_index(x + 1, z + 1);
			add_triangle(top_left, top_right, bottom_left);
			add_triangle(top_right, bottom_right, bottom_left);
		}
	}

	Vector3 *normals_w = normals.ptrw();
	for (int i = 0; i < normals.size(); i++) {
		normals_w[i] = normal_accumulator[i].is_zero_approx() ? Vector3(0, 1, 0) : normal_accumulator[i].normalized();
		if (normals_w[i].y < 0.0) {
			normals_w[i] = -normals_w[i];
		}
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_INDEX] = indices;

	Ref<ArrayMesh> array_mesh;
	array_mesh.instantiate();
	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return array_mesh;
}

void SimpleTerrain3D::_clear_chunks() {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (TerrainChunk &chunk : chunks) {
		if (chunk.instance.is_valid()) {
			// Internal instances are not scene nodes, so SimpleTerrain3D owns their RID
			// lifetime explicitly.
			rs->free_rid(chunk.instance);
		}
	}
	chunks.clear();
}

void SimpleTerrain3D::_sync_chunk_instances() {
	RenderingServer *rs = RenderingServer::get_singleton();
	const RID scenario = is_inside_tree() && get_world_3d().is_valid() ? get_world_3d()->get_scenario() : RID();
	const Transform3D global_transform = get_global_transform();
	for (TerrainChunk &chunk : chunks) {
		if (!chunk.instance.is_valid()) {
			chunk.instance = rs->instance_create();
		}
		// Keep every chunk instance in the node's world/scenario. The chunk
		// mesh vertices are already in SimpleTerrain3D local space, so all chunks use
		// the same node transform.
		rs->instance_set_base(chunk.instance, chunk.mesh.is_valid() ? chunk.mesh->get_rid() : RID());
		rs->instance_set_scenario(chunk.instance, scenario);
		rs->instance_set_transform(chunk.instance, global_transform);
	}
	_sync_chunk_materials();
}

void SimpleTerrain3D::_sync_chunk_materials() {
	RenderingServer *rs = RenderingServer::get_singleton();
	const Ref<Material> material = _get_active_chunk_material();
	const RID material_rid = material.is_valid() ? material->get_rid() : RID();
	// Material changes are cheap compared to mesh rebuilds. Applying the same
	// override to every chunk keeps the material path independent from chunk
	// generation and lets users swap materials at runtime.
	for (TerrainChunk &chunk : chunks) {
		if (chunk.instance.is_valid()) {
			rs->instance_geometry_set_material_override(chunk.instance, material_rid);
		}
	}
}

void SimpleTerrain3D::_rebuild_chunks_for_region(int p_min_x, int p_min_z, int p_max_x, int p_max_z) {
	if (simple_terrain_data.is_null()) {
		return;
	}

	// Convert the edited vertex rectangle to chunk coordinates. The caller pads
	// the rectangle by one vertex so neighboring triangles that depend on edge
	// vertices are also rebuilt.
	const int min_chunk_x = CLAMP(p_min_x / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);
	const int max_chunk_x = CLAMP(p_max_x / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);
	const int min_chunk_z = CLAMP(p_min_z / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);
	const int max_chunk_z = CLAMP(p_max_z / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);

	RenderingServer *rs = RenderingServer::get_singleton();
	for (TerrainChunk &chunk : chunks) {
		const int chunk_x = chunk.origin_x / chunk_size;
		const int chunk_z = chunk.origin_z / chunk_size;
		if (chunk_x < min_chunk_x || chunk_x > max_chunk_x || chunk_z < min_chunk_z || chunk_z > max_chunk_z) {
			continue;
		}
		chunk.mesh = _build_chunk_mesh(chunk.origin_x, chunk.origin_z, chunk.quad_width, chunk.quad_depth);
		if (chunk.instance.is_valid()) {
			rs->instance_set_base(chunk.instance, chunk.mesh->get_rid());
		}
	}
	if (show_chunk_gizmos) {
		update_gizmos();
	}
}

Ref<Material> SimpleTerrain3D::_get_active_chunk_material() {
	// Explicit user material wins because it is the least surprising behavior:
	// assigning terrain_material should fully override built-in shader choices.
	if (terrain_material.is_valid()) {
		return terrain_material;
	}
	if (use_builtin_triplanar_material) {
		_update_builtin_triplanar_material();
		return builtin_triplanar_material;
	}
	return get_material_override();
}

void SimpleTerrain3D::_update_builtin_triplanar_material() {
	if (builtin_triplanar_shader.is_null()) {
		// The shader is generated lazily so projects that use a custom material do
		// not pay the cost or carry shader state they never use.
		builtin_triplanar_shader.instantiate();
		builtin_triplanar_shader->set_code(_get_builtin_triplanar_shader_code());
	}
	if (builtin_triplanar_material.is_null()) {
		builtin_triplanar_material.instantiate();
		builtin_triplanar_material->set_shader(builtin_triplanar_shader);
	}

	builtin_triplanar_material->set_shader_parameter("low_albedo", triplanar_low_texture);
	builtin_triplanar_material->set_shader_parameter("mid_albedo", triplanar_mid_texture);
	builtin_triplanar_material->set_shader_parameter("high_albedo", triplanar_high_texture);
	builtin_triplanar_material->set_shader_parameter("low_color", triplanar_low_color);
	builtin_triplanar_material->set_shader_parameter("mid_color", triplanar_mid_color);
	builtin_triplanar_material->set_shader_parameter("high_color", triplanar_high_color);
	builtin_triplanar_material->set_shader_parameter("low_height", triplanar_low_height);
	builtin_triplanar_material->set_shader_parameter("high_height", triplanar_high_height);
	builtin_triplanar_material->set_shader_parameter("blend_width", triplanar_blend_width);
	builtin_triplanar_material->set_shader_parameter("texture_scale", triplanar_texture_scale);
	builtin_triplanar_material->set_shader_parameter("triplanar_sharpness", triplanar_blend_sharpness);
}

String SimpleTerrain3D::_get_builtin_triplanar_shader_code() {
	// The shader blends three albedo layers by world-space height and samples
	// them through triplanar projection. World-space projection avoids stretched
	// UVs on steep slopes and keeps texture scale stable across chunk boundaries.
	return R"(
shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D low_albedo : source_color, hint_default_white;
uniform sampler2D mid_albedo : source_color, hint_default_white;
uniform sampler2D high_albedo : source_color, hint_default_white;
uniform vec4 low_color : source_color = vec4(0.45, 0.34, 0.22, 1.0);
uniform vec4 mid_color : source_color = vec4(0.26, 0.45, 0.18, 1.0);
uniform vec4 high_color : source_color = vec4(0.55, 0.55, 0.52, 1.0);
uniform float low_height = 1.5;
uniform float high_height = 8.0;
uniform float blend_width = 2.0;
uniform float texture_scale = 0.08;
uniform float triplanar_sharpness = 4.0;

varying vec3 terrain_world_pos;
varying vec3 terrain_world_normal;

void vertex() {
	terrain_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	terrain_world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
}

vec4 triplanar_sample(sampler2D tex, vec3 world_pos, vec3 world_normal) {
	vec3 blend = pow(abs(world_normal), vec3(max(triplanar_sharpness, 0.001)));
	blend /= max(dot(blend, vec3(1.0)), 0.001);
	vec3 coord = world_pos * texture_scale;
	vec4 x_sample = texture(tex, coord.yz);
	vec4 y_sample = texture(tex, coord.xz);
	vec4 z_sample = texture(tex, coord.xy);
	return x_sample * blend.x + y_sample * blend.y + z_sample * blend.z;
}

void fragment() {
	float safe_blend = max(blend_width, 0.001);
	float low_to_mid = smoothstep(low_height - safe_blend, low_height + safe_blend, terrain_world_pos.y);
	float mid_to_high = smoothstep(high_height - safe_blend, high_height + safe_blend, terrain_world_pos.y);
	float low_weight = 1.0 - low_to_mid;
	float mid_weight = low_to_mid * (1.0 - mid_to_high);
	float high_weight = mid_to_high;
	float weight_sum = max(low_weight + mid_weight + high_weight, 0.001);
	low_weight /= weight_sum;
	mid_weight /= weight_sum;
	high_weight /= weight_sum;

	vec4 low_layer = triplanar_sample(low_albedo, terrain_world_pos, terrain_world_normal) * low_color;
	vec4 mid_layer = triplanar_sample(mid_albedo, terrain_world_pos, terrain_world_normal) * mid_color;
	vec4 high_layer = triplanar_sample(high_albedo, terrain_world_pos, terrain_world_normal) * high_color;
	vec4 albedo = low_layer * low_weight + mid_layer * mid_weight + high_layer * high_weight;

	ALBEDO = albedo.rgb;
	ROUGHNESS = 0.9;
}
)";
}

void SimpleTerrain3D::set_simple_terrain_data(const Ref<SimpleTerrainData> &p_simple_terrain_data) {
	if (simple_terrain_data == p_simple_terrain_data) {
		return;
	}
	if (simple_terrain_data.is_valid()) {
		// Disconnect from the previous resource so edits to an old SimpleTerrainData do
		// not continue rebuilding this node.
		simple_terrain_data->disconnect_changed(callable_mp(this, &SimpleTerrain3D::_simple_terrain_data_changed));
	}
	simple_terrain_data = p_simple_terrain_data;
	if (simple_terrain_data.is_valid()) {
		simple_terrain_data->ensure_height_data_size(false);
		simple_terrain_data->connect_changed(callable_mp(this, &SimpleTerrain3D::_simple_terrain_data_changed));
	}
	rebuild_mesh();
}

void SimpleTerrain3D::set_world_placement_library(const Ref<SimpleWorldPlacementLibrary> &p_library) {
	if (world_placement_library == p_library) {
		return;
	}
	world_placement_library = p_library;
	notify_property_list_changed();
}

void SimpleTerrain3D::set_world_placement_data(const Ref<SimpleWorldPlacementData> &p_data) {
	if (world_placement_data == p_data) {
		return;
	}
	world_placement_data = p_data;
	notify_property_list_changed();
}

void SimpleTerrain3D::set_grid_size(int p_grid_size) {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->set_grid_size(p_grid_size);
	syncing_data = false;
	rebuild_mesh();
}

int SimpleTerrain3D::get_grid_size() const {
	return simple_terrain_data.is_valid() ? simple_terrain_data->get_grid_size() : 64;
}

void SimpleTerrain3D::set_cell_size(real_t p_cell_size) {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->set_cell_size(p_cell_size);
	syncing_data = false;
	rebuild_mesh();
}

real_t SimpleTerrain3D::get_cell_size() const {
	return simple_terrain_data.is_valid() ? simple_terrain_data->get_cell_size() : 1.0;
}

void SimpleTerrain3D::set_chunk_size(int p_chunk_size) {
	const int new_chunk_size = MAX(1, p_chunk_size);
	if (chunk_size == new_chunk_size) {
		return;
	}
	chunk_size = new_chunk_size;
	rebuild_mesh();
}

void SimpleTerrain3D::set_show_chunk_gizmos(bool p_show) {
	if (show_chunk_gizmos == p_show) {
		return;
	}
	show_chunk_gizmos = p_show;
	update_gizmos();
}

void SimpleTerrain3D::set_height_data(const PackedFloat32Array &p_height_data) {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->set_height_data(p_height_data);
	syncing_data = false;
	rebuild_mesh();
}

PackedFloat32Array SimpleTerrain3D::get_height_data() const {
	return simple_terrain_data.is_valid() ? simple_terrain_data->get_height_data() : PackedFloat32Array();
}

void SimpleTerrain3D::set_terrain_material(const Ref<Material> &p_material) {
	if (terrain_material == p_material) {
		return;
	}
	terrain_material = p_material;
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_use_builtin_triplanar_material(bool p_use) {
	if (use_builtin_triplanar_material == p_use) {
		return;
	}
	use_builtin_triplanar_material = p_use;
	if (use_builtin_triplanar_material) {
		_update_builtin_triplanar_material();
	}
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_low_texture(const Ref<Texture2D> &p_texture) {
	triplanar_low_texture = p_texture;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_mid_texture(const Ref<Texture2D> &p_texture) {
	triplanar_mid_texture = p_texture;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_high_texture(const Ref<Texture2D> &p_texture) {
	triplanar_high_texture = p_texture;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_low_color(const Color &p_color) {
	triplanar_low_color = p_color;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_mid_color(const Color &p_color) {
	triplanar_mid_color = p_color;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_high_color(const Color &p_color) {
	triplanar_high_color = p_color;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_low_height(real_t p_height) {
	triplanar_low_height = p_height;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_high_height(real_t p_height) {
	triplanar_high_height = p_height;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_blend_width(real_t p_width) {
	triplanar_blend_width = MAX((real_t)0.001, p_width);
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_texture_scale(real_t p_scale) {
	triplanar_texture_scale = MAX((real_t)0.0001, p_scale);
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_blend_sharpness(real_t p_sharpness) {
	triplanar_blend_sharpness = MAX((real_t)0.001, p_sharpness);
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_flat_height(real_t p_flat_height) {
	flat_height = p_flat_height;
}

void SimpleTerrain3D::set_random_height_scale(real_t p_random_height_scale) {
	random_height_scale = p_random_height_scale;
}

void SimpleTerrain3D::set_random_frequency(real_t p_random_frequency) {
	random_frequency = MAX((real_t)0.0001, p_random_frequency);
}

void SimpleTerrain3D::set_random_octaves(int p_random_octaves) {
	random_octaves = CLAMP(p_random_octaves, 1, 12);
}

void SimpleTerrain3D::set_random_seed(int p_random_seed) {
	random_seed = p_random_seed;
}

void SimpleTerrain3D::reset_flat_terrain() {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->fill_flat(flat_height);
	syncing_data = false;
	rebuild_mesh();
}

void SimpleTerrain3D::generate_random_terrain() {
	_ensure_data();
	const int vertex_count = simple_terrain_data->get_vertex_count();
	PackedFloat32Array heights;
	heights.resize(vertex_count * vertex_count);

	// This is intentionally simple deterministic fractal value noise. It is a
	// quick generator for editor iteration, not a full terrain synthesis system.
	for (int z = 0; z < vertex_count; z++) {
		for (int x = 0; x < vertex_count; x++) {
			real_t amplitude = 1.0;
			real_t frequency = random_frequency;
			real_t value = 0.0;
			real_t amplitude_sum = 0.0;
			for (int octave = 0; octave < random_octaves; octave++) {
				value += _sample_value_noise((real_t)x * frequency, (real_t)z * frequency, random_seed + octave * 131) * amplitude;
				amplitude_sum += amplitude;
				amplitude *= 0.5;
				frequency *= 2.0;
			}
			heights.set(simple_terrain_data->get_height_index(x, z), (value / MAX((real_t)0.0001, amplitude_sum)) * random_height_scale);
		}
	}

	set_height_data(heights);
}

void SimpleTerrain3D::randomize_seed() {
	RandomPCG rng;
	rng.randomize();
	random_seed = (int)rng.rand();
	generate_random_terrain();
}

void SimpleTerrain3D::rebuild_mesh() {
	_ensure_data();
	const int grid_size = simple_terrain_data->get_grid_size();
	_clear_chunks();
	set_mesh(Ref<Mesh>());

	// Build one mesh per chunk. The inherited MeshInstance3D mesh is kept empty
	// because RenderingServer instances below provide the actual renderables.
	for (int origin_z = 0; origin_z < grid_size; origin_z += chunk_size) {
		for (int origin_x = 0; origin_x < grid_size; origin_x += chunk_size) {
			TerrainChunk chunk;
			chunk.origin_x = origin_x;
			chunk.origin_z = origin_z;
			chunk.quad_width = MIN(chunk_size, grid_size - origin_x);
			chunk.quad_depth = MIN(chunk_size, grid_size - origin_z);
			chunk.mesh = _build_chunk_mesh(origin_x, origin_z, chunk.quad_width, chunk.quad_depth);
			chunks.push_back(chunk);
		}
	}

	_sync_chunk_instances();
	notify_property_list_changed();
	update_gizmos();
}

void SimpleTerrain3D::apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	apply_brush_with_delta(p_world_position, p_radius, p_strength, p_operation);
}

Dictionary SimpleTerrain3D::apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	Dictionary delta;
	_ensure_data();
	if (p_radius <= 0.0 || p_strength == 0.0) {
		return delta;
	}

	PackedFloat32Array &heights = simple_terrain_data->get_mutable_height_data();
	const PackedFloat32Array original_heights = p_operation == BRUSH_SMOOTH ? simple_terrain_data->get_height_data() : PackedFloat32Array();
	float *heights_w = heights.ptrw();
	PackedInt32Array changed_indices;
	PackedFloat32Array before_values;
	PackedFloat32Array after_values;
	const Vector3 local_position = get_global_transform().affine_inverse().xform(p_world_position);
	const int vertex_count = simple_terrain_data->get_vertex_count();
	const real_t half_size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size() * 0.5;
	const real_t center_x = (local_position.x + half_size) / simple_terrain_data->get_cell_size();
	const real_t center_z = (local_position.z + half_size) / simple_terrain_data->get_cell_size();
	const real_t radius_cells = p_radius / simple_terrain_data->get_cell_size();
	const int min_x = CLAMP(Math::floor(center_x - radius_cells), 0, vertex_count - 1);
	const int max_x = CLAMP(Math::ceil(center_x + radius_cells), 0, vertex_count - 1);
	const int min_z = CLAMP(Math::floor(center_z - radius_cells), 0, vertex_count - 1);
	const int max_z = CLAMP(Math::ceil(center_z + radius_cells), 0, vertex_count - 1);
	const real_t flatten_height = _sample_nearest_height(center_x, center_z);
	bool changed = false;

	// Brush radius is evaluated in world units but converted to heightmap cells.
	// This keeps painting behavior stable even when cell_size changes.
	for (int z = min_z; z <= max_z; z++) {
		for (int x = min_x; x <= max_x; x++) {
			const real_t dx = ((real_t)x - center_x) * simple_terrain_data->get_cell_size();
			const real_t dz = ((real_t)z - center_z) * simple_terrain_data->get_cell_size();
			const real_t distance = Vector2(dx, dz).length();
			if (distance > p_radius) {
				continue;
			}

			const real_t falloff = 1.0 - distance / p_radius;
			const int index = simple_terrain_data->get_height_index(x, z);
			const real_t before = heights_w[index];
			real_t after = before;
			switch (p_operation) {
				case BRUSH_RAISE:
					after = before + p_strength * falloff;
					break;
				case BRUSH_LOWER:
					after = before - p_strength * falloff;
					break;
				case BRUSH_SMOOTH:
					after = Math::lerp(before, _get_average_neighbor_height(original_heights, x, z), CLAMP(p_strength * falloff, (real_t)0.0, (real_t)1.0));
					break;
				case BRUSH_FLATTEN:
					after = Math::lerp(before, flatten_height, CLAMP(p_strength * falloff, (real_t)0.0, (real_t)1.0));
					break;
			}
			if (!Math::is_equal_approx(before, after)) {
				heights_w[index] = after;
				changed_indices.push_back(index);
				before_values.push_back(before);
				after_values.push_back(after);
				changed = true;
			}
		}
	}

	if (changed) {
		syncing_data = true;
		simple_terrain_data->notify_height_data_changed();
		syncing_data = false;
		_rebuild_chunks_for_region(MAX(0, min_x - 1), MAX(0, min_z - 1), MIN(vertex_count - 1, max_x + 1), MIN(vertex_count - 1, max_z + 1));
	}
	delta["indices"] = changed_indices;
	delta["before"] = before_values;
	delta["after"] = after_values;
	return delta;
}

void SimpleTerrain3D::apply_height_patch(const PackedInt32Array &p_indices, const PackedFloat32Array &p_heights) {
	_ensure_data();
	ERR_FAIL_COND(p_indices.size() != p_heights.size());
	if (p_indices.is_empty()) {
		return;
	}

	PackedFloat32Array &heights = simple_terrain_data->get_mutable_height_data();
	float *heights_w = heights.ptrw();
	const int vertex_count = simple_terrain_data->get_vertex_count();
	int min_x = vertex_count - 1;
	int min_z = vertex_count - 1;
	int max_x = 0;
	int max_z = 0;
	bool changed = false;

	for (int i = 0; i < p_indices.size(); i++) {
		const int index = p_indices[i];
		ERR_CONTINUE(index < 0 || index >= heights.size());
		const real_t height = p_heights[i];
		if (Math::is_equal_approx(heights_w[index], height)) {
			continue;
		}
		heights_w[index] = height;
		const int x = index % vertex_count;
		const int z = index / vertex_count;
		min_x = MIN(min_x, x);
		min_z = MIN(min_z, z);
		max_x = MAX(max_x, x);
		max_z = MAX(max_z, z);
		changed = true;
	}

	if (changed) {
		syncing_data = true;
		simple_terrain_data->notify_height_data_changed();
		syncing_data = false;
		_rebuild_chunks_for_region(MAX(0, min_x - 1), MAX(0, min_z - 1), MIN(vertex_count - 1, max_x + 1), MIN(vertex_count - 1, max_z + 1));
	}
}

Dictionary SimpleTerrain3D::get_brush_hit(const Vector3 &p_ray_origin, const Vector3 &p_ray_direction) const {
	Dictionary result;
	if (simple_terrain_data.is_null() || p_ray_direction.is_zero_approx()) {
		return result;
	}

	// Ray picking walks only the XZ cells crossed by the ray using a 2D DDA.
	// This is much cheaper than checking every triangle in the full terrain and
	// keeps brush interaction responsive as grid_size grows.
	const Transform3D inverse_transform = get_global_transform().affine_inverse();
	const Vector3 local_origin = inverse_transform.xform(p_ray_origin);
	const Vector3 local_direction = inverse_transform.basis.xform(p_ray_direction).normalized();
	const real_t half_size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size() * 0.5;
	const real_t min_bound = -half_size;
	const real_t max_bound = half_size;
	real_t entry_t = 0.0;
	real_t exit_t = Math::INF;

	auto clip_axis = [&](real_t p_origin, real_t p_direction) {
		if (Math::abs(p_direction) < CMP_EPSILON) {
			return p_origin >= min_bound && p_origin <= max_bound;
		}
		real_t t0 = (min_bound - p_origin) / p_direction;
		real_t t1 = (max_bound - p_origin) / p_direction;
		if (t0 > t1) {
			SWAP(t0, t1);
		}
		entry_t = MAX(entry_t, t0);
		exit_t = MIN(exit_t, t1);
		return entry_t <= exit_t;
	};

	if (!clip_axis(local_origin.x, local_direction.x) || !clip_axis(local_origin.z, local_direction.z)) {
		return result;
	}

	const int grid_size = simple_terrain_data->get_grid_size();
	const real_t cell_size = simple_terrain_data->get_cell_size();
	const Vector3 entry_position = local_origin + local_direction * MAX(entry_t, (real_t)0.0);
	int cell_x = CLAMP(Math::floor((entry_position.x + half_size) / cell_size), 0, grid_size - 1);
	int cell_z = CLAMP(Math::floor((entry_position.z + half_size) / cell_size), 0, grid_size - 1);
	const int end_x = CLAMP(Math::floor(((local_origin + local_direction * exit_t).x + half_size) / cell_size), 0, grid_size - 1);
	const int end_z = CLAMP(Math::floor(((local_origin + local_direction * exit_t).z + half_size) / cell_size), 0, grid_size - 1);

	const int step_x = local_direction.x > CMP_EPSILON ? 1 : (local_direction.x < -CMP_EPSILON ? -1 : 0);
	const int step_z = local_direction.z > CMP_EPSILON ? 1 : (local_direction.z < -CMP_EPSILON ? -1 : 0);
	const real_t next_boundary_x = min_bound + (real_t)(cell_x + (step_x > 0 ? 1 : 0)) * cell_size;
	const real_t next_boundary_z = min_bound + (real_t)(cell_z + (step_z > 0 ? 1 : 0)) * cell_size;
	real_t next_t_x = step_x == 0 ? Math::INF : (next_boundary_x - local_origin.x) / local_direction.x;
	real_t next_t_z = step_z == 0 ? Math::INF : (next_boundary_z - local_origin.z) / local_direction.z;
	const real_t delta_t_x = step_x == 0 ? Math::INF : cell_size / Math::abs(local_direction.x);
	const real_t delta_t_z = step_z == 0 ? Math::INF : cell_size / Math::abs(local_direction.z);
	real_t closest_distance = Math::INF;
	Vector3 closest_position;
	bool found = false;

	auto test_cell = [&](int p_x, int p_z) {
		const Vector3 top_left = _get_vertex_position(p_x, p_z);
		const Vector3 top_right = _get_vertex_position(p_x + 1, p_z);
		const Vector3 bottom_left = _get_vertex_position(p_x, p_z + 1);
		const Vector3 bottom_right = _get_vertex_position(p_x + 1, p_z + 1);
		Vector3 hit;
		if (Geometry3D::ray_intersects_triangle(local_origin, local_direction, top_left, top_right, bottom_left, &hit)) {
			const real_t distance = local_origin.distance_to(hit);
			if (distance < closest_distance) {
				closest_distance = distance;
				closest_position = hit;
				found = true;
			}
		}
		if (Geometry3D::ray_intersects_triangle(local_origin, local_direction, top_right, bottom_right, bottom_left, &hit)) {
			const real_t distance = local_origin.distance_to(hit);
			if (distance < closest_distance) {
				closest_distance = distance;
				closest_position = hit;
				found = true;
			}
		}
	};

	while (cell_x >= 0 && cell_x < grid_size && cell_z >= 0 && cell_z < grid_size) {
		test_cell(cell_x, cell_z);
		const real_t next_t = MIN(next_t_x, next_t_z);
		if (found && closest_distance <= next_t) {
			break;
		}
		if (cell_x == end_x && cell_z == end_z) {
			break;
		}
		if (next_t_x < next_t_z) {
			cell_x += step_x;
			next_t_x += delta_t_x;
		} else {
			cell_z += step_z;
			next_t_z += delta_t_z;
		}
	}

	if (found) {
		// Public API returns world-space data because editor tools and gameplay
		// scripts usually operate in scene coordinates.
		const Vector3 world_position = get_global_transform().xform(closest_position);
		result["position"] = world_position;
		result["local_position"] = closest_position;
		result["distance"] = p_ray_origin.distance_to(world_position);
	}
	return result;
}

PackedVector3Array SimpleTerrain3D::get_chunk_debug_lines() const {
	PackedVector3Array lines;
	if (simple_terrain_data.is_null() || chunks.is_empty()) {
		return lines;
	}

	// Lines are emitted as pairs. They are local-space points so the editor gizmo
	// can draw them under the SimpleTerrain3D node transform like any other gizmo.
	for (const TerrainChunk &chunk : chunks) {
		const int min_x = chunk.origin_x;
		const int min_z = chunk.origin_z;
		const int max_x = chunk.origin_x + chunk.quad_width;
		const int max_z = chunk.origin_z + chunk.quad_depth;

		lines.push_back(_get_vertex_position(min_x, min_z));
		lines.push_back(_get_vertex_position(max_x, min_z));
		lines.push_back(_get_vertex_position(max_x, min_z));
		lines.push_back(_get_vertex_position(max_x, max_z));
		lines.push_back(_get_vertex_position(max_x, max_z));
		lines.push_back(_get_vertex_position(min_x, max_z));
		lines.push_back(_get_vertex_position(min_x, max_z));
		lines.push_back(_get_vertex_position(min_x, min_z));
	}

	return lines;
}

void SimpleTerrain3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			if (simple_terrain_data.is_null()) {
				// First appearance in a world creates a flat terrain. Random noise
				// generation is intentionally never automatic.
				reset_flat_terrain();
			} else if (chunks.is_empty()) {
				rebuild_mesh();
			}
			_sync_chunk_instances();
			set_notify_transform(true);
		} break;

		case NOTIFICATION_EXIT_WORLD: {
			for (TerrainChunk &chunk : chunks) {
				if (chunk.instance.is_valid()) {
					RenderingServer::get_singleton()->instance_set_scenario(chunk.instance, RID());
				}
			}
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			_sync_chunk_instances();
		} break;
	}
}

void SimpleTerrain3D::_bind_methods() {
	// Everything bound here becomes part of the public script/editor API:
	// GDScript, C#, the Inspector, UndoRedo, and documentation generation all
	// discover SimpleTerrain3D through these ClassDB registrations. Keep runtime-safe
	// methods here, and keep editor-only actions inside SimpleTerrainEditorPlugin.
	ClassDB::bind_method(D_METHOD("set_simple_terrain_data", "simple_terrain_data"), &SimpleTerrain3D::set_simple_terrain_data);
	ClassDB::bind_method(D_METHOD("get_simple_terrain_data"), &SimpleTerrain3D::get_simple_terrain_data);
	ClassDB::bind_method(D_METHOD("set_world_placement_library", "library"), &SimpleTerrain3D::set_world_placement_library);
	ClassDB::bind_method(D_METHOD("get_world_placement_library"), &SimpleTerrain3D::get_world_placement_library);
	ClassDB::bind_method(D_METHOD("set_world_placement_data", "data"), &SimpleTerrain3D::set_world_placement_data);
	ClassDB::bind_method(D_METHOD("get_world_placement_data"), &SimpleTerrain3D::get_world_placement_data);
	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &SimpleTerrain3D::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &SimpleTerrain3D::get_grid_size);
	ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &SimpleTerrain3D::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &SimpleTerrain3D::get_cell_size);
	ClassDB::bind_method(D_METHOD("set_chunk_size", "chunk_size"), &SimpleTerrain3D::set_chunk_size);
	ClassDB::bind_method(D_METHOD("get_chunk_size"), &SimpleTerrain3D::get_chunk_size);
	ClassDB::bind_method(D_METHOD("set_show_chunk_gizmos", "show"), &SimpleTerrain3D::set_show_chunk_gizmos);
	ClassDB::bind_method(D_METHOD("is_showing_chunk_gizmos"), &SimpleTerrain3D::is_showing_chunk_gizmos);
	ClassDB::bind_method(D_METHOD("set_height_data", "height_data"), &SimpleTerrain3D::set_height_data);
	ClassDB::bind_method(D_METHOD("get_height_data"), &SimpleTerrain3D::get_height_data);
	ClassDB::bind_method(D_METHOD("set_terrain_material", "material"), &SimpleTerrain3D::set_terrain_material);
	ClassDB::bind_method(D_METHOD("get_terrain_material"), &SimpleTerrain3D::get_terrain_material);
	ClassDB::bind_method(D_METHOD("set_use_builtin_triplanar_material", "use"), &SimpleTerrain3D::set_use_builtin_triplanar_material);
	ClassDB::bind_method(D_METHOD("is_using_builtin_triplanar_material"), &SimpleTerrain3D::is_using_builtin_triplanar_material);
	ClassDB::bind_method(D_METHOD("set_triplanar_low_texture", "texture"), &SimpleTerrain3D::set_triplanar_low_texture);
	ClassDB::bind_method(D_METHOD("get_triplanar_low_texture"), &SimpleTerrain3D::get_triplanar_low_texture);
	ClassDB::bind_method(D_METHOD("set_triplanar_mid_texture", "texture"), &SimpleTerrain3D::set_triplanar_mid_texture);
	ClassDB::bind_method(D_METHOD("get_triplanar_mid_texture"), &SimpleTerrain3D::get_triplanar_mid_texture);
	ClassDB::bind_method(D_METHOD("set_triplanar_high_texture", "texture"), &SimpleTerrain3D::set_triplanar_high_texture);
	ClassDB::bind_method(D_METHOD("get_triplanar_high_texture"), &SimpleTerrain3D::get_triplanar_high_texture);
	ClassDB::bind_method(D_METHOD("set_triplanar_low_color", "color"), &SimpleTerrain3D::set_triplanar_low_color);
	ClassDB::bind_method(D_METHOD("get_triplanar_low_color"), &SimpleTerrain3D::get_triplanar_low_color);
	ClassDB::bind_method(D_METHOD("set_triplanar_mid_color", "color"), &SimpleTerrain3D::set_triplanar_mid_color);
	ClassDB::bind_method(D_METHOD("get_triplanar_mid_color"), &SimpleTerrain3D::get_triplanar_mid_color);
	ClassDB::bind_method(D_METHOD("set_triplanar_high_color", "color"), &SimpleTerrain3D::set_triplanar_high_color);
	ClassDB::bind_method(D_METHOD("get_triplanar_high_color"), &SimpleTerrain3D::get_triplanar_high_color);
	ClassDB::bind_method(D_METHOD("set_triplanar_low_height", "height"), &SimpleTerrain3D::set_triplanar_low_height);
	ClassDB::bind_method(D_METHOD("get_triplanar_low_height"), &SimpleTerrain3D::get_triplanar_low_height);
	ClassDB::bind_method(D_METHOD("set_triplanar_high_height", "height"), &SimpleTerrain3D::set_triplanar_high_height);
	ClassDB::bind_method(D_METHOD("get_triplanar_high_height"), &SimpleTerrain3D::get_triplanar_high_height);
	ClassDB::bind_method(D_METHOD("set_triplanar_blend_width", "width"), &SimpleTerrain3D::set_triplanar_blend_width);
	ClassDB::bind_method(D_METHOD("get_triplanar_blend_width"), &SimpleTerrain3D::get_triplanar_blend_width);
	ClassDB::bind_method(D_METHOD("set_triplanar_texture_scale", "scale"), &SimpleTerrain3D::set_triplanar_texture_scale);
	ClassDB::bind_method(D_METHOD("get_triplanar_texture_scale"), &SimpleTerrain3D::get_triplanar_texture_scale);
	ClassDB::bind_method(D_METHOD("set_triplanar_blend_sharpness", "sharpness"), &SimpleTerrain3D::set_triplanar_blend_sharpness);
	ClassDB::bind_method(D_METHOD("get_triplanar_blend_sharpness"), &SimpleTerrain3D::get_triplanar_blend_sharpness);
	ClassDB::bind_method(D_METHOD("set_flat_height", "flat_height"), &SimpleTerrain3D::set_flat_height);
	ClassDB::bind_method(D_METHOD("get_flat_height"), &SimpleTerrain3D::get_flat_height);
	ClassDB::bind_method(D_METHOD("set_random_height_scale", "random_height_scale"), &SimpleTerrain3D::set_random_height_scale);
	ClassDB::bind_method(D_METHOD("get_random_height_scale"), &SimpleTerrain3D::get_random_height_scale);
	ClassDB::bind_method(D_METHOD("set_random_frequency", "random_frequency"), &SimpleTerrain3D::set_random_frequency);
	ClassDB::bind_method(D_METHOD("get_random_frequency"), &SimpleTerrain3D::get_random_frequency);
	ClassDB::bind_method(D_METHOD("set_random_octaves", "random_octaves"), &SimpleTerrain3D::set_random_octaves);
	ClassDB::bind_method(D_METHOD("get_random_octaves"), &SimpleTerrain3D::get_random_octaves);
	ClassDB::bind_method(D_METHOD("set_random_seed", "random_seed"), &SimpleTerrain3D::set_random_seed);
	ClassDB::bind_method(D_METHOD("get_random_seed"), &SimpleTerrain3D::get_random_seed);
	ClassDB::bind_method(D_METHOD("reset_flat_terrain"), &SimpleTerrain3D::reset_flat_terrain);
	ClassDB::bind_method(D_METHOD("generate_random_terrain"), &SimpleTerrain3D::generate_random_terrain);
	ClassDB::bind_method(D_METHOD("randomize_seed"), &SimpleTerrain3D::randomize_seed);
	ClassDB::bind_method(D_METHOD("rebuild_mesh"), &SimpleTerrain3D::rebuild_mesh);
	ClassDB::bind_method(D_METHOD("apply_brush", "world_position", "radius", "strength", "operation"), &SimpleTerrain3D::apply_brush);
	ClassDB::bind_method(D_METHOD("apply_brush_with_delta", "world_position", "radius", "strength", "operation"), &SimpleTerrain3D::apply_brush_with_delta);
	ClassDB::bind_method(D_METHOD("apply_height_patch", "indices", "heights"), &SimpleTerrain3D::apply_height_patch);
	ClassDB::bind_method(D_METHOD("get_brush_hit", "ray_origin", "ray_direction"), &SimpleTerrain3D::get_brush_hit);
	ClassDB::bind_method(D_METHOD("get_chunk_debug_lines"), &SimpleTerrain3D::get_chunk_debug_lines);

	// Core terrain data and render chunk controls.
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "simple_terrain_data", PROPERTY_HINT_RESOURCE_TYPE, "SimpleTerrainData"), "set_simple_terrain_data", "get_simple_terrain_data");
	ADD_GROUP("World Placement", "world_placement_");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_placement_library", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementLibrary"), "set_world_placement_library", "get_world_placement_library");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_placement_data", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementData"), "set_world_placement_data", "get_world_placement_data");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size", PROPERTY_HINT_RANGE, "2,512,1,or_greater"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater"), "set_cell_size", "get_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "chunk_size", PROPERTY_HINT_RANGE, "1,256,1,or_greater"), "set_chunk_size", "get_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_chunk_gizmos"), "set_show_chunk_gizmos", "is_showing_chunk_gizmos");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "height_data"), "set_height_data", "get_height_data");

	// Material controls are exposed as node properties so game projects can
	// either provide their own material or use the built-in height triplanar
	// shader without writing shader code.
	ADD_GROUP("Terrain Material", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "BaseMaterial3D,ShaderMaterial"), "set_terrain_material", "get_terrain_material");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_builtin_triplanar_material"), "set_use_builtin_triplanar_material", "is_using_builtin_triplanar_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "triplanar_low_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_triplanar_low_texture", "get_triplanar_low_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "triplanar_mid_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_triplanar_mid_texture", "get_triplanar_mid_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "triplanar_high_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_triplanar_high_texture", "get_triplanar_high_texture");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "triplanar_low_color"), "set_triplanar_low_color", "get_triplanar_low_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "triplanar_mid_color"), "set_triplanar_mid_color", "get_triplanar_mid_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "triplanar_high_color"), "set_triplanar_high_color", "get_triplanar_high_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_low_height"), "set_triplanar_low_height", "get_triplanar_low_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_high_height"), "set_triplanar_high_height", "get_triplanar_high_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_blend_width", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater"), "set_triplanar_blend_width", "get_triplanar_blend_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_triplanar_texture_scale", "get_triplanar_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_blend_sharpness", PROPERTY_HINT_RANGE, "0.001,32,0.001,or_greater"), "set_triplanar_blend_sharpness", "get_triplanar_blend_sharpness");

	// Generation settings are runtime-callable. The default terrain still starts
	// flat; random terrain is generated only when explicitly requested.
	ADD_GROUP("Generation", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flat_height"), "set_flat_height", "get_flat_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_height_scale"), "set_random_height_scale", "get_random_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_frequency", PROPERTY_HINT_RANGE, "0.0001,1,0.0001,or_greater"), "set_random_frequency", "get_random_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_octaves", PROPERTY_HINT_RANGE, "1,12,1"), "set_random_octaves", "get_random_octaves");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");

	BIND_ENUM_CONSTANT(BRUSH_RAISE);
	BIND_ENUM_CONSTANT(BRUSH_LOWER);
	BIND_ENUM_CONSTANT(BRUSH_SMOOTH);
	BIND_ENUM_CONSTANT(BRUSH_FLATTEN);
}

SimpleTerrain3D::SimpleTerrain3D() {
}

AABB SimpleTerrain3D::get_aabb() const {
	if (simple_terrain_data.is_null()) {
		return AABB();
	}

	// The visible renderables are internal chunk instances, but the editor and
	// culling code still ask the SimpleTerrain3D node for an overall local bounds.
	const int vertex_count = simple_terrain_data->get_vertex_count();
	const PackedFloat32Array heights = simple_terrain_data->get_height_data();
	real_t min_height = 0.0;
	real_t max_height = 0.0;
	if (!heights.is_empty()) {
		min_height = heights[0];
		max_height = heights[0];
		for (int i = 1; i < heights.size(); i++) {
			min_height = MIN(min_height, (real_t)heights[i]);
			max_height = MAX(max_height, (real_t)heights[i]);
		}
	}

	const real_t size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size();
	return AABB(Vector3(-size * 0.5, min_height, -size * 0.5), Vector3(size, MAX((real_t)0.01, max_height - min_height), size));
}

PackedStringArray SimpleTerrain3D::get_configuration_warnings() const {
	return GeometryInstance3D::get_configuration_warnings();
}

SimpleTerrain3D::~SimpleTerrain3D() {
	_clear_chunks();
}
