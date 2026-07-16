/**************************************************************************/
/*  open_world_tree_generator_3d.cpp                                      */
/**************************************************************************/

#include "open_world_tree_generator_3d.h"

#include "core/math/random_pcg.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/resources/mesh.h"
#include "scene/resources/shader.h"
#include "scene/resources/surface_tool.h"

namespace {

struct TreeBranchPath {
	Vector<Vector3> points;
	Vector<real_t> radii;
};

static real_t _random_range(RandomPCG &r_random, real_t p_min, real_t p_max) {
	return Math::lerp(p_min, p_max, (real_t)r_random.randf());
}

static Vector3 _sample_polyline(const Vector<Vector3> &p_points, real_t p_ratio) {
	ERR_FAIL_COND_V(p_points.is_empty(), Vector3());
	if (p_points.size() == 1) {
		return p_points[0];
	}
	const real_t offset = CLAMP(p_ratio, (real_t)0.0, (real_t)1.0) * (p_points.size() - 1);
	const int index = MIN((int)Math::floor(offset), p_points.size() - 2);
	return p_points[index].lerp(p_points[index + 1], offset - index);
}

static real_t _sample_radius(const Vector<real_t> &p_radii, real_t p_ratio) {
	ERR_FAIL_COND_V(p_radii.is_empty(), 0.0);
	if (p_radii.size() == 1) {
		return p_radii[0];
	}
	const real_t offset = CLAMP(p_ratio, (real_t)0.0, (real_t)1.0) * (p_radii.size() - 1);
	const int index = MIN((int)Math::floor(offset), p_radii.size() - 2);
	return Math::lerp(p_radii[index], p_radii[index + 1], offset - index);
}

static void _add_surface_vertex(const Ref<SurfaceTool> &p_surface, const Vector3 &p_position, const Vector3 &p_normal, const Vector2 &p_uv, const Color &p_color) {
	p_surface->set_normal(p_normal);
	p_surface->set_uv(p_uv);
	p_surface->set_color(p_color);
	p_surface->add_vertex(p_position);
}

static void _append_branch_surface(const Ref<SurfaceTool> &p_surface, const TreeBranchPath &p_path, int p_radial_sides, real_t p_tree_height, real_t p_local_weight, real_t p_phase, bool p_root = false) {
	if (p_path.points.size() < 2 || p_path.radii.size() != p_path.points.size()) {
		return;
	}

	p_radial_sides = MAX(3, p_radial_sides);
	Vector<Vector<Vector3>> ring_positions;
	Vector<Vector<Vector3>> ring_normals;
	ring_positions.resize(p_path.points.size());
	ring_normals.resize(p_path.points.size());

	Vector3 previous_side;
	for (int point_index = 0; point_index < p_path.points.size(); point_index++) {
		const Vector3 previous_point = p_path.points[MAX(0, point_index - 1)];
		const Vector3 next_point = p_path.points[MIN(point_index + 1, p_path.points.size() - 1)];
		const Vector3 tangent = (next_point - previous_point).normalized();

		Vector3 side;
		if (point_index == 0) {
			const Vector3 reference = Math::abs(tangent.dot(Vector3::UP)) > 0.95 ? Vector3::RIGHT : Vector3::UP;
			side = tangent.cross(reference).normalized();
		} else {
			side = (previous_side - tangent * previous_side.dot(tangent)).normalized();
			if (side.length_squared() < CMP_EPSILON) {
				side = tangent.cross(Vector3::RIGHT).normalized();
			}
		}
		previous_side = side;
		const Vector3 binormal = tangent.cross(side).normalized();

		ring_positions.write[point_index].resize(p_radial_sides + 1);
		ring_normals.write[point_index].resize(p_radial_sides + 1);
		for (int side_index = 0; side_index <= p_radial_sides; side_index++) {
			const real_t angle = Math::TAU * (real_t)side_index / (real_t)p_radial_sides;
			const Vector3 normal = (side * Math::cos(angle) + binormal * Math::sin(angle)).normalized();
			ring_normals.write[point_index].write[side_index] = normal;
			ring_positions.write[point_index].write[side_index] = p_path.points[point_index] + normal * p_path.radii[point_index];
		}
	}

	for (int point_index = 0; point_index < p_path.points.size() - 1; point_index++) {
		const real_t v0 = (real_t)point_index / (real_t)(p_path.points.size() - 1);
		const real_t v1 = (real_t)(point_index + 1) / (real_t)(p_path.points.size() - 1);
		for (int side_index = 0; side_index < p_radial_sides; side_index++) {
			const real_t u0 = (real_t)side_index / (real_t)p_radial_sides;
			const real_t u1 = (real_t)(side_index + 1) / (real_t)p_radial_sides;
			const real_t height0 = p_root ? 0.0 : CLAMP(ring_positions[point_index][side_index].y / MAX((real_t)0.001, p_tree_height), (real_t)0.0, (real_t)1.0);
			const real_t height1 = p_root ? 0.0 : CLAMP(ring_positions[point_index + 1][side_index].y / MAX((real_t)0.001, p_tree_height), (real_t)0.0, (real_t)1.0);
			const Color color0(height0, p_root ? 0.0 : p_local_weight * v0, 0.0, p_phase);
			const Color color1(height1, p_root ? 0.0 : p_local_weight * v1, 0.0, p_phase);

			_add_surface_vertex(p_surface, ring_positions[point_index][side_index], ring_normals[point_index][side_index], Vector2(u0, v0), color0);
			_add_surface_vertex(p_surface, ring_positions[point_index + 1][side_index], ring_normals[point_index + 1][side_index], Vector2(u0, v1), color1);
			_add_surface_vertex(p_surface, ring_positions[point_index + 1][side_index + 1], ring_normals[point_index + 1][side_index + 1], Vector2(u1, v1), color1);

			_add_surface_vertex(p_surface, ring_positions[point_index][side_index], ring_normals[point_index][side_index], Vector2(u0, v0), color0);
			_add_surface_vertex(p_surface, ring_positions[point_index + 1][side_index + 1], ring_normals[point_index + 1][side_index + 1], Vector2(u1, v1), color1);
			_add_surface_vertex(p_surface, ring_positions[point_index][side_index + 1], ring_normals[point_index][side_index + 1], Vector2(u1, v0), color0);
		}
	}
}

static real_t _blob_noise(const Vector3 &p_direction, int p_blob_seed) {
	const real_t phase = p_direction.x * 12.9898 + p_direction.y * 78.233 + p_direction.z * 37.719 + (real_t)p_blob_seed * 0.017;
	return Math::sin(phase) * 0.65 + Math::sin(phase * 2.17 + 1.7) * 0.35;
}

static void _append_blob_vertex(const Ref<SurfaceTool> &p_surface, const Vector3 &p_direction, const Vector3 &p_center, const Vector3 &p_scale, real_t p_roughness, int p_blob_seed, const Color &p_color) {
	const Vector3 direction = p_direction.normalized();
	const real_t radius_scale = MAX((real_t)0.15, (real_t)1.0 + _blob_noise(direction, p_blob_seed) * p_roughness);
	const Vector3 position = p_center + Vector3(direction.x * p_scale.x, direction.y * p_scale.y, direction.z * p_scale.z) * radius_scale;
	const Vector3 normal = Vector3(direction.x / MAX((real_t)0.001, p_scale.x), direction.y / MAX((real_t)0.001, p_scale.y), direction.z / MAX((real_t)0.001, p_scale.z)).normalized();
	const Vector2 uv(Math::atan2(direction.z, direction.x) / Math::TAU + 0.5, Math::acos(CLAMP(direction.y, (real_t)-1.0, (real_t)1.0)) / Math::PI);
	_add_surface_vertex(p_surface, position, normal, uv, p_color);
}

static void _append_canopy_blob(const Ref<SurfaceTool> &p_surface, const Vector3 &p_center, const Vector3 &p_scale, real_t p_roughness, int p_blob_seed, const Color &p_color, bool p_low_detail) {
	static const Vector3 vertices[12] = {
		Vector3(-1, 1.61803398875, 0), Vector3(1, 1.61803398875, 0), Vector3(-1, -1.61803398875, 0), Vector3(1, -1.61803398875, 0),
		Vector3(0, -1, 1.61803398875), Vector3(0, 1, 1.61803398875), Vector3(0, -1, -1.61803398875), Vector3(0, 1, -1.61803398875),
		Vector3(1.61803398875, 0, -1), Vector3(1.61803398875, 0, 1), Vector3(-1.61803398875, 0, -1), Vector3(-1.61803398875, 0, 1),
	};
	static constexpr int faces[20][3] = {
		{ 0, 11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7, 10 }, { 0, 10, 11 },
		{ 1, 5, 9 }, { 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 },
		{ 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 }, { 3, 8, 9 },
		{ 4, 9, 5 }, { 2, 4, 11 }, { 6, 2, 10 }, { 8, 6, 7 }, { 9, 8, 1 },
	};

	for (const int *face : faces) {
		const Vector3 a = vertices[face[0]].normalized();
		const Vector3 b = vertices[face[1]].normalized();
		const Vector3 c = vertices[face[2]].normalized();
		if (p_low_detail) {
			_append_blob_vertex(p_surface, a, p_center, p_scale, p_roughness, p_blob_seed, p_color);
			_append_blob_vertex(p_surface, c, p_center, p_scale, p_roughness, p_blob_seed, p_color);
			_append_blob_vertex(p_surface, b, p_center, p_scale, p_roughness, p_blob_seed, p_color);
			continue;
		}
		const Vector3 ab = (a + b).normalized();
		const Vector3 bc = (b + c).normalized();
		const Vector3 ca = (c + a).normalized();
		const Vector3 triangles[4][3] = {
			{ a, ab, ca }, { b, bc, ab }, { c, ca, bc }, { ab, bc, ca },
		};
		for (const Vector3 *triangle : triangles) {
			_append_blob_vertex(p_surface, triangle[0], p_center, p_scale, p_roughness, p_blob_seed, p_color);
			_append_blob_vertex(p_surface, triangle[2], p_center, p_scale, p_roughness, p_blob_seed, p_color);
			_append_blob_vertex(p_surface, triangle[1], p_center, p_scale, p_roughness, p_blob_seed, p_color);
		}
	}
}
static void _append_frond(const Ref<SurfaceTool> &p_surface, const Vector3 &p_crown, real_t p_angle, real_t p_length, real_t p_width, real_t p_droop, int p_frond_seed, int p_segments, real_t p_tree_height, real_t p_phase) {
	const int segments = MAX(2, p_segments);
	const Vector3 radial(Math::cos(p_angle), 0.0, Math::sin(p_angle));
	const Vector3 side(-Math::sin(p_angle), 0.0, Math::cos(p_angle));
	Vector<Vector3> centers;
	Vector<real_t> half_widths;
	centers.resize(segments + 1);
	half_widths.resize(segments + 1);

	RandomPCG random((uint64_t)(uint32_t)p_frond_seed);
	const real_t lift = p_length * _random_range(random, 0.12, 0.24);
	for (int index = 0; index <= segments; index++) {
		const real_t ratio = (real_t)index / (real_t)segments;
		const real_t width_envelope = Math::pow(MAX((real_t)0.0, (real_t)Math::sin(ratio * Math::PI)), (real_t)0.55);
		centers.write[index] = p_crown + radial * p_length * ratio + Vector3::UP * (Math::sin(ratio * Math::PI) * lift - p_droop * ratio * ratio);
		half_widths.write[index] = p_width * 0.5 * width_envelope;
	}

	for (int index = 0; index < segments; index++) {
		const real_t v0 = (real_t)index / (real_t)segments;
		const real_t v1 = (real_t)(index + 1) / (real_t)segments;
		const Vector3 path_tangent = (centers[index + 1] - centers[index]).normalized();
		Vector3 normal = side.cross(path_tangent).normalized();
		if (normal.dot(Vector3::UP) < 0.0) {
			normal = -normal;
		}
		const Vector3 a = centers[index] - side * half_widths[index];
		const Vector3 b = centers[index] + side * half_widths[index];
		const Vector3 c = centers[index + 1] - side * half_widths[index + 1];
		const Vector3 d = centers[index + 1] + side * half_widths[index + 1];
		const Color color0(CLAMP(centers[index].y / MAX((real_t)0.001, p_tree_height), (real_t)0.0, (real_t)1.0), v0, v0, p_phase);
		const Color color1(CLAMP(centers[index + 1].y / MAX((real_t)0.001, p_tree_height), (real_t)0.0, (real_t)1.0), v1, v1, p_phase);

		_add_surface_vertex(p_surface, a, normal, Vector2(0.0, v0), color0);
		_add_surface_vertex(p_surface, c, normal, Vector2(0.0, v1), color1);
		_add_surface_vertex(p_surface, d, normal, Vector2(1.0, v1), color1);
		_add_surface_vertex(p_surface, a, normal, Vector2(0.0, v0), color0);
		_add_surface_vertex(p_surface, d, normal, Vector2(1.0, v1), color1);
		_add_surface_vertex(p_surface, b, normal, Vector2(1.0, v0), color0);

		_add_surface_vertex(p_surface, d, -normal, Vector2(1.0, v1), color1);
		_add_surface_vertex(p_surface, c, -normal, Vector2(0.0, v1), color1);
		_add_surface_vertex(p_surface, a, -normal, Vector2(0.0, v0), color0);
		_add_surface_vertex(p_surface, b, -normal, Vector2(1.0, v0), color0);
		_add_surface_vertex(p_surface, d, -normal, Vector2(1.0, v1), color1);
		_add_surface_vertex(p_surface, a, -normal, Vector2(0.0, v0), color0);
	}
}
} // namespace

