/**************************************************************************/
/*  open_world_terrain_data.cpp                                           */
/**************************************************************************/

#include "open_world_terrain_data.h"

#include "core/object/class_db.h"
#include "core/templates/hash_set.h"

int OpenWorldTerrainData::_get_tile_array_index(const Vector2i &p_cell) const {
	for (int i = 0; i < created_tile_cells.size(); i++) {
		const Vector2 cell_value = created_tile_cells[i];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		if (cell == p_cell) {
			return i;
		}
	}
	return -1;
}

PackedFloat32Array OpenWorldTerrainData::_make_flat_tile_height_data(real_t p_normalized_height) const {
	PackedFloat32Array heights;
	heights.resize(_get_required_tile_value_count());
	const real_t height = CLAMP(p_normalized_height, (real_t)0.0, (real_t)1.0);
	for (int i = 0; i < heights.size(); i++) {
		heights.set(i, height);
	}
	return heights;
}

PackedColorArray OpenWorldTerrainData::_make_empty_tile_layer_data() const {
	PackedColorArray layers;
	layers.resize(_get_required_tile_value_count());
	for (int i = 0; i < layers.size(); i++) {
		layers.set(i, Color(0.0, 0.0, 0.0, 0.0));
	}
	return layers;
}

void OpenWorldTerrainData::_ensure_tile_data_size(bool p_clear_existing) {
	const int required_count = _get_required_tile_value_count();
	Array new_height_tiles;
	Array new_layer_tiles;
	new_height_tiles.resize(created_tile_cells.size());
	new_layer_tiles.resize(created_tile_cells.size());

	for (int tile_index = 0; tile_index < created_tile_cells.size(); tile_index++) {
		PackedFloat32Array heights;
		if (!p_clear_existing && tile_index < tile_height_data.size()) {
			heights = tile_height_data[tile_index];
		}
		PackedFloat32Array new_heights;
		new_heights.resize(required_count);
		for (int i = 0; i < required_count; i++) {
			const real_t height = i < heights.size() ? heights[i] : 0.0;
			new_heights.set(i, CLAMP(height, (real_t)0.0, (real_t)1.0));
		}
		new_height_tiles[tile_index] = new_heights;

		PackedColorArray layers;
		if (!p_clear_existing && tile_index < tile_layer_data.size()) {
			layers = tile_layer_data[tile_index];
		}
		PackedColorArray new_layers;
		new_layers.resize(required_count);
		for (int i = 0; i < required_count; i++) {
			const Color layer = i < layers.size() ? layers[i] : Color(0.0, 0.0, 0.0, 0.0);
			new_layers.set(i, Color(
					CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.a, (real_t)0.0, (real_t)1.0)));
		}
		new_layer_tiles[tile_index] = new_layers;
	}

	tile_height_data = new_height_tiles;
	tile_layer_data = new_layer_tiles;
}

void OpenWorldTerrainData::_ensure_height_data_size(bool p_clear_existing) {
	const int required_count = _get_required_height_count();
	if (height_data.size() == required_count && !p_clear_existing) {
		return;
	}

	PackedFloat32Array new_heights;
	new_heights.resize(required_count);
	for (int i = 0; i < required_count; i++) {
		new_heights.set(i, 0.0);
	}

	if (!p_clear_existing) {
		const int copy_count = MIN(height_data.size(), required_count);
		for (int i = 0; i < copy_count; i++) {
			new_heights.set(i, CLAMP(height_data[i], (real_t)0.0, (real_t)1.0));
		}
	}

	height_data = new_heights;
}

void OpenWorldTerrainData::_ensure_layer_data_size(bool p_clear_existing) {
	const int required_count = _get_required_height_count();
	if (layer_data.size() == required_count && !p_clear_existing) {
		return;
	}

	PackedColorArray new_layers;
	new_layers.resize(required_count);
	for (int i = 0; i < required_count; i++) {
		new_layers.set(i, Color(0.0, 0.0, 0.0, 0.0));
	}

	if (!p_clear_existing) {
		const int copy_count = MIN(layer_data.size(), required_count);
		for (int i = 0; i < copy_count; i++) {
			const Color layer = layer_data[i];
			new_layers.set(i, Color(
					CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.a, (real_t)0.0, (real_t)1.0)));
		}
	}

	layer_data = new_layers;
}

