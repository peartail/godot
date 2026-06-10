/**************************************************************************/
/*  open_world_terrain_3d.cpp                                             */
/**************************************************************************/

#include "open_world_terrain_3d.h"

#include "core/math/geometry_3d.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/resources/3d/world_3d.h"
#include "scene/resources/shader.h"
#include "servers/rendering/rendering_server.h"

void OpenWorldTerrain3D::_terrain_data_changed() {
	_rebuild_tiles();
}

void OpenWorldTerrain3D::_ensure_data() {
	if (terrain_data.is_valid()) {
		return;
	}

	terrain_data.instantiate();
	terrain_data->connect_changed(callable_mp(this, &OpenWorldTerrain3D::_terrain_data_changed));
}

Vector3 OpenWorldTerrain3D::_get_local_height_position(int p_x, int p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), Vector3());
	const int resolution = terrain_data->get_heightmap_resolution();
	ERR_FAIL_INDEX_V(p_x, resolution, Vector3());
	ERR_FAIL_INDEX_V(p_y, resolution, Vector3());

	const real_t world_size = terrain_data->get_world_size();
	const real_t half_size = world_size * 0.5;
	const real_t u = resolution > 1 ? (real_t)p_x / (real_t)(resolution - 1) : 0.0;
	const real_t v = resolution > 1 ? (real_t)p_y / (real_t)(resolution - 1) : 0.0;
	return Vector3(
			u * world_size - half_size,
			terrain_data->get_height(p_x, p_y) * terrain_data->get_height_scale(),
			v * world_size - half_size);
}

real_t OpenWorldTerrain3D::_sample_height_nearest(int p_x, int p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), 0.0);
	const int resolution = terrain_data->get_heightmap_resolution();
	const int x = CLAMP(p_x, 0, resolution - 1);
	const int y = CLAMP(p_y, 0, resolution - 1);
	return terrain_data->get_height(x, y);
}

real_t OpenWorldTerrain3D::_sample_height_bilinear(real_t p_x, real_t p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), 0.0);
	const int resolution = terrain_data->get_heightmap_resolution();
	const real_t clamped_x = CLAMP(p_x, (real_t)0.0, (real_t)resolution - 1.0);
	const real_t clamped_y = CLAMP(p_y, (real_t)0.0, (real_t)resolution - 1.0);
	const int x0 = CLAMP(Math::floor(clamped_x), 0, resolution - 1);
	const int y0 = CLAMP(Math::floor(clamped_y), 0, resolution - 1);
	const int x1 = CLAMP(x0 + 1, 0, resolution - 1);
	const int y1 = CLAMP(y0 + 1, 0, resolution - 1);
	const real_t tx = clamped_x - (real_t)x0;
	const real_t ty = clamped_y - (real_t)y0;

	const real_t h00 = _sample_height_nearest(x0, y0);
	const real_t h10 = _sample_height_nearest(x1, y0);
	const real_t h01 = _sample_height_nearest(x0, y1);
	const real_t h11 = _sample_height_nearest(x1, y1);
	const real_t h0 = Math::lerp(h00, h10, tx);
	const real_t h1 = Math::lerp(h01, h11, tx);
	return Math::lerp(h0, h1, ty);
}

real_t OpenWorldTerrain3D::_get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), 0.0);
	const int resolution = terrain_data->get_heightmap_resolution();
	real_t total = 0.0;
	int count = 0;

	for (int y = MAX(0, p_y - 1); y <= MIN(resolution - 1, p_y + 1); y++) {
		for (int x = MAX(0, p_x - 1); x <= MIN(resolution - 1, p_x + 1); x++) {
			total += p_source_heights[terrain_data->get_height_index(x, y)];
			count++;
		}
	}

	return count > 0 ? total / (real_t)count : 0.0;
}

real_t OpenWorldTerrain3D::_sample_value_noise(real_t p_x, real_t p_y, int p_seed) const {
	const int x0 = Math::floor(p_x);
	const int y0 = Math::floor(p_y);
	const int x1 = x0 + 1;
	const int y1 = y0 + 1;
	const real_t tx = p_x - (real_t)x0;
	const real_t ty = p_y - (real_t)y0;

	const auto hash_to_unit = [p_seed](int p_x_hash, int p_y_hash) -> real_t {
		uint32_t hash = (uint32_t)p_seed;
		hash ^= (uint32_t)p_x_hash * 374761393U;
		hash = (hash << 13U) ^ hash;
		hash ^= (uint32_t)p_y_hash * 668265263U;
		hash *= 1274126177U;
		return (real_t)(hash & 0x00FFFFFFU) / (real_t)0x00FFFFFFU;
	};

	const real_t sx = tx * tx * (3.0 - 2.0 * tx);
	const real_t sy = ty * ty * (3.0 - 2.0 * ty);
	const real_t a = Math::lerp(hash_to_unit(x0, y0), hash_to_unit(x1, y0), sx);
	const real_t b = Math::lerp(hash_to_unit(x0, y1), hash_to_unit(x1, y1), sx);
	return Math::lerp(a, b, sy);
}

