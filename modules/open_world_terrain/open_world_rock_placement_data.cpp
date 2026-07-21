/**************************************************************************/
/*  open_world_rock_placement_data.cpp                                    */
/**************************************************************************/
#include "open_world_rock_placement_data.h"

#include "core/object/class_db.h"

#define PLACEMENT_SETTER(type, name) void OpenWorldRockPlacementData::set_##name(type p_value) { name = p_value; emit_changed(); }
PLACEMENT_SETTER(const PackedVector3Array &, positions); PLACEMENT_SETTER(const PackedVector3Array &, rotations); PLACEMENT_SETTER(const PackedVector3Array &, scales); PLACEMENT_SETTER(const PackedInt32Array &, variant_indices); PLACEMENT_SETTER(const PackedStringArray &, stable_ids);
#undef PLACEMENT_SETTER
void OpenWorldRockPlacementData::clear() { positions.clear(); rotations.clear(); scales.clear(); variant_indices.clear(); stable_ids.clear(); emit_changed(); }
void OpenWorldRockPlacementData::add_placement(const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, int p_variant_index, const String &p_stable_id) { positions.push_back(p_position); rotations.push_back(p_rotation); scales.push_back(p_scale); variant_indices.push_back(p_variant_index); stable_ids.push_back(p_stable_id); emit_changed(); }
Array OpenWorldRockPlacementData::get_placements_in_cell(const Vector2i &p_cell, real_t p_cell_size) const {
	Array result; if (p_cell_size <= 0.0) return result; for (int i = 0; i < positions.size(); i++) { Vector2i cell(Math::floor(positions[i].x / p_cell_size), Math::floor(positions[i].z / p_cell_size)); if (cell != p_cell) continue; Dictionary entry; entry["position"] = positions[i]; entry["rotation"] = rotations[i]; entry["scale"] = scales[i]; entry["variant_index"] = variant_indices[i]; entry["stable_id"] = stable_ids[i]; result.push_back(entry); } return result;
}
Dictionary OpenWorldRockPlacementData::validate_placements() const {
	Dictionary result; PackedStringArray errors; int count = positions.size(); if (rotations.size() != count || scales.size() != count || variant_indices.size() != count || stable_ids.size() != count) errors.push_back("ARRAY_SIZE_MISMATCH");
	for (int i = 0; i < count; i++) { if (!positions[i].is_finite() || !rotations[i].is_finite() || !scales[i].is_finite()) errors.push_back(vformat("NON_FINITE_TRANSFORM:%d", i)); else if (scales[i].x <= 0.0 || scales[i].y <= 0.0 || scales[i].z <= 0.0) errors.push_back(vformat("INVALID_SCALE:%d", i)); if (i < stable_ids.size() && stable_ids[i].is_empty()) errors.push_back(vformat("STABLE_ID_MISSING:%d", i)); }
	result["success"] = errors.is_empty(); result["error_codes"] = errors; result["placement_count"] = count; return result;
}
void OpenWorldRockPlacementData::_bind_methods() {
#define PLACEMENT_BIND(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldRockPlacementData::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldRockPlacementData::get_##name)
	PLACEMENT_BIND(positions); PLACEMENT_BIND(rotations); PLACEMENT_BIND(scales); PLACEMENT_BIND(variant_indices); PLACEMENT_BIND(stable_ids);
#undef PLACEMENT_BIND
	ClassDB::bind_method(D_METHOD("clear"), &OpenWorldRockPlacementData::clear); ClassDB::bind_method(D_METHOD("add_placement", "position", "rotation", "scale", "variant_index", "stable_id"), &OpenWorldRockPlacementData::add_placement); ClassDB::bind_method(D_METHOD("get_placement_count"), &OpenWorldRockPlacementData::get_placement_count); ClassDB::bind_method(D_METHOD("get_placements_in_cell", "cell", "cell_size"), &OpenWorldRockPlacementData::get_placements_in_cell); ClassDB::bind_method(D_METHOD("validate_placements"), &OpenWorldRockPlacementData::validate_placements);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "positions"), "set_positions", "get_positions"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "rotations"), "set_rotations", "get_rotations"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "scales"), "set_scales", "get_scales"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "variant_indices"), "set_variant_indices", "get_variant_indices"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "stable_ids"), "set_stable_ids", "get_stable_ids");
}
