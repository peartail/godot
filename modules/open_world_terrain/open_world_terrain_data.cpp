/**************************************************************************/
/*  open_world_terrain_data.cpp                                           */
/**************************************************************************/

#include "open_world_terrain_data.h"

#include "core/object/class_db.h"

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

void OpenWorldTerrainData::set_heightmap_resolution(int p_resolution) {
	heightmap_resolution = CLAMP(p_resolution, 2, 16384);
	_ensure_height_data_size(false);
	emit_changed();
}

void OpenWorldTerrainData::set_world_size(real_t p_world_size) {
	world_size = MAX(p_world_size, (real_t)0.001);
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

void OpenWorldTerrainData::resize(int p_resolution, real_t p_world_size, bool p_clear_existing) {
	heightmap_resolution = CLAMP(p_resolution, 2, 16384);
	world_size = MAX(p_world_size, (real_t)0.001);
	_ensure_height_data_size(p_clear_existing);
	emit_changed();
}

void OpenWorldTerrainData::fill_flat(real_t p_normalized_height) {
	_ensure_height_data_size(true);
	const real_t height = CLAMP(p_normalized_height, (real_t)0.0, (real_t)1.0);
	for (int i = 0; i < height_data.size(); i++) {
		height_data.set(i, height);
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

void OpenWorldTerrainData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_heightmap_resolution", "resolution"), &OpenWorldTerrainData::set_heightmap_resolution);
	ClassDB::bind_method(D_METHOD("get_heightmap_resolution"), &OpenWorldTerrainData::get_heightmap_resolution);
	ClassDB::bind_method(D_METHOD("set_world_size", "world_size"), &OpenWorldTerrainData::set_world_size);
	ClassDB::bind_method(D_METHOD("get_world_size"), &OpenWorldTerrainData::get_world_size);
	ClassDB::bind_method(D_METHOD("set_height_scale", "height_scale"), &OpenWorldTerrainData::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &OpenWorldTerrainData::get_height_scale);
	ClassDB::bind_method(D_METHOD("set_height_data", "height_data"), &OpenWorldTerrainData::set_height_data);
	ClassDB::bind_method(D_METHOD("get_height_data"), &OpenWorldTerrainData::get_height_data);
	ClassDB::bind_method(D_METHOD("resize", "resolution", "world_size", "clear_existing"), &OpenWorldTerrainData::resize, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("fill_flat", "normalized_height"), &OpenWorldTerrainData::fill_flat, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("get_height_index", "x", "y"), &OpenWorldTerrainData::get_height_index);
	ClassDB::bind_method(D_METHOD("get_height", "x", "y"), &OpenWorldTerrainData::get_height);
	ClassDB::bind_method(D_METHOD("set_height", "x", "y", "height"), &OpenWorldTerrainData::set_height);
	ClassDB::bind_method(D_METHOD("create_height_image"), &OpenWorldTerrainData::create_height_image);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "heightmap_resolution", PROPERTY_HINT_RANGE, "2,16384,1,or_greater"), "set_heightmap_resolution", "get_heightmap_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "world_size", PROPERTY_HINT_RANGE, "0.001,1000000,0.001,or_greater,suffix:m"), "set_world_size", "get_world_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "-1000000,1000000,0.001,suffix:m"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "height_data"), "set_height_data", "get_height_data");
}

OpenWorldTerrainData::OpenWorldTerrainData() {
	_ensure_height_data_size(true);
}