Ref<ArrayMesh> OpenWorldTerrain3D::_build_tile_mesh(int p_origin_x, int p_origin_y, int p_quad_width, int p_quad_height) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), Ref<ArrayMesh>());
	ERR_FAIL_COND_V(p_quad_width <= 0 || p_quad_height <= 0, Ref<ArrayMesh>());

	const int resolution = terrain_data->get_heightmap_resolution();
	const real_t world_size = terrain_data->get_world_size();
	const real_t half_size = world_size * 0.5;
	const real_t texel_world_size = world_size / (real_t)(resolution - 1);
	const int vertex_width = p_quad_width + 1;
	const int vertex_height = p_quad_height + 1;

	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedVector2Array uvs;
	PackedInt32Array indices;
	vertices.resize(vertex_width * vertex_height);
	normals.resize(vertex_width * vertex_height);
	uvs.resize(vertex_width * vertex_height);
	indices.resize(p_quad_width * p_quad_height * 6);

	for (int y = 0; y < vertex_height; y++) {
		for (int x = 0; x < vertex_width; x++) {
			const int index = y * vertex_width + x;
			const int terrain_x = p_origin_x + x;
			const int terrain_y = p_origin_y + y;
			vertices.set(index, Vector3(
					(real_t)terrain_x * texel_world_size - half_size,
					0.0,
					(real_t)terrain_y * texel_world_size - half_size));
			normals.set(index, Vector3(0.0, 1.0, 0.0));
			uvs.set(index, Vector2((real_t)x / (real_t)p_quad_width, (real_t)y / (real_t)p_quad_height));
		}
	}

	int write_index = 0;
	for (int y = 0; y < p_quad_height; y++) {
		for (int x = 0; x < p_quad_width; x++) {
			const int top_left = y * vertex_width + x;
			const int top_right = top_left + 1;
			const int bottom_left = top_left + vertex_width;
			const int bottom_right = bottom_left + 1;
			indices.set(write_index++, top_left);
			indices.set(write_index++, top_right);
			indices.set(write_index++, bottom_left);
			indices.set(write_index++, top_right);
			indices.set(write_index++, bottom_right);
			indices.set(write_index++, bottom_left);
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

Ref<Image> OpenWorldTerrain3D::_build_tile_height_image(int p_origin_x, int p_origin_y, int p_quad_width, int p_quad_height) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), Ref<Image>());
	const int image_width = p_quad_width + 1;
	const int image_height = p_quad_height + 1;
	Ref<Image> image = Image::create_empty(image_width, image_height, false, Image::FORMAT_RF);
	const PackedFloat32Array &heights = terrain_data->get_height_data_ref();

	for (int y = 0; y < image_height; y++) {
		for (int x = 0; x < image_width; x++) {
			const int terrain_x = p_origin_x + x;
			const int terrain_y = p_origin_y + y;
			const int index = terrain_data->get_height_index(terrain_x, terrain_y);
			const real_t height = index < heights.size() ? CLAMP(heights[index], (real_t)0.0, (real_t)1.0) : 0.0;
			image->set_pixel(x, y, Color(height, 0.0, 0.0, 1.0));
		}
	}
	return image;
}

void OpenWorldTerrain3D::_clear_tiles() {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (TerrainTile &tile : tiles) {
		if (tile.instance.is_valid()) {
			rs->free_rid(tile.instance);
		}
	}
	tiles.clear();
}

void OpenWorldTerrain3D::_sync_tile_instances() {
	RenderingServer *rs = RenderingServer::get_singleton();
	const RID scenario = is_inside_tree() && get_world_3d().is_valid() ? get_world_3d()->get_scenario() : RID();
	const Transform3D global_transform = get_global_transform();
	for (TerrainTile &tile : tiles) {
		if (!tile.instance.is_valid()) {
			tile.instance = rs->instance_create();
		}
		rs->instance_set_base(tile.instance, tile.mesh.is_valid() ? tile.mesh->get_rid() : RID());
		rs->instance_set_scenario(tile.instance, scenario);
		rs->instance_set_transform(tile.instance, global_transform);
	}
	_sync_tile_materials();
}

void OpenWorldTerrain3D::_sync_tile_materials() {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (TerrainTile &tile : tiles) {
		if (!tile.instance.is_valid()) {
			continue;
		}
		const Ref<Material> material = use_builtin_displacement_material ? Ref<Material>(tile.material) : terrain_material;
		rs->instance_geometry_set_material_override(tile.instance, material.is_valid() ? material->get_rid() : RID());
	}
}

void OpenWorldTerrain3D::_rebuild_tiles() {
	_ensure_data();
	_clear_tiles();
	set_mesh(Ref<Mesh>());

	const int resolution = terrain_data->get_heightmap_resolution();
	const int quad_count = resolution - 1;
	const int active_tile_size = CLAMP(tile_size, 1, 4096);

	for (int origin_y = 0; origin_y < quad_count; origin_y += active_tile_size) {
		for (int origin_x = 0; origin_x < quad_count; origin_x += active_tile_size) {
			TerrainTile tile;
			tile.origin_x = origin_x;
			tile.origin_y = origin_y;
			tile.quad_width = MIN(active_tile_size, quad_count - origin_x);
			tile.quad_height = MIN(active_tile_size, quad_count - origin_y);
			tile.mesh = _build_tile_mesh(tile.origin_x, tile.origin_y, tile.quad_width, tile.quad_height);
			tile.height_image = _build_tile_height_image(tile.origin_x, tile.origin_y, tile.quad_width, tile.quad_height);
			tile.height_texture = ImageTexture::create_from_image(tile.height_image);
			_update_tile_material(tile);
			tiles.push_back(tile);
		}
	}

	_sync_tile_instances();
	notify_property_list_changed();
	update_gizmos();
}

void OpenWorldTerrain3D::_rebuild_height_textures() {
	for (TerrainTile &tile : tiles) {
		tile.height_image = _build_tile_height_image(tile.origin_x, tile.origin_y, tile.quad_width, tile.quad_height);
		if (tile.height_texture.is_null() || tile.height_texture->get_width() != tile.height_image->get_width() || tile.height_texture->get_height() != tile.height_image->get_height()) {
			tile.height_texture = ImageTexture::create_from_image(tile.height_image);
		} else {
			tile.height_texture->update(tile.height_image);
		}
		_update_tile_material(tile);
	}
	_sync_tile_materials();
}

