/**************************************************************************/
/*  open_world_terrain_3d.cpp                                             */
/**************************************************************************/

#include "open_world_terrain_3d.h"

#include "core/math/geometry_3d.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/templates/hash_map.h"
#include "scene/resources/3d/world_3d.h"
#include "scene/resources/shader.h"
#include "servers/rendering/rendering_server.h"

void OpenWorldTerrain3D::_terrain_data_changed() {
	if (terrain_data.is_valid()) {
		active_grid_cells = terrain_data->get_created_tile_cells();
		active_grid_cell_set.clear();
		for (int i = 0; i < active_grid_cells.size(); i++) {
			const Vector2 cell_value = active_grid_cells[i];
			active_grid_cell_set.insert(Vector2i(Math::floor(cell_value.x), Math::floor(cell_value.y)));
		}
	}
	_rebuild_tiles();
}

void OpenWorldTerrain3D::_terrain_layer_changed() {
	// Layer resources are edited from a popup inspector, so the terrain needs
	// to listen to resource changes and push their texture/material values back
	// into the active shader materials immediately.
	_update_materials();
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

real_t OpenWorldTerrain3D::_sample_tile_height_nearest(const Vector2i &p_cell, int p_x, int p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), 0.0);
	const int resolution = terrain_data->get_tile_resolution();
	const int x = CLAMP(p_x, 0, resolution - 1);
	const int y = CLAMP(p_y, 0, resolution - 1);
	return terrain_data->get_tile_height(p_cell, x, y);
}

real_t OpenWorldTerrain3D::_sample_tile_height_bilinear(const Vector2i &p_cell, real_t p_x, real_t p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), 0.0);
	const int resolution = terrain_data->get_tile_resolution();
	const real_t clamped_x = CLAMP(p_x, (real_t)0.0, (real_t)resolution - 1.0);
	const real_t clamped_y = CLAMP(p_y, (real_t)0.0, (real_t)resolution - 1.0);
	const int x0 = CLAMP(Math::floor(clamped_x), 0, resolution - 1);
	const int y0 = CLAMP(Math::floor(clamped_y), 0, resolution - 1);
	const int x1 = CLAMP(x0 + 1, 0, resolution - 1);
	const int y1 = CLAMP(y0 + 1, 0, resolution - 1);
	const real_t tx = clamped_x - (real_t)x0;
	const real_t ty = clamped_y - (real_t)y0;

	const real_t h00 = _sample_tile_height_nearest(p_cell, x0, y0);
	const real_t h10 = _sample_tile_height_nearest(p_cell, x1, y0);
	const real_t h01 = _sample_tile_height_nearest(p_cell, x0, y1);
	const real_t h11 = _sample_tile_height_nearest(p_cell, x1, y1);
	const real_t h0 = Math::lerp(h00, h10, tx);
	const real_t h1 = Math::lerp(h01, h11, tx);
	return Math::lerp(h0, h1, ty);
}

