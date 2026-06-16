/**************************************************************************/
/*  simple_world_placement_data.cpp                                       */
/**************************************************************************/

#include "simple_world_placement_data.h"

#include "simple_world_placement_library.h"

#include "core/object/class_db.h"

void SimpleWorldPlacementData::_ensure_record_arrays_size() {
	const int count = profile_ids.size();
	positions.resize(count);
	rotations.resize(count);
	scales.resize(count);
	terrain_normals.resize(count);
	seeds.resize(count);
	chunk_coords.resize(count);

	for (int i = 0; i < count; i++) {
		if (scales[i] == Vector3()) {
			scales.set(i, Vector3(1.0, 1.0, 1.0));
		}
		if (terrain_normals[i] == Vector3()) {
			terrain_normals.set(i, Vector3(0.0, 1.0, 0.0));
		}
	}
}

void SimpleWorldPlacementData::set_terrain_path(const NodePath &p_terrain_path) {
	if (terrain_path == p_terrain_path) {
		return;
	}
	terrain_path = p_terrain_path;
	emit_changed();
}

void SimpleWorldPlacementData::set_library(const Ref<SimpleWorldPlacementLibrary> &p_library) {
	if (library == p_library) {
		return;
	}
	library = p_library;
	emit_changed();
}

void SimpleWorldPlacementData::set_profile_ids(const PackedStringArray &p_profile_ids) {
	profile_ids = p_profile_ids;
	_ensure_record_arrays_size();
	emit_changed();
}

void SimpleWorldPlacementData::set_positions(const PackedVector3Array &p_positions) {
	positions = p_positions;
	positions.resize(profile_ids.size());
	emit_changed();
}

void SimpleWorldPlacementData::set_rotations(const PackedVector3Array &p_rotations) {
	rotations = p_rotations;
	rotations.resize(profile_ids.size());
	emit_changed();
}

void SimpleWorldPlacementData::set_scales(const PackedVector3Array &p_scales) {
	scales = p_scales;
	scales.resize(profile_ids.size());
	_ensure_record_arrays_size();
	emit_changed();
}

void SimpleWorldPlacementData::set_terrain_normals(const PackedVector3Array &p_terrain_normals) {
	terrain_normals = p_terrain_normals;
	terrain_normals.resize(profile_ids.size());
	_ensure_record_arrays_size();
	emit_changed();
}

void SimpleWorldPlacementData::set_seeds(const PackedInt32Array &p_seeds) {
	seeds = p_seeds;
	seeds.resize(profile_ids.size());
	emit_changed();
}

void SimpleWorldPlacementData::set_chunk_coords(const PackedVector2Array &p_chunk_coords) {
	chunk_coords = p_chunk_coords;
	chunk_coords.resize(profile_ids.size());
	emit_changed();
}

Dictionary SimpleWorldPlacementData::get_placement(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, profile_ids.size(), Dictionary());

	Dictionary placement;
	placement["profile_id"] = profile_ids[p_index];
	placement["position"] = p_index < positions.size() ? positions[p_index] : Vector3();
	placement["rotation"] = p_index < rotations.size() ? rotations[p_index] : Vector3();
	placement["scale"] = p_index < scales.size() ? scales[p_index] : Vector3(1.0, 1.0, 1.0);
	placement["terrain_normal"] = p_index < terrain_normals.size() ? terrain_normals[p_index] : Vector3(0.0, 1.0, 0.0);
	placement["seed"] = p_index < seeds.size() ? seeds[p_index] : 0;
	placement["chunk_coord"] = p_index < chunk_coords.size() ? chunk_coords[p_index] : Vector2();
	return placement;
}

void SimpleWorldPlacementData::add_placement(const String &p_profile_id, const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, const Vector3 &p_terrain_normal, int p_seed, const Vector2 &p_chunk_coord) {
	profile_ids.push_back(p_profile_id);
	positions.push_back(p_position);
	rotations.push_back(p_rotation);
	scales.push_back(p_scale);
	terrain_normals.push_back(p_terrain_normal);
	seeds.push_back(p_seed);
	chunk_coords.push_back(p_chunk_coord);
	emit_changed();
}

