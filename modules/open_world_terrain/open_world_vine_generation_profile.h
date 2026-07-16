/**************************************************************************/
/*  open_world_vine_generation_profile.h                                  */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class OpenWorldVineGenerationProfile : public Resource {
	GDCLASS(OpenWorldVineGenerationProfile, Resource);
	RES_BASE_EXTENSION("owvineprofile");

public:
	enum SurfaceGapPolicy {
		SURFACE_GAP_STOP,
		SURFACE_GAP_SWITCH_TO_HANGING,
	};

private:
	real_t stem_radius = 0.055;
	real_t tip_radius_scale = 0.2;
	int radial_sides = 5;
	real_t segment_length = 0.35;
	real_t turn_noise = 0.22;
	int side_branch_count = 2;
	real_t side_branch_length = 1.2;
	real_t leaf_spacing = 0.42;
	real_t leaf_size = 0.24;
	real_t leaf_width_scale = 0.62;
	real_t leaf_density = 1.0;
	real_t surface_offset = 0.035;
	real_t support_probe_distance = 0.75;
	real_t max_slope_degrees = 70.0;
	SurfaceGapPolicy surface_gap_policy = SURFACE_GAP_STOP;
	real_t hanging_sag = 0.65;
	real_t climbing_up_bias = 0.72;
	real_t tree_wrap_turns = 3.0;
	real_t tree_wrap_branch_chance = 0.2;
	real_t lod1_quality = 0.55;
	real_t lod2_quality = 0.2;
	real_t wind_strength = 0.18;
	real_t wind_speed = 1.0;

protected:
	static void _bind_methods();

public:
	#define VINE_PROFILE_ACCESSOR(type, name) \
		void set_##name(type p_value); \
		type get_##name() const { return name; }

	VINE_PROFILE_ACCESSOR(real_t, stem_radius);
	VINE_PROFILE_ACCESSOR(real_t, tip_radius_scale);
	VINE_PROFILE_ACCESSOR(int, radial_sides);
	VINE_PROFILE_ACCESSOR(real_t, segment_length);
	VINE_PROFILE_ACCESSOR(real_t, turn_noise);
	VINE_PROFILE_ACCESSOR(int, side_branch_count);
	VINE_PROFILE_ACCESSOR(real_t, side_branch_length);
	VINE_PROFILE_ACCESSOR(real_t, leaf_spacing);
	VINE_PROFILE_ACCESSOR(real_t, leaf_size);
	VINE_PROFILE_ACCESSOR(real_t, leaf_width_scale);
	VINE_PROFILE_ACCESSOR(real_t, leaf_density);
	VINE_PROFILE_ACCESSOR(real_t, surface_offset);
	VINE_PROFILE_ACCESSOR(real_t, support_probe_distance);
	VINE_PROFILE_ACCESSOR(real_t, max_slope_degrees);
	VINE_PROFILE_ACCESSOR(SurfaceGapPolicy, surface_gap_policy);
	VINE_PROFILE_ACCESSOR(real_t, hanging_sag);
	VINE_PROFILE_ACCESSOR(real_t, climbing_up_bias);
	VINE_PROFILE_ACCESSOR(real_t, tree_wrap_turns);
	VINE_PROFILE_ACCESSOR(real_t, tree_wrap_branch_chance);
	VINE_PROFILE_ACCESSOR(real_t, lod1_quality);
	VINE_PROFILE_ACCESSOR(real_t, lod2_quality);
	VINE_PROFILE_ACCESSOR(real_t, wind_strength);
	VINE_PROFILE_ACCESSOR(real_t, wind_speed);

	#undef VINE_PROFILE_ACCESSOR
};

VARIANT_ENUM_CAST(OpenWorldVineGenerationProfile::SurfaceGapPolicy);