void OpenWorldTerrainData::set_tile_world_size(real_t p_tile_world_size) {
	tile_world_size = MAX(p_tile_world_size, (real_t)0.001);
	world_size = tile_world_size;
	emit_changed();
}

void OpenWorldTerrainData::set_tile_resolution(int p_tile_resolution) {
	tile_resolution = CLAMP(p_tile_resolution, 2, 16384);
	heightmap_resolution = tile_resolution;
	_ensure_height_data_size(false);
	_ensure_layer_data_size(false);
	_ensure_tile_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_heightmap_resolution(int p_resolution) {
	heightmap_resolution = CLAMP(p_resolution, 2, 16384);
	tile_resolution = heightmap_resolution;
	_ensure_height_data_size(false);
	_ensure_layer_data_size(false);
	_ensure_tile_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_world_size(real_t p_world_size) {
	world_size = MAX(p_world_size, (real_t)0.001);
	tile_world_size = world_size;
	emit_changed();
}

void OpenWorldTerrainData::set_height_scale(real_t p_height_scale) {
	height_scale = p_height_scale;
	emit_changed();
}

void OpenWorldTerrainData::set_height_data(const PackedFloat32Array &p_height_data) {
	height_data = p_height_data;
	_ensure_height_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::notify_height_data_changed() {
	_ensure_height_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_layer_data(const PackedColorArray &p_layer_data) {
	layer_data = p_layer_data;
	_ensure_layer_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::notify_layer_data_changed() {
	_ensure_layer_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_created_tile_cells(const PackedVector2Array &p_cells) {
	const PackedVector2Array old_cells = created_tile_cells;
	const Array old_height_tiles = tile_height_data;
	const Array old_layer_tiles = tile_layer_data;

	PackedVector2Array normalized_cells;
	Array normalized_heights;
	Array normalized_layers;
	HashSet<Vector2i> seen_cells;

	for (int i = 0; i < p_cells.size(); i++) {
		const Vector2 cell_value = p_cells[i];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		if (seen_cells.has(cell)) {
			continue;
		}
		seen_cells.insert(cell);
		normalized_cells.push_back(Vector2(cell.x, cell.y));

		int old_index = -1;
		for (int old_cell_index = 0; old_cell_index < old_cells.size(); old_cell_index++) {
			const Vector2 old_cell_value = old_cells[old_cell_index];
			const Vector2i old_cell(Math::floor(old_cell_value.x), Math::floor(old_cell_value.y));
			if (old_cell == cell) {
				old_index = old_cell_index;
				break;
			}
		}

		if (old_index >= 0 && old_index < old_height_tiles.size()) {
			normalized_heights.push_back(old_height_tiles[old_index]);
		} else {
			normalized_heights.push_back(_make_flat_tile_height_data());
		}
		if (old_index >= 0 && old_index < old_layer_tiles.size()) {
			normalized_layers.push_back(old_layer_tiles[old_index]);
		} else {
			normalized_layers.push_back(_make_empty_tile_layer_data());
		}
	}

	created_tile_cells = normalized_cells;
	tile_height_data = normalized_heights;
	tile_layer_data = normalized_layers;
	_ensure_tile_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_tile_height_data_array(const Array &p_tile_height_data) {
	tile_height_data = p_tile_height_data;
	_ensure_tile_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_tile_layer_data_array(const Array &p_tile_layer_data) {
	tile_layer_data = p_tile_layer_data;
	_ensure_tile_data_size(false);
	emit_changed();
}

bool OpenWorldTerrainData::has_tile(const Vector2i &p_cell) const {
	return _get_tile_array_index(p_cell) >= 0;
}

void OpenWorldTerrainData::create_tile(const Vector2i &p_cell) {
	if (has_tile(p_cell)) {
		return;
	}
	created_tile_cells.push_back(Vector2(p_cell.x, p_cell.y));
	tile_height_data.push_back(_make_flat_tile_height_data());
	tile_layer_data.push_back(_make_empty_tile_layer_data());
	emit_changed();
}

void OpenWorldTerrainData::remove_tile(const Vector2i &p_cell) {
	const int tile_index = _get_tile_array_index(p_cell);
	if (tile_index < 0) {
		return;
	}
	created_tile_cells.remove_at(tile_index);
	tile_height_data.remove_at(tile_index);
	tile_layer_data.remove_at(tile_index);
	emit_changed();
}

PackedFloat32Array OpenWorldTerrainData::get_tile_height_data(const Vector2i &p_cell) const {
	const int tile_index = _get_tile_array_index(p_cell);
	if (tile_index < 0 || tile_index >= tile_height_data.size()) {
		return PackedFloat32Array();
	}
	return tile_height_data[tile_index];
}

void OpenWorldTerrainData::set_tile_height_data(const Vector2i &p_cell, const PackedFloat32Array &p_height_data) {
	set_tile_height_data_no_notify(p_cell, p_height_data);
	emit_changed();
}

void OpenWorldTerrainData::set_tile_height_data_no_notify(const Vector2i &p_cell, const PackedFloat32Array &p_height_data) {
	const int tile_index = _get_tile_array_index(p_cell);
	ERR_FAIL_COND(tile_index < 0);
	PackedFloat32Array heights = p_height_data;
	const int required_count = _get_required_tile_value_count();
	if (heights.size() != required_count) {
		PackedFloat32Array resized_heights;
		resized_heights.resize(required_count);
		for (int i = 0; i < required_count; i++) {
			const real_t height = i < heights.size() ? heights[i] : 0.0;
			resized_heights.set(i, CLAMP(height, (real_t)0.0, (real_t)1.0));
		}
		heights = resized_heights;
	}
	tile_height_data[tile_index] = heights;
}

PackedColorArray OpenWorldTerrainData::get_tile_layer_data(const Vector2i &p_cell) const {
	const int tile_index = _get_tile_array_index(p_cell);
	if (tile_index < 0 || tile_index >= tile_layer_data.size()) {
		return PackedColorArray();
	}
	return tile_layer_data[tile_index];
}

void OpenWorldTerrainData::set_tile_layer_data(const Vector2i &p_cell, const PackedColorArray &p_layer_data) {
	set_tile_layer_data_no_notify(p_cell, p_layer_data);
	emit_changed();
}

void OpenWorldTerrainData::set_tile_layer_data_no_notify(const Vector2i &p_cell, const PackedColorArray &p_layer_data) {
	const int tile_index = _get_tile_array_index(p_cell);
	ERR_FAIL_COND(tile_index < 0);
	PackedColorArray layers = p_layer_data;
	const int required_count = _get_required_tile_value_count();
	if (layers.size() != required_count) {
		PackedColorArray resized_layers;
		resized_layers.resize(required_count);
		for (int i = 0; i < required_count; i++) {
			const Color layer = i < layers.size() ? layers[i] : Color(0.0, 0.0, 0.0, 0.0);
			resized_layers.set(i, Color(
					CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.a, (real_t)0.0, (real_t)1.0)));
		}
		layers = resized_layers;
	}
	tile_layer_data[tile_index] = layers;
}

int OpenWorldTerrainData::get_tile_height_index(int p_x, int p_y) const {
	ERR_FAIL_INDEX_V(p_x, tile_resolution, -1);
	ERR_FAIL_INDEX_V(p_y, tile_resolution, -1);
	return p_y * tile_resolution + p_x;
}

real_t OpenWorldTerrainData::get_tile_height(const Vector2i &p_cell, int p_x, int p_y) const {
	const int index = get_tile_height_index(p_x, p_y);
	ERR_FAIL_COND_V(index < 0, 0.0);
	const PackedFloat32Array heights = get_tile_height_data(p_cell);
	ERR_FAIL_INDEX_V(index, heights.size(), 0.0);
	return heights[index];
}

void OpenWorldTerrainData::set_tile_height(const Vector2i &p_cell, int p_x, int p_y, real_t p_height) {
	const int tile_index = _get_tile_array_index(p_cell);
	ERR_FAIL_COND(tile_index < 0);
	const int index = get_tile_height_index(p_x, p_y);
	ERR_FAIL_COND(index < 0);
	PackedFloat32Array heights = tile_height_data[tile_index];
	ERR_FAIL_INDEX(index, heights.size());
	heights.set(index, CLAMP(p_height, (real_t)0.0, (real_t)1.0));
	tile_height_data[tile_index] = heights;
	emit_changed();
}

Color OpenWorldTerrainData::get_tile_layer(const Vector2i &p_cell, int p_x, int p_y) const {
	const int index = get_tile_height_index(p_x, p_y);
	ERR_FAIL_COND_V(index < 0, Color(0.0, 0.0, 0.0, 0.0));
	const PackedColorArray layers = get_tile_layer_data(p_cell);
	ERR_FAIL_INDEX_V(index, layers.size(), Color(0.0, 0.0, 0.0, 0.0));
	return layers[index];
}

void OpenWorldTerrainData::set_tile_layer(const Vector2i &p_cell, int p_x, int p_y, const Color &p_layer) {
	const int tile_index = _get_tile_array_index(p_cell);
	ERR_FAIL_COND(tile_index < 0);
	const int index = get_tile_height_index(p_x, p_y);
	ERR_FAIL_COND(index < 0);
	PackedColorArray layers = tile_layer_data[tile_index];
	ERR_FAIL_INDEX(index, layers.size());
	layers.set(index, Color(
			CLAMP(p_layer.r, (real_t)0.0, (real_t)1.0),
			CLAMP(p_layer.g, (real_t)0.0, (real_t)1.0),
			CLAMP(p_layer.b, (real_t)0.0, (real_t)1.0),
			CLAMP(p_layer.a, (real_t)0.0, (real_t)1.0)));
	tile_layer_data[tile_index] = layers;
	emit_changed();
}

void OpenWorldTerrainData::resize(int p_resolution, real_t p_world_size, bool p_clear_existing) {
	heightmap_resolution = CLAMP(p_resolution, 2, 16384);
	world_size = MAX(p_world_size, (real_t)0.001);
	tile_resolution = heightmap_resolution;
	tile_world_size = world_size;
	_ensure_height_data_size(p_clear_existing);
	_ensure_layer_data_size(p_clear_existing);
	_ensure_tile_data_size(p_clear_existing);
	emit_changed();
}

void OpenWorldTerrainData::fill_flat(real_t p_normalized_height) {
	_ensure_height_data_size(true);
	const real_t height = CLAMP(p_normalized_height, (real_t)0.0, (real_t)1.0);
	for (int i = 0; i < height_data.size(); i++) {
		height_data.set(i, height);
	}
	for (int tile_index = 0; tile_index < tile_height_data.size(); tile_index++) {
		tile_height_data[tile_index] = _make_flat_tile_height_data(height);
	}
	emit_changed();
}

int OpenWorldTerrainData::get_height_index(int p_x, int p_y) const {
	ERR_FAIL_INDEX_V(p_x, heightmap_resolution, -1);
	ERR_FAIL_INDEX_V(p_y, heightmap_resolution, -1);
	return p_y * heightmap_resolution + p_x;
}

real_t OpenWorldTerrainData::get_height(int p_x, int p_y) const {
	const int index = get_height_index(p_x, p_y);
	ERR_FAIL_COND_V(index < 0, 0.0);
	ERR_FAIL_INDEX_V(index, height_data.size(), 0.0);
	return height_data[index];
}

void OpenWorldTerrainData::set_height(int p_x, int p_y, real_t p_height) {
	const int index = get_height_index(p_x, p_y);
	ERR_FAIL_COND(index < 0);
	ERR_FAIL_INDEX(index, height_data.size());
	height_data.set(index, CLAMP(p_height, (real_t)0.0, (real_t)1.0));
	emit_changed();
}

Color OpenWorldTerrainData::get_layer(int p_x, int p_y) const {
	const int index = get_height_index(p_x, p_y);
	ERR_FAIL_COND_V(index < 0, Color(0.0, 0.0, 0.0, 0.0));
	ERR_FAIL_INDEX_V(index, layer_data.size(), Color(0.0, 0.0, 0.0, 0.0));
	return layer_data[index];
}

void OpenWorldTerrainData::set_layer(int p_x, int p_y, const Color &p_layer) {
	const int index = get_height_index(p_x, p_y);
	ERR_FAIL_COND(index < 0);
	ERR_FAIL_INDEX(index, layer_data.size());
	layer_data.set(index, Color(
			CLAMP(p_layer.r, (real_t)0.0, (real_t)1.0),
			CLAMP(p_layer.g, (real_t)0.0, (real_t)1.0),
			CLAMP(p_layer.b, (real_t)0.0, (real_t)1.0),
			CLAMP(p_layer.a, (real_t)0.0, (real_t)1.0)));
	emit_changed();
}

Ref<Image> OpenWorldTerrainData::create_height_image() const {
	Ref<Image> image = Image::create_empty(heightmap_resolution, heightmap_resolution, false, Image::FORMAT_RF);
	for (int y = 0; y < heightmap_resolution; y++) {
		for (int x = 0; x < heightmap_resolution; x++) {
			const int index = y * heightmap_resolution + x;
			const real_t height = index < height_data.size() ? CLAMP(height_data[index], (real_t)0.0, (real_t)1.0) : 0.0;
			image->set_pixel(x, y, Color(height, 0.0, 0.0, 1.0));
		}
	}
	return image;
}

Ref<Image> OpenWorldTerrainData::create_layer_image() const {
	Ref<Image> image = Image::create_empty(heightmap_resolution, heightmap_resolution, false, Image::FORMAT_RGBA8);
	for (int y = 0; y < heightmap_resolution; y++) {
		for (int x = 0; x < heightmap_resolution; x++) {
			const int index = y * heightmap_resolution + x;
			const Color layer = index < layer_data.size() ? layer_data[index] : Color(0.0, 0.0, 0.0, 0.0);
			image->set_pixel(x, y, Color(
					CLAMP(layer.r, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.g, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.b, (real_t)0.0, (real_t)1.0),
					CLAMP(layer.a, (real_t)0.0, (real_t)1.0)));
		}
	}
	return image;
}

void OpenWorldTerrainData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tile_world_size", "tile_world_size"), &OpenWorldTerrainData::set_tile_world_size);
	ClassDB::bind_method(D_METHOD("get_tile_world_size"), &OpenWorldTerrainData::get_tile_world_size);
	ClassDB::bind_method(D_METHOD("set_tile_resolution", "tile_resolution"), &OpenWorldTerrainData::set_tile_resolution);
	ClassDB::bind_method(D_METHOD("get_tile_resolution"), &OpenWorldTerrainData::get_tile_resolution);
	ClassDB::bind_method(D_METHOD("set_heightmap_resolution", "resolution"), &OpenWorldTerrainData::set_heightmap_resolution);
	ClassDB::bind_method(D_METHOD("get_heightmap_resolution"), &OpenWorldTerrainData::get_heightmap_resolution);
	ClassDB::bind_method(D_METHOD("set_world_size", "world_size"), &OpenWorldTerrainData::set_world_size);
	ClassDB::bind_method(D_METHOD("get_world_size"), &OpenWorldTerrainData::get_world_size);
	ClassDB::bind_method(D_METHOD("set_height_scale", "height_scale"), &OpenWorldTerrainData::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &OpenWorldTerrainData::get_height_scale);
	ClassDB::bind_method(D_METHOD("set_height_data", "height_data"), &OpenWorldTerrainData::set_height_data);
	ClassDB::bind_method(D_METHOD("get_height_data"), &OpenWorldTerrainData::get_height_data);
	ClassDB::bind_method(D_METHOD("set_layer_data", "layer_data"), &OpenWorldTerrainData::set_layer_data);
	ClassDB::bind_method(D_METHOD("get_layer_data"), &OpenWorldTerrainData::get_layer_data);
	ClassDB::bind_method(D_METHOD("set_created_tile_cells", "cells"), &OpenWorldTerrainData::set_created_tile_cells);
	ClassDB::bind_method(D_METHOD("get_created_tile_cells"), &OpenWorldTerrainData::get_created_tile_cells);
	ClassDB::bind_method(D_METHOD("set_tile_height_data_array", "tile_height_data"), &OpenWorldTerrainData::set_tile_height_data_array);
	ClassDB::bind_method(D_METHOD("get_tile_height_data_array"), &OpenWorldTerrainData::get_tile_height_data_array);
	ClassDB::bind_method(D_METHOD("set_tile_layer_data_array", "tile_layer_data"), &OpenWorldTerrainData::set_tile_layer_data_array);
	ClassDB::bind_method(D_METHOD("get_tile_layer_data_array"), &OpenWorldTerrainData::get_tile_layer_data_array);
	ClassDB::bind_method(D_METHOD("has_tile", "cell"), &OpenWorldTerrainData::has_tile);
	ClassDB::bind_method(D_METHOD("create_tile", "cell"), &OpenWorldTerrainData::create_tile);
	ClassDB::bind_method(D_METHOD("remove_tile", "cell"), &OpenWorldTerrainData::remove_tile);
	ClassDB::bind_method(D_METHOD("get_tile_height_data", "cell"), &OpenWorldTerrainData::get_tile_height_data);
	ClassDB::bind_method(D_METHOD("set_tile_height_data", "cell", "height_data"), &OpenWorldTerrainData::set_tile_height_data);
	ClassDB::bind_method(D_METHOD("get_tile_layer_data", "cell"), &OpenWorldTerrainData::get_tile_layer_data);
	ClassDB::bind_method(D_METHOD("set_tile_layer_data", "cell", "layer_data"), &OpenWorldTerrainData::set_tile_layer_data);
	ClassDB::bind_method(D_METHOD("get_tile_height_index", "x", "y"), &OpenWorldTerrainData::get_tile_height_index);
	ClassDB::bind_method(D_METHOD("get_tile_height", "cell", "x", "y"), &OpenWorldTerrainData::get_tile_height);
	ClassDB::bind_method(D_METHOD("set_tile_height", "cell", "x", "y", "height"), &OpenWorldTerrainData::set_tile_height);
	ClassDB::bind_method(D_METHOD("get_tile_layer", "cell", "x", "y"), &OpenWorldTerrainData::get_tile_layer);
	ClassDB::bind_method(D_METHOD("set_tile_layer", "cell", "x", "y", "layer"), &OpenWorldTerrainData::set_tile_layer);
	ClassDB::bind_method(D_METHOD("resize", "resolution", "world_size", "clear_existing"), &OpenWorldTerrainData::resize, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("fill_flat", "normalized_height"), &OpenWorldTerrainData::fill_flat, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("get_height_index", "x", "y"), &OpenWorldTerrainData::get_height_index);
	ClassDB::bind_method(D_METHOD("get_height", "x", "y"), &OpenWorldTerrainData::get_height);
	ClassDB::bind_method(D_METHOD("set_height", "x", "y", "height"), &OpenWorldTerrainData::set_height);
	ClassDB::bind_method(D_METHOD("get_layer", "x", "y"), &OpenWorldTerrainData::get_layer);
	ClassDB::bind_method(D_METHOD("set_layer", "x", "y", "layer"), &OpenWorldTerrainData::set_layer);
	ClassDB::bind_method(D_METHOD("create_height_image"), &OpenWorldTerrainData::create_height_image);
	ClassDB::bind_method(D_METHOD("create_layer_image"), &OpenWorldTerrainData::create_layer_image);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tile_world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m"), "set_tile_world_size", "get_tile_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_resolution", PROPERTY_HINT_RANGE, "2,16384,1,or_greater"), "set_tile_resolution", "get_tile_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "-1000000,1000000,0.001,suffix:m"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "created_tile_cells"), "set_created_tile_cells", "get_created_tile_cells");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tile_height_data", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat32Array"), "set_tile_height_data_array", "get_tile_height_data_array");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tile_layer_data", PROPERTY_HINT_ARRAY_TYPE, "PackedColorArray"), "set_tile_layer_data_array", "get_tile_layer_data_array");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "heightmap_resolution", PROPERTY_HINT_RANGE, "2,16384,1,or_greater", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_heightmap_resolution", "get_heightmap_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_world_size", "get_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "height_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_height_data", "get_height_data");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "layer_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_layer_data", "get_layer_data");
}

OpenWorldTerrainData::OpenWorldTerrainData() {
	_ensure_height_data_size(true);
	_ensure_layer_data_size(true);
	_ensure_tile_data_size(true);
}
