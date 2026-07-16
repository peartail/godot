/**************************************************************************/
/*  open_world_vine_variant.cpp                                           */
/**************************************************************************/
#include "open_world_vine_variant.h"
#include "core/object/class_db.h"

#define VSET(type, name, expression) void OpenWorldVineVariant::set_##name(type p_value) { auto value = (expression); if (name == value) return; name = value; emit_changed(); }
VSET(const String &, variant_name, p_value);
VSET(int, source_seed, p_value);
VSET(OpenWorldVineGenerationRequest::VineMode, source_mode, (OpenWorldVineGenerationRequest::VineMode)CLAMP((int)p_value, 0, 3));
VSET(real_t, lod1_distance, MAX((real_t)0.0, p_value));
VSET(real_t, lod2_distance, MAX(lod1_distance, p_value));
VSET(real_t, max_distance, MAX(lod2_distance, p_value));
VSET(int, support_stable_id, MAX(0, p_value));
VSET(SupportLostPolicy, support_lost_policy, (SupportLostPolicy)CLAMP((int)p_value, 0, 2));
#undef VSET
void OpenWorldVineVariant::set_lod0_mesh(const Ref<Mesh> &p_value) { lod_meshes[0] = p_value; emit_changed(); }
void OpenWorldVineVariant::set_lod1_mesh(const Ref<Mesh> &p_value) { lod_meshes[1] = p_value; emit_changed(); }
void OpenWorldVineVariant::set_lod2_mesh(const Ref<Mesh> &p_value) { lod_meshes[2] = p_value; emit_changed(); }
Ref<Mesh> OpenWorldVineVariant::get_lod_mesh(int p_lod) const { ERR_FAIL_INDEX_V(p_lod, 3, Ref<Mesh>()); if (lod_meshes[p_lod].is_valid()) return lod_meshes[p_lod]; for (int i = p_lod - 1; i >= 0; i--) if (lod_meshes[i].is_valid()) return lod_meshes[i]; return Ref<Mesh>(); }
int OpenWorldVineVariant::get_lod_index_for_distance(real_t p_distance) const { if (p_distance > max_distance) return -1; int desired = p_distance >= lod2_distance ? 2 : (p_distance >= lod1_distance ? 1 : 0); if (lod_meshes[desired].is_valid()) return desired; for (int i = desired - 1; i >= 0; i--) if (lod_meshes[i].is_valid()) return i; return -1; }

void OpenWorldVineVariant::_bind_methods() {
	BIND_ENUM_CONSTANT(SUPPORT_KEEP); BIND_ENUM_CONSTANT(SUPPORT_HIDE); BIND_ENUM_CONSTANT(SUPPORT_DETACH);
	#define VBIND(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldVineVariant::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldVineVariant::get_##name)
	VBIND(variant_name); VBIND(source_seed); VBIND(source_mode); VBIND(lod0_mesh); VBIND(lod1_mesh); VBIND(lod2_mesh); VBIND(lod1_distance); VBIND(lod2_distance); VBIND(max_distance); VBIND(support_stable_id); VBIND(support_lost_policy);
	#undef VBIND
	ClassDB::bind_method(D_METHOD("get_lod_mesh", "lod"), &OpenWorldVineVariant::get_lod_mesh); ClassDB::bind_method(D_METHOD("get_lod_index_for_distance", "distance"), &OpenWorldVineVariant::get_lod_index_for_distance);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "variant_name"), "set_variant_name", "get_variant_name"); ADD_PROPERTY(PropertyInfo(Variant::INT, "source_seed"), "set_source_seed", "get_source_seed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "source_mode", PROPERTY_HINT_ENUM, "Creeping,Climbing,Hanging,Tree Wrap"), "set_source_mode", "get_source_mode");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod0_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod0_mesh", "get_lod0_mesh"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod1_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod1_mesh", "get_lod1_mesh"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod2_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod2_mesh", "get_lod2_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod1_distance", "get_lod1_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod2_distance", "get_lod2_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_max_distance", "get_max_distance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "support_stable_id", PROPERTY_HINT_RANGE, "0,2147483647,1"), "set_support_stable_id", "get_support_stable_id"); ADD_PROPERTY(PropertyInfo(Variant::INT, "support_lost_policy", PROPERTY_HINT_ENUM, "Keep,Hide,Detach"), "set_support_lost_policy", "get_support_lost_policy");
}
