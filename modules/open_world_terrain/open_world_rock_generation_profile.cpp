/**************************************************************************/
/*  open_world_rock_generation_profile.cpp                                */
/**************************************************************************/
#include "open_world_rock_generation_profile.h"

#include "core/object/class_db.h"

#define ROCK_SETTER(type, name, expression) \
	void OpenWorldRockGenerationProfile::set_##name(type p_value) { \
		auto value = (expression); if (name == value) return; name = value; emit_changed(); \
	}

ROCK_SETTER(int, point_count, CLAMP(p_value, 12, 256));
ROCK_SETTER(real_t, roughness, CLAMP(p_value, (real_t)0.0, (real_t)0.8));
ROCK_SETTER(real_t, asymmetry, CLAMP(p_value, (real_t)0.0, (real_t)0.75));
ROCK_SETTER(const Vector3 &, axis_scale, Vector3(MAX((real_t)0.05, p_value.x), MAX((real_t)0.05, p_value.y), MAX((real_t)0.05, p_value.z)));
ROCK_SETTER(real_t, bottom_flatten, CLAMP(p_value, (real_t)0.0, (real_t)0.8));
ROCK_SETTER(real_t, ground_inset, CLAMP(p_value, (real_t)0.0, (real_t)0.5));
ROCK_SETTER(real_t, strata_strength, CLAMP(p_value, (real_t)0.0, (real_t)0.8));
ROCK_SETTER(const Vector3 &, strata_direction, p_value.is_zero_approx() ? Vector3::UP : p_value.normalized());
ROCK_SETTER(real_t, directional_facet_strength, CLAMP(p_value, (real_t)0.0, (real_t)0.75));
ROCK_SETTER(int, lobe_count, CLAMP(p_value, 1, 4));
ROCK_SETTER(real_t, lobe_overlap, CLAMP(p_value, (real_t)0.25, (real_t)0.95));
ROCK_SETTER(real_t, lod1_quality, CLAMP(p_value, (real_t)0.35, (real_t)0.8));
ROCK_SETTER(int, lod2_point_count, CLAMP(p_value, 8, 20));
ROCK_SETTER(int, collision_point_limit, CLAMP(p_value, 8, 32));
#undef ROCK_SETTER

void OpenWorldRockGenerationProfile::_bind_methods() {
#define ROCK_BIND(type, name, hint, hint_string) \
	ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldRockGenerationProfile::set_##name); \
	ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldRockGenerationProfile::get_##name); \
	ADD_PROPERTY(PropertyInfo(type, #name, hint, hint_string), "set_" #name, "get_" #name)
	ADD_GROUP("Shape", "");
	ROCK_BIND(Variant::INT, point_count, PROPERTY_HINT_RANGE, "8,256,1");
	ROCK_BIND(Variant::FLOAT, roughness, PROPERTY_HINT_RANGE, "0,0.8,0.01");
	ROCK_BIND(Variant::FLOAT, asymmetry, PROPERTY_HINT_RANGE, "0,0.75,0.01");
	ROCK_BIND(Variant::VECTOR3, axis_scale, PROPERTY_HINT_NONE, "");
	ROCK_BIND(Variant::FLOAT, bottom_flatten, PROPERTY_HINT_RANGE, "0,0.8,0.01");
	ROCK_BIND(Variant::FLOAT, ground_inset, PROPERTY_HINT_RANGE, "0,0.5,0.005,suffix:m");
	ADD_GROUP("Facets And Lobes", "");
	ROCK_BIND(Variant::FLOAT, strata_strength, PROPERTY_HINT_RANGE, "0,0.8,0.01");
	ROCK_BIND(Variant::VECTOR3, strata_direction, PROPERTY_HINT_NONE, "");
	ROCK_BIND(Variant::FLOAT, directional_facet_strength, PROPERTY_HINT_RANGE, "0,0.75,0.01");
	ROCK_BIND(Variant::INT, lobe_count, PROPERTY_HINT_RANGE, "1,4,1");
	ROCK_BIND(Variant::FLOAT, lobe_overlap, PROPERTY_HINT_RANGE, "0.25,0.95,0.01");
	ADD_GROUP("LOD And Collision", "");
	ROCK_BIND(Variant::FLOAT, lod1_quality, PROPERTY_HINT_RANGE, "0.35,0.8,0.01");
	ROCK_BIND(Variant::INT, lod2_point_count, PROPERTY_HINT_RANGE, "8,20,1");
	ROCK_BIND(Variant::INT, collision_point_limit, PROPERTY_HINT_RANGE, "8,32,1");
#undef ROCK_BIND
}
