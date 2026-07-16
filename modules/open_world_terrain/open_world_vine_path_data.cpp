/**************************************************************************/
/*  open_world_vine_path_data.cpp                                         */
/**************************************************************************/

#include "open_world_vine_path_data.h"

#include "core/object/class_db.h"

void OpenWorldVinePathData::set_paths(const Array &p_paths) { paths = p_paths; emit_changed(); }
void OpenWorldVinePathData::set_normals(const Array &p_normals) { normals = p_normals; emit_changed(); }
void OpenWorldVinePathData::set_attached_flags(const Array &p_flags) { attached_flags = p_flags; emit_changed(); }
void OpenWorldVinePathData::set_parent_paths(const PackedInt32Array &p_parents) { parent_paths = p_parents; emit_changed(); }
void OpenWorldVinePathData::set_support_segments(const PackedInt32Array &p_segments) { support_segments = p_segments; emit_changed(); }

void OpenWorldVinePathData::clear() {
	paths.clear(); normals.clear(); attached_flags.clear(); parent_paths.clear(); support_segments.clear(); emit_changed();
}

void OpenWorldVinePathData::add_path(const PackedVector3Array &p_points, const PackedVector3Array &p_normals, const PackedByteArray &p_attached, int p_parent_path, int p_support_segment) {
	paths.push_back(p_points);
	normals.push_back(p_normals);
	attached_flags.push_back(p_attached);
	parent_paths.push_back(p_parent_path);
	support_segments.push_back(p_support_segment);
	emit_changed();
}

PackedVector3Array OpenWorldVinePathData::get_path_points(int p_index) const { ERR_FAIL_INDEX_V(p_index, paths.size(), PackedVector3Array()); return paths[p_index]; }
PackedVector3Array OpenWorldVinePathData::get_path_normals(int p_index) const { ERR_FAIL_INDEX_V(p_index, normals.size(), PackedVector3Array()); return normals[p_index]; }
PackedByteArray OpenWorldVinePathData::get_path_attached_flags(int p_index) const { ERR_FAIL_INDEX_V(p_index, attached_flags.size(), PackedByteArray()); return attached_flags[p_index]; }

real_t OpenWorldVinePathData::get_total_length() const {
	real_t total = 0.0;
	for (int path_index = 0; path_index < paths.size(); path_index++) {
		PackedVector3Array points = paths[path_index];
		for (int i = 1; i < points.size(); i++) total += points[i - 1].distance_to(points[i]);
	}
	return total;
}

void OpenWorldVinePathData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_paths", "paths"), &OpenWorldVinePathData::set_paths);
	ClassDB::bind_method(D_METHOD("get_paths"), &OpenWorldVinePathData::get_paths);
	ClassDB::bind_method(D_METHOD("set_normals", "normals"), &OpenWorldVinePathData::set_normals);
	ClassDB::bind_method(D_METHOD("get_normals"), &OpenWorldVinePathData::get_normals);
	ClassDB::bind_method(D_METHOD("set_attached_flags", "flags"), &OpenWorldVinePathData::set_attached_flags);
	ClassDB::bind_method(D_METHOD("get_attached_flags"), &OpenWorldVinePathData::get_attached_flags);
	ClassDB::bind_method(D_METHOD("set_parent_paths", "parents"), &OpenWorldVinePathData::set_parent_paths);
	ClassDB::bind_method(D_METHOD("get_parent_paths"), &OpenWorldVinePathData::get_parent_paths);
	ClassDB::bind_method(D_METHOD("set_support_segments", "segments"), &OpenWorldVinePathData::set_support_segments);
	ClassDB::bind_method(D_METHOD("get_support_segments"), &OpenWorldVinePathData::get_support_segments);
	ClassDB::bind_method(D_METHOD("clear"), &OpenWorldVinePathData::clear);
	ClassDB::bind_method(D_METHOD("add_path", "points", "normals", "attached", "parent_path", "support_segment"), &OpenWorldVinePathData::add_path, DEFVAL(PackedVector3Array()), DEFVAL(PackedByteArray()), DEFVAL(-1), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_path_count"), &OpenWorldVinePathData::get_path_count);
	ClassDB::bind_method(D_METHOD("get_path_points", "index"), &OpenWorldVinePathData::get_path_points);
	ClassDB::bind_method(D_METHOD("get_path_normals", "index"), &OpenWorldVinePathData::get_path_normals);
	ClassDB::bind_method(D_METHOD("get_path_attached_flags", "index"), &OpenWorldVinePathData::get_path_attached_flags);
	ClassDB::bind_method(D_METHOD("get_total_length"), &OpenWorldVinePathData::get_total_length);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "paths", PROPERTY_HINT_ARRAY_TYPE, "PackedVector3Array"), "set_paths", "get_paths");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "normals", PROPERTY_HINT_ARRAY_TYPE, "PackedVector3Array"), "set_normals", "get_normals");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "attached_flags", PROPERTY_HINT_ARRAY_TYPE, "PackedByteArray"), "set_attached_flags", "get_attached_flags");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "parent_paths"), "set_parent_paths", "get_parent_paths");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "support_segments"), "set_support_segments", "get_support_segments");
}
