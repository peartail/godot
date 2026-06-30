/**************************************************************************/
/*  simple_terrain_data.cpp                                                      */
/**************************************************************************/

#include "simple_terrain_data.h"

#include "core/object/class_db.h"
#include "core/templates/hash_set.h"


int SimpleTerrainData::_get_tile_array_index(const Vector2i &p_cell) const {
	for (int i = 0; i < created_tile_cells.size(); i++) {
		const Vector2 cell_value = created_tile_cells[i];
		const Vector2i cell(Math::floor(cell_value.x), Math::floor(cell_value.y));
		if (cell == p_cell) {
			return i;
		}
	}
	return -1;
}

PackedFloat32Array SimpleTerrainData::_make_flat_tile_height_data(real_t p_height) const {
	PackedFloat32Array heights;
	heights.resize(_get_required_tile_height_count());
	for (int i = 0; i < heights.size(); i++) {
		heights.set(i, p_height);
	}
	return heights;
}

void SimpleTerrainData::_ensure_tile_height_data_size(bool p_clear_existing) {
	const int required_size = _get_required_tile_height_count();
	Array new_height_tiles;
	new_height_tiles.resize(created_tile_cells.size());

	for (int tile_index = 0; tile_index < created_tile_cells.size(); tile_index++) {
		PackedFloat32Array previous;
		if (!p_clear_existing && tile_index < tile_height_data.size()) {
			previous = tile_height_data[tile_index];
		}

		PackedFloat32Array heights;
		heights.resize(required_size);
		const int copy_count = p_clear_existing ? 0 : MIN(previous.size(), required_size);
		for (int i = 0; i < copy_count; i++) {
			heights.set(i, previous[i]);
		}
		for (int i = copy_count; i < required_size; i++) {
			heights.set(i, 0.0);
		}
		new_height_tiles[tile_index] = heights;
	}

	tile_height_data = new_height_tiles;
}

void SimpleTerrainData::set_cell_size(real_t p_cell_size) {
	const real_t new_cell_size = MAX((real_t)0.01, p_cell_size);
	if (Math::is_equal_approx(cell_size, new_cell_size)) {
		return;
	}
	cell_size = new_cell_size;
	emit_changed();
}

void SimpleTerrainData::set_tile_size(int p_tile_size) {
	const int new_tile_size = MAX(2, p_tile_size);
	if (tile_size == new_tile_size) {
		return;
	}
	tile_size = new_tile_size;
	_ensure_tile_height_data_size(false);
	emit_changed();
}

void SimpleTerrainData::set_created_tile_cells(const PackedVector2Array &p_cells) {
	const PackedVector2Array old_cells = created_tile_cells;
	const Array old_height_tiles = tile_height_data;

	PackedVector2Array normalized_cells;
	Array normalized_heights;
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
	}

	created_tile_cells = normalized_cells;
	tile_height_data = normalized_heights;
	_ensure_tile_height_data_size(false);
	emit_changed();
}

void SimpleTerrainData::set_tile_height_data_array(const Array &p_tile_height_data) {
	tile_height_data = p_tile_height_data;
	_ensure_tile_height_data_size(false);
	emit_changed();
}

void SimpleTerrainData::notify_height_data_changed() {
	emit_changed();
}

void SimpleTerrainData::fill_flat(real_t p_height) {
	for (int tile_index = 0; tile_index < tile_height_data.size(); tile_index++) {
		tile_height_data[tile_index] = _make_flat_tile_height_data(p_height);
	}
	emit_changed();
}

bool SimpleTerrainData::has_tile(const Vector2i &p_cell) const {
	return _get_tile_array_index(p_cell) >= 0;
}

void SimpleTerrainData::create_tile(const Vector2i &p_cell) {
	if (has_tile(p_cell)) {
		return;
	}
	created_tile_cells.push_back(Vector2(p_cell.x, p_cell.y));
	tile_height_data.push_back(_make_flat_tile_height_data());
	emit_changed();
}

void SimpleTerrainData::remove_tile(const Vector2i &p_cell) {
	const int tile_index = _get_tile_array_index(p_cell);
	if (tile_index < 0) {
		return;
	}
	created_tile_cells.remove_at(tile_index);
	tile_height_data.remove_at(tile_index);
	emit_changed();
}

PackedFloat32Array SimpleTerrainData::get_tile_height_data(const Vector2i &p_cell) const {
	const int tile_index = _get_tile_array_index(p_cell);
	if (tile_index < 0 || tile_index >= tile_height_data.size()) {
		return PackedFloat32Array();
	}
	return tile_height_data[tile_index];
}

void SimpleTerrainData::set_tile_height_data(const Vector2i &p_cell, const PackedFloat32Array &p_height_data) {
	set_tile_height_data_no_notify(p_cell, p_height_data);
	emit_changed();
}