void OpenWorldTerrain3D::_refresh_height_texture_region(int p_min_x, int p_min_y, int p_max_x, int p_max_y) {
	_ensure_data();
	const int resolution = terrain_data->get_heightmap_resolution();
	if (resolution < 1) {
		return;
	}
	if (tiles.is_empty()) {
		_rebuild_tiles();
		return;
	}

	const int min_x = CLAMP(p_min_x - 1, 0, resolution - 1);
	const int min_y = CLAMP(p_min_y - 1, 0, resolution - 1);
	const int max_x = CLAMP(p_max_x + 1, 0, resolution - 1);
	const int max_y = CLAMP(p_max_y + 1, 0, resolution - 1);
	if (min_x > max_x || min_y > max_y) {
		return;
	}

	const PackedFloat32Array &heights = terrain_data->get_height_data_ref();
	for (TerrainTile &tile : tiles) {
		const int tile_min_x = tile.origin_x;
		const int tile_min_y = tile.origin_y;
		const int tile_max_x = tile.origin_x + tile.quad_width;
		const int tile_max_y = tile.origin_y + tile.quad_height;
		const int dirty_min_x = MAX(min_x, tile_min_x);
		const int dirty_min_y = MAX(min_y, tile_min_y);
		const int dirty_max_x = MIN(max_x, tile_max_x);
		const int dirty_max_y = MIN(max_y, tile_max_y);
		if (dirty_min_x > dirty_max_x || dirty_min_y > dirty_max_y) {
			continue;
		}

		if (tile.height_image.is_null() || tile.height_texture.is_null()) {
			tile.height_image = _build_tile_height_image(tile.origin_x, tile.origin_y, tile.quad_width, tile.quad_height);
			tile.height_texture = ImageTexture::create_from_image(tile.height_image);
			_update_tile_material(tile);
		} else {
			const int patch_width = dirty_max_x - dirty_min_x + 1;
			const int patch_height = dirty_max_y - dirty_min_y + 1;
			Ref<Image> patch_image = Image::create_empty(patch_width, patch_height, false, Image::FORMAT_RF);
			for (int y = dirty_min_y; y <= dirty_max_y; y++) {
				for (int x = dirty_min_x; x <= dirty_max_x; x++) {
					const int index = terrain_data->get_height_index(x, y);
					const real_t height = index < heights.size() ? CLAMP(heights[index], (real_t)0.0, (real_t)1.0) : 0.0;
					tile.height_image->set_pixel(x - tile.origin_x, y - tile.origin_y, Color(height, 0.0, 0.0, 1.0));
					patch_image->set_pixel(x - dirty_min_x, y - dirty_min_y, Color(height, 0.0, 0.0, 1.0));
				}
			}
			tile.height_texture->update_region(patch_image, Point2i(dirty_min_x - tile.origin_x, dirty_min_y - tile.origin_y));
		}
	}
}

void OpenWorldTerrain3D::_update_tile_material(TerrainTile &r_tile) {
	_ensure_data();
	if (!use_builtin_displacement_material) {
		return;
	}

	if (displacement_shader.is_null()) {
		displacement_shader.instantiate();
		displacement_shader->set_code(_get_builtin_displacement_shader_code());
	}
	if (r_tile.material.is_null()) {
		r_tile.material.instantiate();
		r_tile.material->set_shader(displacement_shader);
	}

	r_tile.material->set_shader_parameter("height_texture", r_tile.height_texture);
	if (low_texture.is_valid() && low_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("low_albedo", low_texture);
	} else {
		r_tile.material->set_shader_parameter("low_albedo", Variant());
	}
	if (mid_texture.is_valid() && mid_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("mid_albedo", mid_texture);
	} else {
		r_tile.material->set_shader_parameter("mid_albedo", Variant());
	}
	if (high_texture.is_valid() && high_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("high_albedo", high_texture);
	} else {
		r_tile.material->set_shader_parameter("high_albedo", Variant());
	}
	r_tile.material->set_shader_parameter("height_scale", terrain_data->get_height_scale());
	r_tile.material->set_shader_parameter("height_texel_size", Vector2(1.0 / (real_t)(r_tile.quad_width + 1), 1.0 / (real_t)(r_tile.quad_height + 1)));
	r_tile.material->set_shader_parameter("height_world_texel_size", terrain_data->get_world_size() / MAX(1.0, (real_t)terrain_data->get_heightmap_resolution() - 1.0));
	r_tile.material->set_shader_parameter("low_color", low_color);
	r_tile.material->set_shader_parameter("mid_color", mid_color);
	r_tile.material->set_shader_parameter("high_color", high_color);
	r_tile.material->set_shader_parameter("low_height", low_height);
	r_tile.material->set_shader_parameter("high_height", high_height);
	r_tile.material->set_shader_parameter("blend_width", blend_width);
	r_tile.material->set_shader_parameter("texture_scale", texture_scale);
	r_tile.material->set_shader_parameter("low_texture_scale", low_texture_scale);
	r_tile.material->set_shader_parameter("mid_texture_scale", mid_texture_scale);
	r_tile.material->set_shader_parameter("high_texture_scale", high_texture_scale);
	r_tile.material->set_shader_parameter("triplanar_sharpness", triplanar_sharpness);
	r_tile.material->set_shader_parameter("slope_start", slope_start);
	r_tile.material->set_shader_parameter("slope_end", slope_end);
	r_tile.material->set_shader_parameter("slope_high_strength", slope_high_strength);
	r_tile.material->set_shader_parameter("splat_debug_mode", (int)splat_debug_mode);
}

void OpenWorldTerrain3D::_update_materials() {
	for (TerrainTile &tile : tiles) {
		_update_tile_material(tile);
	}
	_sync_tile_materials();
}

