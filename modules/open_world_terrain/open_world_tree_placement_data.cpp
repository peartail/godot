/**************************************************************************/
/*  open_world_tree_placement_data.cpp                                    */
/**************************************************************************/

#include "open_world_tree_placement_data.h"

#include "core/object/class_db.h"

void OpenWorldTreePlacementData::_ensure_record_arrays_size() {
	const int count = positions.size();

	const int old_rotation_count = rotations.size();
	const int old_scale_count = scales.size();
	const int old_seed_count = seeds.size();
	const int old_variant_count = variant_indices.size();
	const int old_id_count = instance_ids.size();
	const int old_enabled_count = enabled.size();
	const int old_color_count = colors.size();
	const int old_custom_count = custom_data.size();

	rotations.resize(count);
	scales.resize(count);
	seeds.resize(count);
	variant_indices.resize(count);
	instance_ids.resize(count);
	enabled.resize(count);
	colors.resize(count);
	custom_data.resize(count);

	int next_id = _get_next_instance_id();
	for (int i = old_rotation_count; i < count; i++) {
		rotations.set(i, Vector3());
	}
	for (int i = old_scale_count; i < count; i++) {
		scales.set(i, Vector3(1.0, 1.0, 1.0));
	}
	for (int i = old_seed_count; i < count; i++) {
		seeds.set(i, 0);
	}
	for (int i = old_variant_count; i < count; i++) {
		variant_indices.set(i, -1);
	}
	for (int i = old_id_count; i < count; i++) {
		instance_ids.set(i, next_id++);
	}
	for (int i = old_enabled_count; i < count; i++) {
		enabled.set(i, 1);
	}
	for (int i = old_color_count; i < count; i++) {
		colors.set(i, Color(1.0, 1.0, 1.0, 1.0));
	}
	for (int i = old_custom_count; i < count; i++) {
		custom_data.set(i, Color());
	}
}

int OpenWorldTreePlacementData::_get_next_instance_id() const {
	int next_id = 1;
	for (int i = 0; i < instance_ids.size(); i++) {
		next_id = MAX(next_id, instance_ids[i] + 1);
	}
	return next_id;
}

void OpenWorldTreePlacementData::set_positions(const PackedVector3Array &p_positions) {
	positions = p_positions;
	_ensure_record_arrays_size();
	emit_changed();
}

void OpenWorldTreePlacementData::set_rotations(const PackedVector3Array &p_rotations) {
	rotations = p_rotations;
	rotations.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_scales(const PackedVector3Array &p_scales) {
	scales = p_scales;
	scales.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_seeds(const PackedInt32Array &p_seeds) {
	seeds = p_seeds;
	seeds.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_variant_indices(const PackedInt32Array &p_indices) {
	variant_indices = p_indices;
	variant_indices.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_instance_ids(const PackedInt32Array &p_ids) {
	instance_ids = p_ids;
	instance_ids.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_enabled(const PackedByteArray &p_enabled) {
	enabled = p_enabled;
	enabled.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_colors(const PackedColorArray &p_colors) {
	colors = p_colors;
	colors.resize(positions.size());
	emit_changed();
}

void OpenWorldTreePlacementData::set_custom_data(const PackedColorArray &p_custom_data) {
	custom_data = p_custom_data;
	custom_data.resize(positions.size());
	emit_changed();
}

Dictionary OpenWorldTreePlacementData::get_instance(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, positions.size(), Dictionary());
	Dictionary instance;
	instance["instance_id"] = instance_ids[p_index];
	instance["position"] = positions[p_index];
	instance["rotation"] = rotations[p_index];
	instance["scale"] = scales[p_index];
	instance["seed"] = seeds[p_index];
	instance["variant_index"] = variant_indices[p_index];
	instance["enabled"] = enabled[p_index] != 0;
	instance["color"] = colors[p_index];
	instance["custom_data"] = custom_data[p_index];
	return instance;
}

Transform3D OpenWorldTreePlacementData::get_instance_transform(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, positions.size(), Transform3D());
	Transform3D transform(Basis::from_euler(rotations[p_index]), positions[p_index]);
	transform.basis.scale(scales[p_index]);
	return transform;
}

int OpenWorldTreePlacementData::find_index_by_id(int p_instance_id) const {
	for (int i = 0; i < instance_ids.size(); i++) {
		if (instance_ids[i] == p_instance_id) {
			return i;
		}
	}
	return -1;
}

int OpenWorldTreePlacementData::add_tree(const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, int p_seed, int p_variant_index, const Color &p_color, const Color &p_custom_data) {
	const int instance_id = _get_next_instance_id();
	positions.push_back(p_position);
	rotations.push_back(p_rotation);
	scales.push_back(p_scale);
	seeds.push_back(p_seed);
	variant_indices.push_back(p_variant_index);
	instance_ids.push_back(instance_id);
	enabled.push_back(1);
	colors.push_back(p_color);
	custom_data.push_back(p_custom_data);
	emit_changed();
	return instance_id;
}

void OpenWorldTreePlacementData::remove_tree_at(int p_index) {
	ERR_FAIL_INDEX(p_index, positions.size());
	positions.remove_at(p_index);
	rotations.remove_at(p_index);
	scales.remove_at(p_index);
	seeds.remove_at(p_index);
	variant_indices.remove_at(p_index);
	instance_ids.remove_at(p_index);
	enabled.remove_at(p_index);
	colors.remove_at(p_index);
	custom_data.remove_at(p_index);
	emit_changed();
}

void OpenWorldTreePlacementData::set_instance_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, positions.size());
	const uint8_t new_enabled = p_enabled ? 1 : 0;
	if (enabled[p_index] == new_enabled) {
		return;
	}
	enabled.set(p_index, new_enabled);
	emit_changed();
}

bool OpenWorldTreePlacementData::is_instance_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, positions.size(), false);
	return enabled[p_index] != 0;
}