void OpenWorldTreeGenerator3D::_profile_changed() {
	if (auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::_update_wind_materials() {
	static const char *wind_shader_code = R"(
shader_type spatial;
render_mode cull_disabled, depth_draw_opaque;

uniform vec4 base_color : source_color = vec4(1.0);
uniform vec2 wind_direction = vec2(1.0, 0.35);
uniform float wind_strength : hint_range(0.0, 2.0) = 0.22;
uniform float wind_speed : hint_range(0.0, 8.0) = 1.0;
uniform float gust_strength : hint_range(0.0, 2.0) = 0.35;

void vertex() {
	vec2 direction = length(wind_direction) > 0.0001 ? normalize(wind_direction) : vec2(1.0, 0.0);
	float instance_phase = fract(sin(dot(MODEL_MATRIX[3].xz, vec2(12.9898, 78.233))) * 43758.5453);
	float phase = COLOR.a * 6.2831853 + instance_phase * 6.2831853;
	float sway = sin(TIME * wind_speed + phase + VERTEX.y * 0.31);
	float gust = sin(TIME * wind_speed * 0.37 + phase * 1.71) * gust_strength;
	float branch = sin(TIME * wind_speed * 1.73 + phase + VERTEX.y * 0.83) * COLOR.g;
	float flutter = sin(TIME * wind_speed * 3.8 + phase + UV.y * 8.0) * COLOR.b;
	VERTEX.xz += direction * (sway + gust) * wind_strength * COLOR.r;
	VERTEX.xz += vec2(-direction.y, direction.x) * branch * wind_strength * 0.32;
	VERTEX.y += flutter * wind_strength * 0.12;
}

void fragment() {
	ALBEDO = base_color.rgb;
	ROUGHNESS = 0.82;
}
)";

	if (wind_trunk_material.is_null()) {
		Ref<Shader> shader;
		shader.instantiate();
		shader->set_code(wind_shader_code);
		wind_trunk_material.instantiate();
		wind_trunk_material->set_shader(shader);
	}
	if (wind_foliage_material.is_null()) {
		Ref<Shader> shader;
		shader.instantiate();
		shader->set_code(wind_shader_code);
		wind_foliage_material.instantiate();
		wind_foliage_material->set_shader(shader);
	}

	for (const Ref<ShaderMaterial> &material : { wind_trunk_material, wind_foliage_material }) {
		material->set_shader_parameter(SNAME("wind_direction"), wind_direction);
		material->set_shader_parameter(SNAME("wind_strength"), wind_strength);
		material->set_shader_parameter(SNAME("wind_speed"), wind_speed);
		material->set_shader_parameter(SNAME("gust_strength"), wind_gust_strength);
	}
	wind_trunk_material->set_shader_parameter(SNAME("base_color"), wind_trunk_color);
	wind_foliage_material->set_shader_parameter(SNAME("base_color"), wind_foliage_color);
}

void OpenWorldTreeGenerator3D::_apply_materials() {
	if (wind_enabled) {
		_update_wind_materials();
	}
	for (int lod = 0; lod < 3; lod++) {
		Ref<ArrayMesh> lod_mesh = generated_lod_meshes[lod];
		if (lod_mesh.is_null()) {
			continue;
		}
		if (lod_mesh->get_surface_count() > 0) {
			lod_mesh->surface_set_material(0, wind_enabled ? Ref<Material>(wind_trunk_material) : trunk_material);
		}
		if (lod_mesh->get_surface_count() > 1) {
			lod_mesh->surface_set_material(1, wind_enabled ? Ref<Material>(wind_foliage_material) : foliage_material);
		}
	}
}

void OpenWorldTreeGenerator3D::_update_preview_mesh() {
	int lod = CLAMP((int)preview_lod, 0, 2);
	while (lod > 0 && generated_lod_meshes[lod].is_null()) {
		lod--;
	}
	set_mesh(generated_lod_meshes[lod]);
	update_gizmos();
}

void OpenWorldTreeGenerator3D::set_generate_lod1(bool p_enabled) {
	if (generate_lod1 == p_enabled) {
		return;
	}
	generate_lod1 = p_enabled;
	if (auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::set_generate_lod2(bool p_enabled) {
	if (generate_lod2 == p_enabled) {
		return;
	}
	generate_lod2 = p_enabled;
	if (auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::set_lod1_quality(real_t p_quality) {
	p_quality = CLAMP(p_quality, (real_t)0.1, (real_t)1.0);
	if (Math::is_equal_approx(lod1_quality, p_quality)) {
		return;
	}
	lod1_quality = p_quality;
	if (auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::set_lod2_quality(real_t p_quality) {
	p_quality = CLAMP(p_quality, (real_t)0.05, (real_t)0.75);
	if (Math::is_equal_approx(lod2_quality, p_quality)) {
		return;
	}
	lod2_quality = p_quality;
	if (auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::set_lod1_distance(real_t p_distance) {
	lod1_distance = MAX((real_t)0.0, p_distance);
	lod2_distance = MAX(lod2_distance, lod1_distance);
	max_distance = MAX(max_distance, lod2_distance);
}

void OpenWorldTreeGenerator3D::set_lod2_distance(real_t p_distance) {
	lod2_distance = MAX(lod1_distance, p_distance);
	max_distance = MAX(max_distance, lod2_distance);
}

void OpenWorldTreeGenerator3D::set_max_distance(real_t p_distance) {
	max_distance = MAX(lod2_distance, p_distance);
}

void OpenWorldTreeGenerator3D::set_preview_lod(PreviewLOD p_lod) {
	p_lod = (PreviewLOD)CLAMP((int)p_lod, 0, 2);
	if (preview_lod == p_lod) {
		return;
	}
	preview_lod = p_lod;
	_update_preview_mesh();
}

void OpenWorldTreeGenerator3D::set_wind_enabled(bool p_enabled) {
	if (wind_enabled == p_enabled) {
		return;
	}
	wind_enabled = p_enabled;
	_apply_materials();
	_update_preview_mesh();
}

void OpenWorldTreeGenerator3D::set_wind_strength(real_t p_strength) {
	wind_strength = CLAMP(p_strength, (real_t)0.0, (real_t)2.0);
	_update_wind_materials();
}

void OpenWorldTreeGenerator3D::set_wind_speed(real_t p_speed) {
	wind_speed = CLAMP(p_speed, (real_t)0.0, (real_t)8.0);
	_update_wind_materials();
}

void OpenWorldTreeGenerator3D::set_wind_gust_strength(real_t p_strength) {
	wind_gust_strength = CLAMP(p_strength, (real_t)0.0, (real_t)2.0);
	_update_wind_materials();
}

void OpenWorldTreeGenerator3D::set_wind_direction(const Vector2 &p_direction) {
	wind_direction = p_direction.length_squared() > CMP_EPSILON ? p_direction.normalized() : Vector2(1.0, 0.0);
	_update_wind_materials();
}

void OpenWorldTreeGenerator3D::set_wind_trunk_color(const Color &p_color) {
	wind_trunk_color = p_color;
	_update_wind_materials();
}

void OpenWorldTreeGenerator3D::set_wind_foliage_color(const Color &p_color) {
	wind_foliage_color = p_color;
	_update_wind_materials();
}

Ref<ArrayMesh> OpenWorldTreeGenerator3D::get_generated_lod_mesh(int p_lod) const {
	ERR_FAIL_INDEX_V(p_lod, 3, Ref<ArrayMesh>());
	return generated_lod_meshes[p_lod];
}

Dictionary OpenWorldTreeGenerator3D::get_lod_statistics(int p_lod) const {
	Dictionary result;
	ERR_FAIL_INDEX_V(p_lod, 3, result);
	Ref<ArrayMesh> lod_mesh = generated_lod_meshes[p_lod];
	result["lod"] = p_lod;
	result["available"] = lod_mesh.is_valid();
	int vertices = 0;
	int triangles = 0;
	if (lod_mesh.is_valid()) {
		for (int surface = 0; surface < lod_mesh->get_surface_count(); surface++) {
			const Array arrays = lod_mesh->surface_get_arrays(surface);
			const PackedVector3Array surface_vertices = arrays[Mesh::ARRAY_VERTEX];
			const PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];
			vertices += surface_vertices.size();
			triangles += (indices.is_empty() ? surface_vertices.size() : indices.size()) / 3;
		}
		result["surfaces"] = lod_mesh->get_surface_count();
		result["aabb"] = lod_mesh->get_aabb();
	} else {
		result["surfaces"] = 0;
	}
	result["vertices"] = vertices;
	result["triangles"] = triangles;
	return result;
}

void OpenWorldTreeGenerator3D::set_generation_profile(const Ref<OpenWorldTreeGenerationProfile> &p_profile) {
	if (generation_profile == p_profile) {
		return;
	}
	if (generation_profile.is_valid()) {
		generation_profile->disconnect_changed(callable_mp(this, &OpenWorldTreeGenerator3D::_profile_changed));
	}
	generation_profile = p_profile;
	if (generation_profile.is_valid()) {
		generation_profile->connect_changed(callable_mp(this, &OpenWorldTreeGenerator3D::_profile_changed));
	}
	_profile_changed();
}

void OpenWorldTreeGenerator3D::set_seed(int p_seed) {
	if (seed == p_seed) {
		return;
	}
	seed = p_seed;
	if (auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::set_auto_generate(bool p_enabled) {
	if (auto_generate == p_enabled) {
		return;
	}
	auto_generate = p_enabled;
	if (auto_generate && generated_mesh.is_null()) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::set_trunk_material(const Ref<Material> &p_material) {
	trunk_material = p_material;
	_apply_materials();
}

void OpenWorldTreeGenerator3D::set_foliage_material(const Ref<Material> &p_material) {
	foliage_material = p_material;
	_apply_materials();
}

Ref<ArrayMesh> OpenWorldTreeGenerator3D::_generate_lod_mesh(int p_lod, Ref<OpenWorldTreeSupportGraph> *r_support_graph) const {
	if (generation_profile.is_null()) {
		return Ref<ArrayMesh>();
	}

	p_lod = CLAMP(p_lod, 0, 2);
	const real_t quality = p_lod == 0 ? 1.0 : (p_lod == 1 ? lod1_quality : lod2_quality);
	const int branch_stride = p_lod == 0 ? 1 : MAX(2, (int)Math::round(1.0 / MAX((real_t)0.05, quality)));
	const int foliage_stride = 1;
	const real_t wind_phase = Math::fposmod((real_t)(uint32_t)seed * (real_t)0.0000001192092896, (real_t)1.0);
	RandomPCG random((uint64_t)(uint32_t)seed);
	const OpenWorldTreeGenerationProfile::TreeArchetype archetype = generation_profile->get_archetype();
	OpenWorldTreeGenerationProfile::CrownShape crown_shape = generation_profile->get_crown_shape();
	if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_AUTO) {
		switch (archetype) {
			case OpenWorldTreeGenerationProfile::ARCHETYPE_UMBRELLA:
				crown_shape = OpenWorldTreeGenerationProfile::CROWN_UMBRELLA;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_CONIFER:
				crown_shape = OpenWorldTreeGenerationProfile::CROWN_CONICAL;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_MANGROVE:
				crown_shape = OpenWorldTreeGenerationProfile::CROWN_TIERED;
				break;
			default:
				crown_shape = OpenWorldTreeGenerationProfile::CROWN_ROUND;
				break;
		}
	}

	OpenWorldTreeGenerationProfile::RootStyle root_style = generation_profile->get_root_style();
	if (root_style == OpenWorldTreeGenerationProfile::ROOT_AUTO) {
		switch (archetype) {
			case OpenWorldTreeGenerationProfile::ARCHETYPE_TROPICAL_BROADLEAF:
				root_style = OpenWorldTreeGenerationProfile::ROOT_BUTTRESS;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_UMBRELLA:
			case OpenWorldTreeGenerationProfile::ARCHETYPE_PALM:
				root_style = OpenWorldTreeGenerationProfile::ROOT_FLARE;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_MANGROVE:
				root_style = OpenWorldTreeGenerationProfile::ROOT_PROP;
				break;
			default:
				root_style = OpenWorldTreeGenerationProfile::ROOT_NONE;
				break;
		}
	}

	const int trunk_segments = MAX(2, (int)Math::round(generation_profile->get_trunk_segments() * Math::lerp((real_t)0.35, (real_t)1.0, quality)));
	const real_t tree_height = generation_profile->get_tree_height();
	const real_t root_ratio = MIN((real_t)1.0, generation_profile->get_root_height() / tree_height);
	const real_t phase_x = _random_range(random, 0.0, Math::TAU);
	const real_t phase_z = _random_range(random, 0.0, Math::TAU);

	TreeBranchPath trunk;
	trunk.points.resize(trunk_segments + 1);
	trunk.radii.resize(trunk_segments + 1);
	for (int point_index = 0; point_index <= trunk_segments; point_index++) {
		const real_t ratio = (real_t)point_index / (real_t)trunk_segments;
		const real_t bend_envelope = Math::pow(ratio, (real_t)1.25);
		const real_t bend = generation_profile->get_trunk_bend();
		const real_t offset_x = (Math::sin(phase_x + ratio * 2.3) * 0.7 + Math::sin(phase_x * 1.7 + ratio * 5.1) * 0.3) * bend * bend_envelope;
		const real_t offset_z = (Math::sin(phase_z + ratio * 1.9) * 0.7 + Math::sin(phase_z * 1.3 + ratio * 4.3) * 0.3) * bend * bend_envelope;
		trunk.points.write[point_index] = Vector3(offset_x, tree_height * ratio, offset_z);
		real_t tip_radius = generation_profile->get_trunk_tip_radius();
		if (archetype == OpenWorldTreeGenerationProfile::ARCHETYPE_PALM) {
			tip_radius = MAX(tip_radius, generation_profile->get_trunk_base_radius() * (real_t)0.58);
		}
		real_t radius = Math::lerp(generation_profile->get_trunk_base_radius(), tip_radius, Math::pow(ratio, (real_t)0.85));
		if (root_style != OpenWorldTreeGenerationProfile::ROOT_NONE && ratio < root_ratio) {
			const real_t flare_weight = (real_t)1.0 - ratio / MAX(root_ratio, (real_t)0.001);
			radius *= Math::lerp((real_t)1.0, generation_profile->get_root_flare_scale(), flare_weight * flare_weight);
		}
		trunk.radii.write[point_index] = radius;
	}

	Vector<TreeBranchPath> roots;
	if ((root_style == OpenWorldTreeGenerationProfile::ROOT_BUTTRESS || root_style == OpenWorldTreeGenerationProfile::ROOT_PROP) && generation_profile->get_root_count() > 0) {
		const real_t initial_root_rotation = _random_range(random, 0.0, Math::TAU);
		for (int root_index = 0; root_index < generation_profile->get_root_count(); root_index++) {
			const real_t angle = initial_root_rotation + Math::TAU * (real_t)root_index / (real_t)generation_profile->get_root_count() + _random_range(random, -0.12, 0.12);
			const Vector3 radial(Math::cos(angle), 0.0, Math::sin(angle));
			const real_t root_length = generation_profile->get_root_length() * _random_range(random, 0.8, 1.2);
			const real_t base_radius = generation_profile->get_trunk_base_radius();
			TreeBranchPath root;
			root.points.resize(4);
			root.radii.resize(4);
			if (root_style == OpenWorldTreeGenerationProfile::ROOT_PROP) {
				const real_t start_height = MIN(tree_height * (real_t)0.4, generation_profile->get_root_height() * _random_range(random, 1.0, 1.5));
				const real_t start_ratio = start_height / tree_height;
				root.points.write[0] = _sample_polyline(trunk.points, start_ratio) + radial * _sample_radius(trunk.radii, start_ratio) * 0.6;
				root.points.write[1] = root.points[0] + radial * root_length * 0.35 - Vector3::UP * start_height * 0.2;
				root.points.write[2] = radial * root_length * 0.75 + Vector3::UP * 0.12;
				root.points.write[3] = radial * root_length + Vector3::UP * 0.02;
				root.radii.write[0] = base_radius * 0.26;
				root.radii.write[1] = base_radius * 0.20;
			} else {
				root.points.write[0] = radial * base_radius * 0.25 + Vector3::UP * generation_profile->get_root_height();
				root.points.write[1] = radial * root_length * 0.35 + Vector3::UP * generation_profile->get_root_height() * 0.38;
				root.points.write[2] = radial * root_length * 0.72 + Vector3::UP * 0.08;
				root.points.write[3] = radial * root_length + Vector3::UP * 0.02;
				root.radii.write[0] = base_radius * 0.42;
				root.radii.write[1] = base_radius * 0.28;
			}
			root.radii.write[2] = base_radius * 0.12;
			root.radii.write[3] = MAX((real_t)0.008, base_radius * 0.025);
			roots.push_back(root);
		}
	}

	Vector<TreeBranchPath> branches;
	Vector<real_t> branch_attachment_ratios;
	Vector<TreeBranchPath> secondary_branches;
	Vector<Vector3> canopy_anchors;
	canopy_anchors.push_back(trunk.points[trunk.points.size() - 1]);

	if (archetype != OpenWorldTreeGenerationProfile::ARCHETYPE_PALM) {
		real_t start_ratio = generation_profile->get_branch_start_ratio();
		real_t end_ratio = generation_profile->get_branch_end_ratio();
		real_t length_multiplier = 1.0;
		real_t elevation_offset = 0.0;
		switch (archetype) {
			case OpenWorldTreeGenerationProfile::ARCHETYPE_TROPICAL_BROADLEAF:
				start_ratio = MAX(start_ratio, (real_t)0.42);
				length_multiplier = 1.18;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_UMBRELLA:
				start_ratio = MAX(start_ratio, (real_t)0.55);
				elevation_offset = -12.0;
				length_multiplier = 1.2;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_CONIFER:
				start_ratio = MIN(start_ratio, (real_t)0.18);
				end_ratio = MAX(end_ratio, (real_t)0.94);
				elevation_offset = -18.0;
				break;
			case OpenWorldTreeGenerationProfile::ARCHETYPE_MANGROVE:
				start_ratio = MIN(start_ratio, (real_t)0.2);
				end_ratio = MIN(end_ratio, (real_t)0.72);
				length_multiplier = 0.9;
				break;
			default:
				break;
		}

		const real_t interval_ratio = generation_profile->get_branch_interval() / tree_height;
		const int maximum_branch_count = 128;
		const real_t initial_rotation = _random_range(random, 0.0, Math::TAU);
		for (int branch_index = 0; branch_index < maximum_branch_count; branch_index++) {
			const real_t nominal_ratio = start_ratio + interval_ratio * branch_index;
			if (nominal_ratio > end_ratio + CMP_EPSILON) {
				break;
			}
			const real_t attachment_ratio = CLAMP(nominal_ratio + _random_range(random, -interval_ratio * 0.18, interval_ratio * 0.18), start_ratio, end_ratio);
			const real_t rotation = initial_rotation + Math::deg_to_rad(generation_profile->get_phyllotaxy_angle_degrees()) * branch_index + _random_range(random, -0.12, 0.12);
			const Vector3 radial(Math::cos(rotation), 0.0, Math::sin(rotation));
			const real_t elevation = Math::deg_to_rad(generation_profile->get_branch_elevation_degrees() + elevation_offset + _random_range(random, -8.0, 8.0));
			const Vector3 direction = (radial * Math::cos(elevation) + Vector3::UP * Math::sin(elevation)).normalized();
			const real_t crown_ratio = end_ratio > start_ratio ? (attachment_ratio - start_ratio) / (end_ratio - start_ratio) : 1.0;
			real_t length_shape = 0.65 + Math::sin(crown_ratio * Math::PI) * 0.35;
			if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_CONICAL) {
				length_shape = Math::lerp((real_t)1.15, (real_t)0.28, crown_ratio);
			} else if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_UMBRELLA) {
				length_shape = Math::lerp((real_t)0.62, (real_t)1.1, crown_ratio);
			} else if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_TIERED) {
				length_shape = 0.72 + (branch_index % 3 == 0 ? 0.32 : 0.0);
			}
			const real_t branch_length = _random_range(random, generation_profile->get_branch_length_min(), generation_profile->get_branch_length_max()) * length_shape * length_multiplier;
			const real_t trunk_radius = _sample_radius(trunk.radii, attachment_ratio);
			const real_t base_radius = MAX((real_t)0.01, trunk_radius * generation_profile->get_branch_base_radius_scale());
			const real_t tip_radius = MAX((real_t)0.006, base_radius * 0.12);
			const Vector3 origin = _sample_polyline(trunk.points, attachment_ratio) - radial * trunk_radius * 0.2;
			const Vector3 lateral = direction.cross(Vector3::UP).normalized();
			const real_t lateral_curve = _random_range(random, -branch_length * 0.12, branch_length * 0.12);
			const real_t archetype_droop = archetype == OpenWorldTreeGenerationProfile::ARCHETYPE_CONIFER ? branch_length * 0.16 : 0.0;
			const real_t total_droop = generation_profile->get_branch_bend() + generation_profile->get_branch_droop() + archetype_droop;

			TreeBranchPath branch;
			const int branch_segments = MAX(1, (int)Math::round(generation_profile->get_branch_segments() * Math::lerp((real_t)0.35, (real_t)1.0, quality)));
			branch.points.resize(branch_segments + 1);
			branch.radii.resize(branch_segments + 1);
			for (int point_index = 0; point_index <= branch_segments; point_index++) {
				const real_t ratio = (real_t)point_index / (real_t)branch_segments;
				Vector3 point = origin + direction * branch_length * ratio;
				point.y -= total_droop * ratio * ratio;
				point += lateral * Math::sin(ratio * Math::PI) * lateral_curve;
				branch.points.write[point_index] = point;
				branch.radii.write[point_index] = Math::lerp(base_radius, tip_radius, Math::pow(ratio, (real_t)0.8));
			}
			if (attachment_ratio >= Math::lerp(start_ratio, end_ratio, (real_t)0.2)) {
				canopy_anchors.push_back(branch.points[branch.points.size() - 1]);
			}

			for (int secondary_index = 0; secondary_index < generation_profile->get_secondary_branch_count(); secondary_index++) {
				const real_t secondary_ratio = Math::lerp((real_t)0.5, (real_t)0.82, (real_t)(secondary_index + 1) / (real_t)(generation_profile->get_secondary_branch_count() + 1));
				const real_t side_sign = secondary_index % 2 == 0 ? 1.0 : -1.0;
				const real_t secondary_angle = rotation + side_sign * _random_range(random, 0.65, 1.1);
				const Vector3 secondary_radial(Math::cos(secondary_angle), 0.0, Math::sin(secondary_angle));
				const Vector3 secondary_direction = (secondary_radial + Vector3::UP * _random_range(random, 0.05, 0.3)).normalized();
				const real_t secondary_length = branch_length * generation_profile->get_secondary_branch_scale() * _random_range(random, 0.72, 1.0);
				const real_t secondary_base = MAX((real_t)0.006, _sample_radius(branch.radii, secondary_ratio) * 0.55);
				TreeBranchPath secondary;
				secondary.points.resize(branch_segments + 1);
				secondary.radii.resize(branch_segments + 1);
				const Vector3 secondary_origin = _sample_polyline(branch.points, secondary_ratio);
				for (int point_index = 0; point_index <= branch_segments; point_index++) {
					const real_t ratio = (real_t)point_index / (real_t)branch_segments;
					secondary.points.write[point_index] = secondary_origin + secondary_direction * secondary_length * ratio - Vector3::UP * total_droop * 0.5 * ratio * ratio;
					secondary.radii.write[point_index] = Math::lerp(secondary_base, MAX((real_t)0.004, secondary_base * (real_t)0.08), ratio);
				}
				canopy_anchors.push_back(secondary.points[secondary.points.size() - 1]);
				secondary_branches.push_back(secondary);
			}
			branches.push_back(branch);
			branch_attachment_ratios.push_back(attachment_ratio);
		}
	}

	if (r_support_graph != nullptr) {
		Ref<OpenWorldTreeSupportGraph> graph;
		graph.instantiate();
		PackedVector3Array trunk_points;
		PackedFloat32Array trunk_radii;
		for (int i = 0; i < trunk.points.size(); i++) { trunk_points.push_back(trunk.points[i]); trunk_radii.push_back(trunk.radii[i]); }
		graph->add_path(trunk_points, trunk_radii, -1, 0.0);
		for (int branch_index = 0; branch_index < branches.size(); branch_index++) {
			PackedVector3Array points;
			PackedFloat32Array radii;
			for (int i = 0; i < branches[branch_index].points.size(); i++) { points.push_back(branches[branch_index].points[i]); radii.push_back(branches[branch_index].radii[i]); }
			graph->add_path(points, radii, 0, branch_attachment_ratios[branch_index]);
		}
		*r_support_graph = graph;
	}

	Ref<SurfaceTool> trunk_surface;
	trunk_surface.instantiate();
	trunk_surface->begin(Mesh::PRIMITIVE_TRIANGLES);
	_append_branch_surface(trunk_surface, trunk, MAX(3, (int)Math::round(generation_profile->get_trunk_radial_sides() * Math::lerp((real_t)0.55, (real_t)1.0, quality))), tree_height, 0.12, wind_phase);
	const int branch_sides = MAX(3, (int)Math::round((generation_profile->get_trunk_radial_sides() - 2) * Math::lerp((real_t)0.6, (real_t)1.0, quality)));
	for (int root_index = 0; root_index < roots.size(); root_index++) {
		if (root_index % branch_stride == 0) {
			_append_branch_surface(trunk_surface, roots[root_index], branch_sides, tree_height, 0.0, wind_phase, true);
		}
	}
	for (int branch_index = 0; branch_index < branches.size(); branch_index++) {
		if (branch_index % branch_stride == 0) {
			_append_branch_surface(trunk_surface, branches[branch_index], branch_sides, tree_height, 1.0, wind_phase);
		}
	}
	if (p_lod == 0) {
		for (const TreeBranchPath &branch : secondary_branches) {
			_append_branch_surface(trunk_surface, branch, MAX(3, branch_sides - 1), tree_height, 1.0, wind_phase);
		}
	}
	trunk_surface->index();
	trunk_surface->generate_tangents();
	Ref<ArrayMesh> lod_mesh = trunk_surface->commit();

	const bool generate_fronds = archetype == OpenWorldTreeGenerationProfile::ARCHETYPE_PALM;
	const bool generate_blobs = !generate_fronds && generation_profile->get_canopy_blob_count() > 0 && !canopy_anchors.is_empty();
	if (generate_fronds || generate_blobs) {
		Ref<SurfaceTool> foliage_surface;
		foliage_surface.instantiate();
		foliage_surface->begin(Mesh::PRIMITIVE_TRIANGLES);
		if (generate_fronds) {
			const Vector3 crown = trunk.points[trunk.points.size() - 1];
			const real_t initial_angle = _random_range(random, 0.0, Math::TAU);
			for (int frond_index = 0; frond_index < generation_profile->get_palm_frond_count(); frond_index++) {
				const real_t angle = initial_angle + Math::TAU * (real_t)frond_index / (real_t)generation_profile->get_palm_frond_count() + _random_range(random, -0.08, 0.08);
				const real_t length = generation_profile->get_palm_frond_length() * _random_range(random, 0.82, 1.15);
				const real_t width = generation_profile->get_palm_frond_width() * _random_range(random, 0.85, 1.15);
				_random_range(random, 0.82, 1.0);
				_random_range(random, 0.9, 1.0);
				_random_range(random, 0.78, 0.96);
				if (frond_index % foliage_stride == 0) {
					const int frond_segments = MAX(2, (int)Math::round(7.0 * Math::lerp((real_t)0.35, (real_t)1.0, quality)));
					_append_frond(foliage_surface, crown, angle, length, width, generation_profile->get_palm_frond_droop(), seed ^ (frond_index * 3571), frond_segments, tree_height, wind_phase);
				}
			}
		} else {
			for (int blob_index = 0; blob_index < generation_profile->get_canopy_blob_count(); blob_index++) {
				const int anchor_index = random.rand() % canopy_anchors.size();
				const real_t jitter = generation_profile->get_canopy_position_jitter();
				Vector3 center = canopy_anchors[anchor_index] + Vector3(_random_range(random, -jitter, jitter), _random_range(random, -jitter * 0.5, jitter), _random_range(random, -jitter, jitter));
				real_t radius = _random_range(random, generation_profile->get_canopy_radius_min(), generation_profile->get_canopy_radius_max());
				real_t vertical_scale = generation_profile->get_canopy_vertical_scale();
				if (archetype == OpenWorldTreeGenerationProfile::ARCHETYPE_TROPICAL_BROADLEAF) {
					radius *= 1.18;
				}
				if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_UMBRELLA) {
					center.y = Math::lerp(center.y, tree_height * (real_t)0.9, (real_t)0.65);
					vertical_scale *= 0.55;
					radius *= 1.12;
				} else if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_CONICAL) {
					const real_t height_ratio = CLAMP(center.y / tree_height, (real_t)0.0, (real_t)1.0);
					radius *= Math::lerp((real_t)1.25, (real_t)0.55, height_ratio);
					vertical_scale *= 0.9;
				} else if (crown_shape == OpenWorldTreeGenerationProfile::CROWN_TIERED) {
					const real_t tier_height = MAX((real_t)0.25, generation_profile->get_canopy_radius_min() * 0.7);
					center.y = Math::round(center.y / tier_height) * tier_height;
					vertical_scale *= 0.72;
				}
				if (archetype == OpenWorldTreeGenerationProfile::ARCHETYPE_MANGROVE) {
					radius *= 0.82;
				}
				const Vector3 scale(radius * _random_range(random, 0.8, 1.2), radius * vertical_scale * _random_range(random, 0.85, 1.15), radius * _random_range(random, 0.8, 1.2));
				_random_range(random, 0.82, 1.0);
				_random_range(random, 0.88, 1.0);
				_random_range(random, 0.82, 1.0);
				if (blob_index % foliage_stride == 0) {
					const real_t height_weight = CLAMP(center.y / MAX((real_t)0.001, tree_height), (real_t)0.0, (real_t)1.0);
					const Color wind_data(height_weight, 0.72, 0.65, wind_phase);
					_append_canopy_blob(foliage_surface, center, scale, generation_profile->get_canopy_roughness(), seed ^ (blob_index * 7919), wind_data, p_lod > 0);
				}
			}
		}
		foliage_surface->index();
		foliage_surface->generate_tangents();
		foliage_surface->commit(lod_mesh);
	}

	lod_mesh->set_name(vformat("StylizedTree_%d_%d_LOD%d", (int)archetype, seed, p_lod));
	return lod_mesh;
}

void OpenWorldTreeGenerator3D::generate_tree() {
	if (generation_profile.is_null()) {
		clear_generated_tree();
		return;
	}
	generated_lod_meshes[0] = _generate_lod_mesh(0, &generated_support_graph);
	generated_lod_meshes[1] = generate_lod1 ? _generate_lod_mesh(1) : Ref<ArrayMesh>();
	generated_lod_meshes[2] = generate_lod2 ? _generate_lod_mesh(2) : Ref<ArrayMesh>();
	generated_mesh = generated_lod_meshes[0];
	_apply_materials();
	_update_preview_mesh();
	emit_signal(SNAME("tree_generated"), generated_mesh);
	update_gizmos();
}

void OpenWorldTreeGenerator3D::randomize_seed() {
	RandomPCG random;
	random.randomize();
	set_seed((int)random.rand());
	if (!auto_generate) {
		generate_tree();
	}
}

void OpenWorldTreeGenerator3D::clear_generated_tree() {
	generated_mesh.unref();
	generated_support_graph.unref();
	for (int lod = 0; lod < 3; lod++) {
		generated_lod_meshes[lod].unref();
	}
	set_mesh(Ref<Mesh>());
	update_gizmos();
}

Ref<OpenWorldTreeVariant> OpenWorldTreeGenerator3D::create_baked_variant() const {
	ERR_FAIL_COND_V_MSG(generated_lod_meshes[0].is_null(), Ref<OpenWorldTreeVariant>(), "Generate a tree before baking a variant.");
	Ref<OpenWorldTreeVariant> variant;
	variant.instantiate();
	variant->set_variant_name(vformat("Stylized Tree %d", seed));
	variant->set_source_seed(seed);
	variant->set_lod0_mesh(generated_lod_meshes[0]->duplicate(true));
	if (generated_lod_meshes[1].is_valid()) {
		variant->set_lod1_mesh(generated_lod_meshes[1]->duplicate(true));
	}
	if (generated_lod_meshes[2].is_valid()) {
		variant->set_lod2_mesh(generated_lod_meshes[2]->duplicate(true));
	}
	variant->set_lod1_distance(lod1_distance);
	variant->set_lod2_distance(lod2_distance);
	variant->set_max_distance(max_distance);
	variant->set_collision_radius(generation_profile.is_valid() ? generation_profile->get_trunk_base_radius() : 0.5);
	variant->set_collision_height(generation_profile.is_valid() ? generation_profile->get_tree_height() : 2.0);
	if (generated_support_graph.is_valid()) variant->set_support_graph(generated_support_graph->duplicate(true));
	return variant;
}

void OpenWorldTreeGenerator3D::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE && auto_generate && generated_mesh.is_null()) {
		generate_tree();
	}
}

OpenWorldTreeGenerator3D::OpenWorldTreeGenerator3D() {
}

OpenWorldTreeGenerator3D::~OpenWorldTreeGenerator3D() {
	if (generation_profile.is_valid()) {
		generation_profile->disconnect_changed(callable_mp(this, &OpenWorldTreeGenerator3D::_profile_changed));
	}
}

void OpenWorldTreeGenerator3D::_bind_methods() {
	BIND_ENUM_CONSTANT(PREVIEW_LOD0);
	BIND_ENUM_CONSTANT(PREVIEW_LOD1);
	BIND_ENUM_CONSTANT(PREVIEW_LOD2);

	ClassDB::bind_method(D_METHOD("set_generate_lod1", "enabled"), &OpenWorldTreeGenerator3D::set_generate_lod1);
	ClassDB::bind_method(D_METHOD("is_generate_lod1_enabled"), &OpenWorldTreeGenerator3D::is_generate_lod1_enabled);
	ClassDB::bind_method(D_METHOD("set_generate_lod2", "enabled"), &OpenWorldTreeGenerator3D::set_generate_lod2);
	ClassDB::bind_method(D_METHOD("is_generate_lod2_enabled"), &OpenWorldTreeGenerator3D::is_generate_lod2_enabled);
	ClassDB::bind_method(D_METHOD("set_lod1_quality", "quality"), &OpenWorldTreeGenerator3D::set_lod1_quality);
	ClassDB::bind_method(D_METHOD("get_lod1_quality"), &OpenWorldTreeGenerator3D::get_lod1_quality);
	ClassDB::bind_method(D_METHOD("set_lod2_quality", "quality"), &OpenWorldTreeGenerator3D::set_lod2_quality);
	ClassDB::bind_method(D_METHOD("get_lod2_quality"), &OpenWorldTreeGenerator3D::get_lod2_quality);
	ClassDB::bind_method(D_METHOD("set_lod1_distance", "distance"), &OpenWorldTreeGenerator3D::set_lod1_distance);
	ClassDB::bind_method(D_METHOD("get_lod1_distance"), &OpenWorldTreeGenerator3D::get_lod1_distance);
	ClassDB::bind_method(D_METHOD("set_lod2_distance", "distance"), &OpenWorldTreeGenerator3D::set_lod2_distance);
	ClassDB::bind_method(D_METHOD("get_lod2_distance"), &OpenWorldTreeGenerator3D::get_lod2_distance);
	ClassDB::bind_method(D_METHOD("set_max_distance", "distance"), &OpenWorldTreeGenerator3D::set_max_distance);
	ClassDB::bind_method(D_METHOD("get_max_distance"), &OpenWorldTreeGenerator3D::get_max_distance);
	ClassDB::bind_method(D_METHOD("set_preview_lod", "lod"), &OpenWorldTreeGenerator3D::set_preview_lod);
	ClassDB::bind_method(D_METHOD("get_preview_lod"), &OpenWorldTreeGenerator3D::get_preview_lod);
	ClassDB::bind_method(D_METHOD("set_wind_enabled", "enabled"), &OpenWorldTreeGenerator3D::set_wind_enabled);
	ClassDB::bind_method(D_METHOD("is_wind_enabled"), &OpenWorldTreeGenerator3D::is_wind_enabled);
	ClassDB::bind_method(D_METHOD("set_wind_strength", "strength"), &OpenWorldTreeGenerator3D::set_wind_strength);
	ClassDB::bind_method(D_METHOD("get_wind_strength"), &OpenWorldTreeGenerator3D::get_wind_strength);
	ClassDB::bind_method(D_METHOD("set_wind_speed", "speed"), &OpenWorldTreeGenerator3D::set_wind_speed);
	ClassDB::bind_method(D_METHOD("get_wind_speed"), &OpenWorldTreeGenerator3D::get_wind_speed);
	ClassDB::bind_method(D_METHOD("set_wind_gust_strength", "strength"), &OpenWorldTreeGenerator3D::set_wind_gust_strength);
	ClassDB::bind_method(D_METHOD("get_wind_gust_strength"), &OpenWorldTreeGenerator3D::get_wind_gust_strength);
	ClassDB::bind_method(D_METHOD("set_wind_direction", "direction"), &OpenWorldTreeGenerator3D::set_wind_direction);
	ClassDB::bind_method(D_METHOD("get_wind_direction"), &OpenWorldTreeGenerator3D::get_wind_direction);
	ClassDB::bind_method(D_METHOD("set_wind_trunk_color", "color"), &OpenWorldTreeGenerator3D::set_wind_trunk_color);
	ClassDB::bind_method(D_METHOD("get_wind_trunk_color"), &OpenWorldTreeGenerator3D::get_wind_trunk_color);
	ClassDB::bind_method(D_METHOD("set_wind_foliage_color", "color"), &OpenWorldTreeGenerator3D::set_wind_foliage_color);
	ClassDB::bind_method(D_METHOD("get_wind_foliage_color"), &OpenWorldTreeGenerator3D::get_wind_foliage_color);
	ClassDB::bind_method(D_METHOD("get_generated_lod_mesh", "lod"), &OpenWorldTreeGenerator3D::get_generated_lod_mesh);
	ClassDB::bind_method(D_METHOD("get_lod_statistics", "lod"), &OpenWorldTreeGenerator3D::get_lod_statistics);
	ClassDB::bind_method(D_METHOD("get_generated_support_graph"), &OpenWorldTreeGenerator3D::get_generated_support_graph);


	ClassDB::bind_method(D_METHOD("set_generation_profile", "profile"), &OpenWorldTreeGenerator3D::set_generation_profile);
	ClassDB::bind_method(D_METHOD("get_generation_profile"), &OpenWorldTreeGenerator3D::get_generation_profile);
	ClassDB::bind_method(D_METHOD("set_seed", "seed"), &OpenWorldTreeGenerator3D::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &OpenWorldTreeGenerator3D::get_seed);
	ClassDB::bind_method(D_METHOD("set_auto_generate", "enabled"), &OpenWorldTreeGenerator3D::set_auto_generate);
	ClassDB::bind_method(D_METHOD("is_auto_generate"), &OpenWorldTreeGenerator3D::is_auto_generate);
	ClassDB::bind_method(D_METHOD("set_trunk_material", "material"), &OpenWorldTreeGenerator3D::set_trunk_material);
	ClassDB::bind_method(D_METHOD("get_trunk_material"), &OpenWorldTreeGenerator3D::get_trunk_material);
	ClassDB::bind_method(D_METHOD("set_foliage_material", "material"), &OpenWorldTreeGenerator3D::set_foliage_material);
	ClassDB::bind_method(D_METHOD("get_foliage_material"), &OpenWorldTreeGenerator3D::get_foliage_material);
	ClassDB::bind_method(D_METHOD("generate_tree"), &OpenWorldTreeGenerator3D::generate_tree);
	ClassDB::bind_method(D_METHOD("randomize_seed"), &OpenWorldTreeGenerator3D::randomize_seed);
	ClassDB::bind_method(D_METHOD("clear_generated_tree"), &OpenWorldTreeGenerator3D::clear_generated_tree);
	ClassDB::bind_method(D_METHOD("get_generated_mesh"), &OpenWorldTreeGenerator3D::get_generated_mesh);
	ClassDB::bind_method(D_METHOD("create_baked_variant"), &OpenWorldTreeGenerator3D::create_baked_variant);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "generation_profile", PROPERTY_HINT_RESOURCE_TYPE, OpenWorldTreeGenerationProfile::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_generation_profile", "get_generation_profile");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate"), "set_auto_generate", "is_auto_generate");
	ADD_GROUP("LOD Generation", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "generate_lod1"), "set_generate_lod1", "is_generate_lod1_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "generate_lod2"), "set_generate_lod2", "is_generate_lod2_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_quality", PROPERTY_HINT_RANGE, "0.1,1,0.01"), "set_lod1_quality", "get_lod1_quality");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_quality", PROPERTY_HINT_RANGE, "0.05,0.75,0.01"), "set_lod2_quality", "get_lod2_quality");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod1_distance", "get_lod1_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod2_distance", "get_lod2_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_max_distance", "get_max_distance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "preview_lod", PROPERTY_HINT_ENUM, "LOD0,LOD1,LOD2"), "set_preview_lod", "get_preview_lod");

	ADD_GROUP("Wind Preview", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wind_enabled"), "set_wind_enabled", "is_wind_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_strength", PROPERTY_HINT_RANGE, "0,2,0.01"), "set_wind_strength", "get_wind_strength");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_speed", PROPERTY_HINT_RANGE, "0,8,0.05"), "set_wind_speed", "get_wind_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_gust_strength", PROPERTY_HINT_RANGE, "0,2,0.01"), "set_wind_gust_strength", "get_wind_gust_strength");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "wind_direction"), "set_wind_direction", "get_wind_direction");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "wind_trunk_color"), "set_wind_trunk_color", "get_wind_trunk_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "wind_foliage_color"), "set_wind_foliage_color", "get_wind_foliage_color");

	ADD_GROUP("Materials", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "trunk_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_trunk_material", "get_trunk_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "foliage_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_foliage_material", "get_foliage_material");

	ADD_SIGNAL(MethodInfo("tree_generated", PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "ArrayMesh")));
}