String OpenWorldTerrain3D::_get_builtin_displacement_shader_code() {
	return R"(
shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D height_texture : repeat_disable, filter_linear;
uniform sampler2D low_albedo : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D mid_albedo : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D high_albedo : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform float height_scale = 128.0;
uniform vec2 height_texel_size = vec2(0.00390625, 0.00390625);
uniform float height_world_texel_size = 4.0;
uniform vec4 low_color : source_color = vec4(0.22, 0.38, 0.18, 1.0);
uniform vec4 mid_color : source_color = vec4(0.42, 0.34, 0.22, 1.0);
uniform vec4 high_color : source_color = vec4(0.78, 0.78, 0.72, 1.0);
uniform float low_height = 0.25;
uniform float high_height = 0.7;
uniform float blend_width = 0.15;
uniform float texture_scale = 0.08;
uniform float low_texture_scale = 0.08;
uniform float mid_texture_scale = 0.08;
uniform float high_texture_scale = 0.08;
uniform float triplanar_sharpness = 4.0;
uniform float slope_start = 0.35;
uniform float slope_end = 0.75;
uniform float slope_high_strength = 1.0;
uniform int splat_debug_mode = 0;

varying float terrain_height;
varying vec3 terrain_world_pos;
varying vec3 terrain_world_normal;

void vertex() {
	float height = texture(height_texture, UV).r;
	float left_height = texture(height_texture, UV + vec2(-height_texel_size.x, 0.0)).r;
	float right_height = texture(height_texture, UV + vec2(height_texel_size.x, 0.0)).r;
	float back_height = texture(height_texture, UV + vec2(0.0, -height_texel_size.y)).r;
	float front_height = texture(height_texture, UV + vec2(0.0, height_texel_size.y)).r;

	VERTEX.y += height * height_scale;
	NORMAL = normalize(vec3((left_height - right_height) * height_scale, 2.0 * height_world_texel_size, (back_height - front_height) * height_scale));
	terrain_height = height;
	terrain_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	terrain_world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
}

vec4 triplanar_sample(sampler2D tex, vec3 world_pos, vec3 world_normal, float layer_texture_scale) {
	vec3 blend = pow(abs(world_normal), vec3(max(triplanar_sharpness, 0.001)));
	blend /= max(dot(blend, vec3(1.0)), 0.001);
	vec3 coord = world_pos * max(layer_texture_scale, 0.0001);
	vec4 x_sample = texture(tex, coord.yz);
	vec4 y_sample = texture(tex, coord.xz);
	vec4 z_sample = texture(tex, coord.xy);
	return x_sample * blend.x + y_sample * blend.y + z_sample * blend.z;
}

void fragment() {
	float safe_blend = max(blend_width, 0.001);
	float low_to_mid = smoothstep(low_height - safe_blend, low_height + safe_blend, terrain_height);
	float mid_to_high = smoothstep(high_height - safe_blend, high_height + safe_blend, terrain_height);
	float slope = 1.0 - clamp(abs(normalize(terrain_world_normal).y), 0.0, 1.0);
	float slope_high = smoothstep(slope_start, slope_end, slope) * clamp(slope_high_strength, 0.0, 1.0);

	float low_weight = 1.0 - low_to_mid;
	float mid_weight = low_to_mid * (1.0 - mid_to_high);
	float high_weight = max(mid_to_high, slope_high);
	float weight_sum = max(low_weight + mid_weight + high_weight, 0.001);
	low_weight /= weight_sum;
	mid_weight /= weight_sum;
	high_weight /= weight_sum;

	vec3 final_albedo = vec3(0.0);
	if (splat_debug_mode == 1) {
		final_albedo = vec3(terrain_height);
	} else if (splat_debug_mode == 2) {
		final_albedo = vec3(slope);
	} else if (splat_debug_mode == 3) {
		final_albedo = vec3(low_weight, mid_weight, high_weight);
	} else {
		vec4 low_layer = triplanar_sample(low_albedo, terrain_world_pos, terrain_world_normal, low_texture_scale) * low_color;
		vec4 mid_layer = triplanar_sample(mid_albedo, terrain_world_pos, terrain_world_normal, mid_texture_scale) * mid_color;
		vec4 high_layer = triplanar_sample(high_albedo, terrain_world_pos, terrain_world_normal, high_texture_scale) * high_color;
		vec4 albedo = low_layer * low_weight + mid_layer * mid_weight + high_layer * high_weight;
		final_albedo = albedo.rgb;
	}

	ALBEDO = final_albedo;
	ROUGHNESS = 0.92;
}
)";
}

void OpenWorldTerrain3D::set_terrain_data(const Ref<OpenWorldTerrainData> &p_terrain_data) {
	if (terrain_data == p_terrain_data) {
		return;
	}
	if (terrain_data.is_valid()) {
		terrain_data->disconnect_changed(callable_mp(this, &OpenWorldTerrain3D::_terrain_data_changed));
	}

	terrain_data = p_terrain_data;

	if (terrain_data.is_valid()) {
		terrain_data->connect_changed(callable_mp(this, &OpenWorldTerrain3D::_terrain_data_changed));
	}

	rebuild();
}

void OpenWorldTerrain3D::set_patch_resolution(int p_patch_resolution) {
	patch_resolution = CLAMP(p_patch_resolution, 1, 4096);
	_rebuild_tiles();
	_update_materials();
}

void OpenWorldTerrain3D::set_tile_size(int p_tile_size) {
	tile_size = CLAMP(p_tile_size, 1, 4096);
	_rebuild_tiles();
}

void OpenWorldTerrain3D::set_world_size(real_t p_world_size) {
	_ensure_data();
	terrain_data->set_world_size(p_world_size);
	_rebuild_tiles();
}

real_t OpenWorldTerrain3D::get_world_size() const {
	return terrain_data.is_valid() ? terrain_data->get_world_size() : 1024.0;
}

void OpenWorldTerrain3D::set_height_scale(real_t p_height_scale) {
	_ensure_data();
	terrain_data->set_height_scale(p_height_scale);
	_update_materials();
}

real_t OpenWorldTerrain3D::get_height_scale() const {
	return terrain_data.is_valid() ? terrain_data->get_height_scale() : 128.0;
}

void OpenWorldTerrain3D::set_use_builtin_displacement_material(bool p_use) {
	use_builtin_displacement_material = p_use;
	_update_materials();
}