real_t OpenWorldTerrain3D::_get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_y) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), 0.0);
	const int resolution = terrain_data->get_tile_resolution();
	real_t total = 0.0;
	int count = 0;

	for (int y = MAX(0, p_y - 1); y <= MIN(resolution - 1, p_y + 1); y++) {
		for (int x = MAX(0, p_x - 1); x <= MIN(resolution - 1, p_x + 1); x++) {
			total += p_source_heights[terrain_data->get_tile_height_index(x, y)];
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

Ref<ArrayMesh> OpenWorldTerrain3D::_build_tile_mesh() const {
	ERR_FAIL_COND_V(terrain_data.is_null(), Ref<ArrayMesh>());
	const int resolution = terrain_data->get_tile_resolution();
	ERR_FAIL_COND_V(resolution < 2, Ref<ArrayMesh>());

	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const real_t texel_world_size = tile_world_size / (real_t)(resolution - 1);
	const int quad_count = resolution - 1;
	const int vertex_width = resolution;
	const int vertex_height = resolution;

	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedVector2Array uvs;
	PackedInt32Array indices;
	vertices.resize(vertex_width * vertex_height);
	normals.resize(vertex_width * vertex_height);
	uvs.resize(vertex_width * vertex_height);
	indices.resize(quad_count * quad_count * 6);

	for (int y = 0; y < vertex_height; y++) {
		for (int x = 0; x < vertex_width; x++) {
			const int index = y * vertex_width + x;
			vertices.set(index, Vector3(
					(real_t)x * texel_world_size,
					0.0,
					(real_t)y * texel_world_size));
			normals.set(index, Vector3(0.0, 1.0, 0.0));
			uvs.set(index, Vector2((real_t)x / (real_t)quad_count, (real_t)y / (real_t)quad_count));
		}
	}

	int write_index = 0;
	for (int y = 0; y < quad_count; y++) {
		for (int x = 0; x < quad_count; x++) {
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

Ref<Image> OpenWorldTerrain3D::_build_tile_height_image(const Vector2i &p_cell) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), Ref<Image>());
	const int image_width = terrain_data->get_tile_resolution();
	const int image_height = terrain_data->get_tile_resolution();
	Ref<Image> image = Image::create_empty(image_width, image_height, false, Image::FORMAT_RF);
	const PackedFloat32Array heights = terrain_data->get_tile_height_data(p_cell);

	for (int y = 0; y < image_height; y++) {
		for (int x = 0; x < image_width; x++) {
			const int index = terrain_data->get_tile_height_index(x, y);
			const real_t height = index < heights.size() ? CLAMP(heights[index], (real_t)0.0, (real_t)1.0) : 0.0;
			image->set_pixel(x, y, Color(height, 0.0, 0.0, 1.0));
		}
	}
	return image;
}

Ref<Image> OpenWorldTerrain3D::_build_tile_layer_image(const Vector2i &p_cell) const {
	ERR_FAIL_COND_V(terrain_data.is_null(), Ref<Image>());
	const int image_width = terrain_data->get_tile_resolution();
	const int image_height = terrain_data->get_tile_resolution();
	Ref<Image> image = Image::create_empty(image_width, image_height, false, Image::FORMAT_RGBA8);
	const PackedColorArray layer_values = terrain_data->get_tile_layer_data(p_cell);

	for (int y = 0; y < image_height; y++) {
		for (int x = 0; x < image_width; x++) {
			const int index = terrain_data->get_tile_height_index(x, y);
			const Color layer = index < layer_values.size() ? layer_values[index] : Color(0.0, 0.0, 0.0, 0.0);
			image->set_pixel(x, y, Color(
					CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.a, (real_t)0.0, (real_t)1.0)));
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
	const Transform3D global_transform = is_inside_tree() ? get_global_transform() : Transform3D();
	for (TerrainTile &tile : tiles) {
		if (!tile.instance.is_valid()) {
			tile.instance = rs->instance_create();
		}
		Transform3D tile_transform = global_transform;
		if (terrain_data.is_valid()) {
			const real_t tile_world_size = terrain_data->get_tile_world_size();
			tile_transform.origin += global_transform.basis.xform(Vector3((real_t)tile.cell.x * tile_world_size, 0.0, (real_t)tile.cell.y * tile_world_size));
		}
		rs->instance_set_base(tile.instance, tile.mesh.is_valid() ? tile.mesh->get_rid() : RID());
		rs->instance_set_scenario(tile.instance, scenario);
		rs->instance_set_transform(tile.instance, tile_transform);
	}
	_update_tile_visibility();
	_sync_tile_materials();
}

void OpenWorldTerrain3D::_update_tile_visibility() {
	if (!is_inside_tree()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	const bool visible = is_visible_in_tree();
	for (TerrainTile &tile : tiles) {
		if (tile.instance.is_valid()) {
			rs->instance_set_visible(tile.instance, visible);
		}
	}
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

	const int resolution = terrain_data->get_tile_resolution();
	if (resolution < 2) {
		_sync_tile_instances();
		notify_property_list_changed();
		update_gizmos();
		return;
	}

	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	for (int i = 0; i < created_cells.size(); i++) {
		const Vector2 cell_value = created_cells[i];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		TerrainTile tile;
		tile.cell = cell;
		tile.quad_width = resolution - 1;
		tile.quad_height = resolution - 1;
		tile.mesh = _build_tile_mesh();
		tile.height_image = _build_tile_height_image(tile.cell);
		tile.height_texture = ImageTexture::create_from_image(tile.height_image);
		tile.layer_image = _build_tile_layer_image(tile.cell);
		tile.layer_texture = ImageTexture::create_from_image(tile.layer_image);
		_update_tile_material(tile);
		tiles.push_back(tile);
	}

	_sync_tile_instances();
	notify_property_list_changed();
	update_gizmos();
}

bool OpenWorldTerrain3D::_is_grid_cell_active(const Vector2i &p_cell) const {
	return terrain_data.is_valid() ? terrain_data->has_tile(p_cell) : active_grid_cell_set.has(p_cell);
}

bool OpenWorldTerrain3D::_is_quad_origin_active(int p_origin_x, int p_origin_y) const {
	if (terrain_data.is_null()) {
		return false;
	}
	const int active_tile_size = CLAMP(tile_size, 1, 4096);
	return _is_grid_cell_active(Vector2i(p_origin_x / active_tile_size, p_origin_y / active_tile_size));
}

Vector2i OpenWorldTerrain3D::_get_grid_cell_for_local_position(const Vector3 &p_local_position) const {
	return _get_tile_cell_for_local_position(p_local_position);
}

Vector2i OpenWorldTerrain3D::_get_tile_cell_for_local_position(const Vector3 &p_local_position) const {
	if (terrain_data.is_null()) {
		return Vector2i(-1, -1);
	}
	const real_t tile_world_size = terrain_data->get_tile_world_size();
	if (tile_world_size <= 0.0) {
		return Vector2i(-1, -1);
	}
	return Vector2i(Math::floor(p_local_position.x / tile_world_size), Math::floor(p_local_position.z / tile_world_size));
}

void OpenWorldTerrain3D::_rebuild_height_textures() {
	for (TerrainTile &tile : tiles) {
		tile.height_image = _build_tile_height_image(tile.cell);
		if (tile.height_texture.is_null() || tile.height_texture->get_width() != tile.height_image->get_width() || tile.height_texture->get_height() != tile.height_image->get_height()) {
			tile.height_texture = ImageTexture::create_from_image(tile.height_image);
		} else {
			tile.height_texture->update(tile.height_image);
		}
		_update_tile_material(tile);
	}
	_sync_tile_materials();
}

void OpenWorldTerrain3D::_rebuild_layer_textures() {
	for (TerrainTile &tile : tiles) {
		tile.layer_image = _build_tile_layer_image(tile.cell);
		if (tile.layer_texture.is_null() || tile.layer_texture->get_width() != tile.layer_image->get_width() || tile.layer_texture->get_height() != tile.layer_image->get_height()) {
			tile.layer_texture = ImageTexture::create_from_image(tile.layer_image);
		} else {
			tile.layer_texture->update(tile.layer_image);
		}
		_update_tile_material(tile);
	}
	_sync_tile_materials();
}

void OpenWorldTerrain3D::_refresh_height_texture_region(const Vector2i &p_cell, int p_min_x, int p_min_y, int p_max_x, int p_max_y) {
	_ensure_data();
	const int resolution = terrain_data->get_tile_resolution();
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

	const PackedFloat32Array heights = terrain_data->get_tile_height_data(p_cell);
	for (TerrainTile &tile : tiles) {
		if (tile.cell != p_cell) {
			continue;
		}

		if (tile.height_image.is_null() || tile.height_texture.is_null()) {
			tile.height_image = _build_tile_height_image(tile.cell);
			tile.height_texture = ImageTexture::create_from_image(tile.height_image);
			_update_tile_material(tile);
		} else {
			const int patch_width = max_x - min_x + 1;
			const int patch_height = max_y - min_y + 1;
			Ref<Image> patch_image = Image::create_empty(patch_width, patch_height, false, Image::FORMAT_RF);
			for (int y = min_y; y <= max_y; y++) {
				for (int x = min_x; x <= max_x; x++) {
					const int index = terrain_data->get_tile_height_index(x, y);
					const real_t height = index < heights.size() ? CLAMP(heights[index], (real_t)0.0, (real_t)1.0) : 0.0;
					tile.height_image->set_pixel(x, y, Color(height, 0.0, 0.0, 1.0));
					patch_image->set_pixel(x - min_x, y - min_y, Color(height, 0.0, 0.0, 1.0));
				}
			}
			tile.height_texture->update_region(patch_image, Point2i(min_x, min_y));
		}
		break;
	}
}

void OpenWorldTerrain3D::_refresh_layer_texture_region(const Vector2i &p_cell, int p_min_x, int p_min_y, int p_max_x, int p_max_y) {
	_ensure_data();
	const int resolution = terrain_data->get_tile_resolution();
	if (resolution < 1) {
		return;
	}
	if (tiles.is_empty()) {
		_rebuild_tiles();
		return;
	}

	const int min_x = CLAMP(p_min_x, 0, resolution - 1);
	const int min_y = CLAMP(p_min_y, 0, resolution - 1);
	const int max_x = CLAMP(p_max_x, 0, resolution - 1);
	const int max_y = CLAMP(p_max_y, 0, resolution - 1);
	if (min_x > max_x || min_y > max_y) {
		return;
	}

	const PackedColorArray layer_values = terrain_data->get_tile_layer_data(p_cell);
	for (TerrainTile &tile : tiles) {
		if (tile.cell != p_cell) {
			continue;
		}

		if (tile.layer_image.is_null() || tile.layer_texture.is_null()) {
			tile.layer_image = _build_tile_layer_image(tile.cell);
			tile.layer_texture = ImageTexture::create_from_image(tile.layer_image);
			_update_tile_material(tile);
		} else {
			const int patch_width = max_x - min_x + 1;
			const int patch_height = max_y - min_y + 1;
			Ref<Image> patch_image = Image::create_empty(patch_width, patch_height, false, Image::FORMAT_RGBA8);
			for (int y = min_y; y <= max_y; y++) {
				for (int x = min_x; x <= max_x; x++) {
					const int index = terrain_data->get_tile_height_index(x, y);
					const Color layer = index < layer_values.size() ? layer_values[index] : Color(0.0, 0.0, 0.0, 0.0);
					const Color clamped_layer = Color(
							CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
							CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
							CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
							CLAMP(layer.a, (real_t)0.0, (real_t)1.0));
					tile.layer_image->set_pixel(x, y, clamped_layer);
					patch_image->set_pixel(x - min_x, y - min_y, clamped_layer);
				}
			}
			tile.layer_texture->update_region(patch_image, Point2i(min_x, min_y));
		}
		break;
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
	r_tile.material->set_shader_parameter("layer_texture", r_tile.layer_texture);
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
	if (low_normal_texture.is_valid() && low_normal_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("low_normal", low_normal_texture);
		r_tile.material->set_shader_parameter("use_low_normal", true);
	} else {
		r_tile.material->set_shader_parameter("low_normal", Variant());
		r_tile.material->set_shader_parameter("use_low_normal", false);
	}
	if (mid_normal_texture.is_valid() && mid_normal_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("mid_normal", mid_normal_texture);
		r_tile.material->set_shader_parameter("use_mid_normal", true);
	} else {
		r_tile.material->set_shader_parameter("mid_normal", Variant());
		r_tile.material->set_shader_parameter("use_mid_normal", false);
	}
	if (high_normal_texture.is_valid() && high_normal_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("high_normal", high_normal_texture);
		r_tile.material->set_shader_parameter("use_high_normal", true);
	} else {
		r_tile.material->set_shader_parameter("high_normal", Variant());
		r_tile.material->set_shader_parameter("use_high_normal", false);
	}
	if (low_roughness_texture.is_valid() && low_roughness_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("low_roughness_texture", low_roughness_texture);
		r_tile.material->set_shader_parameter("use_low_roughness_texture", true);
	} else {
		r_tile.material->set_shader_parameter("low_roughness_texture", Variant());
		r_tile.material->set_shader_parameter("use_low_roughness_texture", false);
	}
	if (mid_roughness_texture.is_valid() && mid_roughness_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("mid_roughness_texture", mid_roughness_texture);
		r_tile.material->set_shader_parameter("use_mid_roughness_texture", true);
	} else {
		r_tile.material->set_shader_parameter("mid_roughness_texture", Variant());
		r_tile.material->set_shader_parameter("use_mid_roughness_texture", false);
	}
	if (high_roughness_texture.is_valid() && high_roughness_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("high_roughness_texture", high_roughness_texture);
		r_tile.material->set_shader_parameter("use_high_roughness_texture", true);
	} else {
		r_tile.material->set_shader_parameter("high_roughness_texture", Variant());
		r_tile.material->set_shader_parameter("use_high_roughness_texture", false);
	}
	if (low_ao_texture.is_valid() && low_ao_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("low_ao_texture", low_ao_texture);
		r_tile.material->set_shader_parameter("use_low_ao_texture", true);
	} else {
		r_tile.material->set_shader_parameter("low_ao_texture", Variant());
		r_tile.material->set_shader_parameter("use_low_ao_texture", false);
	}
	if (mid_ao_texture.is_valid() && mid_ao_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("mid_ao_texture", mid_ao_texture);
		r_tile.material->set_shader_parameter("use_mid_ao_texture", true);
	} else {
		r_tile.material->set_shader_parameter("mid_ao_texture", Variant());
		r_tile.material->set_shader_parameter("use_mid_ao_texture", false);
	}
	if (high_ao_texture.is_valid() && high_ao_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("high_ao_texture", high_ao_texture);
		r_tile.material->set_shader_parameter("use_high_ao_texture", true);
	} else {
		r_tile.material->set_shader_parameter("high_ao_texture", Variant());
		r_tile.material->set_shader_parameter("use_high_ao_texture", false);
	}
	if (low_parallax_texture.is_valid() && low_parallax_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("low_parallax_texture", low_parallax_texture);
		r_tile.material->set_shader_parameter("use_low_parallax_texture", true);
	} else {
		r_tile.material->set_shader_parameter("low_parallax_texture", Variant());
		r_tile.material->set_shader_parameter("use_low_parallax_texture", false);
	}
	if (mid_parallax_texture.is_valid() && mid_parallax_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("mid_parallax_texture", mid_parallax_texture);
		r_tile.material->set_shader_parameter("use_mid_parallax_texture", true);
	} else {
		r_tile.material->set_shader_parameter("mid_parallax_texture", Variant());
		r_tile.material->set_shader_parameter("use_mid_parallax_texture", false);
	}
	if (high_parallax_texture.is_valid() && high_parallax_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("high_parallax_texture", high_parallax_texture);
		r_tile.material->set_shader_parameter("use_high_parallax_texture", true);
	} else {
		r_tile.material->set_shader_parameter("high_parallax_texture", Variant());
		r_tile.material->set_shader_parameter("use_high_parallax_texture", false);
	}
	if (macro_variation_texture.is_valid() && macro_variation_texture->get_rid().is_valid()) {
		r_tile.material->set_shader_parameter("macro_variation_texture", macro_variation_texture);
		r_tile.material->set_shader_parameter("use_macro_variation_texture", true);
	} else {
		r_tile.material->set_shader_parameter("macro_variation_texture", Variant());
		r_tile.material->set_shader_parameter("use_macro_variation_texture", false);
	}
	r_tile.material->set_shader_parameter("height_scale", terrain_data->get_height_scale());
	r_tile.material->set_shader_parameter("height_texel_size", Vector2(1.0 / (real_t)(r_tile.quad_width + 1), 1.0 / (real_t)(r_tile.quad_height + 1)));
	r_tile.material->set_shader_parameter("height_world_texel_size", terrain_data->get_tile_world_size() / MAX(1.0, (real_t)terrain_data->get_tile_resolution() - 1.0));
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
	r_tile.material->set_shader_parameter("low_roughness", low_roughness);
	r_tile.material->set_shader_parameter("mid_roughness", mid_roughness);
	r_tile.material->set_shader_parameter("high_roughness", high_roughness);
	r_tile.material->set_shader_parameter("ao_strength", ao_strength);
	r_tile.material->set_shader_parameter("parallax_enabled", parallax_enabled);
	r_tile.material->set_shader_parameter("parallax_scale", parallax_scale);
	r_tile.material->set_shader_parameter("parallax_flip", parallax_flip);
	r_tile.material->set_shader_parameter("parallax_fade_start", parallax_fade_start);
	r_tile.material->set_shader_parameter("parallax_fade_end", parallax_fade_end);
	r_tile.material->set_shader_parameter("anti_tiling_enabled", anti_tiling_enabled);
	r_tile.material->set_shader_parameter("anti_tiling_strength", anti_tiling_strength);
	r_tile.material->set_shader_parameter("macro_variation_scale", macro_variation_scale);
	r_tile.material->set_shader_parameter("macro_variation_strength", macro_variation_strength);
	r_tile.material->set_shader_parameter("splat_debug_mode", (int)splat_debug_mode);
}

void OpenWorldTerrain3D::_update_materials() {
	_sync_material_layers_from_resources();
	for (TerrainTile &tile : tiles) {
		_update_tile_material(tile);
	}
	_sync_tile_materials();
}

void OpenWorldTerrain3D::_sync_material_layers_from_resources() {
	if (terrain_layers.is_empty()) {
		return;
	}

	Vector<Ref<OpenWorldTerrainLayer>> material_layers;
	for (int i = 0; i < terrain_layers.size() && material_layers.size() < 3; i++) {
		Ref<OpenWorldTerrainLayer> layer = terrain_layers[i];
		if (layer.is_null() || !layer->is_material_enabled()) {
			continue;
		}
		material_layers.push_back(layer);
	}

	for (int i = 0; i < material_layers.size(); i++) {
		const Ref<OpenWorldTerrainLayer> &layer = material_layers[i];
		switch (i) {
			case 0:
				low_texture = layer->get_albedo_texture();
				low_normal_texture = layer->get_normal_texture();
				low_roughness_texture = layer->get_roughness_texture();
				low_ao_texture = layer->get_ao_texture();
				low_parallax_texture = layer->get_parallax_texture();
				low_color = layer->get_tint_color();
				low_texture_scale = layer->get_texture_scale();
				low_roughness = layer->get_roughness();
				break;
			case 1:
				mid_texture = layer->get_albedo_texture();
				mid_normal_texture = layer->get_normal_texture();
				mid_roughness_texture = layer->get_roughness_texture();
				mid_ao_texture = layer->get_ao_texture();
				mid_parallax_texture = layer->get_parallax_texture();
				mid_color = layer->get_tint_color();
				mid_texture_scale = layer->get_texture_scale();
				mid_roughness = layer->get_roughness();
				break;
			case 2:
				high_texture = layer->get_albedo_texture();
				high_normal_texture = layer->get_normal_texture();
				high_roughness_texture = layer->get_roughness_texture();
				high_ao_texture = layer->get_ao_texture();
				high_parallax_texture = layer->get_parallax_texture();
				high_color = layer->get_tint_color();
				high_texture_scale = layer->get_texture_scale();
				high_roughness = layer->get_roughness();
				break;
		}
	}
}

String OpenWorldTerrain3D::_get_builtin_displacement_shader_code() {
	return R"(
shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D height_texture : repeat_disable, filter_linear;
uniform sampler2D layer_texture : repeat_disable, filter_linear;
uniform sampler2D low_albedo : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D mid_albedo : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D high_albedo : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D low_normal : hint_normal, repeat_enable, filter_linear_mipmap;
uniform sampler2D mid_normal : hint_normal, repeat_enable, filter_linear_mipmap;
uniform sampler2D high_normal : hint_normal, repeat_enable, filter_linear_mipmap;
uniform sampler2D low_roughness_texture : hint_roughness_r, repeat_enable, filter_linear_mipmap;
uniform sampler2D mid_roughness_texture : hint_roughness_r, repeat_enable, filter_linear_mipmap;
uniform sampler2D high_roughness_texture : hint_roughness_r, repeat_enable, filter_linear_mipmap;
uniform sampler2D low_ao_texture : hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D mid_ao_texture : hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D high_ao_texture : hint_default_white, repeat_enable, filter_linear_mipmap;
uniform sampler2D low_parallax_texture : hint_default_black, repeat_enable, filter_linear_mipmap;
uniform sampler2D mid_parallax_texture : hint_default_black, repeat_enable, filter_linear_mipmap;
uniform sampler2D high_parallax_texture : hint_default_black, repeat_enable, filter_linear_mipmap;
uniform sampler2D macro_variation_texture : source_color, hint_default_white, repeat_enable, filter_linear_mipmap;
uniform bool use_low_normal = false;
uniform bool use_mid_normal = false;
uniform bool use_high_normal = false;
uniform bool use_low_roughness_texture = false;
uniform bool use_mid_roughness_texture = false;
uniform bool use_high_roughness_texture = false;
uniform bool use_low_ao_texture = false;
uniform bool use_mid_ao_texture = false;
uniform bool use_high_ao_texture = false;
uniform bool use_low_parallax_texture = false;
uniform bool use_mid_parallax_texture = false;
uniform bool use_high_parallax_texture = false;
uniform bool use_macro_variation_texture = false;
uniform float height_scale = 128.0;
uniform vec2 height_texel_size = vec2(0.00390625, 0.00390625);
uniform float height_world_texel_size = 4.0;
uniform vec4 low_color : source_color = vec4(0.22, 0.38, 0.18, 1.0);
uniform vec4 mid_color : source_color = vec4(0.42, 0.34, 0.22, 1.0);
uniform vec4 high_color : source_color = vec4(0.78, 0.78, 0.72, 1.0);
uniform float low_height = 0.28;
uniform float high_height = 0.68;
uniform float blend_width = 0.12;
uniform float texture_scale = 0.08;
uniform float low_texture_scale = 0.08;
uniform float mid_texture_scale = 0.08;
uniform float high_texture_scale = 0.08;
uniform float triplanar_sharpness = 4.0;
uniform float slope_start = 0.45;
uniform float slope_end = 0.85;
uniform float slope_high_strength = 0.65;
uniform float low_roughness = 0.92;
uniform float mid_roughness = 0.92;
uniform float high_roughness = 0.92;
uniform float ao_strength = 1.0;
uniform bool parallax_enabled = false;
uniform float parallax_scale = 0.03;
uniform bool parallax_flip = false;
uniform float parallax_fade_start = 120.0;
uniform float parallax_fade_end = 300.0;
uniform bool anti_tiling_enabled = false;
uniform float anti_tiling_strength = 0.15;
uniform float macro_variation_scale = 0.0025;
uniform float macro_variation_strength = 0.25;
uniform int splat_debug_mode = 0;

varying float terrain_height;
varying vec3 terrain_world_pos;
varying vec3 terrain_world_normal;
varying vec3 terrain_world_view_dir;
varying float terrain_camera_distance;

void vertex() {
	float height = texture(height_texture, UV).r;
	float left_height = texture(height_texture, UV + vec2(-height_texel_size.x, 0.0)).r;
	float right_height = texture(height_texture, UV + vec2(height_texel_size.x, 0.0)).r;
	float back_height = texture(height_texture, UV + vec2(0.0, -height_texel_size.y)).r;
	float front_height = texture(height_texture, UV + vec2(0.0, height_texel_size.y)).r;

	VERTEX.y += height * height_scale;
	NORMAL = normalize(vec3((left_height - right_height) * height_scale, 2.0 * height_world_texel_size, (back_height - front_height) * height_scale));
	TANGENT = normalize(vec3(0.0, 0.0, -1.0) * abs(NORMAL.x) + vec3(1.0, 0.0, 0.0) * abs(NORMAL.y) + vec3(1.0, 0.0, 0.0) * abs(NORMAL.z));
	BINORMAL = normalize(vec3(0.0, 1.0, 0.0) * abs(NORMAL.x) + vec3(0.0, 0.0, -1.0) * abs(NORMAL.y) + vec3(0.0, 1.0, 0.0) * abs(NORMAL.z));
	terrain_height = height;
	terrain_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	terrain_world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
	terrain_world_view_dir = normalize(CAMERA_POSITION_WORLD - terrain_world_pos);
	terrain_camera_distance = distance(CAMERA_POSITION_WORLD, terrain_world_pos);
}

float parallax_height_sample(sampler2D tex, vec2 coord) {
	float height = texture(tex, coord).r;
	return parallax_flip ? 1.0 - height : height;
}

vec2 apply_parallax_coord(sampler2D parallax_tex, bool use_parallax_tex, vec2 coord, vec2 view_plane) {
	if (!parallax_enabled || !use_parallax_tex) {
		return coord;
	}
	float height = parallax_height_sample(parallax_tex, coord) - 0.5;
	float fade_range = max(parallax_fade_end - parallax_fade_start, 0.001);
	float fade = 1.0 - smoothstep(parallax_fade_start, parallax_fade_start + fade_range, terrain_camera_distance);
	return coord - view_plane * height * parallax_scale * fade;
}

float hash21(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float value_noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	float a = hash21(i);
	float b = hash21(i + vec2(1.0, 0.0));
	float c = hash21(i + vec2(0.0, 1.0));
	float d = hash21(i + vec2(1.0, 1.0));
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

vec3 get_anti_tiling_coord_offset(vec3 world_pos) {
	if (!anti_tiling_enabled) {
		return vec3(0.0);
	}
	vec2 macro_coord = world_pos.xz * max(macro_variation_scale, 0.000001);
	float x_offset = value_noise(macro_coord + vec2(17.0, 43.0)) * 2.0 - 1.0;
	float z_offset = value_noise(macro_coord + vec2(71.0, 29.0)) * 2.0 - 1.0;
	float y_offset = value_noise(macro_coord + vec2(11.0, 89.0)) * 2.0 - 1.0;
	return vec3(x_offset, y_offset, z_offset) * anti_tiling_strength;
}

vec4 triplanar_sample(sampler2D tex, sampler2D parallax_tex, bool use_parallax_tex, vec3 world_pos, vec3 world_normal, vec3 view_dir, float layer_texture_scale) {
	vec3 blend = pow(abs(world_normal), vec3(max(triplanar_sharpness, 0.001)));
	blend /= max(dot(blend, vec3(1.0)), 0.001);
	vec3 coord = world_pos * max(layer_texture_scale, 0.0001) + get_anti_tiling_coord_offset(world_pos);
	vec4 x_sample = texture(tex, apply_parallax_coord(parallax_tex, use_parallax_tex, coord.yz, view_dir.yz));
	vec4 y_sample = texture(tex, apply_parallax_coord(parallax_tex, use_parallax_tex, coord.xz, view_dir.xz));
	vec4 z_sample = texture(tex, apply_parallax_coord(parallax_tex, use_parallax_tex, coord.xy, view_dir.xy));
	return x_sample * blend.x + y_sample * blend.y + z_sample * blend.z;
}

vec3 triplanar_normal_map_sample(sampler2D tex, sampler2D parallax_tex, bool use_parallax_tex, vec3 world_pos, vec3 world_normal, vec3 view_dir, float layer_texture_scale) {
	vec3 blend = pow(abs(world_normal), vec3(max(triplanar_sharpness, 0.001)));
	blend /= max(dot(blend, vec3(1.0)), 0.001);
	vec3 coord = world_pos * max(layer_texture_scale, 0.0001) + get_anti_tiling_coord_offset(world_pos);
	vec3 x_normal = texture(tex, apply_parallax_coord(parallax_tex, use_parallax_tex, coord.yz, view_dir.yz)).rgb;
	vec3 y_normal = texture(tex, apply_parallax_coord(parallax_tex, use_parallax_tex, coord.xz, view_dir.xz)).rgb;
	vec3 z_normal = texture(tex, apply_parallax_coord(parallax_tex, use_parallax_tex, coord.xy, view_dir.xy)).rgb;
	return x_normal * blend.x + y_normal * blend.y + z_normal * blend.z;
}

vec3 sample_macro_variation(vec3 world_pos) {
	if (!anti_tiling_enabled || !use_macro_variation_texture) {
		return vec3(1.0);
	}
	vec3 macro = texture(macro_variation_texture, world_pos.xz * max(macro_variation_scale, 0.000001)).rgb;
	return mix(vec3(1.0), macro * 2.0, clamp(macro_variation_strength, 0.0, 1.0));
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
	vec3 auto_weights = vec3(low_weight, mid_weight, high_weight);
	vec4 painted_layer = texture(layer_texture, UV);
	vec3 painted_weights = max(painted_layer.rgb, vec3(0.0));
	float painted_weight_sum = dot(painted_weights, vec3(1.0));
	if (painted_weight_sum > 0.001) {
		painted_weights /= painted_weight_sum;
		vec3 blended_weights = mix(auto_weights, painted_weights, clamp(painted_layer.a, 0.0, 1.0));
		float blended_weight_sum = max(dot(blended_weights, vec3(1.0)), 0.001);
		low_weight = blended_weights.r / blended_weight_sum;
		mid_weight = blended_weights.g / blended_weight_sum;
		high_weight = blended_weights.b / blended_weight_sum;
	}

	vec3 final_albedo = vec3(0.0);
	if (splat_debug_mode == 1) {
		final_albedo = vec3(terrain_height);
	} else if (splat_debug_mode == 2) {
		final_albedo = vec3(slope);
	} else if (splat_debug_mode == 3) {
		final_albedo = vec3(low_weight, mid_weight, high_weight);
	} else if (splat_debug_mode == 4) {
		final_albedo = vec3(painted_layer.rgb) * painted_layer.a;
	} else {
		vec3 view_dir = normalize(terrain_world_view_dir);
		vec4 low_layer = triplanar_sample(low_albedo, low_parallax_texture, use_low_parallax_texture, terrain_world_pos, terrain_world_normal, view_dir, low_texture_scale) * low_color;
		vec4 mid_layer = triplanar_sample(mid_albedo, mid_parallax_texture, use_mid_parallax_texture, terrain_world_pos, terrain_world_normal, view_dir, mid_texture_scale) * mid_color;
		vec4 high_layer = triplanar_sample(high_albedo, high_parallax_texture, use_high_parallax_texture, terrain_world_pos, terrain_world_normal, view_dir, high_texture_scale) * high_color;
		vec4 albedo = low_layer * low_weight + mid_layer * mid_weight + high_layer * high_weight;
		final_albedo = albedo.rgb * sample_macro_variation(terrain_world_pos);

		vec3 terrain_base_normal_map = vec3(0.5, 0.5, 1.0);
		vec3 terrain_base_normal = normalize(terrain_world_normal);
		vec3 low_layer_normal = use_low_normal ? triplanar_normal_map_sample(low_normal, low_parallax_texture, use_low_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, low_texture_scale) : terrain_base_normal_map;
		vec3 mid_layer_normal = use_mid_normal ? triplanar_normal_map_sample(mid_normal, mid_parallax_texture, use_mid_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, mid_texture_scale) : terrain_base_normal_map;
		vec3 high_layer_normal = use_high_normal ? triplanar_normal_map_sample(high_normal, high_parallax_texture, use_high_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, high_texture_scale) : terrain_base_normal_map;
		NORMAL_MAP = low_layer_normal * low_weight + mid_layer_normal * mid_weight + high_layer_normal * high_weight;
		NORMAL_MAP_DEPTH = 1.0;
	}

	ALBEDO = final_albedo;
	vec3 terrain_base_normal = normalize(terrain_world_normal);
	vec3 view_dir = normalize(terrain_world_view_dir);
	float low_layer_roughness = use_low_roughness_texture ? triplanar_sample(low_roughness_texture, low_parallax_texture, use_low_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, low_texture_scale).r : low_roughness;
	float mid_layer_roughness = use_mid_roughness_texture ? triplanar_sample(mid_roughness_texture, mid_parallax_texture, use_mid_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, mid_texture_scale).r : mid_roughness;
	float high_layer_roughness = use_high_roughness_texture ? triplanar_sample(high_roughness_texture, high_parallax_texture, use_high_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, high_texture_scale).r : high_roughness;
	ROUGHNESS = clamp(low_layer_roughness * low_weight + mid_layer_roughness * mid_weight + high_layer_roughness * high_weight, 0.0, 1.0);

	float low_layer_ao = use_low_ao_texture ? triplanar_sample(low_ao_texture, low_parallax_texture, use_low_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, low_texture_scale).r : 1.0;
	float mid_layer_ao = use_mid_ao_texture ? triplanar_sample(mid_ao_texture, mid_parallax_texture, use_mid_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, mid_texture_scale).r : 1.0;
	float high_layer_ao = use_high_ao_texture ? triplanar_sample(high_ao_texture, high_parallax_texture, use_high_parallax_texture, terrain_world_pos, terrain_base_normal, view_dir, high_texture_scale).r : 1.0;
	float blended_ao = clamp(low_layer_ao * low_weight + mid_layer_ao * mid_weight + high_layer_ao * high_weight, 0.0, 1.0);
	AO = mix(1.0, blended_ao, clamp(ao_strength, 0.0, 1.0));
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
		active_grid_cells = terrain_data->get_created_tile_cells();
		active_grid_cell_set.clear();
		for (int i = 0; i < active_grid_cells.size(); i++) {
			const Vector2 cell_value = active_grid_cells[i];
			active_grid_cell_set.insert(Vector2i(Math::floor(cell_value.x), Math::floor(cell_value.y)));
		}
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
	set_tile_resolution(tile_size + 1);
}

void OpenWorldTerrain3D::set_tile_world_size(real_t p_tile_world_size) {
	_ensure_data();
	terrain_data->set_tile_world_size(p_tile_world_size);
	_rebuild_tiles();
}

real_t OpenWorldTerrain3D::get_tile_world_size() const {
	return terrain_data.is_valid() ? terrain_data->get_tile_world_size() : 256.0;
}

void OpenWorldTerrain3D::set_tile_resolution(int p_tile_resolution) {
	_ensure_data();
	terrain_data->set_tile_resolution(p_tile_resolution);
	tile_size = MAX(1, terrain_data->get_tile_resolution() - 1);
	_rebuild_tiles();
}

int OpenWorldTerrain3D::get_tile_resolution() const {
	return terrain_data.is_valid() ? terrain_data->get_tile_resolution() : 257;
}

void OpenWorldTerrain3D::set_active_grid_cells(const PackedVector2Array &p_cells) {
	active_grid_cells = p_cells;
	active_grid_cell_set.clear();
	for (int i = 0; i < active_grid_cells.size(); i++) {
		const Vector2 cell_value = active_grid_cells[i];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		active_grid_cell_set.insert(cell);
	}

	PackedVector2Array normalized_cells;
	for (const Vector2i &cell : active_grid_cell_set) {
		normalized_cells.push_back(Vector2(cell.x, cell.y));
	}
	active_grid_cells = normalized_cells;
	_ensure_data();
	terrain_data->set_created_tile_cells(active_grid_cells);
	_rebuild_tiles();
}

bool OpenWorldTerrain3D::has_grid_cell(const Vector2i &p_cell) const {
	return _is_grid_cell_active(p_cell);
}

void OpenWorldTerrain3D::create_grid_cell(const Vector2i &p_cell) {
	_ensure_data();
	if (_is_grid_cell_active(p_cell)) {
		return;
	}
	terrain_data->create_tile(p_cell);
	active_grid_cells = terrain_data->get_created_tile_cells();
	active_grid_cell_set.insert(p_cell);
	_rebuild_tiles();
}

void OpenWorldTerrain3D::remove_grid_cell(const Vector2i &p_cell) {
	_ensure_data();
	if (!_is_grid_cell_active(p_cell)) {
		return;
	}
	terrain_data->remove_tile(p_cell);
	active_grid_cells = terrain_data->get_created_tile_cells();
	active_grid_cell_set.erase(p_cell);
	_rebuild_tiles();
}

void OpenWorldTerrain3D::set_world_size(real_t p_world_size) {
	_ensure_data();
	terrain_data->set_tile_world_size(p_world_size);
	_rebuild_tiles();
}

real_t OpenWorldTerrain3D::get_world_size() const {
	return terrain_data.is_valid() ? terrain_data->get_tile_world_size() : 256.0;
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

void OpenWorldTerrain3D::set_terrain_layers(const Array &p_layers) {
	for (int i = 0; i < terrain_layers.size(); i++) {
		Ref<OpenWorldTerrainLayer> previous_layer = terrain_layers[i];
		if (previous_layer.is_valid() && previous_layer->is_connected(CoreStringName(changed), callable_mp(this, &OpenWorldTerrain3D::_terrain_layer_changed))) {
			previous_layer->disconnect_changed(callable_mp(this, &OpenWorldTerrain3D::_terrain_layer_changed));
		}
	}

	terrain_layers.clear();
	for (int i = 0; i < p_layers.size(); i++) {
		Ref<OpenWorldTerrainLayer> layer = p_layers[i];
		if (layer.is_null()) {
			terrain_layers.push_back(Variant());
			continue;
		}
		if (!layer->is_connected(CoreStringName(changed), callable_mp(this, &OpenWorldTerrain3D::_terrain_layer_changed))) {
			layer->connect_changed(callable_mp(this, &OpenWorldTerrain3D::_terrain_layer_changed));
		}
		terrain_layers.push_back(layer);
	}
	_update_materials();
	notify_property_list_changed();
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

void OpenWorldTerrain3D::set_low_normal_texture(const Ref<Texture2D> &p_texture) {
	low_normal_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_normal_texture(const Ref<Texture2D> &p_texture) {
	mid_normal_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_high_normal_texture(const Ref<Texture2D> &p_texture) {
	high_normal_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_roughness_texture(const Ref<Texture2D> &p_texture) {
	low_roughness_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_roughness_texture(const Ref<Texture2D> &p_texture) {
	mid_roughness_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_high_roughness_texture(const Ref<Texture2D> &p_texture) {
	high_roughness_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_ao_texture(const Ref<Texture2D> &p_texture) {
	low_ao_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_ao_texture(const Ref<Texture2D> &p_texture) {
	mid_ao_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_high_ao_texture(const Ref<Texture2D> &p_texture) {
	high_ao_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_low_parallax_texture(const Ref<Texture2D> &p_texture) {
	low_parallax_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_parallax_texture(const Ref<Texture2D> &p_texture) {
	mid_parallax_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_high_parallax_texture(const Ref<Texture2D> &p_texture) {
	high_parallax_texture = p_texture;
	_update_materials();
}

void OpenWorldTerrain3D::set_macro_variation_texture(const Ref<Texture2D> &p_texture) {
	macro_variation_texture = p_texture;
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

void OpenWorldTerrain3D::set_low_roughness(real_t p_roughness) {
	low_roughness = CLAMP(p_roughness, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_mid_roughness(real_t p_roughness) {
	mid_roughness = CLAMP(p_roughness, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_high_roughness(real_t p_roughness) {
	high_roughness = CLAMP(p_roughness, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_ao_strength(real_t p_strength) {
	ao_strength = CLAMP(p_strength, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_parallax_enabled(bool p_enabled) {
	parallax_enabled = p_enabled;
	_update_materials();
}

void OpenWorldTerrain3D::set_parallax_scale(real_t p_scale) {
	parallax_scale = CLAMP(p_scale, (real_t)-1.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_parallax_flip(bool p_flip) {
	parallax_flip = p_flip;
	_update_materials();
}

void OpenWorldTerrain3D::set_parallax_fade_start(real_t p_distance) {
	parallax_fade_start = MAX((real_t)0.0, p_distance);
	if (parallax_fade_end < parallax_fade_start) {
		parallax_fade_end = parallax_fade_start;
	}
	_update_materials();
}

void OpenWorldTerrain3D::set_parallax_fade_end(real_t p_distance) {
	parallax_fade_end = MAX(parallax_fade_start, p_distance);
	_update_materials();
}

void OpenWorldTerrain3D::set_anti_tiling_enabled(bool p_enabled) {
	anti_tiling_enabled = p_enabled;
	_update_materials();
}

void OpenWorldTerrain3D::set_anti_tiling_strength(real_t p_strength) {
	anti_tiling_strength = CLAMP(p_strength, (real_t)0.0, (real_t)2.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_macro_variation_scale(real_t p_scale) {
	macro_variation_scale = MAX((real_t)0.000001, p_scale);
	_update_materials();
}

void OpenWorldTerrain3D::set_macro_variation_strength(real_t p_strength) {
	macro_variation_strength = CLAMP(p_strength, (real_t)0.0, (real_t)1.0);
	_update_materials();
}

void OpenWorldTerrain3D::set_splat_debug_mode(SplatDebugMode p_mode) {
	splat_debug_mode = p_mode;
	_update_materials();
}

void OpenWorldTerrain3D::set_flatten_height(real_t p_height) {
	flatten_height = CLAMP(p_height, (real_t)0.0, (real_t)1.0);
}

void OpenWorldTerrain3D::set_brush_falloff(real_t p_falloff) {
	brush_falloff = CLAMP(p_falloff, (real_t)0.0, (real_t)8.0);
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
	const int resolution = terrain_data->get_tile_resolution();
	if (resolution < 2) {
		return;
	}

	const real_t amplitude = MAX((real_t)0.0, p_amplitude);
	const real_t frequency = MAX((real_t)0.00001, p_frequency);
	const int octaves = CLAMP(p_octaves, 1, 12);
	real_t amplitude_sum = 0.0;
	for (int octave = 0; octave < octaves; octave++) {
		amplitude_sum += Math::pow((real_t)0.5, (real_t)octave);
	}

	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	for (int tile_index = 0; tile_index < created_cells.size(); tile_index++) {
		const Vector2 cell_value = created_cells[tile_index];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		PackedFloat32Array heights;
		heights.resize(resolution * resolution);
		for (int y = 0; y < resolution; y++) {
			for (int x = 0; x < resolution; x++) {
				real_t value = 0.0;
				real_t octave_amplitude = 1.0;
				real_t octave_frequency = frequency;
				const real_t sample_x = (real_t)(cell.x * (resolution - 1) + x);
				const real_t sample_y = (real_t)(cell.y * (resolution - 1) + y);
				for (int octave = 0; octave < octaves; octave++) {
					value += _sample_value_noise(sample_x * octave_frequency, sample_y * octave_frequency, p_seed + octave * 1013) * octave_amplitude;
					octave_amplitude *= 0.5;
					octave_frequency *= 2.0;
				}
				const real_t normalized_value = CLAMP((value / MAX((real_t)0.0001, amplitude_sum)) * amplitude, (real_t)0.0, (real_t)1.0);
				heights.set(terrain_data->get_tile_height_index(x, y), normalized_value);
			}
		}
		terrain_data->set_tile_height_data_no_notify(cell, heights);
	}
	_rebuild_height_textures();
	update_gizmos();
}

void OpenWorldTerrain3D::rebuild() {
	_ensure_data();
	_rebuild_tiles();
	_rebuild_height_textures();
	_rebuild_layer_textures();
	_update_materials();
}

void OpenWorldTerrain3D::apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	apply_brush_with_delta(p_world_position, p_radius, p_strength, p_operation);
}

Dictionary OpenWorldTerrain3D::apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	Dictionary delta;
	_ensure_data();

	const int resolution = terrain_data->get_tile_resolution();
	if (resolution < 2 || p_radius <= 0.0 || Math::is_zero_approx(p_strength)) {
		return delta;
	}

	const Vector3 local_position = get_global_transform().affine_inverse().xform(p_world_position);
	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	HashMap<Vector2i, int> tile_data_indices;
	for (int i = 0; i < created_cells.size(); i++) {
		const Vector2 cell_value = created_cells[i];
		tile_data_indices[Vector2i(Math::floor(cell_value.x), Math::floor(cell_value.y))] = i;
	}

	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const real_t texel_world_size = tile_world_size / (real_t)(resolution - 1);
	const real_t radius_texels = p_radius / texel_world_size;
	const Vector2i min_cell(Math::floor((local_position.x - p_radius) / tile_world_size), Math::floor((local_position.z - p_radius) / tile_world_size));
	const Vector2i max_cell(Math::floor((local_position.x + p_radius) / tile_world_size), Math::floor((local_position.z + p_radius) / tile_world_size));

	HashMap<Vector2i, PackedFloat32Array> edited_tiles;
	HashMap<Vector2i, Rect2i> dirty_rects;
	HashSet<Vector2i> edited_cells;
	HashMap<int, int> delta_positions;

	PackedInt32Array changed_indices;
	PackedFloat32Array before_values;
	PackedFloat32Array after_values;

	const auto get_tile_heights = [&](const Vector2i &p_cell) -> PackedFloat32Array {
		HashMap<Vector2i, PackedFloat32Array>::Iterator edited_iter = edited_tiles.find(p_cell);
		if (edited_iter) {
			return edited_iter->value;
		}
		return terrain_data->get_tile_height_data(p_cell);
	};

	const auto mark_dirty = [&](const Vector2i &p_cell, int p_x, int p_y) {
		Rect2i dirty_rect = dirty_rects.has(p_cell) ? dirty_rects[p_cell] : Rect2i(p_x, p_y, 1, 1);
		const int min_x = MIN(dirty_rect.position.x, p_x);
		const int min_y = MIN(dirty_rect.position.y, p_y);
		const int max_x = MAX(dirty_rect.position.x + dirty_rect.size.x - 1, p_x);
		const int max_y = MAX(dirty_rect.position.y + dirty_rect.size.y - 1, p_y);
		dirty_rects[p_cell] = Rect2i(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
	};

	const auto record_height_change = [&](const Vector2i &p_cell, PackedFloat32Array &r_heights, int p_x, int p_y, real_t p_after) {
		HashMap<Vector2i, int>::Iterator tile_index_iter = tile_data_indices.find(p_cell);
		if (!tile_index_iter) {
			return;
		}

		const int index = terrain_data->get_tile_height_index(p_x, p_y);
		const real_t before = r_heights[index];
		const real_t after = CLAMP(p_after, (real_t)0.0, (real_t)1.0);
		if (Math::is_equal_approx(before, after)) {
			return;
		}

		r_heights.set(index, after);
		edited_tiles[p_cell] = r_heights;
		edited_cells.insert(p_cell);
		mark_dirty(p_cell, p_x, p_y);

		const int encoded_index = tile_index_iter->value * resolution * resolution + index;
		HashMap<int, int>::Iterator delta_position_iter = delta_positions.find(encoded_index);
		if (delta_position_iter) {
			after_values.set(delta_position_iter->value, after);
		} else {
			delta_positions[encoded_index] = changed_indices.size();
			changed_indices.push_back(encoded_index);
			before_values.push_back(before);
			after_values.push_back(after);
		}
	};

	for (int cell_y = min_cell.y; cell_y <= max_cell.y; cell_y++) {
		for (int cell_x = min_cell.x; cell_x <= max_cell.x; cell_x++) {
			const Vector2i cell(cell_x, cell_y);
			if (!terrain_data->has_tile(cell)) {
				continue;
			}

			PackedFloat32Array heights = get_tile_heights(cell);
			if (heights.size() != resolution * resolution) {
				continue;
			}
			const PackedFloat32Array original_heights = p_operation == BRUSH_SMOOTH ? heights : PackedFloat32Array();
			const Vector2 local_in_tile(local_position.x - (real_t)cell.x * tile_world_size, local_position.z - (real_t)cell.y * tile_world_size);
			const real_t center_x = local_in_tile.x / texel_world_size;
			const real_t center_y = local_in_tile.y / texel_world_size;
			const int min_x = CLAMP(Math::floor(center_x - radius_texels), 0, resolution - 1);
			const int max_x = CLAMP(Math::ceil(center_x + radius_texels), 0, resolution - 1);
			const int min_y = CLAMP(Math::floor(center_y - radius_texels), 0, resolution - 1);
			const int max_y = CLAMP(Math::ceil(center_y + radius_texels), 0, resolution - 1);

			for (int y = min_y; y <= max_y; y++) {
				for (int x = min_x; x <= max_x; x++) {
					const real_t dx = ((real_t)x - center_x) * texel_world_size;
					const real_t dy = ((real_t)y - center_y) * texel_world_size;
					const real_t distance = Math::sqrt(dx * dx + dy * dy);
					if (distance > p_radius) {
						continue;
					}

					const real_t normalized_distance = CLAMP(distance / p_radius, (real_t)0.0, (real_t)1.0);
					real_t weight = 1.0;
					if (brush_falloff > 0.001) {
						const real_t falloff = 1.0 - normalized_distance;
						const real_t smooth_weight = falloff * falloff * (3.0 - 2.0 * falloff);
						weight = Math::pow(smooth_weight, brush_falloff);
					}
					const int index = terrain_data->get_tile_height_index(x, y);
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

					record_height_change(cell, heights, x, y, after);
				}
			}
		}
	}

	if (!edited_cells.is_empty()) {
		Vector<Vector2i> cells_to_sync;
		for (const Vector2i &cell : edited_cells) {
			cells_to_sync.push_back(cell);
		}

		const auto synchronize_edge = [&](const Vector2i &p_cell, const Vector2i &p_neighbor, bool p_horizontal) {
			if (!terrain_data->has_tile(p_cell) || !terrain_data->has_tile(p_neighbor)) {
				return;
			}
			if (!dirty_rects.has(p_cell) && !dirty_rects.has(p_neighbor)) {
				return;
			}

			const Rect2i cell_dirty = dirty_rects.has(p_cell) ? dirty_rects[p_cell] : Rect2i();
			const Rect2i neighbor_dirty = dirty_rects.has(p_neighbor) ? dirty_rects[p_neighbor] : Rect2i();
			const bool cell_edge_dirty = dirty_rects.has(p_cell) && (p_horizontal ?
							(p_neighbor.x > p_cell.x ? cell_dirty.position.x + cell_dirty.size.x - 1 >= resolution - 1 : cell_dirty.position.x <= 0) :
							(p_neighbor.y > p_cell.y ? cell_dirty.position.y + cell_dirty.size.y - 1 >= resolution - 1 : cell_dirty.position.y <= 0));
			const bool neighbor_edge_dirty = dirty_rects.has(p_neighbor) && (p_horizontal ?
							(p_neighbor.x > p_cell.x ? neighbor_dirty.position.x <= 0 : neighbor_dirty.position.x + neighbor_dirty.size.x - 1 >= resolution - 1) :
							(p_neighbor.y > p_cell.y ? neighbor_dirty.position.y <= 0 : neighbor_dirty.position.y + neighbor_dirty.size.y - 1 >= resolution - 1));
			if (!cell_edge_dirty && !neighbor_edge_dirty) {
				return;
			}

			int min_i = resolution - 1;
			int max_i = 0;
			if (cell_edge_dirty) {
				const int dirty_min_i = p_horizontal ? cell_dirty.position.y : cell_dirty.position.x;
				const int dirty_max_i = p_horizontal ? cell_dirty.position.y + cell_dirty.size.y - 1 : cell_dirty.position.x + cell_dirty.size.x - 1;
				min_i = MIN(min_i, dirty_min_i);
				max_i = MAX(max_i, dirty_max_i);
			}
			if (neighbor_edge_dirty) {
				const int dirty_min_i = p_horizontal ? neighbor_dirty.position.y : neighbor_dirty.position.x;
				const int dirty_max_i = p_horizontal ? neighbor_dirty.position.y + neighbor_dirty.size.y - 1 : neighbor_dirty.position.x + neighbor_dirty.size.x - 1;
				min_i = MIN(min_i, dirty_min_i);
				max_i = MAX(max_i, dirty_max_i);
			}
			min_i = CLAMP(min_i, 0, resolution - 1);
			max_i = CLAMP(max_i, 0, resolution - 1);

			PackedFloat32Array cell_heights = get_tile_heights(p_cell);
			PackedFloat32Array neighbor_heights = get_tile_heights(p_neighbor);
			if (cell_heights.size() != resolution * resolution || neighbor_heights.size() != resolution * resolution) {
				return;
			}

			for (int i = min_i; i <= max_i; i++) {
				const int cell_x = p_horizontal ? (p_neighbor.x > p_cell.x ? resolution - 1 : 0) : i;
				const int cell_y = p_horizontal ? i : (p_neighbor.y > p_cell.y ? resolution - 1 : 0);
				const int neighbor_x = p_horizontal ? (p_neighbor.x > p_cell.x ? 0 : resolution - 1) : i;
				const int neighbor_y = p_horizontal ? i : (p_neighbor.y > p_cell.y ? 0 : resolution - 1);
				const real_t cell_height = cell_heights[terrain_data->get_tile_height_index(cell_x, cell_y)];
				const real_t neighbor_height = neighbor_heights[terrain_data->get_tile_height_index(neighbor_x, neighbor_y)];
				const real_t synchronized_height = (cell_height + neighbor_height) * (real_t)0.5;
				record_height_change(p_cell, cell_heights, cell_x, cell_y, synchronized_height);
				record_height_change(p_neighbor, neighbor_heights, neighbor_x, neighbor_y, synchronized_height);
			}
		};

		for (const Vector2i &cell : cells_to_sync) {
			synchronize_edge(cell, cell + Vector2i(1, 0), true);
			synchronize_edge(cell, cell + Vector2i(-1, 0), true);
			synchronize_edge(cell, cell + Vector2i(0, 1), false);
			synchronize_edge(cell, cell + Vector2i(0, -1), false);
		}

		const auto synchronize_corner = [&](const Vector2i &p_corner) {
			const Vector2i corner_cells[4] = {
				Vector2i(p_corner.x - 1, p_corner.y - 1),
				Vector2i(p_corner.x, p_corner.y - 1),
				Vector2i(p_corner.x - 1, p_corner.y),
				Vector2i(p_corner.x, p_corner.y),
			};
			const Vector2i corner_indices[4] = {
				Vector2i(resolution - 1, resolution - 1),
				Vector2i(0, resolution - 1),
				Vector2i(resolution - 1, 0),
				Vector2i(0, 0),
			};

			bool dirty_corner = false;
			for (int i = 0; i < 4; i++) {
				if (!dirty_rects.has(corner_cells[i])) {
					continue;
				}
				const Rect2i dirty_rect = dirty_rects[corner_cells[i]];
				const Vector2i corner_index = corner_indices[i];
				if (corner_index.x >= dirty_rect.position.x &&
						corner_index.x < dirty_rect.position.x + dirty_rect.size.x &&
						corner_index.y >= dirty_rect.position.y &&
						corner_index.y < dirty_rect.position.y + dirty_rect.size.y) {
					dirty_corner = true;
					break;
				}
			}
			if (!dirty_corner) {
				return;
			}

			real_t total = 0.0;
			int count = 0;
			PackedFloat32Array corner_heights[4];
			for (int i = 0; i < 4; i++) {
				if (!terrain_data->has_tile(corner_cells[i])) {
					continue;
				}
				corner_heights[i] = get_tile_heights(corner_cells[i]);
				if (corner_heights[i].size() != resolution * resolution) {
					continue;
				}
				total += corner_heights[i][terrain_data->get_tile_height_index(corner_indices[i].x, corner_indices[i].y)];
				count++;
			}
			if (count < 2) {
				return;
			}

			const real_t synchronized_height = total / (real_t)count;
			for (int i = 0; i < 4; i++) {
				if (corner_heights[i].size() != resolution * resolution) {
					continue;
				}
				record_height_change(corner_cells[i], corner_heights[i], corner_indices[i].x, corner_indices[i].y, synchronized_height);
			}
		};

		for (const Vector2i &cell : cells_to_sync) {
			synchronize_corner(cell);
			synchronize_corner(cell + Vector2i(1, 0));
			synchronize_corner(cell + Vector2i(0, 1));
			synchronize_corner(cell + Vector2i(1, 1));
		}
	}

	if (!edited_tiles.is_empty()) {
		for (const KeyValue<Vector2i, PackedFloat32Array> &E : edited_tiles) {
			terrain_data->set_tile_height_data_no_notify(E.key, E.value);
		}
		for (const KeyValue<Vector2i, Rect2i> &E : dirty_rects) {
			const Rect2i dirty_rect = E.value;
			_refresh_height_texture_region(E.key, dirty_rect.position.x, dirty_rect.position.y, dirty_rect.position.x + dirty_rect.size.x - 1, dirty_rect.position.y + dirty_rect.size.y - 1);
		}
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

	const int resolution = terrain_data->get_tile_resolution();
	const int tile_value_count = resolution * resolution;
	HashMap<int, PackedFloat32Array> edited_tiles;
	HashMap<int, Rect2i> dirty_rects;
	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	for (int i = 0; i < p_indices.size(); i++) {
		const int encoded_index = p_indices[i];
		ERR_FAIL_COND(encoded_index < 0);
		const int tile_data_index = encoded_index / tile_value_count;
		const int index = encoded_index % tile_value_count;
		ERR_FAIL_INDEX(tile_data_index, created_cells.size());
		const real_t height = CLAMP(p_heights[i], (real_t)0.0, (real_t)1.0);
		PackedFloat32Array heights;
		HashMap<int, PackedFloat32Array>::Iterator height_iter = edited_tiles.find(tile_data_index);
		if (height_iter) {
			heights = height_iter->value;
		} else {
			const Vector2 cell_value = created_cells[tile_data_index];
			heights = terrain_data->get_tile_height_data(Vector2i(Math::floor(cell_value.x), Math::floor(cell_value.y)));
		}
		ERR_FAIL_INDEX(index, heights.size());
		if (Math::is_equal_approx(heights[index], height)) {
			continue;
		}
		heights.set(index, height);
		edited_tiles[tile_data_index] = heights;
		const int x = index % resolution;
		const int y = index / resolution;
		Rect2i dirty_rect = dirty_rects.has(tile_data_index) ? dirty_rects[tile_data_index] : Rect2i(x, y, 1, 1);
		const int min_x = MIN(dirty_rect.position.x, x);
		const int min_y = MIN(dirty_rect.position.y, y);
		const int max_x = MAX(dirty_rect.position.x + dirty_rect.size.x - 1, x);
		const int max_y = MAX(dirty_rect.position.y + dirty_rect.size.y - 1, y);
		dirty_rect = Rect2i(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
		dirty_rects[tile_data_index] = dirty_rect;
	}

	for (const KeyValue<int, PackedFloat32Array> &E : edited_tiles) {
		const int tile_data_index = E.key;
		const Vector2 cell_value = created_cells[tile_data_index];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		terrain_data->set_tile_height_data_no_notify(cell, E.value);
		const Rect2i dirty_rect = dirty_rects[tile_data_index];
		_refresh_height_texture_region(cell, dirty_rect.position.x, dirty_rect.position.y, dirty_rect.position.x + dirty_rect.size.x - 1, dirty_rect.position.y + dirty_rect.size.y - 1);
	}
	if (!edited_tiles.is_empty()) {
		update_gizmos();
	}
}

void OpenWorldTerrain3D::apply_layer_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, LayerPaintTarget p_target, real_t p_alpha, LayerBlendMode p_blend_mode) {
	apply_layer_brush_with_delta(p_world_position, p_radius, p_strength, p_target, p_alpha, p_blend_mode);
}

Dictionary OpenWorldTerrain3D::apply_layer_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, LayerPaintTarget p_target, real_t p_alpha, LayerBlendMode p_blend_mode) {
	Dictionary delta;
	_ensure_data();

	const int resolution = terrain_data->get_tile_resolution();
	if (resolution < 2 || p_radius <= 0.0 || Math::is_zero_approx(p_strength)) {
		return delta;
	}

	const Vector3 local_position = get_global_transform().affine_inverse().xform(p_world_position);
	const Vector2i cell = _get_tile_cell_for_local_position(local_position);
	if (!terrain_data->has_tile(cell)) {
		return delta;
	}
	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	int tile_data_index = -1;
	for (int i = 0; i < created_cells.size(); i++) {
		const Vector2 cell_value = created_cells[i];
		if (Vector2i(Math::floor(cell_value.x), Math::floor(cell_value.y)) == cell) {
			tile_data_index = i;
			break;
		}
	}
	ERR_FAIL_COND_V(tile_data_index < 0, delta);

	PackedColorArray mutable_layers = terrain_data->get_tile_layer_data(cell);
	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const real_t texel_world_size = tile_world_size / (real_t)(resolution - 1);
	const Vector2 local_in_tile(local_position.x - (real_t)cell.x * tile_world_size, local_position.z - (real_t)cell.y * tile_world_size);
	const real_t center_x = local_in_tile.x / texel_world_size;
	const real_t center_y = local_in_tile.y / texel_world_size;
	const real_t radius_texels = p_radius / texel_world_size;
	const int min_x = CLAMP(Math::floor(center_x - radius_texels), 0, resolution - 1);
	const int max_x = CLAMP(Math::ceil(center_x + radius_texels), 0, resolution - 1);
	const int min_y = CLAMP(Math::floor(center_y - radius_texels), 0, resolution - 1);
	const int max_y = CLAMP(Math::ceil(center_y + radius_texels), 0, resolution - 1);

	Color target_layer(1.0, 0.0, 0.0, 1.0);
	switch (p_target) {
		case LAYER_PAINT_LOW:
			target_layer = Color(1.0, 0.0, 0.0, 1.0);
			break;
		case LAYER_PAINT_MID:
			target_layer = Color(0.0, 1.0, 0.0, 1.0);
			break;
		case LAYER_PAINT_HIGH:
			target_layer = Color(0.0, 0.0, 1.0, 1.0);
			break;
	}
	target_layer.a = CLAMP(p_alpha, (real_t)0.0, (real_t)1.0);

	PackedInt32Array changed_indices;
	PackedColorArray before_values;
	PackedColorArray after_values;

	for (int y = min_y; y <= max_y; y++) {
		for (int x = min_x; x <= max_x; x++) {
			const real_t dx = ((real_t)x - center_x) * texel_world_size;
			const real_t dy = ((real_t)y - center_y) * texel_world_size;
			const real_t distance = Math::sqrt(dx * dx + dy * dy);
			if (distance > p_radius) {
				continue;
			}

			const real_t normalized_distance = CLAMP(distance / p_radius, (real_t)0.0, (real_t)1.0);
			real_t weight = 1.0;
			if (brush_falloff > 0.001) {
				const real_t falloff = 1.0 - normalized_distance;
				const real_t smooth_weight = falloff * falloff * (3.0 - 2.0 * falloff);
				weight = Math::pow(smooth_weight, brush_falloff);
			}

			const int index = terrain_data->get_tile_height_index(x, y);
			const Color before = mutable_layers[index];
			const real_t influence = CLAMP(p_strength * weight, (real_t)0.0, (real_t)1.0);
			Color blended_layer = target_layer;
			switch (p_blend_mode) {
				case LAYER_BLEND_NORMAL:
					blended_layer = target_layer;
					break;
				case LAYER_BLEND_MULTIPLY:
					blended_layer = Color(before.r * target_layer.r, before.g * target_layer.g, before.b * target_layer.b, target_layer.a);
					break;
				case LAYER_BLEND_DARKEN:
					blended_layer = Color(MIN(before.r, target_layer.r), MIN(before.g, target_layer.g), MIN(before.b, target_layer.b), target_layer.a);
					break;
				case LAYER_BLEND_LIGHTEN:
					blended_layer = Color(MAX(before.r, target_layer.r), MAX(before.g, target_layer.g), MAX(before.b, target_layer.b), target_layer.a);
					break;
			}
			Color after = before.lerp(blended_layer, influence);
			after.r = CLAMP(after.r, (real_t)0.0, (real_t)1.0);
			after.g = CLAMP(after.g, (real_t)0.0, (real_t)1.0);
			after.b = CLAMP(after.b, (real_t)0.0, (real_t)1.0);
			after.a = CLAMP(Math::lerp(before.a, target_layer.a, influence), (real_t)0.0, (real_t)1.0);

			if (before.is_equal_approx(after)) {
				continue;
			}

			mutable_layers.set(index, after);
			changed_indices.push_back(tile_data_index * resolution * resolution + index);
			before_values.push_back(before);
			after_values.push_back(after);
		}
	}

	if (!changed_indices.is_empty()) {
		terrain_data->set_tile_layer_data_no_notify(cell, mutable_layers);
		_refresh_layer_texture_region(cell, min_x, min_y, max_x, max_y);
		delta["indices"] = changed_indices;
		delta["before"] = before_values;
		delta["after"] = after_values;
	}

	return delta;
}

void OpenWorldTerrain3D::apply_layer_patch(const PackedInt32Array &p_indices, const PackedColorArray &p_layers) {
	_ensure_data();
	ERR_FAIL_COND(p_indices.size() != p_layers.size());

	const int resolution = terrain_data->get_tile_resolution();
	const int tile_value_count = resolution * resolution;
	HashMap<int, PackedColorArray> edited_tiles;
	HashMap<int, Rect2i> dirty_rects;
	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	for (int i = 0; i < p_indices.size(); i++) {
		const int encoded_index = p_indices[i];
		ERR_FAIL_COND(encoded_index < 0);
		const int tile_data_index = encoded_index / tile_value_count;
		const int index = encoded_index % tile_value_count;
		ERR_FAIL_INDEX(tile_data_index, created_cells.size());
		const Color layer = p_layers[i];
		const Color clamped_layer(
				CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
				CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
				CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
				CLAMP(layer.a, (real_t)0.0, (real_t)1.0));
		PackedColorArray mutable_layers;
		HashMap<int, PackedColorArray>::Iterator layer_iter = edited_tiles.find(tile_data_index);
		if (layer_iter) {
			mutable_layers = layer_iter->value;
		} else {
			const Vector2 cell_value = created_cells[tile_data_index];
			mutable_layers = terrain_data->get_tile_layer_data(Vector2i(Math::floor(cell_value.x), Math::floor(cell_value.y)));
		}
		ERR_FAIL_INDEX(index, mutable_layers.size());
		if (mutable_layers[index].is_equal_approx(clamped_layer)) {
			continue;
		}
		mutable_layers.set(index, clamped_layer);
		edited_tiles[tile_data_index] = mutable_layers;
		const int x = index % resolution;
		const int y = index / resolution;
		Rect2i dirty_rect = dirty_rects.has(tile_data_index) ? dirty_rects[tile_data_index] : Rect2i(x, y, 1, 1);
		const int min_x = MIN(dirty_rect.position.x, x);
		const int min_y = MIN(dirty_rect.position.y, y);
		const int max_x = MAX(dirty_rect.position.x + dirty_rect.size.x - 1, x);
		const int max_y = MAX(dirty_rect.position.y + dirty_rect.size.y - 1, y);
		dirty_rect = Rect2i(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
		dirty_rects[tile_data_index] = dirty_rect;
	}

	for (const KeyValue<int, PackedColorArray> &E : edited_tiles) {
		const int tile_data_index = E.key;
		const Vector2 cell_value = created_cells[tile_data_index];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		terrain_data->set_tile_layer_data_no_notify(cell, E.value);
		const Rect2i dirty_rect = dirty_rects[tile_data_index];
		_refresh_layer_texture_region(cell, dirty_rect.position.x, dirty_rect.position.y, dirty_rect.position.x + dirty_rect.size.x - 1, dirty_rect.position.y + dirty_rect.size.y - 1);
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
	const int resolution = terrain_data->get_tile_resolution();
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

	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const real_t texel_world_size = tile_world_size / (real_t)(resolution - 1);
	const real_t march_step = MAX(texel_world_size * 0.5, (real_t)0.25);
	Vector3 previous_position = local_origin + local_direction * t_min;
	bool previous_above = true;

	const int max_steps = CLAMP(Math::ceil((t_max - t_min) / march_step), 1, 2048);
	for (int step = 0; step <= max_steps; step++) {
		const real_t t = Math::lerp(t_min, t_max, (real_t)step / (real_t)max_steps);
		const Vector3 position = local_origin + local_direction * t;
		const Vector2i cell = _get_tile_cell_for_local_position(position);
		if (!_is_grid_cell_active(cell)) {
			previous_position = position;
			previous_above = true;
			continue;
		}

		const Vector2 local_in_tile(position.x - (real_t)cell.x * tile_world_size, position.z - (real_t)cell.y * tile_world_size);
		const real_t sample_x = local_in_tile.x / texel_world_size;
		const real_t sample_y = local_in_tile.y / texel_world_size;
		if (sample_x < 0.0 || sample_y < 0.0 || sample_x > (real_t)resolution - 1.0 || sample_y > (real_t)resolution - 1.0) {
			previous_position = position;
			continue;
		}

		const real_t terrain_height = _sample_tile_height_bilinear(cell, sample_x, sample_y) * terrain_data->get_height_scale();
		const bool above = position.y > terrain_height;
		if (step > 0 && previous_above && !above) {
			const int center_x = CLAMP(Math::floor(sample_x), 0, resolution - 2);
			const int center_y = CLAMP(Math::floor(sample_y), 0, resolution - 2);
			const Vector3 tile_origin((real_t)cell.x * tile_world_size, 0.0, (real_t)cell.y * tile_world_size);
			const auto get_tile_position = [&](int p_x, int p_y) -> Vector3 {
				return tile_origin + Vector3((real_t)p_x * texel_world_size, terrain_data->get_tile_height(cell, p_x, p_y) * terrain_data->get_height_scale(), (real_t)p_y * texel_world_size);
			};
			Vector3 best_position;
			real_t best_distance = Math::INF;
			bool found = false;

			for (int y = MAX(0, center_y - 1); y <= MIN(resolution - 2, center_y + 1); y++) {
				for (int x = MAX(0, center_x - 1); x <= MIN(resolution - 2, center_x + 1); x++) {
					const Vector3 v00 = get_tile_position(x, y);
					const Vector3 v10 = get_tile_position(x + 1, y);
					const Vector3 v01 = get_tile_position(x, y + 1);
					const Vector3 v11 = get_tile_position(x + 1, y + 1);
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

	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const real_t height_scale = terrain_data->get_height_scale();
	const real_t top = MAX((real_t)0.0, height_scale);
	const real_t bottom = MIN((real_t)0.0, height_scale);
	const int edge_indices[24] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
	};
	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	for (int tile_index = 0; tile_index < created_cells.size(); tile_index++) {
		const Vector2 cell_value = created_cells[tile_index];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		const real_t min_x = (real_t)cell.x * tile_world_size;
		const real_t min_z = (real_t)cell.y * tile_world_size;
		const real_t max_x = min_x + tile_world_size;
		const real_t max_z = min_z + tile_world_size;
		const Vector3 corners[8] = {
			Vector3(min_x, bottom, min_z),
			Vector3(max_x, bottom, min_z),
			Vector3(max_x, bottom, max_z),
			Vector3(min_x, bottom, max_z),
			Vector3(min_x, top, min_z),
			Vector3(max_x, top, min_z),
			Vector3(max_x, top, max_z),
			Vector3(min_x, top, max_z),
		};
		for (int i = 0; i < 24; i++) {
			lines.push_back(corners[edge_indices[i]]);
		}
	}

	return lines;
}

Ref<Texture2D> OpenWorldTerrain3D::get_height_texture() const {
	return tiles.is_empty() ? Ref<Texture2D>() : Ref<Texture2D>(tiles[0].height_texture);
}

AABB OpenWorldTerrain3D::get_aabb() const {
	if (terrain_data.is_null() || terrain_data->get_created_tile_cells().is_empty()) {
		return AABB(Vector3(), Vector3(0.001, 0.001, 0.001));
	}
	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const real_t height_scale = terrain_data.is_valid() ? terrain_data->get_height_scale() : 128.0;
	const real_t min_y = MIN((real_t)0.0, height_scale);
	const real_t max_y = MAX((real_t)0.0, height_scale);
	const PackedVector2Array created_cells = terrain_data->get_created_tile_cells();
	const Vector2 first_cell_value = created_cells[0];
	Vector2i first_cell(Math::floor(first_cell_value.x), Math::floor(first_cell_value.y));
	real_t min_x = (real_t)first_cell.x * tile_world_size;
	real_t min_z = (real_t)first_cell.y * tile_world_size;
	real_t max_x = min_x + tile_world_size;
	real_t max_z = min_z + tile_world_size;
	for (int i = 1; i < created_cells.size(); i++) {
		const Vector2 cell_value = created_cells[i];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		min_x = MIN(min_x, (real_t)cell.x * tile_world_size);
		min_z = MIN(min_z, (real_t)cell.y * tile_world_size);
		max_x = MAX(max_x, ((real_t)cell.x + 1.0) * tile_world_size);
		max_z = MAX(max_z, ((real_t)cell.y + 1.0) * tile_world_size);
	}
	return AABB(Vector3(min_x, min_y, min_z), Vector3(max_x - min_x, max_y - min_y, max_z - min_z));
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

		case NOTIFICATION_VISIBILITY_CHANGED: {
			_update_tile_visibility();
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
	ClassDB::bind_method(D_METHOD("set_tile_world_size", "tile_world_size"), &OpenWorldTerrain3D::set_tile_world_size);
	ClassDB::bind_method(D_METHOD("get_tile_world_size"), &OpenWorldTerrain3D::get_tile_world_size);
	ClassDB::bind_method(D_METHOD("set_tile_resolution", "tile_resolution"), &OpenWorldTerrain3D::set_tile_resolution);
	ClassDB::bind_method(D_METHOD("get_tile_resolution"), &OpenWorldTerrain3D::get_tile_resolution);
	ClassDB::bind_method(D_METHOD("set_active_grid_cells", "cells"), &OpenWorldTerrain3D::set_active_grid_cells);
	ClassDB::bind_method(D_METHOD("get_active_grid_cells"), &OpenWorldTerrain3D::get_active_grid_cells);
	ClassDB::bind_method(D_METHOD("set_world_size", "world_size"), &OpenWorldTerrain3D::set_world_size);
	ClassDB::bind_method(D_METHOD("get_world_size"), &OpenWorldTerrain3D::get_world_size);
	ClassDB::bind_method(D_METHOD("set_height_scale", "height_scale"), &OpenWorldTerrain3D::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &OpenWorldTerrain3D::get_height_scale);
	ClassDB::bind_method(D_METHOD("set_use_builtin_displacement_material", "use"), &OpenWorldTerrain3D::set_use_builtin_displacement_material);
	ClassDB::bind_method(D_METHOD("is_using_builtin_displacement_material"), &OpenWorldTerrain3D::is_using_builtin_displacement_material);
	ClassDB::bind_method(D_METHOD("set_terrain_material", "material"), &OpenWorldTerrain3D::set_terrain_material);
	ClassDB::bind_method(D_METHOD("get_terrain_material"), &OpenWorldTerrain3D::get_terrain_material);
	ClassDB::bind_method(D_METHOD("set_terrain_layers", "layers"), &OpenWorldTerrain3D::set_terrain_layers);
	ClassDB::bind_method(D_METHOD("get_terrain_layers"), &OpenWorldTerrain3D::get_terrain_layers);
	ClassDB::bind_method(D_METHOD("set_low_texture", "texture"), &OpenWorldTerrain3D::set_low_texture);
	ClassDB::bind_method(D_METHOD("get_low_texture"), &OpenWorldTerrain3D::get_low_texture);
	ClassDB::bind_method(D_METHOD("set_mid_texture", "texture"), &OpenWorldTerrain3D::set_mid_texture);
	ClassDB::bind_method(D_METHOD("get_mid_texture"), &OpenWorldTerrain3D::get_mid_texture);
	ClassDB::bind_method(D_METHOD("set_high_texture", "texture"), &OpenWorldTerrain3D::set_high_texture);
	ClassDB::bind_method(D_METHOD("get_high_texture"), &OpenWorldTerrain3D::get_high_texture);
	ClassDB::bind_method(D_METHOD("set_low_normal_texture", "texture"), &OpenWorldTerrain3D::set_low_normal_texture);
	ClassDB::bind_method(D_METHOD("get_low_normal_texture"), &OpenWorldTerrain3D::get_low_normal_texture);
	ClassDB::bind_method(D_METHOD("set_mid_normal_texture", "texture"), &OpenWorldTerrain3D::set_mid_normal_texture);
	ClassDB::bind_method(D_METHOD("get_mid_normal_texture"), &OpenWorldTerrain3D::get_mid_normal_texture);
	ClassDB::bind_method(D_METHOD("set_high_normal_texture", "texture"), &OpenWorldTerrain3D::set_high_normal_texture);
	ClassDB::bind_method(D_METHOD("get_high_normal_texture"), &OpenWorldTerrain3D::get_high_normal_texture);
	ClassDB::bind_method(D_METHOD("set_low_roughness_texture", "texture"), &OpenWorldTerrain3D::set_low_roughness_texture);
	ClassDB::bind_method(D_METHOD("get_low_roughness_texture"), &OpenWorldTerrain3D::get_low_roughness_texture);
	ClassDB::bind_method(D_METHOD("set_mid_roughness_texture", "texture"), &OpenWorldTerrain3D::set_mid_roughness_texture);
	ClassDB::bind_method(D_METHOD("get_mid_roughness_texture"), &OpenWorldTerrain3D::get_mid_roughness_texture);
	ClassDB::bind_method(D_METHOD("set_high_roughness_texture", "texture"), &OpenWorldTerrain3D::set_high_roughness_texture);
	ClassDB::bind_method(D_METHOD("get_high_roughness_texture"), &OpenWorldTerrain3D::get_high_roughness_texture);
	ClassDB::bind_method(D_METHOD("set_low_ao_texture", "texture"), &OpenWorldTerrain3D::set_low_ao_texture);
	ClassDB::bind_method(D_METHOD("get_low_ao_texture"), &OpenWorldTerrain3D::get_low_ao_texture);
	ClassDB::bind_method(D_METHOD("set_mid_ao_texture", "texture"), &OpenWorldTerrain3D::set_mid_ao_texture);
	ClassDB::bind_method(D_METHOD("get_mid_ao_texture"), &OpenWorldTerrain3D::get_mid_ao_texture);
	ClassDB::bind_method(D_METHOD("set_high_ao_texture", "texture"), &OpenWorldTerrain3D::set_high_ao_texture);
	ClassDB::bind_method(D_METHOD("get_high_ao_texture"), &OpenWorldTerrain3D::get_high_ao_texture);
	ClassDB::bind_method(D_METHOD("set_low_parallax_texture", "texture"), &OpenWorldTerrain3D::set_low_parallax_texture);
	ClassDB::bind_method(D_METHOD("get_low_parallax_texture"), &OpenWorldTerrain3D::get_low_parallax_texture);
	ClassDB::bind_method(D_METHOD("set_mid_parallax_texture", "texture"), &OpenWorldTerrain3D::set_mid_parallax_texture);
	ClassDB::bind_method(D_METHOD("get_mid_parallax_texture"), &OpenWorldTerrain3D::get_mid_parallax_texture);
	ClassDB::bind_method(D_METHOD("set_high_parallax_texture", "texture"), &OpenWorldTerrain3D::set_high_parallax_texture);
	ClassDB::bind_method(D_METHOD("get_high_parallax_texture"), &OpenWorldTerrain3D::get_high_parallax_texture);
	ClassDB::bind_method(D_METHOD("set_macro_variation_texture", "texture"), &OpenWorldTerrain3D::set_macro_variation_texture);
	ClassDB::bind_method(D_METHOD("get_macro_variation_texture"), &OpenWorldTerrain3D::get_macro_variation_texture);
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
	ClassDB::bind_method(D_METHOD("set_low_roughness", "roughness"), &OpenWorldTerrain3D::set_low_roughness);
	ClassDB::bind_method(D_METHOD("get_low_roughness"), &OpenWorldTerrain3D::get_low_roughness);
	ClassDB::bind_method(D_METHOD("set_mid_roughness", "roughness"), &OpenWorldTerrain3D::set_mid_roughness);
	ClassDB::bind_method(D_METHOD("get_mid_roughness"), &OpenWorldTerrain3D::get_mid_roughness);
	ClassDB::bind_method(D_METHOD("set_high_roughness", "roughness"), &OpenWorldTerrain3D::set_high_roughness);
	ClassDB::bind_method(D_METHOD("get_high_roughness"), &OpenWorldTerrain3D::get_high_roughness);
	ClassDB::bind_method(D_METHOD("set_ao_strength", "strength"), &OpenWorldTerrain3D::set_ao_strength);
	ClassDB::bind_method(D_METHOD("get_ao_strength"), &OpenWorldTerrain3D::get_ao_strength);
	ClassDB::bind_method(D_METHOD("set_parallax_enabled", "enabled"), &OpenWorldTerrain3D::set_parallax_enabled);
	ClassDB::bind_method(D_METHOD("is_parallax_enabled"), &OpenWorldTerrain3D::is_parallax_enabled);
	ClassDB::bind_method(D_METHOD("set_parallax_scale", "scale"), &OpenWorldTerrain3D::set_parallax_scale);
	ClassDB::bind_method(D_METHOD("get_parallax_scale"), &OpenWorldTerrain3D::get_parallax_scale);
	ClassDB::bind_method(D_METHOD("set_parallax_flip", "flip"), &OpenWorldTerrain3D::set_parallax_flip);
	ClassDB::bind_method(D_METHOD("is_parallax_flipped"), &OpenWorldTerrain3D::is_parallax_flipped);
	ClassDB::bind_method(D_METHOD("set_parallax_fade_start", "distance"), &OpenWorldTerrain3D::set_parallax_fade_start);
	ClassDB::bind_method(D_METHOD("get_parallax_fade_start"), &OpenWorldTerrain3D::get_parallax_fade_start);
	ClassDB::bind_method(D_METHOD("set_parallax_fade_end", "distance"), &OpenWorldTerrain3D::set_parallax_fade_end);
	ClassDB::bind_method(D_METHOD("get_parallax_fade_end"), &OpenWorldTerrain3D::get_parallax_fade_end);
	ClassDB::bind_method(D_METHOD("set_anti_tiling_enabled", "enabled"), &OpenWorldTerrain3D::set_anti_tiling_enabled);
	ClassDB::bind_method(D_METHOD("is_anti_tiling_enabled"), &OpenWorldTerrain3D::is_anti_tiling_enabled);
	ClassDB::bind_method(D_METHOD("set_anti_tiling_strength", "strength"), &OpenWorldTerrain3D::set_anti_tiling_strength);
	ClassDB::bind_method(D_METHOD("get_anti_tiling_strength"), &OpenWorldTerrain3D::get_anti_tiling_strength);
	ClassDB::bind_method(D_METHOD("set_macro_variation_scale", "scale"), &OpenWorldTerrain3D::set_macro_variation_scale);
	ClassDB::bind_method(D_METHOD("get_macro_variation_scale"), &OpenWorldTerrain3D::get_macro_variation_scale);
	ClassDB::bind_method(D_METHOD("set_macro_variation_strength", "strength"), &OpenWorldTerrain3D::set_macro_variation_strength);
	ClassDB::bind_method(D_METHOD("get_macro_variation_strength"), &OpenWorldTerrain3D::get_macro_variation_strength);
	ClassDB::bind_method(D_METHOD("set_splat_debug_mode", "mode"), &OpenWorldTerrain3D::set_splat_debug_mode);
	ClassDB::bind_method(D_METHOD("get_splat_debug_mode"), &OpenWorldTerrain3D::get_splat_debug_mode);
	ClassDB::bind_method(D_METHOD("set_flatten_height", "height"), &OpenWorldTerrain3D::set_flatten_height);
	ClassDB::bind_method(D_METHOD("get_flatten_height"), &OpenWorldTerrain3D::get_flatten_height);
	ClassDB::bind_method(D_METHOD("set_brush_falloff", "falloff"), &OpenWorldTerrain3D::set_brush_falloff);
	ClassDB::bind_method(D_METHOD("get_brush_falloff"), &OpenWorldTerrain3D::get_brush_falloff);
	ClassDB::bind_method(D_METHOD("set_show_debug_gizmo", "show"), &OpenWorldTerrain3D::set_show_debug_gizmo);
	ClassDB::bind_method(D_METHOD("is_showing_debug_gizmo"), &OpenWorldTerrain3D::is_showing_debug_gizmo);
	ClassDB::bind_method(D_METHOD("reset_flat_terrain", "normalized_height"), &OpenWorldTerrain3D::reset_flat_terrain, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("generate_random_terrain", "seed", "amplitude", "frequency", "octaves"), &OpenWorldTerrain3D::generate_random_terrain, DEFVAL(1), DEFVAL(1.0), DEFVAL(0.025), DEFVAL(4));
	ClassDB::bind_method(D_METHOD("rebuild"), &OpenWorldTerrain3D::rebuild);
	ClassDB::bind_method(D_METHOD("has_grid_cell", "cell"), &OpenWorldTerrain3D::has_grid_cell);
	ClassDB::bind_method(D_METHOD("create_grid_cell", "cell"), &OpenWorldTerrain3D::create_grid_cell);
	ClassDB::bind_method(D_METHOD("remove_grid_cell", "cell"), &OpenWorldTerrain3D::remove_grid_cell);
	ClassDB::bind_method(D_METHOD("apply_brush", "world_position", "radius", "strength", "operation"), &OpenWorldTerrain3D::apply_brush);
	ClassDB::bind_method(D_METHOD("apply_brush_with_delta", "world_position", "radius", "strength", "operation"), &OpenWorldTerrain3D::apply_brush_with_delta);
	ClassDB::bind_method(D_METHOD("apply_height_patch", "indices", "heights"), &OpenWorldTerrain3D::apply_height_patch);
	ClassDB::bind_method(D_METHOD("apply_layer_brush", "world_position", "radius", "strength", "target", "alpha", "blend_mode"), &OpenWorldTerrain3D::apply_layer_brush, DEFVAL(1.0), DEFVAL(LAYER_BLEND_NORMAL));
	ClassDB::bind_method(D_METHOD("apply_layer_brush_with_delta", "world_position", "radius", "strength", "target", "alpha", "blend_mode"), &OpenWorldTerrain3D::apply_layer_brush_with_delta, DEFVAL(1.0), DEFVAL(LAYER_BLEND_NORMAL));
	ClassDB::bind_method(D_METHOD("apply_layer_patch", "indices", "layers"), &OpenWorldTerrain3D::apply_layer_patch);
	ClassDB::bind_method(D_METHOD("get_brush_hit", "ray_origin", "ray_direction"), &OpenWorldTerrain3D::get_brush_hit);
	ClassDB::bind_method(D_METHOD("get_debug_lines"), &OpenWorldTerrain3D::get_debug_lines);
	ClassDB::bind_method(D_METHOD("get_height_texture"), &OpenWorldTerrain3D::get_height_texture);

	ADD_GROUP("Terrain", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_data", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldTerrainData"), "set_terrain_data", "get_terrain_data");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tile_world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m"), "set_tile_world_size", "get_tile_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_resolution", PROPERTY_HINT_RANGE, "2,16384,1,or_greater"), "set_tile_resolution", "get_tile_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "patch_resolution", PROPERTY_HINT_RANGE, "1,4096,1,or_greater", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_patch_resolution", "get_patch_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_size", PROPERTY_HINT_RANGE, "1,4096,1,or_greater", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_tile_size", "get_tile_size");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "active_grid_cells", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_active_grid_cells", "get_active_grid_cells");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_world_size", "get_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "-1000000,1000000,0.001,suffix:m"), "set_height_scale", "get_height_scale");
	ADD_GROUP("Material", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_builtin_displacement_material"), "set_use_builtin_displacement_material", "is_using_builtin_displacement_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_terrain_material", "get_terrain_material");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "terrain_layers", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("OpenWorldTerrainLayer")), "set_terrain_layers", "get_terrain_layers");
	ADD_GROUP("Height Splatting", "");
	ADD_SUBGROUP("Textures", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "low_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_low_texture", "get_low_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mid_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_mid_texture", "get_mid_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "high_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_high_texture", "get_high_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "low_normal_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_low_normal_texture", "get_low_normal_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mid_normal_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_mid_normal_texture", "get_mid_normal_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "high_normal_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_high_normal_texture", "get_high_normal_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "low_roughness_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_low_roughness_texture", "get_low_roughness_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mid_roughness_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_mid_roughness_texture", "get_mid_roughness_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "high_roughness_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_high_roughness_texture", "get_high_roughness_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "low_ao_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_low_ao_texture", "get_low_ao_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mid_ao_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_mid_ao_texture", "get_mid_ao_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "high_ao_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_high_ao_texture", "get_high_ao_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "low_parallax_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_low_parallax_texture", "get_low_parallax_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mid_parallax_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_mid_parallax_texture", "get_mid_parallax_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "high_parallax_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_high_parallax_texture", "get_high_parallax_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "macro_variation_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_macro_variation_texture", "get_macro_variation_texture");
	ADD_SUBGROUP("Colors and Heights", "");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "low_color"), "set_low_color", "get_low_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "mid_color"), "set_mid_color", "get_mid_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "high_color"), "set_high_color", "get_high_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_low_height", "get_low_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "high_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_high_height", "get_high_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blend_width", PROPERTY_HINT_RANGE, "0.001,1,0.001"), "set_blend_width", "get_blend_width");
	ADD_SUBGROUP("Mapping", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_texture_scale", "get_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_low_texture_scale", "get_low_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mid_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_mid_texture_scale", "get_mid_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "high_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_high_texture_scale", "get_high_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_sharpness", PROPERTY_HINT_RANGE, "0.001,32,0.001,or_greater"), "set_triplanar_sharpness", "get_triplanar_sharpness");
	ADD_SUBGROUP("Slope", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_start", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_slope_start", "get_slope_start");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_end", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_slope_end", "get_slope_end");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_high_strength", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_slope_high_strength", "get_slope_high_strength");
	ADD_SUBGROUP("Surface Response", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_roughness", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_low_roughness", "get_low_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mid_roughness", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_mid_roughness", "get_mid_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "high_roughness", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_high_roughness", "get_high_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ao_strength", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_ao_strength", "get_ao_strength");
	ADD_SUBGROUP("Parallax", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "parallax_enabled"), "set_parallax_enabled", "is_parallax_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "parallax_scale", PROPERTY_HINT_RANGE, "-1,1,0.001"), "set_parallax_scale", "get_parallax_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "parallax_flip"), "set_parallax_flip", "is_parallax_flipped");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "parallax_fade_start", PROPERTY_HINT_RANGE, "0,1000000,0.001,or_greater,suffix:m"), "set_parallax_fade_start", "get_parallax_fade_start");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "parallax_fade_end", PROPERTY_HINT_RANGE, "0,1000000,0.001,or_greater,suffix:m"), "set_parallax_fade_end", "get_parallax_fade_end");
	ADD_SUBGROUP("Anti Tiling", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anti_tiling_enabled"), "set_anti_tiling_enabled", "is_anti_tiling_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "anti_tiling_strength", PROPERTY_HINT_RANGE, "0,2,0.001"), "set_anti_tiling_strength", "get_anti_tiling_strength");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "macro_variation_scale", PROPERTY_HINT_RANGE, "0.000001,10,0.000001,or_greater"), "set_macro_variation_scale", "get_macro_variation_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "macro_variation_strength", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_macro_variation_strength", "get_macro_variation_strength");
	ADD_SUBGROUP("Debug", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "splat_debug_mode", PROPERTY_HINT_ENUM, "None,Height,Slope,Weights,Painted Layers"), "set_splat_debug_mode", "get_splat_debug_mode");
	ADD_GROUP("Brush", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flatten_height", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_flatten_height", "get_flatten_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "brush_falloff", PROPERTY_HINT_RANGE, "0,8,0.001"), "set_brush_falloff", "get_brush_falloff");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug_gizmo"), "set_show_debug_gizmo", "is_showing_debug_gizmo");

	BIND_ENUM_CONSTANT(BRUSH_RAISE);
	BIND_ENUM_CONSTANT(BRUSH_LOWER);
	BIND_ENUM_CONSTANT(BRUSH_SMOOTH);
	BIND_ENUM_CONSTANT(BRUSH_FLATTEN);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_NONE);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_HEIGHT);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_SLOPE);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_WEIGHTS);
	BIND_ENUM_CONSTANT(SPLAT_DEBUG_PAINTED_LAYERS);
	BIND_ENUM_CONSTANT(LAYER_PAINT_LOW);
	BIND_ENUM_CONSTANT(LAYER_PAINT_MID);
	BIND_ENUM_CONSTANT(LAYER_PAINT_HIGH);
	BIND_ENUM_CONSTANT(LAYER_BLEND_NORMAL);
	BIND_ENUM_CONSTANT(LAYER_BLEND_MULTIPLY);
	BIND_ENUM_CONSTANT(LAYER_BLEND_DARKEN);
	BIND_ENUM_CONSTANT(LAYER_BLEND_LIGHTEN);
}

OpenWorldTerrain3D::OpenWorldTerrain3D() {
}

OpenWorldTerrain3D::~OpenWorldTerrain3D() {
	_clear_tiles();
}
