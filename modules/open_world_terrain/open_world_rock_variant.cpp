/**************************************************************************/
/*  open_world_rock_variant.cpp                                           */
/**************************************************************************/
#include "open_world_rock_variant.h"

#include "core/object/class_db.h"

#define VARIANT_SETTER(type, name, expression) void OpenWorldRockVariant::set_##name(type p_value) { auto value = (expression); if (name == value) return; name = value; emit_changed(); }
VARIANT_SETTER(const String &, variant_name, p_value);
VARIANT_SETTER(OpenWorldRockGenerationRequest::RockMode, source_mode, (OpenWorldRockGenerationRequest::RockMode)CLAMP((int)p_value, 0, 2));
VARIANT_SETTER(int, source_seed, p_value);
VARIANT_SETTER(const Ref<Shape3D> &, collision_shape, p_value);
VARIANT_SETTER(const PackedVector3Array &, collision_points, p_value);
VARIANT_SETTER(real_t, lod1_distance, MAX((real_t)0.0, p_value));
VARIANT_SETTER(real_t, lod2_distance, MAX(lod1_distance, p_value));
VARIANT_SETTER(real_t, max_distance, MAX(lod2_distance, p_value));
VARIANT_SETTER(const AABB &, local_bounds, p_value);
VARIANT_SETTER(const String &, stable_id, p_value);
VARIANT_SETTER(const PackedStringArray &, tags, p_value);
VARIANT_SETTER(GameplayState, gameplay_state, (GameplayState)CLAMP((int)p_value, 0, 2));
VARIANT_SETTER(const Ref<OpenWorldRockVariant> &, damaged_variant, p_value);
VARIANT_SETTER(const Ref<OpenWorldRockVariant> &, depleted_variant, p_value);
#undef VARIANT_SETTER
void OpenWorldRockVariant::set_lod0_mesh(const Ref<Mesh> &p_value) { lod_meshes[0] = p_value; emit_changed(); }
void OpenWorldRockVariant::set_lod1_mesh(const Ref<Mesh> &p_value) { lod_meshes[1] = p_value; emit_changed(); }
void OpenWorldRockVariant::set_lod2_mesh(const Ref<Mesh> &p_value) { lod_meshes[2] = p_value; emit_changed(); }
Ref<Mesh> OpenWorldRockVariant::get_lod_mesh(int p_lod) const { ERR_FAIL_INDEX_V(p_lod, 3, Ref<Mesh>()); if (lod_meshes[p_lod].is_valid()) return lod_meshes[p_lod]; for (int i = p_lod - 1; i >= 0; i--) if (lod_meshes[i].is_valid()) return lod_meshes[i]; return Ref<Mesh>(); }
int OpenWorldRockVariant::get_lod_index_for_distance(real_t p_distance) const { if (p_distance > max_distance) return -1; int lod = p_distance >= lod2_distance ? 2 : (p_distance >= lod1_distance ? 1 : 0); return get_lod_mesh(lod).is_valid() ? lod : -1; }
Ref<OpenWorldRockVariant> OpenWorldRockVariant::get_variant_for_state(GameplayState p_state) const { if (p_state == STATE_DAMAGED && damaged_variant.is_valid()) return damaged_variant; if (p_state == STATE_DEPLETED && depleted_variant.is_valid()) return depleted_variant; return Ref<OpenWorldRockVariant>(const_cast<OpenWorldRockVariant *>(this)); }

void OpenWorldRockVariant::_bind_methods() {
	BIND_ENUM_CONSTANT(STATE_INTACT); BIND_ENUM_CONSTANT(STATE_DAMAGED); BIND_ENUM_CONSTANT(STATE_DEPLETED);
#define VARIANT_BIND(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldRockVariant::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldRockVariant::get_##name)
	VARIANT_BIND(variant_name); VARIANT_BIND(source_mode); VARIANT_BIND(source_seed); VARIANT_BIND(lod0_mesh); VARIANT_BIND(lod1_mesh); VARIANT_BIND(lod2_mesh); VARIANT_BIND(collision_shape); VARIANT_BIND(collision_points); VARIANT_BIND(lod1_distance); VARIANT_BIND(lod2_distance); VARIANT_BIND(max_distance); VARIANT_BIND(local_bounds); VARIANT_BIND(stable_id); VARIANT_BIND(tags); VARIANT_BIND(gameplay_state); VARIANT_BIND(damaged_variant); VARIANT_BIND(depleted_variant);
#undef VARIANT_BIND
	ClassDB::bind_method(D_METHOD("get_lod_mesh", "lod"), &OpenWorldRockVariant::get_lod_mesh); ClassDB::bind_method(D_METHOD("get_lod_index_for_distance", "distance"), &OpenWorldRockVariant::get_lod_index_for_distance); ClassDB::bind_method(D_METHOD("get_variant_for_state", "state"), &OpenWorldRockVariant::get_variant_for_state);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "variant_name"), "set_variant_name", "get_variant_name"); ADD_PROPERTY(PropertyInfo(Variant::INT, "source_mode", PROPERTY_HINT_ENUM, "Boulder,Slab,Shard"), "set_source_mode", "get_source_mode"); ADD_PROPERTY(PropertyInfo(Variant::INT, "source_seed"), "set_source_seed", "get_source_seed");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod0_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod0_mesh", "get_lod0_mesh"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod1_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod1_mesh", "get_lod1_mesh"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod2_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod2_mesh", "get_lod2_mesh"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "collision_shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape3D"), "set_collision_shape", "get_collision_shape"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "collision_points"), "set_collision_points", "get_collision_points");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod1_distance", "get_lod1_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod2_distance", "get_lod2_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_max_distance", "get_max_distance");
	ADD_PROPERTY(PropertyInfo(Variant::AABB, "local_bounds"), "set_local_bounds", "get_local_bounds"); ADD_PROPERTY(PropertyInfo(Variant::STRING, "stable_id"), "set_stable_id", "get_stable_id"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tags"), "set_tags", "get_tags"); ADD_PROPERTY(PropertyInfo(Variant::INT, "gameplay_state", PROPERTY_HINT_ENUM, "Intact,Damaged,Depleted"), "set_gameplay_state", "get_gameplay_state"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "damaged_variant", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldRockVariant"), "set_damaged_variant", "get_damaged_variant"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "depleted_variant", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldRockVariant"), "set_depleted_variant", "get_depleted_variant");
}
