/**************************************************************************/
/*  open_world_tree_generation_profile.h                                  */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class OpenWorldTreeGenerationProfile : public Resource {
	GDCLASS(OpenWorldTreeGenerationProfile, Resource);
	RES_BASE_EXTENSION("owtreeprofile");

public:
	enum TreeArchetype {
		ARCHETYPE_TEMPERATE_BROADLEAF,
		ARCHETYPE_TROPICAL_BROADLEAF,
		ARCHETYPE_UMBRELLA,
		ARCHETYPE_CONIFER,
		ARCHETYPE_PALM,
		ARCHETYPE_MANGROVE,
	};

	enum CrownShape {
		CROWN_AUTO,
		CROWN_ROUND,
		CROWN_UMBRELLA,
		CROWN_CONICAL,
		CROWN_TIERED,
	};

	enum RootStyle {
		ROOT_AUTO,
		ROOT_NONE,
		ROOT_FLARE,
		ROOT_BUTTRESS,
		ROOT_PROP,
	};

private:
	TreeArchetype archetype = ARCHETYPE_TEMPERATE_BROADLEAF;
	CrownShape crown_shape = CROWN_AUTO;
	RootStyle root_style = ROOT_AUTO;

	real_t tree_height = 6.0;
	int trunk_segments = 8;
	real_t trunk_base_radius = 0.35;
	real_t trunk_tip_radius = 0.08;
	int trunk_radial_sides = 7;
	real_t trunk_bend = 0.25;

	real_t branch_start_ratio = 0.3;
	real_t branch_end_ratio = 0.9;
	real_t branch_interval = 0.55;
	real_t phyllotaxy_angle_degrees = 137.5;
	real_t branch_elevation_degrees = 20.0;
	real_t branch_length_min = 1.2;
	real_t branch_length_max = 2.4;
	int branch_segments = 4;
	real_t branch_base_radius_scale = 0.32;
	real_t branch_bend = 0.3;
	real_t branch_droop = 0.0;
	int secondary_branch_count = 0;
	real_t secondary_branch_scale = 0.5;

	int canopy_blob_count = 12;
	real_t canopy_radius_min = 0.7;
	real_t canopy_radius_max = 1.2;
	real_t canopy_vertical_scale = 0.75;
	real_t canopy_position_jitter = 0.25;
	real_t canopy_roughness = 0.12;

	real_t root_flare_scale = 1.6;
	real_t root_height = 0.8;
	int root_count = 5;
	real_t root_length = 1.4;

	int palm_frond_count = 10;
	real_t palm_frond_length = 2.6;
	real_t palm_frond_width = 0.45;
	real_t palm_frond_droop = 0.8;

protected:
	static void _bind_methods();

public:
	void set_archetype(TreeArchetype p_value);
	TreeArchetype get_archetype() const { return archetype; }
	void set_crown_shape(CrownShape p_value);
	CrownShape get_crown_shape() const { return crown_shape; }
	void set_root_style(RootStyle p_value);
	RootStyle get_root_style() const { return root_style; }

	void set_tree_height(real_t p_value);
	real_t get_tree_height() const { return tree_height; }
	void set_trunk_segments(int p_value);
	int get_trunk_segments() const { return trunk_segments; }
	void set_trunk_base_radius(real_t p_value);
	real_t get_trunk_base_radius() const { return trunk_base_radius; }
	void set_trunk_tip_radius(real_t p_value);
	real_t get_trunk_tip_radius() const { return trunk_tip_radius; }
	void set_trunk_radial_sides(int p_value);
	int get_trunk_radial_sides() const { return trunk_radial_sides; }
	void set_trunk_bend(real_t p_value);
	real_t get_trunk_bend() const { return trunk_bend; }

	void set_branch_start_ratio(real_t p_value);
	real_t get_branch_start_ratio() const { return branch_start_ratio; }
	void set_branch_end_ratio(real_t p_value);
	real_t get_branch_end_ratio() const { return branch_end_ratio; }
	void set_branch_interval(real_t p_value);
	real_t get_branch_interval() const { return branch_interval; }
	void set_phyllotaxy_angle_degrees(real_t p_value);
	real_t get_phyllotaxy_angle_degrees() const { return phyllotaxy_angle_degrees; }
	void set_branch_elevation_degrees(real_t p_value);
	real_t get_branch_elevation_degrees() const { return branch_elevation_degrees; }
	void set_branch_length_min(real_t p_value);
	real_t get_branch_length_min() const { return branch_length_min; }
	void set_branch_length_max(real_t p_value);
	real_t get_branch_length_max() const { return branch_length_max; }
	void set_branch_segments(int p_value);
	int get_branch_segments() const { return branch_segments; }
	void set_branch_base_radius_scale(real_t p_value);
	real_t get_branch_base_radius_scale() const { return branch_base_radius_scale; }
	void set_branch_bend(real_t p_value);
	real_t get_branch_bend() const { return branch_bend; }
	void set_branch_droop(real_t p_value);
	real_t get_branch_droop() const { return branch_droop; }
	void set_secondary_branch_count(int p_value);
	int get_secondary_branch_count() const { return secondary_branch_count; }
	void set_secondary_branch_scale(real_t p_value);
	real_t get_secondary_branch_scale() const { return secondary_branch_scale; }

	void set_canopy_blob_count(int p_value);
	int get_canopy_blob_count() const { return canopy_blob_count; }
	void set_canopy_radius_min(real_t p_value);
	real_t get_canopy_radius_min() const { return canopy_radius_min; }
	void set_canopy_radius_max(real_t p_value);
	real_t get_canopy_radius_max() const { return canopy_radius_max; }
	void set_canopy_vertical_scale(real_t p_value);
	real_t get_canopy_vertical_scale() const { return canopy_vertical_scale; }
	void set_canopy_position_jitter(real_t p_value);
	real_t get_canopy_position_jitter() const { return canopy_position_jitter; }
	void set_canopy_roughness(real_t p_value);
	real_t get_canopy_roughness() const { return canopy_roughness; }

	void set_root_flare_scale(real_t p_value);
	real_t get_root_flare_scale() const { return root_flare_scale; }
	void set_root_height(real_t p_value);
	real_t get_root_height() const { return root_height; }
	void set_root_count(int p_value);
	int get_root_count() const { return root_count; }
	void set_root_length(real_t p_value);
	real_t get_root_length() const { return root_length; }

	void set_palm_frond_count(int p_value);
	int get_palm_frond_count() const { return palm_frond_count; }
	void set_palm_frond_length(real_t p_value);
	real_t get_palm_frond_length() const { return palm_frond_length; }
	void set_palm_frond_width(real_t p_value);
	real_t get_palm_frond_width() const { return palm_frond_width; }
	void set_palm_frond_droop(real_t p_value);
	real_t get_palm_frond_droop() const { return palm_frond_droop; }

};

VARIANT_ENUM_CAST(OpenWorldTreeGenerationProfile::TreeArchetype);
VARIANT_ENUM_CAST(OpenWorldTreeGenerationProfile::CrownShape);
VARIANT_ENUM_CAST(OpenWorldTreeGenerationProfile::RootStyle);

