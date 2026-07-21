/**************************************************************************/
/*  open_world_rock_generation_profile.h                                  */
/**************************************************************************/
#pragma once

#include "core/io/resource.h"

class OpenWorldRockGenerationProfile : public Resource {
	GDCLASS(OpenWorldRockGenerationProfile, Resource);
	RES_BASE_EXTENSION("owrockprofile");

	int point_count = 32;
	real_t roughness = 0.22;
	real_t asymmetry = 0.15;
	Vector3 axis_scale = Vector3(1.0, 0.8, 1.0);
	real_t bottom_flatten = 0.18;
	real_t ground_inset = 0.03;
	real_t strata_strength = 0.12;
	Vector3 strata_direction = Vector3::UP;
	real_t directional_facet_strength = 0.1;
	int lobe_count = 1;
	real_t lobe_overlap = 0.65;
	real_t lod1_quality = 0.6;
	int lod2_point_count = 10;
	int collision_point_limit = 12;

protected:
	static void _bind_methods();

public:
	void set_point_count(int p_value); int get_point_count() const { return point_count; }
	void set_roughness(real_t p_value); real_t get_roughness() const { return roughness; }
	void set_asymmetry(real_t p_value); real_t get_asymmetry() const { return asymmetry; }
	void set_axis_scale(const Vector3 &p_value); Vector3 get_axis_scale() const { return axis_scale; }
	void set_bottom_flatten(real_t p_value); real_t get_bottom_flatten() const { return bottom_flatten; }
	void set_ground_inset(real_t p_value); real_t get_ground_inset() const { return ground_inset; }
	void set_strata_strength(real_t p_value); real_t get_strata_strength() const { return strata_strength; }
	void set_strata_direction(const Vector3 &p_value); Vector3 get_strata_direction() const { return strata_direction; }
	void set_directional_facet_strength(real_t p_value); real_t get_directional_facet_strength() const { return directional_facet_strength; }
	void set_lobe_count(int p_value); int get_lobe_count() const { return lobe_count; }
	void set_lobe_overlap(real_t p_value); real_t get_lobe_overlap() const { return lobe_overlap; }
	void set_lod1_quality(real_t p_value); real_t get_lod1_quality() const { return lod1_quality; }
	void set_lod2_point_count(int p_value); int get_lod2_point_count() const { return lod2_point_count; }
	void set_collision_point_limit(int p_value); int get_collision_point_limit() const { return collision_point_limit; }
};
