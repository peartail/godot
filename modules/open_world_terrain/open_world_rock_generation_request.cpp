/**************************************************************************/
/*  open_world_rock_generation_request.cpp                                */
/**************************************************************************/
#include "open_world_rock_generation_request.h"

#include "core/object/class_db.h"
#include "core/object/callable_mp.h"

void OpenWorldRockGenerationRequest::_profile_changed() { emit_changed(); }
void OpenWorldRockGenerationRequest::set_mode(RockMode p_value) { mode = (RockMode)CLAMP((int)p_value, 0, 2); emit_changed(); }
void OpenWorldRockGenerationRequest::set_seed(int p_value) { seed = p_value; emit_changed(); }
void OpenWorldRockGenerationRequest::set_profile(const Ref<OpenWorldRockGenerationProfile> &p_value) {
	if (profile == p_value) return;
	if (profile.is_valid()) profile->disconnect_changed(callable_mp(this, &OpenWorldRockGenerationRequest::_profile_changed));
	profile = p_value;
	if (profile.is_valid()) profile->connect_changed(callable_mp(this, &OpenWorldRockGenerationRequest::_profile_changed));
	emit_changed();
}
void OpenWorldRockGenerationRequest::set_size(const Vector3 &p_value) { size = p_value; emit_changed(); }
void OpenWorldRockGenerationRequest::set_primary_axis(const Vector3 &p_value) { primary_axis = p_value; emit_changed(); }
void OpenWorldRockGenerationRequest::set_explicit_points(const PackedVector3Array &p_value) { explicit_points = p_value; emit_changed(); }
void OpenWorldRockGenerationRequest::set_stable_id(const String &p_value) { stable_id = p_value; emit_changed(); }
void OpenWorldRockGenerationRequest::set_tags(const PackedStringArray &p_value) { tags = p_value; emit_changed(); }
OpenWorldRockGenerationRequest::~OpenWorldRockGenerationRequest() { if (profile.is_valid() && profile->is_connected("changed", callable_mp(this, &OpenWorldRockGenerationRequest::_profile_changed))) profile->disconnect_changed(callable_mp(this, &OpenWorldRockGenerationRequest::_profile_changed)); }

void OpenWorldRockGenerationRequest::_bind_methods() {
	BIND_ENUM_CONSTANT(MODE_BOULDER); BIND_ENUM_CONSTANT(MODE_SLAB); BIND_ENUM_CONSTANT(MODE_SHARD);
#define REQUEST_BIND(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldRockGenerationRequest::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldRockGenerationRequest::get_##name)
	REQUEST_BIND(mode); REQUEST_BIND(seed); REQUEST_BIND(profile); REQUEST_BIND(size); REQUEST_BIND(primary_axis); REQUEST_BIND(explicit_points); REQUEST_BIND(stable_id); REQUEST_BIND(tags);
#undef REQUEST_BIND
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Boulder,Slab,Shard"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldRockGenerationProfile", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_profile", "get_profile");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "primary_axis"), "set_primary_axis", "get_primary_axis");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "explicit_points"), "set_explicit_points", "get_explicit_points");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "stable_id"), "set_stable_id", "get_stable_id");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tags"), "set_tags", "get_tags");
}
