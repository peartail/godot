/**************************************************************************/
/*  open_world_placement_data.cpp                                         */
/**************************************************************************/

#include "open_world_placement_data.h"

#include "core/object/class_db.h"
#include "core/templates/hash_set.h"

#define PLACEMENT_DATA_SETTER(type, name) \
	void OpenWorldPlacementData::set_##name(const type &p_value) { \
		name = p_value; \
		emit_changed(); \
	}

PLACEMENT_DATA_SETTER(PackedStringArray, stable_ids);
PLACEMENT_DATA_SETTER(Array, source_entries);
PLACEMENT_DATA_SETTER(PackedVector3Array, positions);
PLACEMENT_DATA_SETTER(PackedVector3Array, rotations);
PLACEMENT_DATA_SETTER(PackedVector3Array, scales);
PLACEMENT_DATA_SETTER(PackedVector3Array, terrain_normals);
PLACEMENT_DATA_SETTER(PackedInt32Array, seeds);
PLACEMENT_DATA_SETTER(PackedFloat32Array, spacing_radii);

#undef PLACEMENT_DATA_SETTER

Dictionary OpenWorldPlacementData::get_placement(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, get_placement_count(), Dictionary());
	Dictionary result;
	result["stable_id"] = stable_ids[p_index];
	result["source_entry"] = source_entries[p_index];
	result["position"] = positions[p_index];
	result["rotation"] = rotations[p_index];
	result["scale"] = scales[p_index];
	result["terrain_normal"] = terrain_normals[p_index];
	result["seed"] = seeds[p_index];
	result["spacing_radius"] = spacing_radii[p_index];
	return result;
}

void OpenWorldPlacementData::add_placement(const String &p_stable_id, const Ref<OpenWorldPlacementBrushEntry> &p_entry, const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, const Vector3 &p_terrain_normal, int p_seed, real_t p_spacing_radius) {
	ERR_FAIL_COND_MSG(p_stable_id.is_empty(), "Placement stable_id must not be empty.");
	ERR_FAIL_COND_MSG(p_entry.is_null(), "Placement source entry must not be null.");
	stable_ids.push_back(p_stable_id);
	source_entries.push_back(p_entry);
	positions.push_back(p_position);
	rotations.push_back(p_rotation);
	scales.push_back(p_scale);
	terrain_normals.push_back(p_terrain_normal.is_zero_approx() ? Vector3::UP : p_terrain_normal.normalized());
	seeds.push_back(p_seed);
	spacing_radii.push_back(MAX((real_t)0.0, p_spacing_radius));
	emit_changed();
}

void OpenWorldPlacementData::remove_placement_at(int p_index) {
	ERR_FAIL_INDEX(p_index, get_placement_count());
	stable_ids.remove_at(p_index);
	source_entries.remove_at(p_index);
	positions.remove_at(p_index);
	rotations.remove_at(p_index);
	scales.remove_at(p_index);
	terrain_normals.remove_at(p_index);
	seeds.remove_at(p_index);
	spacing_radii.remove_at(p_index);
	emit_changed();
}

int OpenWorldPlacementData::find_placement(const String &p_stable_id) const {
	return stable_ids.find(p_stable_id);
}

void OpenWorldPlacementData::clear_placements() {
	stable_ids.clear();
	source_entries.clear();
	positions.clear();
	rotations.clear();
	scales.clear();
	terrain_normals.clear();
	seeds.clear();
	spacing_radii.clear();
	emit_changed();
}

Dictionary OpenWorldPlacementData::validate_data() const {
	Dictionary report;
	PackedStringArray errors;
	PackedStringArray error_codes;
	HashSet<String> ids;
	const int expected_count = stable_ids.size();
	if (source_entries.size() != expected_count || positions.size() != expected_count || rotations.size() != expected_count || scales.size() != expected_count || terrain_normals.size() != expected_count || seeds.size() != expected_count || spacing_radii.size() != expected_count) {
		error_codes.push_back("ARRAY_LENGTH_MISMATCH");
		errors.push_back("ARRAY_LENGTH_MISMATCH: all placement arrays must have the same length.");
		report["success"] = false;
		report["errors"] = errors;
		report["error_codes"] = error_codes;
		report["error_count"] = errors.size();
		report["placement_count"] = expected_count;
		return report;
	}
	for (int i = 0; i < get_placement_count(); i++) {
		if (stable_ids[i].is_empty()) {
			error_codes.push_back("STABLE_ID_MISSING");
			errors.push_back(vformat("STABLE_ID_MISSING: placement %d has no stable ID.", i));
		} else if (ids.has(stable_ids[i])) {
			error_codes.push_back("STABLE_ID_DUPLICATE");
			errors.push_back(vformat("STABLE_ID_DUPLICATE: '%s'.", stable_ids[i]));
		}
		ids.insert(stable_ids[i]);
		Ref<OpenWorldPlacementBrushEntry> entry = source_entries[i];
		if (entry.is_null()) {
			error_codes.push_back("SOURCE_ENTRY_MISSING");
			errors.push_back(vformat("SOURCE_ENTRY_MISSING: placement %d.", i));
		}
		if (!positions[i].is_finite() || !rotations[i].is_finite() || !scales[i].is_finite()) {
			error_codes.push_back("TRANSFORM_INVALID");
			errors.push_back(vformat("TRANSFORM_INVALID: placement %d.", i));
		}
	}
	report["success"] = errors.is_empty();
	report["errors"] = errors;
	report["error_codes"] = error_codes;
	report["error_count"] = errors.size();
	report["placement_count"] = get_placement_count();
	return report;
}

void OpenWorldPlacementData::_bind_methods() {
#define BIND_ACCESSOR(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldPlacementData::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldPlacementData::get_##name)
	BIND_ACCESSOR(stable_ids);
	BIND_ACCESSOR(source_entries);
	BIND_ACCESSOR(positions);
	BIND_ACCESSOR(rotations);
	BIND_ACCESSOR(scales);
	BIND_ACCESSOR(terrain_normals);
	BIND_ACCESSOR(seeds);
	BIND_ACCESSOR(spacing_radii);
#undef BIND_ACCESSOR
	ClassDB::bind_method(D_METHOD("get_placement_count"), &OpenWorldPlacementData::get_placement_count);
	ClassDB::bind_method(D_METHOD("get_placement", "index"), &OpenWorldPlacementData::get_placement);
	ClassDB::bind_method(D_METHOD("add_placement", "stable_id", "entry", "position", "rotation", "scale", "terrain_normal", "seed", "spacing_radius"), &OpenWorldPlacementData::add_placement);
	ClassDB::bind_method(D_METHOD("remove_placement_at", "index"), &OpenWorldPlacementData::remove_placement_at);
	ClassDB::bind_method(D_METHOD("find_placement", "stable_id"), &OpenWorldPlacementData::find_placement);
	ClassDB::bind_method(D_METHOD("clear_placements"), &OpenWorldPlacementData::clear_placements);
	ClassDB::bind_method(D_METHOD("validate_data"), &OpenWorldPlacementData::validate_data);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "stable_ids", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_stable_ids", "get_stable_ids");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "source_entries", PROPERTY_HINT_ARRAY_TYPE, "OpenWorldPlacementBrushEntry", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_source_entries", "get_source_entries");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "positions", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_positions", "get_positions");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "rotations", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_rotations", "get_rotations");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "scales", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_scales", "get_scales");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "terrain_normals", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_terrain_normals", "get_terrain_normals");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "seeds", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_seeds", "get_seeds");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "spacing_radii", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_spacing_radii", "get_spacing_radii");
}