void SimpleWorldPlacementData::remove_placement_at(int p_index) {
	ERR_FAIL_INDEX(p_index, profile_ids.size());
	profile_ids.remove_at(p_index);
	positions.remove_at(p_index);
	rotations.remove_at(p_index);
	scales.remove_at(p_index);
	terrain_normals.remove_at(p_index);
	seeds.remove_at(p_index);
	chunk_coords.remove_at(p_index);
	emit_changed();
}

void SimpleWorldPlacementData::clear_placements() {
	if (profile_ids.is_empty()) {
		return;
	}
	profile_ids.clear();
	positions.clear();
	rotations.clear();
	scales.clear();
	terrain_normals.clear();
	seeds.clear();
	chunk_coords.clear();
	emit_changed();
}

void SimpleWorldPlacementData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_terrain_path", "terrain_path"), &SimpleWorldPlacementData::set_terrain_path);
	ClassDB::bind_method(D_METHOD("get_terrain_path"), &SimpleWorldPlacementData::get_terrain_path);
	ClassDB::bind_method(D_METHOD("set_library", "library"), &SimpleWorldPlacementData::set_library);
	ClassDB::bind_method(D_METHOD("get_library"), &SimpleWorldPlacementData::get_library);
	ClassDB::bind_method(D_METHOD("set_profile_ids", "profile_ids"), &SimpleWorldPlacementData::set_profile_ids);
	ClassDB::bind_method(D_METHOD("get_profile_ids"), &SimpleWorldPlacementData::get_profile_ids);
	ClassDB::bind_method(D_METHOD("set_positions", "positions"), &SimpleWorldPlacementData::set_positions);
	ClassDB::bind_method(D_METHOD("get_positions"), &SimpleWorldPlacementData::get_positions);
	ClassDB::bind_method(D_METHOD("set_rotations", "rotations"), &SimpleWorldPlacementData::set_rotations);
	ClassDB::bind_method(D_METHOD("get_rotations"), &SimpleWorldPlacementData::get_rotations);
	ClassDB::bind_method(D_METHOD("set_scales", "scales"), &SimpleWorldPlacementData::set_scales);
	ClassDB::bind_method(D_METHOD("get_scales"), &SimpleWorldPlacementData::get_scales);
	ClassDB::bind_method(D_METHOD("set_terrain_normals", "terrain_normals"), &SimpleWorldPlacementData::set_terrain_normals);
	ClassDB::bind_method(D_METHOD("get_terrain_normals"), &SimpleWorldPlacementData::get_terrain_normals);
	ClassDB::bind_method(D_METHOD("set_seeds", "seeds"), &SimpleWorldPlacementData::set_seeds);
	ClassDB::bind_method(D_METHOD("get_seeds"), &SimpleWorldPlacementData::get_seeds);
	ClassDB::bind_method(D_METHOD("set_chunk_coords", "chunk_coords"), &SimpleWorldPlacementData::set_chunk_coords);
	ClassDB::bind_method(D_METHOD("get_chunk_coords"), &SimpleWorldPlacementData::get_chunk_coords);
	ClassDB::bind_method(D_METHOD("get_placement_count"), &SimpleWorldPlacementData::get_placement_count);
	ClassDB::bind_method(D_METHOD("get_placement", "index"), &SimpleWorldPlacementData::get_placement);
	ClassDB::bind_method(D_METHOD("add_placement", "profile_id", "position", "rotation", "scale", "terrain_normal", "seed", "chunk_coord"), &SimpleWorldPlacementData::add_placement, DEFVAL(0), DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("remove_placement_at", "index"), &SimpleWorldPlacementData::remove_placement_at);
	ClassDB::bind_method(D_METHOD("clear_placements"), &SimpleWorldPlacementData::clear_placements);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "terrain_path"), "set_terrain_path", "get_terrain_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "library", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementLibrary"), "set_library", "get_library");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "profile_ids"), "set_profile_ids", "get_profile_ids");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "positions"), "set_positions", "get_positions");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "rotations"), "set_rotations", "get_rotations");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "scales"), "set_scales", "get_scales");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "terrain_normals"), "set_terrain_normals", "get_terrain_normals");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "seeds"), "set_seeds", "get_seeds");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "chunk_coords"), "set_chunk_coords", "get_chunk_coords");
}
