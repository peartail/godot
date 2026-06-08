/**************************************************************************/
/*  open_world_terrain_3d.cpp                                             */
/**************************************************************************/

#include "open_world_terrain_3d.h"

#include "core/math/geometry_3d.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/resources/shader.h"

void OpenWorldTerrain3D::_terrain_data_changed() {
	_rebuild_height_texture();
	_update_material();
	update_gizmos();
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

void OpenWorldTerrain3D::_rebuild_patch_mesh() {
	_ensure_data();

	const int resolution = CLAMP(patch_resolution, 1, 4096);
	const real_t world_size = terrain_data->get_world_size();
	const real_t half_size = world_size * 0.5;

	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedVector2Array uvs;
	PackedInt32Array indices;

	const int vertex_count = resolution + 1;
	vertices.resize(vertex_count * vertex_count);
	normals.resize(vertex_count * vertex_count);
	uvs.resize(vertex_count * vertex_count);
	indices.resize(resolution * resolution * 6);

	for (int z = 0; z < vertex_count; z++) {
		for (int x = 0; x < vertex_count; x++) {
			const int index = z * vertex_count + x;
			const real_t u = (real_t)x / (real_t)resolution;
			const real_t v = (real_t)z / (real_t)resolution;
			vertices.set(index, Vector3(u * world_size - half_size, 0.0, v * world_size - half_size));
			normals.set(index, Vector3(0.0, 1.0, 0.0));
			uvs.set(index, Vector2(u, v));
		}
	}

	int write_index = 0;
	for (int z = 0; z < resolution; z++) {
		for (int x = 0; x < resolution; x++) {
			const int top_left = z * vertex_count + x;
			const int top_right = top_left + 1;
			const int bottom_left = top_left + vertex_count;
			const int bottom_right = bottom_left + 1;

			// Godot's front face expects this winding for the terrain to be visible
			// from above. The patch is generated in X/Z space, so reversing the old
			// order fixes the terrain being culled when viewed from the top.
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
	set_mesh(array_mesh);
}

void OpenWorldTerrain3D::_rebuild_height_texture() {
	_ensure_data();
	const Ref<Image> height_image = terrain_data->create_height_image();
	if (height_texture.is_null() || height_texture->get_width() != height_image->get_width() || height_texture->get_height() != height_image->get_height()) {
		height_texture = ImageTexture::create_from_image(height_image);
	} else {
		height_texture->update(height_image);
	}
}

void OpenWorldTerrain3D::_update_material() {
	_ensure_data();
	if (!use_builtin_displacement_material) {
		set_surface_override_material(0, terrain_material);
		return;
	}

	if (displacement_shader.is_null()) {
		displacement_shader.instantiate();
		displacement_shader->set_code(_get_builtin_displacement_shader_code());
	}
	if (displacement_material.is_null()) {
		displacement_material.instantiate();
		displacement_material->set_shader(displacement_shader);
	}

	displacement_material->set_shader_parameter("height_texture", height_texture);
	displacement_material->set_shader_parameter("height_scale", terrain_data->get_height_scale());
	displacement_material->set_shader_parameter("height_texel_size", 1.0 / (real_t)terrain_data->get_heightmap_resolution());
	displacement_material->set_shader_parameter("height_world_texel_size", terrain_data->get_world_size() / MAX(1.0, (real_t)terrain_data->get_heightmap_resolution() - 1.0));
	displacement_material->set_shader_parameter("low_color", low_color);
	displacement_material->set_shader_parameter("mid_color", mid_color);
	displacement_material->set_shader_parameter("high_color", high_color);
	set_surface_override_material(0, displacement_material);
}

String OpenWorldTerrain3D::_get_builtin_displacement_shader_code() {
	return R"(
shader_type spatial;

uniform sampler2D height_texture : repeat_disable, filter_linear;
uniform float height_scale = 128.0;
uniform float height_texel_size = 0.00390625;
uniform float height_world_texel_size = 4.0;
uniform vec4 low_color : source_color = vec4(0.22, 0.38, 0.18, 1.0);
uniform vec4 mid_color : source_color = vec4(0.42, 0.34, 0.22, 1.0);
uniform vec4 high_color : source_color = vec4(0.78, 0.78, 0.72, 1.0);

varying float terrain_height;

void vertex() {
	float height = texture(height_texture, UV).r;
	float left_height = texture(height_texture, UV + vec2(-height_texel_size, 0.0)).r;
	float right_height = texture(height_texture, UV + vec2(height_texel_size, 0.0)).r;
	float back_height = texture(height_texture, UV + vec2(0.0, -height_texel_size)).r;
	float front_height = texture(height_texture, UV + vec2(0.0, height_texel_size)).r;

	VERTEX.y += height * height_scale;
	NORMAL = normalize(vec3((left_height - right_height) * height_scale, 2.0 * height_world_texel_size, (back_height - front_height) * height_scale));
	terrain_height = height;
}

void fragment() {
	vec3 base_color = mix(low_color.rgb, mid_color.rgb, smoothstep(0.15, 0.55, terrain_height));
	base_color = mix(base_color, high_color.rgb, smoothstep(0.55, 0.9, terrain_height));
	ALBEDO = base_color;
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
	_rebuild_patch_mesh();
	_update_material();
}

void OpenWorldTerrain3D::set_world_size(real_t p_world_size) {
	_ensure_data();
	terrain_data->set_world_size(p_world_size);
	_rebuild_patch_mesh();
}

real_t OpenWorldTerrain3D::get_world_size() const {
	return terrain_data.is_valid() ? terrain_data->get_world_size() : 1024.0;
}

void OpenWorldTerrain3D::set_height_scale(real_t p_height_scale) {
	_ensure_data();
	terrain_data->set_height_scale(p_height_scale);
	_update_material();
}

real_t OpenWorldTerrain3D::get_height_scale() const {
	return terrain_data.is_valid() ? terrain_data->get_height_scale() : 128.0;
}

void OpenWorldTerrain3D::set_use_builtin_displacement_material(bool p_use) {
	use_builtin_displacement_material = p_use;
	_update_material();
}

void OpenWorldTerrain3D::set_terrain_material(const Ref<Material> &p_material) {
	terrain_material = p_material;
	_update_material();
}

void OpenWorldTerrain3D::set_low_color(const Color &p_color) {
	low_color = p_color;
	_update_material();
}

void OpenWorldTerrain3D::set_mid_color(const Color &p_color) {
	mid_color = p_color;
	_update_material();
}

void OpenWorldTerrain3D::set_high_color(const Color &p_color) {
	high_color = p_color;
	_update_material();
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
	_rebuild_patch_mesh();
	_rebuild_height_texture();
	_update_material();
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
		terrain_data->notify_height_data_changed();
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
	for (int i = 0; i < p_indices.size(); i++) {
		const int index = p_indices[i];
		ERR_FAIL_INDEX(index, heights.size());
		const real_t height = CLAMP(p_heights[i], (real_t)0.0, (real_t)1.0);
		if (Math::is_equal_approx(heights[index], height)) {
			continue;
		}
		heights.set(index, height);
		changed = true;
	}

	if (changed) {
		terrain_data->notify_height_data_changed();
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
	if (!get_aabb().intersects_ray(local_origin, local_direction)) {
		return hit;
	}

	real_t best_distance = Math::INF;
	Vector3 best_position;
	bool found = false;

	for (int y = 0; y < resolution - 1; y++) {
		for (int x = 0; x < resolution - 1; x++) {
			const Vector3 v00 = _get_local_height_position(x, y);
			const Vector3 v10 = _get_local_height_position(x + 1, y);
			const Vector3 v01 = _get_local_height_position(x, y + 1);
			const Vector3 v11 = _get_local_height_position(x + 1, y + 1);
			Vector3 intersection;

			if (Geometry3D::ray_intersects_triangle(local_origin, local_direction, v00, v01, v10, &intersection)) {
				const real_t distance = local_origin.distance_to(intersection);
				if (distance < best_distance) {
					best_distance = distance;
					best_position = intersection;
					found = true;
				}
			}
			if (Geometry3D::ray_intersects_triangle(local_origin, local_direction, v10, v01, v11, &intersection)) {
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
	return height_texture;
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
	if (p_what == NOTIFICATION_READY) {
		rebuild();
	}
}

void OpenWorldTerrain3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_terrain_data", "terrain_data"), &OpenWorldTerrain3D::set_terrain_data);
	ClassDB::bind_method(D_METHOD("get_terrain_data"), &OpenWorldTerrain3D::get_terrain_data);
	ClassDB::bind_method(D_METHOD("set_patch_resolution", "patch_resolution"), &OpenWorldTerrain3D::set_patch_resolution);
	ClassDB::bind_method(D_METHOD("get_patch_resolution"), &OpenWorldTerrain3D::get_patch_resolution);
	ClassDB::bind_method(D_METHOD("set_world_size", "world_size"), &OpenWorldTerrain3D::set_world_size);
	ClassDB::bind_method(D_METHOD("get_world_size"), &OpenWorldTerrain3D::get_world_size);
	ClassDB::bind_method(D_METHOD("set_height_scale", "height_scale"), &OpenWorldTerrain3D::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &OpenWorldTerrain3D::get_height_scale);
	ClassDB::bind_method(D_METHOD("set_use_builtin_displacement_material", "use"), &OpenWorldTerrain3D::set_use_builtin_displacement_material);
	ClassDB::bind_method(D_METHOD("is_using_builtin_displacement_material"), &OpenWorldTerrain3D::is_using_builtin_displacement_material);
	ClassDB::bind_method(D_METHOD("set_terrain_material", "material"), &OpenWorldTerrain3D::set_terrain_material);
	ClassDB::bind_method(D_METHOD("get_terrain_material"), &OpenWorldTerrain3D::get_terrain_material);
	ClassDB::bind_method(D_METHOD("set_low_color", "color"), &OpenWorldTerrain3D::set_low_color);
	ClassDB::bind_method(D_METHOD("get_low_color"), &OpenWorldTerrain3D::get_low_color);
	ClassDB::bind_method(D_METHOD("set_mid_color", "color"), &OpenWorldTerrain3D::set_mid_color);
	ClassDB::bind_method(D_METHOD("get_mid_color"), &OpenWorldTerrain3D::get_mid_color);
	ClassDB::bind_method(D_METHOD("set_high_color", "color"), &OpenWorldTerrain3D::set_high_color);
	ClassDB::bind_method(D_METHOD("get_high_color"), &OpenWorldTerrain3D::get_high_color);
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
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m"), "set_world_size", "get_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "-1000000,1000000,0.001,suffix:m"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_builtin_displacement_material"), "set_use_builtin_displacement_material", "is_using_builtin_displacement_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_terrain_material", "get_terrain_material");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "low_color"), "set_low_color", "get_low_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "mid_color"), "set_mid_color", "get_mid_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "high_color"), "set_high_color", "get_high_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flatten_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_flatten_height", "get_flatten_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug_gizmo"), "set_show_debug_gizmo", "is_showing_debug_gizmo");

	BIND_ENUM_CONSTANT(BRUSH_RAISE);
	BIND_ENUM_CONSTANT(BRUSH_LOWER);
	BIND_ENUM_CONSTANT(BRUSH_SMOOTH);
	BIND_ENUM_CONSTANT(BRUSH_FLATTEN);
}

OpenWorldTerrain3D::OpenWorldTerrain3D() {
}
