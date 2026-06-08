/**************************************************************************/
/*  simple_terrain_data.cpp                                                      */
/**************************************************************************/

#include "simple_terrain_data.h"

#include "core/object/class_db.h"

void SimpleTerrainData::set_grid_size(int p_grid_size) {
	const int new_grid_size = MAX(2, p_grid_size);
	if (grid_size == new_grid_size) {
		return;
	}
	grid_size = new_grid_size;
	ensure_height_data_size(false);
	emit_changed();
}

void SimpleTerrainData::set_cell_size(real_t p_cell_size) {
	const real_t new_cell_size = MAX((real_t)0.01, p_cell_size);
	if (Math::is_equal_approx(cell_size, new_cell_size)) {
		return;
	}
	cell_size = new_cell_size;
	emit_changed();
}

void SimpleTerrainData::set_height_data(const PackedFloat32Array &p_height_data) {
	height_data = p_height_data;
	// External callers may pass arrays from scripts, undo/redo, or saved data.
	// Normalize the array immediately so all indexed reads remain safe.
	ensure_height_data_size(false);
	emit_changed();
}

void SimpleTerrainData::notify_height_data_changed() {
	emit_changed();
}

void SimpleTerrainData::ensure_height_data_size(bool p_clear_existing) {
	const int required_size = _get_required_height_count();
	if (height_data.size() == required_size) {
		return;
	}

	PackedFloat32Array previous = height_data;
	height_data.resize(required_size);

	if (p_clear_existing) {
		// Used for new flat terrains where preserving stale values would be
		// surprising. The default height is intentionally 0 instead of noise.
		for (int i = 0; i < required_size; i++) {
			height_data.set(i, 0.0);
		}
		return;
	}

	// Resizing keeps the overlapping prefix. This is a simple stable behavior
	// for inspector edits and undo/redo; future versions can remap by x/z if
	// non-destructive resampling becomes important.
	const int copy_count = MIN(previous.size(), required_size);
	for (int i = 0; i < copy_count; i++) {
		height_data.set(i, previous[i]);
	}
	for (int i = copy_count; i < required_size; i++) {
		height_data.set(i, 0.0);
	}
}

void SimpleTerrainData::resize(int p_grid_size, real_t p_cell_size, bool p_clear_existing) {
	grid_size = MAX(2, p_grid_size);
	cell_size = MAX((real_t)0.01, p_cell_size);
	ensure_height_data_size(p_clear_existing);
	emit_changed();
}

void SimpleTerrainData::fill_flat(real_t p_height) {
	const int required_size = _get_required_height_count();
	height_data.resize(required_size);
	for (int i = 0; i < required_size; i++) {
		height_data.set(i, p_height);
	}
	emit_changed();
}

int SimpleTerrainData::get_height_index(int p_x, int p_z) const {
	ERR_FAIL_INDEX_V(p_x, get_vertex_count(), -1);
	ERR_FAIL_INDEX_V(p_z, get_vertex_count(), -1);
	return p_z * get_vertex_count() + p_x;
}

real_t SimpleTerrainData::get_height(int p_x, int p_z) const {
	const int index = get_height_index(p_x, p_z);
	ERR_FAIL_INDEX_V(index, height_data.size(), 0.0);
	return height_data[index];
}

void SimpleTerrainData::set_height(int p_x, int p_z, real_t p_height) {
	const int index = get_height_index(p_x, p_z);
	ERR_FAIL_INDEX(index, height_data.size());
	height_data.set(index, p_height);
	emit_changed();
}

void SimpleTerrainData::_bind_methods() {
	// ClassDB exposes SimpleTerrainData as a scriptable Resource. Game projects can
	// create, save, load, and mutate this resource independently from SimpleTerrain3D,
	// then assign it to a SimpleTerrain3D node for rendering.
	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &SimpleTerrainData::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &SimpleTerrainData::get_grid_size);
	ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &SimpleTerrainData::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &SimpleTerrainData::get_cell_size);
	ClassDB::bind_method(D_METHOD("set_height_data", "height_data"), &SimpleTerrainData::set_height_data);
	ClassDB::bind_method(D_METHOD("get_height_data"), &SimpleTerrainData::get_height_data);
	ClassDB::bind_method(D_METHOD("resize", "grid_size", "cell_size", "clear_existing"), &SimpleTerrainData::resize, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("fill_flat", "height"), &SimpleTerrainData::fill_flat, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("get_vertex_count"), &SimpleTerrainData::get_vertex_count);
	ClassDB::bind_method(D_METHOD("get_height_index", "x", "z"), &SimpleTerrainData::get_height_index);
	ClassDB::bind_method(D_METHOD("get_height", "x", "z"), &SimpleTerrainData::get_height);
	ClassDB::bind_method(D_METHOD("set_height", "x", "z", "height"), &SimpleTerrainData::set_height);

	// These properties appear in the Inspector and are serialized with the
	// resource, so they are the stable data contract for saved terrain assets.
	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size", PROPERTY_HINT_RANGE, "2,512,1,or_greater"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater"), "set_cell_size", "get_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "height_data"), "set_height_data", "get_height_data");
}

SimpleTerrainData::SimpleTerrainData() {
	ensure_height_data_size(true);
}