void SimpleTerrainData::set_tile_height_data_no_notify(const Vector2i &p_cell, const PackedFloat32Array &p_height_data) {
	const int tile_index = _get_tile_array_index(p_cell);
	ERR_FAIL_COND(tile_index < 0);

	PackedFloat32Array previous = p_height_data;
	PackedFloat32Array heights;
	const int required_size = _get_required_tile_height_count();
	heights.resize(required_size);
	const int copy_count = MIN(previous.size(), required_size);
	for (int i = 0; i < copy_count; i++) {
		heights.set(i, previous[i]);
	}
	for (int i = copy_count; i < required_size; i++) {
		heights.set(i, 0.0);
	}
	tile_height_data[tile_index] = heights;
}

int SimpleTerrainData::get_tile_height_index(int p_x, int p_z) const {
	ERR_FAIL_INDEX_V(p_x, get_tile_vertex_count(), -1);
	ERR_FAIL_INDEX_V(p_z, get_tile_vertex_count(), -1);
	return p_z * get_tile_vertex_count() + p_x;
}

real_t SimpleTerrainData::get_tile_height(const Vector2i &p_cell, int p_x, int p_z) const {
	const int index = get_tile_height_index(p_x, p_z);
	ERR_FAIL_COND_V(index < 0, 0.0);
	const PackedFloat32Array heights = get_tile_height_data(p_cell);
	ERR_FAIL_INDEX_V(index, heights.size(), 0.0);
	return heights[index];
}

void SimpleTerrainData::set_tile_height(const Vector2i &p_cell, int p_x, int p_z, real_t p_height) {
	const int tile_index = _get_tile_array_index(p_cell);
	ERR_FAIL_COND(tile_index < 0);
	const int index = get_tile_height_index(p_x, p_z);
	ERR_FAIL_COND(index < 0);
	PackedFloat32Array heights = tile_height_data[tile_index];
	ERR_FAIL_INDEX(index, heights.size());
	heights.set(index, p_height);
	tile_height_data[tile_index] = heights;
	emit_changed();
}

void SimpleTerrainData::_bind_methods() {
	// ClassDB exposes SimpleTerrainData as a scriptable Resource. Game projects can
	// create, save, load, and mutate this resource independently from SimpleTerrain3D,
	// then assign it to a SimpleTerrain3D node for rendering.
	ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &SimpleTerrainData::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &SimpleTerrainData::get_cell_size);
	ClassDB::bind_method(D_METHOD("set_tile_size", "tile_size"), &SimpleTerrainData::set_tile_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &SimpleTerrainData::get_tile_size);
	ClassDB::bind_method(D_METHOD("set_created_tile_cells", "cells"), &SimpleTerrainData::set_created_tile_cells);
	ClassDB::bind_method(D_METHOD("get_created_tile_cells"), &SimpleTerrainData::get_created_tile_cells);
	ClassDB::bind_method(D_METHOD("set_tile_height_data_array", "tile_height_data"), &SimpleTerrainData::set_tile_height_data_array);
	ClassDB::bind_method(D_METHOD("get_tile_height_data_array"), &SimpleTerrainData::get_tile_height_data_array);
	ClassDB::bind_method(D_METHOD("fill_flat", "height"), &SimpleTerrainData::fill_flat, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("get_tile_vertex_count"), &SimpleTerrainData::get_tile_vertex_count);
	ClassDB::bind_method(D_METHOD("has_tile", "cell"), &SimpleTerrainData::has_tile);
	ClassDB::bind_method(D_METHOD("create_tile", "cell"), &SimpleTerrainData::create_tile);
	ClassDB::bind_method(D_METHOD("remove_tile", "cell"), &SimpleTerrainData::remove_tile);
	ClassDB::bind_method(D_METHOD("get_tile_height_data", "cell"), &SimpleTerrainData::get_tile_height_data);
	ClassDB::bind_method(D_METHOD("set_tile_height_data", "cell", "height_data"), &SimpleTerrainData::set_tile_height_data);
	ClassDB::bind_method(D_METHOD("get_tile_height_index", "x", "z"), &SimpleTerrainData::get_tile_height_index);
	ClassDB::bind_method(D_METHOD("get_tile_height", "cell", "x", "z"), &SimpleTerrainData::get_tile_height);
	ClassDB::bind_method(D_METHOD("set_tile_height", "cell", "x", "z", "height"), &SimpleTerrainData::set_tile_height);

	// These properties appear in the Inspector and are serialized with the
	// resource, so they are the stable data contract for saved terrain assets.
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater"), "set_cell_size", "get_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_size", PROPERTY_HINT_RANGE, "2,4096,1,or_greater"), "set_tile_size", "get_tile_size");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "created_tile_cells"), "set_created_tile_cells", "get_created_tile_cells");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tile_height_data", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat32Array"), "set_tile_height_data_array", "get_tile_height_data_array");
}

SimpleTerrainData::SimpleTerrainData() {
	_ensure_tile_height_data_size(true);
}
