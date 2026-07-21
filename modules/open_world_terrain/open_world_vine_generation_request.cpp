/**************************************************************************/
/*  open_world_vine_generation_request.cpp                                */
/**************************************************************************/

#include "open_world_vine_generation_request.h"

#include "core/object/class_db.h"
#include "core/object/callable_mp.h"

#define REQUEST_SETTER(type, name, expression) \
	void OpenWorldVineGenerationRequest::set_##name(type p_value) { \
		auto value = (expression); \
		if (name == value) { return; } \
		name = value; emit_changed(); \
	}

REQUEST_SETTER(VineMode, mode, (VineMode)CLAMP((int)p_value, 0, 4));
REQUEST_SETTER(int, seed, p_value);
REQUEST_SETTER(const Vector3 &, start_position, p_value);
REQUEST_SETTER(const Vector3 &, start_direction, p_value.is_zero_approx() ? Vector3::FORWARD : p_value.normalized());
REQUEST_SETTER(const Vector3 &, target_position, p_value);
REQUEST_SETTER(bool, target_enabled, p_value);
REQUEST_SETTER(real_t, desired_length, MAX((real_t)0.0, p_value));
REQUEST_SETTER(const NodePath &, support_path, p_value);
REQUEST_SETTER(int, support_stable_id, MAX(0, p_value));
REQUEST_SETTER(int, branch_budget, CLAMP(p_value, 0, 64));
REQUEST_SETTER(const PackedVector3Array &, explicit_anchors, p_value);
REQUEST_SETTER(const PackedVector3Array &, explicit_normals, p_value);

#undef REQUEST_SETTER

void OpenWorldVineGenerationRequest::_profile_changed() { emit_changed(); }
void OpenWorldVineGenerationRequest::set_profile(const Ref<OpenWorldVineGenerationProfile> &p_profile) {
	if (profile == p_profile) return;
	if (profile.is_valid()) profile->disconnect_changed(callable_mp(this, &OpenWorldVineGenerationRequest::_profile_changed));
	profile = p_profile;
	if (profile.is_valid()) profile->connect_changed(callable_mp(this, &OpenWorldVineGenerationRequest::_profile_changed));
	emit_changed();
}
OpenWorldVineGenerationRequest::~OpenWorldVineGenerationRequest() { if (profile.is_valid()) profile->disconnect_changed(callable_mp(this, &OpenWorldVineGenerationRequest::_profile_changed)); }

void OpenWorldVineGenerationRequest::_bind_methods() {
	BIND_ENUM_CONSTANT(MODE_CREEPING);
	BIND_ENUM_CONSTANT(MODE_CLIMBING);
	BIND_ENUM_CONSTANT(MODE_HANGING);
	BIND_ENUM_CONSTANT(MODE_TREE_WRAP);
	BIND_ENUM_CONSTANT(MODE_BRAMBLE);

	#define REQUEST_BIND(method, type, hint, hint_string) \
		ClassDB::bind_method(D_METHOD("set_" #method, "value"), &OpenWorldVineGenerationRequest::set_##method); \
		ClassDB::bind_method(D_METHOD("get_" #method), &OpenWorldVineGenerationRequest::get_##method); \
		ADD_PROPERTY(PropertyInfo(type, #method, hint, hint_string), "set_" #method, "get_" #method)

	REQUEST_BIND(mode, Variant::INT, PROPERTY_HINT_ENUM, "Creeping,Climbing,Hanging,Tree Wrap,Bramble");
	REQUEST_BIND(seed, Variant::INT, PROPERTY_HINT_NONE, "");
	ClassDB::bind_method(D_METHOD("set_profile", "profile"), &OpenWorldVineGenerationRequest::set_profile);
	ClassDB::bind_method(D_METHOD("get_profile"), &OpenWorldVineGenerationRequest::get_profile);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, OpenWorldVineGenerationProfile::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_profile", "get_profile");
	REQUEST_BIND(start_position, Variant::VECTOR3, PROPERTY_HINT_NONE, "");
	REQUEST_BIND(start_direction, Variant::VECTOR3, PROPERTY_HINT_NONE, "");
	REQUEST_BIND(target_position, Variant::VECTOR3, PROPERTY_HINT_NONE, "");
	ClassDB::bind_method(D_METHOD("set_target_enabled", "enabled"), &OpenWorldVineGenerationRequest::set_target_enabled);
	ClassDB::bind_method(D_METHOD("is_target_enabled"), &OpenWorldVineGenerationRequest::is_target_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "target_enabled"), "set_target_enabled", "is_target_enabled");
	REQUEST_BIND(desired_length, Variant::FLOAT, PROPERTY_HINT_RANGE, "0,10000,0.01,suffix:m");
	REQUEST_BIND(support_path, Variant::NODE_PATH, PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D");
	REQUEST_BIND(support_stable_id, Variant::INT, PROPERTY_HINT_RANGE, "0,2147483647,1");
	REQUEST_BIND(branch_budget, Variant::INT, PROPERTY_HINT_RANGE, "0,64,1");
	REQUEST_BIND(explicit_anchors, Variant::PACKED_VECTOR3_ARRAY, PROPERTY_HINT_NONE, "");
	REQUEST_BIND(explicit_normals, Variant::PACKED_VECTOR3_ARRAY, PROPERTY_HINT_NONE, "");

	#undef REQUEST_BIND
}