void OpenWorldTreePlacementData::clear_trees() {
	if (positions.is_empty()) {
		return;
	}
	positions.clear();
	rotations.clear();
	scales.clear();
	seeds.clear();
	variant_indices.clear();
	instance_ids.clear();
	enabled.clear();
	colors.clear();
	custom_data.clear();
	emit_changed();
}

void OpenWorldTreePlacementData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_positions", "positions"), &OpenWorldTreePlacementData::set_positions);
	ClassDB::bind_method(D_METHOD("get_positions"), &OpenWorldTreePlacementData::get_positions);
	ClassDB::bind_method(D_METHOD("set_rotations", "rotations"), &OpenWorldTreePlacementData::set_rotations);
	ClassDB::bind_method(D_METHOD("get_rotations"), &OpenWorldTreePlacementData::get_rotations);
	ClassDB::bind_method(D_METHOD("set_scales", "scales"), &OpenWorldTreePlacementData::set_scales);
	ClassDB::bind_method(D_METHOD("get_scales"), &OpenWorldTreePlacementData::get_scales);
	ClassDB::bind_method(D_METHOD("set_seeds", "seeds"), &OpenWorldTreePlacementData::set_seeds);
	ClassDB::bind_method(D_METHOD("get_seeds"), &OpenWorldTreePlacementData::get_seeds);
	ClassDB::bind_method(D_METHOD("set_variant_indices", "variant_indices"), &OpenWorldTreePlacementData::set_variant_indices);
	ClassDB::bind_method(D_METHOD("get_variant_indices"), &OpenWorldTreePlacementData::get_variant_indices);
	ClassDB::bind_method(D_METHOD("set_instance_ids", "instance_ids"), &OpenWorldTreePlacementData::set_instance_ids);
	ClassDB::bind_method(D_METHOD("get_instance_ids"), &OpenWorldTreePlacementData::get_instance_ids);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &OpenWorldTreePlacementData::set_enabled);
	ClassDB::bind_method(D_METHOD("get_enabled"), &OpenWorldTreePlacementData::get_enabled);
	ClassDB::bind_method(D_METHOD("set_colors", "colors"), &OpenWorldTreePlacementData::set_colors);
	ClassDB::bind_method(D_METHOD("get_colors"), &OpenWorldTreePlacementData::get_colors);
	ClassDB::bind_method(D_METHOD("set_custom_data", "custom_data"), &OpenWorldTreePlacementData::set_custom_data);
	ClassDB::bind_method(D_METHOD("get_custom_data"), &OpenWorldTreePlacementData::get_custom_data);
	ClassDB::bind_method(D_METHOD("get_instance_count"), &OpenWorldTreePlacementData::get_instance_count);
	ClassDB::bind_method(D_METHOD("get_instance", "index"), &OpenWorldTreePlacementData::get_instance);
	ClassDB::bind_method(D_METHOD("get_instance_transform", "index"), &OpenWorldTreePlacementData::get_instance_transform);
	ClassDB::bind_method(D_METHOD("find_index_by_id", "instance_id"), &OpenWorldTreePlacementData::find_index_by_id);
	ClassDB::bind_method(D_METHOD("add_tree", "position", "rotation", "scale", "seed", "variant_index", "color", "custom_data"), &OpenWorldTreePlacementData::add_tree, DEFVAL(Vector3()), DEFVAL(Vector3(1.0, 1.0, 1.0)), DEFVAL(0), DEFVAL(-1), DEFVAL(Color(1.0, 1.0, 1.0, 1.0)), DEFVAL(Color()));
	ClassDB::bind_method(D_METHOD("remove_tree_at", "index"), &OpenWorldTreePlacementData::remove_tree_at);
	ClassDB::bind_method(D_METHOD("set_instance_enabled", "index", "enabled"), &OpenWorldTreePlacementData::set_instance_enabled);
	ClassDB::bind_method(D_METHOD("is_instance_enabled", "index"), &OpenWorldTreePlacementData::is_instance_enabled);
	ClassDB::bind_method(D_METHOD("clear_trees"), &OpenWorldTreePlacementData::clear_trees);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "positions"), "set_positions", "get_positions");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "rotations"), "set_rotations", "get_rotations");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "scales"), "set_scales", "get_scales");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "seeds"), "set_seeds", "get_seeds");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "variant_indices"), "set_variant_indices", "get_variant_indices");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "instance_ids"), "set_instance_ids", "get_instance_ids");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "enabled"), "set_enabled", "get_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "colors"), "set_colors", "get_colors");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "custom_data"), "set_custom_data", "get_custom_data");
}
