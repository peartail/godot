/**************************************************************************/
/*  open_world_vine_generation_profile.cpp                                */
/**************************************************************************/

#include "open_world_vine_generation_profile.h"

#include "core/object/class_db.h"

#define VINE_SETTER(type, name, expression) \
	void OpenWorldVineGenerationProfile::set_##name(type p_value) { \
		type value = (expression); \
		if (name == value) { return; } \
		name = value; \
		emit_changed(); \
	}

VINE_SETTER(real_t, stem_radius, MAX((real_t)0.001, p_value));
VINE_SETTER(real_t, tip_radius_scale, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(int, radial_sides, CLAMP(p_value, 3, 12));
VINE_SETTER(real_t, segment_length, MAX((real_t)0.02, p_value));
VINE_SETTER(real_t, turn_noise, CLAMP(p_value, (real_t)0.0, (real_t)1.5));
VINE_SETTER(int, side_branch_count, CLAMP(p_value, 0, 64));
VINE_SETTER(real_t, side_branch_length, MAX((real_t)0.05, p_value));
VINE_SETTER(real_t, branch_junction_scale, CLAMP(p_value, (real_t)1.0, (real_t)3.0));
VINE_SETTER(real_t, branch_junction_length, MAX((real_t)0.0, p_value));
VINE_SETTER(real_t, leaf_spacing, MAX((real_t)0.02, p_value));
VINE_SETTER(real_t, leaf_size, MAX((real_t)0.005, p_value));
VINE_SETTER(real_t, leaf_width_scale, CLAMP(p_value, (real_t)0.05, (real_t)2.0));
VINE_SETTER(real_t, leaf_density, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(int, leaf_cluster_card_count, CLAMP(p_value, 1, 8));
VINE_SETTER(real_t, leaf_cluster_spread, CLAMP(p_value, (real_t)0.0, (real_t)1.5));
VINE_SETTER(real_t, leaf_size_variation, CLAMP(p_value, (real_t)0.0, (real_t)0.75));
VINE_SETTER(real_t, thorn_density, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(real_t, thorn_spacing, MAX((real_t)0.05, p_value));
VINE_SETTER(real_t, thorn_size, MAX((real_t)0.005, p_value));
VINE_SETTER(real_t, surface_offset, MAX((real_t)0.0, p_value));
VINE_SETTER(real_t, support_probe_distance, MAX((real_t)0.02, p_value));
VINE_SETTER(real_t, max_slope_degrees, CLAMP(p_value, (real_t)0.0, (real_t)90.0));
VINE_SETTER(SurfaceGapPolicy, surface_gap_policy, (SurfaceGapPolicy)CLAMP((int)p_value, 0, 1));
VINE_SETTER(real_t, hanging_sag, MAX((real_t)0.0, p_value));
VINE_SETTER(real_t, climbing_up_bias, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(real_t, tree_wrap_turns, MAX((real_t)0.0, p_value));
VINE_SETTER(real_t, tree_wrap_branch_chance, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(real_t, bramble_radius, MAX((real_t)0.1, p_value));
VINE_SETTER(real_t, bramble_height, MAX((real_t)0.1, p_value));
VINE_SETTER(int, bramble_stem_count, CLAMP(p_value, 1, 64));
VINE_SETTER(real_t, bramble_tangle_strength, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(real_t, bramble_surface_bias, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(real_t, bramble_ground_anchor_ratio, CLAMP(p_value, (real_t)0.0, (real_t)1.0));
VINE_SETTER(real_t, lod1_quality, CLAMP(p_value, (real_t)0.1, (real_t)1.0));
VINE_SETTER(real_t, lod2_quality, CLAMP(p_value, (real_t)0.05, (real_t)0.75));
VINE_SETTER(real_t, wind_strength, MAX((real_t)0.0, p_value));
VINE_SETTER(real_t, wind_speed, MAX((real_t)0.0, p_value));

#undef VINE_SETTER

void OpenWorldVineGenerationProfile::_bind_methods() {
	BIND_ENUM_CONSTANT(SURFACE_GAP_STOP);
	BIND_ENUM_CONSTANT(SURFACE_GAP_SWITCH_TO_HANGING);

	#define VINE_BIND(type, name, hint, hint_string) \
		ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldVineGenerationProfile::set_##name); \
		ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldVineGenerationProfile::get_##name); \
		ADD_PROPERTY(PropertyInfo(type, #name, hint, hint_string), "set_" #name, "get_" #name)

	ADD_GROUP("Stem", "");
	VINE_BIND(Variant::FLOAT, stem_radius, PROPERTY_HINT_RANGE, "0.001,1,0.001,suffix:m");
	VINE_BIND(Variant::FLOAT, tip_radius_scale, PROPERTY_HINT_RANGE, "0,1,0.01");
	VINE_BIND(Variant::INT, radial_sides, PROPERTY_HINT_RANGE, "3,12,1");
	VINE_BIND(Variant::FLOAT, segment_length, PROPERTY_HINT_RANGE, "0.02,5,0.01,suffix:m");
	VINE_BIND(Variant::FLOAT, turn_noise, PROPERTY_HINT_RANGE, "0,1.5,0.01");
	VINE_BIND(Variant::INT, side_branch_count, PROPERTY_HINT_RANGE, "0,64,1");
	VINE_BIND(Variant::FLOAT, side_branch_length, PROPERTY_HINT_RANGE, "0.05,20,0.05,suffix:m");
	VINE_BIND(Variant::FLOAT, branch_junction_scale, PROPERTY_HINT_RANGE, "1,3,0.01");
	VINE_BIND(Variant::FLOAT, branch_junction_length, PROPERTY_HINT_RANGE, "0,2,0.01,suffix:m");
	ADD_GROUP("Leaves", "");
	VINE_BIND(Variant::FLOAT, leaf_spacing, PROPERTY_HINT_RANGE, "0.02,5,0.01,suffix:m");
	VINE_BIND(Variant::FLOAT, leaf_size, PROPERTY_HINT_RANGE, "0.005,5,0.005,suffix:m");
	VINE_BIND(Variant::FLOAT, leaf_width_scale, PROPERTY_HINT_RANGE, "0.05,2,0.01");
	VINE_BIND(Variant::FLOAT, leaf_density, PROPERTY_HINT_RANGE, "0,1,0.01");
	VINE_BIND(Variant::INT, leaf_cluster_card_count, PROPERTY_HINT_RANGE, "1,8,1");
	VINE_BIND(Variant::FLOAT, leaf_cluster_spread, PROPERTY_HINT_RANGE, "0,1.5,0.01");
	VINE_BIND(Variant::FLOAT, leaf_size_variation, PROPERTY_HINT_RANGE, "0,0.75,0.01");
	ADD_GROUP("Thorns", "");
	VINE_BIND(Variant::FLOAT, thorn_density, PROPERTY_HINT_RANGE, "0,1,0.01");
	VINE_BIND(Variant::FLOAT, thorn_spacing, PROPERTY_HINT_RANGE, "0.05,5,0.01,suffix:m");
	VINE_BIND(Variant::FLOAT, thorn_size, PROPERTY_HINT_RANGE, "0.005,1,0.005,suffix:m");
	ADD_GROUP("Support", "");
	VINE_BIND(Variant::FLOAT, surface_offset, PROPERTY_HINT_RANGE, "0,1,0.001,suffix:m");
	VINE_BIND(Variant::FLOAT, support_probe_distance, PROPERTY_HINT_RANGE, "0.02,20,0.01,suffix:m");
	VINE_BIND(Variant::FLOAT, max_slope_degrees, PROPERTY_HINT_RANGE, "0,90,0.5,suffix:°");
	VINE_BIND(Variant::INT, surface_gap_policy, PROPERTY_HINT_ENUM, "Stop,Switch To Hanging");
	VINE_BIND(Variant::FLOAT, hanging_sag, PROPERTY_HINT_RANGE, "0,10,0.01,suffix:m");
	VINE_BIND(Variant::FLOAT, climbing_up_bias, PROPERTY_HINT_RANGE, "0,1,0.01");
	VINE_BIND(Variant::FLOAT, tree_wrap_turns, PROPERTY_HINT_RANGE, "0,20,0.05");
	VINE_BIND(Variant::FLOAT, tree_wrap_branch_chance, PROPERTY_HINT_RANGE, "0,1,0.01");
	ADD_GROUP("Bramble", "");
	VINE_BIND(Variant::FLOAT, bramble_radius, PROPERTY_HINT_RANGE, "0.1,20,0.05,suffix:m");
	VINE_BIND(Variant::FLOAT, bramble_height, PROPERTY_HINT_RANGE, "0.1,20,0.05,suffix:m");
	VINE_BIND(Variant::INT, bramble_stem_count, PROPERTY_HINT_RANGE, "1,64,1");
	VINE_BIND(Variant::FLOAT, bramble_tangle_strength, PROPERTY_HINT_RANGE, "0,1,0.01");
	VINE_BIND(Variant::FLOAT, bramble_surface_bias, PROPERTY_HINT_RANGE, "0,1,0.01");
	VINE_BIND(Variant::FLOAT, bramble_ground_anchor_ratio, PROPERTY_HINT_RANGE, "0,1,0.01");
	ADD_GROUP("LOD And Wind", "");
	VINE_BIND(Variant::FLOAT, lod1_quality, PROPERTY_HINT_RANGE, "0.1,1,0.01");
	VINE_BIND(Variant::FLOAT, lod2_quality, PROPERTY_HINT_RANGE, "0.05,0.75,0.01");
	VINE_BIND(Variant::FLOAT, wind_strength, PROPERTY_HINT_RANGE, "0,2,0.01");
	VINE_BIND(Variant::FLOAT, wind_speed, PROPERTY_HINT_RANGE, "0,8,0.05");

	#undef VINE_BIND
}