void OpenWorldTerrain3D::set_terrain_material(const Ref<Material> &p_material) {
	terrain_material = p_material;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_texture(const Ref<Texture2D> &p_texture) {
	low_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_texture(const Ref<Texture2D> &p_texture) {
	mid_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_high_texture(const Ref<Texture2D> &p_texture) {
	high_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_color(const Color &p_color) {
	low_color = p_color;
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_color(const Color &p_color) {
	mid_color = p_color;
	_update_materials();
}

void OpenWorldTerrain3D::set_high_color(const Color &p_color) {
	high_color = p_color;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_height(real_t p_height) {
	low_height = CLAMP(p_height, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_high_height(real_t p_height) {
	high_height = CLAMP(p_height, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_blend_width(real_t p_width) {
	blend_width = MAX((real_t)0.001, p_width);
	_update_materials();
}

void OpenWorldTerrain3D::set_texture_scale(real_t p_scale) {
	texture_scale = MAX((real_t)0.0001, p_scale);
	low_texture_scale = texture_scale;
	mid_texture_scale = texture_scale;
	high_texture_scale = texture_scale;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_texture_scale(real_t p_scale) {
	low_texture_scale = MAX((real_t)0.0001, p_scale);
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_texture_scale(real_t p_scale) {
	mid_texture_scale = MAX((real_t)0.0001, p_scale);
	_update_materials();
}

void OpenWorldTerrain3D::set_high_texture_scale(real_t p_scale) {
	high_texture_scale = MAX((real_t)0.0001, p_scale);
	_update_materials();
}

void OpenWorldTerrain3D::set_triplanar_sharpness(real_t p_sharpness) {
	triplanar_sharpness = MAX((real_t)0.001, p_sharpness);
	_update_materials();
}

void OpenWorldTerrain3D::set_slope_start(real_t p_slope) {
	slope_start = CLAMP(p_slope, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_slope_end(real_t p_slope) {
	slope_end = CLAMP(p_slope, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_slope_high_strength(real_t p_strength) {
	slope_high_strength = CLAMP(p_strength, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_splat_debug_mode(SplatDebugMode p_mode) {
	splat_debug_mode = p_mode;
	_update_materials();
}

void OpenWorldTerrain3D::set_flatten_height(real_t p_height) {
	flatten_height = CLAMP(p_height, (real_t)0.0, (real_t)1.0);
}

void OpenWorldTerrain3D::set_show_debug_gizmo(bool p_show) {
	show_debug_gizmo = p_show;
	update_gizmos();
}

void OpenWorldTerrain3D::reset_flat_terrain(real_t p_normalized_height) {
	_ensure_data();
	terrain_data->fill_flat(p_normalized_height);
}

void OpenWorldTerrain3D::generate_random_terrain(int p_seed, real_t p_amplitude, real_t p_frequency, int p_octaves) {
	_ensure_data();
	PackedFloat32Array heights;
	const int resolution = terrain_data->get_heightmap_resolution();
	heights.resize(resolution * resolution);

	const real_t amplitude = MAX((real_t)0.0, p_amplitude);
	const real_t frequency = MAX((real_t)0.00001, p_frequency);
	const int octaves = CLAMP(p_octaves, 1, 12);
	real_t amplitude_sum = 0.0;
	for (int octave = 0; octave < octaves; octave++) {
		amplitude_sum += Math::pow((real_t)0.5, (real_t)octave);
	}

	for (int y = 0; y < resolution; y++) {
		for (int x = 0; x < resolution; x++) {
			real_t value = 0.0;
			real_t octave_amplitude = 1.0;
			real_t octave_frequency = frequency;
			for (int octave = 0; octave < octaves; octave++) {
				value += _sample_value_noise((real_t)x * octave_frequency, (real_t)y * octave_frequency, p_seed + octave * 1013) * octave_amplitude;
				octave_amplitude *= 0.5;
				octave_frequency *= 2.0;
			}
			const real_t normalized_value = CLAMP((value / MAX((real_t)0.0001, amplitude_sum)) * amplitude, (real_t)0.0, (real_t)1.0);
			heights.set(terrain_data->get_height_index(x, y), normalized_value);
		}
	}

	terrain_data->set_height_data(heights);
}

void OpenWorldTerrain3D::rebuild() {
	_ensure_data();
	_rebuild_tiles();
	_rebuild_height_textures();
	_update_materials();
}

void OpenWorldTerrain3D::apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	apply_brush_with_delta(p_world_position, p_radius, p_strength, p_operation);
}

Dictionary OpenWorldTerrain3D::apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	Dictionary delta;
	_ensure_data();

	const int resolution = terrain_data->get_heightmap_resolution();
	if (resolution < 2 || p_radius <= 0.0 || Math::is_zero_approx(p_strength)) {
		return delta;
	}

	PackedFloat32Array &heights = terrain_data->get_mutable_height_data();
	const PackedFloat32Array original_heights = p_operation == BRUSH_SMOOTH ? terrain_data->get_height_data() : PackedFloat32Array();
	const Vector3 local_position = get_global_transform().affine_inverse().xform(p_world_position);
	const real_t world_size = terrain_data->get_world_size();
	const real_t half_size = world_size * 0.5;
	const real_t texel_world_size = world_size / (real_t)(resolution - 1);
	const real_t center_x = (local_position.x + half_size) / texel_world_size;
	const real_t center_y = (local_position.z + half_size) / texel_world_size;
	const real_t radius_texels = p_radius / texel_world_size;
	const int min_x = CLAMP(Math::floor(center_x - radius_texels), 0, resolution - 1);
	const int max_x = CLAMP(Math::ceil(center_x + radius_texels), 0, resolution - 1);
	const int min_y = CLAMP(Math::floor(center_y - radius_texels), 0, resolution - 1);
	const int max_y = CLAMP(Math::ceil(center_y + radius_texels), 0, resolution - 1);

	PackedInt32Array changed_indices;
	PackedFloat32Array before_values;
	PackedFloat32Array after_values;

	for (int y = min_y; y <= max_y; y++) {
		for (int x = min_x; x <= max_x; x++) {
			const real_t dx = ((real_t)x - center_x) * texel_world_size;
			const real_t dy = ((real_t)y - center_y) * texel_world_size;
			const real_t distance = Math::sqrt(dx * dx + dy * dy);
			if (distance > p_radius) {
				continue;
			}

			const real_t falloff = 1.0 - CLAMP(distance / p_radius, (real_t)0.0, (real_t)1.0);
			const real_t weight = falloff * falloff * (3.0 - 2.0 * falloff);
			const int index = terrain_data->get_height_index(x, y);
			const real_t before = heights[index];
			real_t after = before;

			switch (p_operation) {
				case BRUSH_RAISE:
					after = before + p_strength * weight;
					break;
				case BRUSH_LOWER:
					after = before - p_strength * weight;
					break;
				case BRUSH_SMOOTH:
					after = Math::lerp(before, _get_average_neighbor_height(original_heights, x, y), CLAMP(p_strength * weight, (real_t)0.0, (real_t)1.0));
					break;
				case BRUSH_FLATTEN:
					after = Math::lerp(before, flatten_height, CLAMP(p_strength * weight, (real_t)0.0, (real_t)1.0));
					break;
			}

			after = CLAMP(after, (real_t)0.0, (real_t)1.0);
			if (Math::is_equal_approx(before, after)) {
				continue;
			}

			heights.set(index, after);
			changed_indices.push_back(index);
			before_values.push_back(before);
			after_values.push_back(after);
		}
	}

	if (!changed_indices.is_empty()) {
		_refresh_height_texture_region(min_x, min_y, max_x, max_y);
		update_gizmos();
		delta["indices"] = changed_indices;
		delta["before"] = before_values;
		delta["after"] = after_values;
	}

	return delta;
}

void OpenWorldTerrain3D::apply_height_patch(const PackedInt32Array &p_indices, const PackedFloat32Array &p_heights) {
	_ensure_data();
	ERR_FAIL_COND(p_indices.size() != p_heights.size());

	PackedFloat32Array &heights = terrain_data->get_mutable_height_data();
	bool changed = false;
	int min_x = terrain_data->get_heightmap_resolution() - 1;
	int min_y = terrain_data->get_heightmap_resolution() - 1;
	int max_x = 0;
	int max_y = 0;
	for (int i = 0; i < p_indices.size(); i++) {
		const int index = p_indices[i];
		ERR_FAIL_INDEX(index, heights.size());
		const real_t height = CLAMP(p_heights[i], (real_t)0.0, (real_t)1.0);
		if (Math::is_equal_approx(heights[index], height)) {
			continue;
		}
		heights.set(index, height);
		const int x = index % terrain_data->get_heightmap_resolution();
		const int y = index / terrain_data->get_heightmap_resolution();
		min_x = MIN(min_x, x);
		min_y = MIN(min_y, y);
		max_x = MAX(max_x, x);
		max_y = MAX(max_y, y);
		changed = true;
	}

	if (changed) {
		_refresh_height_texture_region(min_x, min_y, max_x, max_y);
		update_gizmos();
	}
}

Dictionary OpenWorldTerrain3D::get_brush_hit(const Vector3 &p_ray_origin, const Vector3 &p_ray_direction) const {
	Dictionary hit;
	if (terrain_data.is_null() || p_ray_direction.is_zero_approx()) {
		return hit;
	}

	const Transform3D inverse_transform = get_global_transform().affine_inverse();
	const Vector3 local_origin = inverse_transform.xform(p_ray_origin);
	const Vector3 local_direction = inverse_transform.basis.xform(p_ray_direction).normalized();
	const int resolution = terrain_data->get_heightmap_resolution();
	if (resolution < 2) {
		return hit;
	}

	const AABB bounds = get_aabb();
	const Vector3 bounds_min = bounds.position;
	const Vector3 bounds_max = bounds.position + bounds.size;
	real_t t_min = 0.0;
	real_t t_max = Math::INF;

	const auto update_slab = [&t_min, &t_max](real_t p_origin, real_t p_direction, real_t p_min, real_t p_max) -> bool {
		if (Math::is_zero_approx(p_direction)) {
			return p_origin >= p_min && p_origin <= p_max;
		}
		real_t t1 = (p_min - p_origin) / p_direction;
		real_t t2 = (p_max - p_origin) / p_direction;
		if (t1 > t2) {
			SWAP(t1, t2);
		}
		t_min = MAX(t_min, t1);
		t_max = MIN(t_max, t2);
		return t_min <= t_max;
	};

	if (!update_slab(local_origin.x, local_direction.x, bounds_min.x, bounds_max.x) ||
			!update_slab(local_origin.y, local_direction.y, bounds_min.y, bounds_max.y) ||
			!update_slab(local_origin.z, local_direction.z, bounds_min.z, bounds_max.z)) {
		return hit;
	}

	const real_t world_size = terrain_data->get_world_size();
	const real_t half_size = world_size * 0.5;
	const real_t texel_world_size = world_size / (real_t)(resolution - 1);
	const real_t march_step = MAX(texel_world_size * 0.5, (real_t)0.25);
	Vector3 previous_position = local_origin + local_direction * t_min;
	bool previous_above = true;

	const int max_steps = CLAMP(Math::ceil((t_max - t_min) / march_step), 1, 2048);
	for (int step = 0; step <= max_steps; step++) {
		const real_t t = Math::lerp(t_min, t_max, (real_t)step / (real_t)max_steps);
		const Vector3 position = local_origin + local_direction * t;
		const real_t sample_x = (position.x + half_size) / texel_world_size;
		const real_t sample_y = (position.z + half_size) / texel_world_size;

		if (sample_x < 0.0 || sample_y < 0.0 || sample_x > (real_t)resolution - 1.0 || sample_y > (real_t)resolution - 1.0) {
			previous_position = position;
			continue;
		}

		const real_t terrain_height = _sample_height_bilinear(sample_x, sample_y) * terrain_data->get_height_scale();
		const bool above = position.y > terrain_height;
		if (step > 0 && previous_above && !above) {
			const int center_x = CLAMP(Math::floor(sample_x), 0, resolution - 2);
			const int center_y = CLAMP(Math::floor(sample_y), 0, resolution - 2);
			Vector3 best_position;
			real_t best_distance = Math::INF;
			bool found = false;

			for (int y = MAX(0, center_y - 1); y <= MIN(resolution - 2, center_y + 1); y++) {
				for (int x = MAX(0, center_x - 1); x <= MIN(resolution - 2, center_x + 1); x++) {
					const Vector3 v00 = _get_local_height_position(x, y);
					const Vector3 v10 = _get_local_height_position(x + 1, y);
					const Vector3 v01 = _get_local_height_position(x, y + 1);
					const Vector3 v11 = _get_local_height_position(x + 1, y + 1);
					Vector3 intersection;

					if (Geometry3D::segment_intersects_triangle(previous_position, position, v00, v01, v10, &intersection)) {
						const real_t distance = local_origin.distance_to(intersection);
						if (distance < best_distance) {
							best_distance = distance;
							best_position = intersection;
							found = true;
						}
					}
					if (Geometry3D::segment_intersects_triangle(previous_position, position, v10, v01, v11, &intersection)) {
						const real_t distance = local_origin.distance_to(intersection);
						if (distance < best_distance) {
							best_distance = distance;
							best_position = intersection;
							found = true;
						}
					}
				}
			}

			if (found) {
				hit["position"] = get_global_transform().xform(best_position);
				hit["local_position"] = best_position;
				hit["distance"] = best_distance;
				return hit;
			}
		}

		previous_position = position;
		previous_above = above;
	}

	return hit;
}

PackedVector3Array OpenWorldTerrain3D::get_debug_lines() const {
	PackedVector3Array lines;
	if (terrain_data.is_null()) {
		return lines;
	}

	const real_t world_size = terrain_data->get_world_size();
	const real_t height_scale = terrain_data->get_height_scale();
	const real_t half_size = world_size * 0.5;
	const real_t top = MAX((real_t)0.0, height_scale);
	const real_t bottom = MIN((real_t)0.0, height_scale);

	const Vector3 corners[8] = {
		Vector3(-half_size, bottom, -half_size),
		Vector3(half_size, bottom, -half_size),
		Vector3(half_size, bottom, half_size),
		Vector3(-half_size, bottom, half_size),
		Vector3(-half_size, top, -half_size),
		Vector3(half_size, top, -half_size),
		Vector3(half_size, top, half_size),
		Vector3(-half_size, top, half_size),
	};
	const int edge_indices[24] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
	};
	for (int i = 0; i < 24; i++) {
		lines.push_back(corners[edge_indices[i]]);
	}

	return lines;
}

Ref<Texture2D> OpenWorldTerrain3D::get_height_texture() const {
	return tiles.is_empty() ? Ref<Texture2D>() : Ref<Texture2D>(tiles[0].height_texture);
}

AABB OpenWorldTerrain3D::get_aabb() const {
	const real_t world_size = terrain_data.is_valid() ? terrain_data->get_world_size() : 1024.0;
	const real_t height_scale = terrain_data.is_valid() ? terrain_data->get_height_scale() : 128.0;
	const real_t min_y = MIN((real_t)0.0, height_scale);
	const real_t max_y = MAX((real_t)0.0, height_scale);
	return AABB(Vector3(-world_size * 0.5, min_y, -world_size * 0.5), Vector3(world_size, max_y - min_y, world_size));
}

PackedStringArray OpenWorldTerrain3D::get_configuration_warnings() const {
	PackedStringArray warnings;
	if (!use_builtin_displacement_material && terrain_material.is_null()) {
		warnings.push_back(RTR("Assign a terrain_material or enable use_builtin_displacement_material."));
	}
	return warnings;
}

void OpenWorldTerrain3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_ENTER_WORLD: {
			rebuild();
			set_notify_transform(true);
		} break;

		case NOTIFICATION_EXIT_WORLD: {
			for (TerrainTile &tile : tiles) {
				if (tile.instance.is_valid()) {
					RenderingServer::get_singleton()->instance_set_scenario(tile.instance, RID());
				}
			}
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			_sync_tile_instances();
		} break;
	}
}

void OpenWorldTerrain3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_terrain_data", "terrain_data"), &OpenWorldTerrain3D::set_terrain_data);
	ClassDB::bind_method(D_METHOD("get_terrain_data"), &OpenWorldTerrain3D::get_terrain_data);
	ClassDB::bind_method(D_METHOD("set_patch_resolution", "patch_resolution"), &OpenWorldTerrain3D::set_patch_resolution);
	ClassDB::bind_method(D_METHOD("get_patch_resolution"), &OpenWorldTerrain3D::get_patch_resolution);
	ClassDB::bind_method(D_METHOD("set_tile_size", "tile_size"), &OpenWorldTerrain3D::set_tile_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &OpenWorldTerrain3D::get_tile_size);
	ClassDB::bind_method(D_METHOD("set_world_size", "world_size"), &OpenWorldTerrain3D::set_world_size);
	ClassDB::bind_method(D_METHOD("get_world_size"), &OpenWorldTerrain3D::get_world_size);
	ClassDB::bind_method(D_METHOD("set_height_scale", "height_scale"), &OpenWorldTerrain3D::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &OpenWorldTerrain3D::get_height_scale);
	ClassDB::bind_method(D_METHOD("set_use_builtin_displacement_material", "use"), &OpenWorldTerrain3D::set_use_builtin_displacement_material);
	ClassDB::bind_method(D_METHOD("is_using_builtin_displacement_material"), &OpenWorldTerrain3D::is_using_builtin_displacement_material);
	ClassDB::bind_method(D_METHOD("set_terrain_material", "material"), &OpenWorldTerrain3D::set_terrain_material);
	ClassDB::bind_method(D_METHOD("get_terrain_material"), &OpenWorldTerrain3D::get_terrain_material);
	ClassDB::bind_method(D_METHOD("set_low_texture", "texture"), &OpenWorldTerrain3D::set_low_texture);
	ClassDB::bind_method(D_METHOD("get_low_texture"), &OpenWorldTerrain3D::get_low_texture);
	ClassDB::bind_method(D_METHOD("set_mid_texture", "texture"), &OpenWorldTerrain3D::set_mid_texture);
	ClassDB::bind_method(D_METHOD("get_mid_texture"), &OpenWorldTerrain3D::get_mid_texture);
	ClassDB::bind_method(D_METHOD("set_high_texture", "texture"), &OpenWorldTerrain3D::set_high_texture);
	ClassDB::bind_method(D_METHOD("get_high_texture"), &OpenWorldTerrain3D::get_high_texture);
	ClassDB::bind_method(D_METHOD("set_low_color", "color"), &OpenWorldTerrain3D::set_low_color);
	ClassDB::bind_method(D_METHOD("get_low_color"), &OpenWorldTerrain3D::get_low_color);
	ClassDB::bind_method(D_METHOD("set_mid_color", "color"), &OpenWorldTerrain3D::set_mid_color);
	ClassDB::bind_method(D_METHOD("get_mid_color"), &OpenWorldTerrain3D::get_mid_color);
	ClassDB::bind_method(D_METHOD("set_high_color", "color"), &OpenWorldTerrain3D::set_high_color);
	ClassDB::bind_method(D_METHOD("get_high_color"), &OpenWorldTerrain3D::get_high_color);
	ClassDB::bind_method(D_METHOD("set_low_height", "height"), &OpenWorldTerrain3D::set_low_height);
	ClassDB::bind_method(D_METHOD("get_low_height"), &OpenWorldTerrain3D::get_low_height);
	ClassDB::bind_method(D_METHOD("set_high_height", "height"), &OpenWorldTerrain3D::set_high_height);
	ClassDB::bind_method(D_METHOD("get_high_height"), &OpenWorldTerrain3D::get_high_height);
	ClassDB::bind_method(D_METHOD("set_blend_width", "width"), &OpenWorldTerrain3D::set_blend_width);
	ClassDB::bind_method(D_METHOD("get_blend_width"), &OpenWorldTerrain3D::get_blend_width);
	ClassDB::bind_method(D_METHOD("set_texture_scale", "scale"), &OpenWorldTerrain3D::set_texture_scale);
	ClassDB::bind_method(D_METHOD("get_texture_scale"), &OpenWorldTerrain3D::get_texture_scale);
	ClassDB::bind_method(D_METHOD("set_low_texture_scale", "scale"), &OpenWorldTerrain3D::set_low_texture_scale);
	ClassDB::bind_method(D_METHOD("get_low_texture_scale"), &OpenWorldTerrain3D::get_low_texture_scale);
	ClassDB::bind_method(D_METHOD("set_mid_texture_scale", "scale"), &OpenWorldTerrain3D::set_mid_texture_scale);
	ClassDB::bind_method(D_METHOD("get_mid_texture_scale"), &OpenWorldTerrain3D::get_mid_texture_scale);
	ClassDB::bind_method(D_METHOD("set_high_texture_scale", "scale"), &OpenWorldTerrain3D::set_high_texture_scale);
	ClassDB::bind_method(D_METHOD("get_high_texture_scale"), &OpenWorldTerrain3D::get_high_texture_scale);
	ClassDB::bind_method(D_METHOD("set_triplanar_sharpness", "sharpness"), &OpenWorldTerrain3D::set_triplanar_sharpness);
	ClassDB::bind_method(D_METHOD("get_triplanar_sharpness"), &OpenWorldTerrain3D::get_triplanar_sharpness);
	ClassDB::bind_method(D_METHOD("set_slope_start", "slope"), &OpenWorldTerrain3D::set_slope_start);
	ClassDB::bind_method(D_METHOD("get_slope_start"), &OpenWorldTerrain3D::get_slope_start);
	ClassDB::bind_method(D_METHOD("set_slope_end", "slope"), &OpenWorldTerrain3D::set_slope_end);
	ClassDB::bind_method(D_METHOD("get_slope_end"), &OpenWorldTerrain3D::get_slope_end);
	ClassDB::bind_method(D_METHOD("set_slope_high_strength", "strength"), &OpenWorldTerrain3D::set_slope_high_strength);
	ClassDB::bind_method(D_METHOD("get_slope_high_strength"), &OpenWorldTerrain3D::get_slope_high_strength);
	ClassDB::bind_method(D_METHOD("set_splat_debug_mode", "mode"), &OpenWorldTerrain3D::set_splat_debug_mode);
	ClassDB::bind_method(D_METHOD("get_splat_debug_mode"), &OpenWorldTerrain3D::get_splat_debug_mode);
	ClassDB::bind_method(D_METHOD("set_flatten_height", "height"), &OpenWorldTerrain3D::set_flatten_height);
	ClassDB::bind_method(D_METHOD("get_flatten_height"), &OpenWorldTerrain3D::get_flatten_height);
	ClassDB::bind_method(D_METHOD("set_show_debug_gizmo", "show"), &OpenWorldTerrain3D::set_show_debug_gizmo);
	ClassDB::bind_method(D_METHOD("is_showing_debug_gizmo"), &OpenWorldTerrain3D::is_showing_debug_gizmo);
	ClassDB::bind_method(D_METHOD("reset_flat_terrain", "normalized_height"), &OpenWorldTerrain3D::reset_flat_terrain, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("generate_random_terrain", "seed", "amplitude", "frequency", "octaves"), &OpenWorldTerrain3D::generate_random_terrain, DEFVAL(1), DEFVAL(1.0), DEFVAL(0.025), DEFVAL(4));
	ClassDB::bind_method(D_METHOD("rebuild"), &OpenWorldTerrain3D::rebuild);
	ClassDB::bind_method(D_METHOD("apply_brush", "world_position", "radius", "strength", "operation"), &OpenWorldTerrain3D::apply_brush);
	ClassDB::bind_method(D_METHOD("apply_brush_with_delta", "world_position", "radius", "strength", "operation"), &OpenWorldTerrain3D::apply_brush_with_delta);
	ClassDB::bind_method(D_METHOD("apply_height_patch", "indices", "heights"), &OpenWorldTerrain3D::apply_height_patch);
	ClassDB::bind_method(D_METHOD("get_brush_hit", "ray_origin", "ray_direction"), &OpenWorldTerrain3D::get_brush_hit);
	ClassDB::bind_method(D_METHOD("get_debug_lines"), &OpenWorldTerrain3D::get_debug_lines);
	ClassDB::bind_method(D_METHOD("get_height_texture"), &OpenWorldTerrain3D::get_height_texture);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_data", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldTerrainData"), "set_terrain_data", "get_terrain_data");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "patch_resolution", PROPERTY_HINT_RANGE, "1,4096,1,or_greater"), "set_patch_resolution", "get_patch_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_size", PROPERTY_HINT_RANGE, "1,4096,1,or_greater"), "set_tile_size", "get_tile_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m"), "set_world_size", "get_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "-1000000,1000000,0.001,suffix:m"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_builtin_displacement_material"), "set_use_builtin_displacement_material", "is_using_builtin_displacement_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_terrain_material", "get_terrain_material");
	ADD_GROUP("Height Splatting", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "low_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_low_texture", "get_low_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mid_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_mid_texture", "get_mid_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "high_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_high_texture", "get_high_texture");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "low_color"), "set_low_color", "get_low_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "mid_color"), "set_mid_color", "get_mid_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "high_color"), "set_high_color", "get_high_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_low_height", "get_low_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "high_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_high_height", "get_high_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blend_width", PROPERTY_HINT_RANGE, "0.001,1,0.001"), "set_blend_width", "get_blend_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_texture_scale", "get_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_low_texture_scale", "get_low_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mid_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_mid_texture_scale", "get_mid_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "high_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_high_texture_scale", "get_high_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_sharpness", PROPERTY_HINT_RANGE, "0.001,32,0.001,or_greater"), "set_triplanar_sharpness", "get_triplanar_sharpness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_start", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_slope_start", "get_slope_start");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_end", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_slope_end", "get_slope_end");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_high_strength", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_slope_high_strength", "get_slope_high_strength");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "splat_debug_mode", PROPERTY_HINT_ENUM, "None,Height,Slope,Weights"), "set_splat_debug_mode", "get_splat_debug_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flatten_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_flatten_height", "get_flatten_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug_gizmo"), "set_show_debug_gizmo", "is_showing_debug_gizmo");

	BIND_ENUM_CONSTANT(BRUSH_RAISE);
	BIND_ENUM_CONSTANT(BRUSH_LOWER);
	BIND_ENUM_CONSTANT(BRUSH_SMOOTH);
	BIND_ENUM_CONSTANT(BRUSH_FLATTEN);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_NONE);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_HEIGHT);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_SLOPE);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_WEIGHTS);
}

OpenWorldTerrain3D::OpenWorldTerrain3D() {
}

OpenWorldTerrain3D::~OpenWorldTerrain3D() {
	_clear_tiles();
}
