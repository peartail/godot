/**************************************************************************/
/*  open_world_tree_generation_profile.cpp                                */
/**************************************************************************/

#include "open_world_tree_generation_profile.h"

#include "core/object/class_db.h"

void OpenWorldTreeGenerationProfile::set_archetype(TreeArchetype p_value) {
	p_value = (TreeArchetype)CLAMP((int)p_value, (int)ARCHETYPE_TEMPERATE_BROADLEAF, (int)ARCHETYPE_MANGROVE);
	if (archetype != p_value) {
		archetype = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_crown_shape(CrownShape p_value) {
	p_value = (CrownShape)CLAMP((int)p_value, (int)CROWN_AUTO, (int)CROWN_TIERED);
	if (crown_shape != p_value) {
		crown_shape = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_root_style(RootStyle p_value) {
	p_value = (RootStyle)CLAMP((int)p_value, (int)ROOT_AUTO, (int)ROOT_PROP);
	if (root_style != p_value) {
		root_style = p_value;
		emit_changed();
	}
}
void OpenWorldTreeGenerationProfile::set_tree_height(real_t p_value) {
	p_value = MAX((real_t)0.1, p_value);
	if (!Math::is_equal_approx(tree_height, p_value)) {
		tree_height = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_trunk_segments(int p_value) {
	p_value = CLAMP(p_value, 2, 64);
	if (trunk_segments != p_value) {
		trunk_segments = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_trunk_base_radius(real_t p_value) {
	p_value = MAX((real_t)0.01, p_value);
	if (!Math::is_equal_approx(trunk_base_radius, p_value)) {
		trunk_base_radius = p_value;
		trunk_tip_radius = MIN(trunk_tip_radius, trunk_base_radius);
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_trunk_tip_radius(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.001, trunk_base_radius);
	if (!Math::is_equal_approx(trunk_tip_radius, p_value)) {
		trunk_tip_radius = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_trunk_radial_sides(int p_value) {
	p_value = CLAMP(p_value, 3, 32);
	if (trunk_radial_sides != p_value) {
		trunk_radial_sides = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_trunk_bend(real_t p_value) {
	p_value = MAX((real_t)0.0, p_value);
	if (!Math::is_equal_approx(trunk_bend, p_value)) {
		trunk_bend = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_start_ratio(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.0, (real_t)1.0);
	if (!Math::is_equal_approx(branch_start_ratio, p_value)) {
		branch_start_ratio = p_value;
		branch_end_ratio = MAX(branch_end_ratio, branch_start_ratio);
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_end_ratio(real_t p_value) {
	p_value = CLAMP(p_value, branch_start_ratio, (real_t)1.0);
	if (!Math::is_equal_approx(branch_end_ratio, p_value)) {
		branch_end_ratio = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_interval(real_t p_value) {
	p_value = MAX((real_t)0.05, p_value);
	if (!Math::is_equal_approx(branch_interval, p_value)) {
		branch_interval = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_phyllotaxy_angle_degrees(real_t p_value) {
	p_value = Math::fposmod(p_value, (real_t)360.0);
	if (!Math::is_equal_approx(phyllotaxy_angle_degrees, p_value)) {
		phyllotaxy_angle_degrees = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_elevation_degrees(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)-89.0, (real_t)89.0);
	if (!Math::is_equal_approx(branch_elevation_degrees, p_value)) {
		branch_elevation_degrees = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_length_min(real_t p_value) {
	p_value = MAX((real_t)0.05, p_value);
	if (!Math::is_equal_approx(branch_length_min, p_value)) {
		branch_length_min = p_value;
		branch_length_max = MAX(branch_length_max, branch_length_min);
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_length_max(real_t p_value) {
	p_value = MAX(branch_length_min, p_value);
	if (!Math::is_equal_approx(branch_length_max, p_value)) {
		branch_length_max = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_segments(int p_value) {
	p_value = CLAMP(p_value, 1, 32);
	if (branch_segments != p_value) {
		branch_segments = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_base_radius_scale(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.01, (real_t)1.0);
	if (!Math::is_equal_approx(branch_base_radius_scale, p_value)) {
		branch_base_radius_scale = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_bend(real_t p_value) {
	p_value = MAX((real_t)0.0, p_value);
	if (!Math::is_equal_approx(branch_bend, p_value)) {
		branch_bend = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_branch_droop(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.0, (real_t)10.0);
	if (!Math::is_equal_approx(branch_droop, p_value)) {
		branch_droop = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_secondary_branch_count(int p_value) {
	p_value = CLAMP(p_value, 0, 8);
	if (secondary_branch_count != p_value) {
		secondary_branch_count = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_secondary_branch_scale(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.1, (real_t)1.0);
	if (!Math::is_equal_approx(secondary_branch_scale, p_value)) {
		secondary_branch_scale = p_value;
		emit_changed();
	}
}
void OpenWorldTreeGenerationProfile::set_canopy_blob_count(int p_value) {
	p_value = CLAMP(p_value, 0, 256);
	if (canopy_blob_count != p_value) {
		canopy_blob_count = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_canopy_radius_min(real_t p_value) {
	p_value = MAX((real_t)0.05, p_value);
	if (!Math::is_equal_approx(canopy_radius_min, p_value)) {
		canopy_radius_min = p_value;
		canopy_radius_max = MAX(canopy_radius_max, canopy_radius_min);
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_canopy_radius_max(real_t p_value) {
	p_value = MAX(canopy_radius_min, p_value);
	if (!Math::is_equal_approx(canopy_radius_max, p_value)) {
		canopy_radius_max = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_canopy_vertical_scale(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.05, (real_t)4.0);
	if (!Math::is_equal_approx(canopy_vertical_scale, p_value)) {
		canopy_vertical_scale = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_canopy_position_jitter(real_t p_value) {
	p_value = MAX((real_t)0.0, p_value);
	if (!Math::is_equal_approx(canopy_position_jitter, p_value)) {
		canopy_position_jitter = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_canopy_roughness(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.0, (real_t)1.0);
	if (!Math::is_equal_approx(canopy_roughness, p_value)) {
		canopy_roughness = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_root_flare_scale(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)1.0, (real_t)8.0);
	if (!Math::is_equal_approx(root_flare_scale, p_value)) {
		root_flare_scale = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_root_height(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.05, (real_t)20.0);
	if (!Math::is_equal_approx(root_height, p_value)) {
		root_height = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_root_count(int p_value) {
	p_value = CLAMP(p_value, 0, 16);
	if (root_count != p_value) {
		root_count = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_root_length(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.05, (real_t)20.0);
	if (!Math::is_equal_approx(root_length, p_value)) {
		root_length = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_palm_frond_count(int p_value) {
	p_value = CLAMP(p_value, 3, 64);
	if (palm_frond_count != p_value) {
		palm_frond_count = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_palm_frond_length(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.1, (real_t)30.0);
	if (!Math::is_equal_approx(palm_frond_length, p_value)) {
		palm_frond_length = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_palm_frond_width(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.02, (real_t)10.0);
	if (!Math::is_equal_approx(palm_frond_width, p_value)) {
		palm_frond_width = p_value;
		emit_changed();
	}
}

void OpenWorldTreeGenerationProfile::set_palm_frond_droop(real_t p_value) {
	p_value = CLAMP(p_value, (real_t)0.0, (real_t)10.0);
	if (!Math::is_equal_approx(palm_frond_droop, p_value)) {
		palm_frond_droop = p_value;
		emit_changed();
	}
}
void OpenWorldTreeGenerationProfile::_bind_methods() {
	BIND_ENUM_CONSTANT(ARCHETYPE_TEMPERATE_BROADLEAF);
	BIND_ENUM_CONSTANT(ARCHETYPE_TROPICAL_BROADLEAF);
	BIND_ENUM_CONSTANT(ARCHETYPE_UMBRELLA);
	BIND_ENUM_CONSTANT(ARCHETYPE_CONIFER);
	BIND_ENUM_CONSTANT(ARCHETYPE_PALM);
	BIND_ENUM_CONSTANT(ARCHETYPE_MANGROVE);
	BIND_ENUM_CONSTANT(CROWN_AUTO);
	BIND_ENUM_CONSTANT(CROWN_ROUND);
	BIND_ENUM_CONSTANT(CROWN_UMBRELLA);
	BIND_ENUM_CONSTANT(CROWN_CONICAL);
	BIND_ENUM_CONSTANT(CROWN_TIERED);
	BIND_ENUM_CONSTANT(ROOT_AUTO);
	BIND_ENUM_CONSTANT(ROOT_NONE);
	BIND_ENUM_CONSTANT(ROOT_FLARE);
	BIND_ENUM_CONSTANT(ROOT_BUTTRESS);
	BIND_ENUM_CONSTANT(ROOT_PROP);

	ClassDB::bind_method(D_METHOD("set_archetype", "value"), &OpenWorldTreeGenerationProfile::set_archetype);
	ClassDB::bind_method(D_METHOD("get_archetype"), &OpenWorldTreeGenerationProfile::get_archetype);
	ClassDB::bind_method(D_METHOD("set_crown_shape", "value"), &OpenWorldTreeGenerationProfile::set_crown_shape);
	ClassDB::bind_method(D_METHOD("get_crown_shape"), &OpenWorldTreeGenerationProfile::get_crown_shape);
	ClassDB::bind_method(D_METHOD("set_root_style", "value"), &OpenWorldTreeGenerationProfile::set_root_style);
	ClassDB::bind_method(D_METHOD("get_root_style"), &OpenWorldTreeGenerationProfile::get_root_style);
	ClassDB::bind_method(D_METHOD("set_tree_height", "value"), &OpenWorldTreeGenerationProfile::set_tree_height);
	ClassDB::bind_method(D_METHOD("get_tree_height"), &OpenWorldTreeGenerationProfile::get_tree_height);
	ClassDB::bind_method(D_METHOD("set_trunk_segments", "value"), &OpenWorldTreeGenerationProfile::set_trunk_segments);
	ClassDB::bind_method(D_METHOD("get_trunk_segments"), &OpenWorldTreeGenerationProfile::get_trunk_segments);
	ClassDB::bind_method(D_METHOD("set_trunk_base_radius", "value"), &OpenWorldTreeGenerationProfile::set_trunk_base_radius);
	ClassDB::bind_method(D_METHOD("get_trunk_base_radius"), &OpenWorldTreeGenerationProfile::get_trunk_base_radius);
	ClassDB::bind_method(D_METHOD("set_trunk_tip_radius", "value"), &OpenWorldTreeGenerationProfile::set_trunk_tip_radius);
	ClassDB::bind_method(D_METHOD("get_trunk_tip_radius"), &OpenWorldTreeGenerationProfile::get_trunk_tip_radius);
	ClassDB::bind_method(D_METHOD("set_trunk_radial_sides", "value"), &OpenWorldTreeGenerationProfile::set_trunk_radial_sides);
	ClassDB::bind_method(D_METHOD("get_trunk_radial_sides"), &OpenWorldTreeGenerationProfile::get_trunk_radial_sides);
	ClassDB::bind_method(D_METHOD("set_trunk_bend", "value"), &OpenWorldTreeGenerationProfile::set_trunk_bend);
	ClassDB::bind_method(D_METHOD("get_trunk_bend"), &OpenWorldTreeGenerationProfile::get_trunk_bend);

	ClassDB::bind_method(D_METHOD("set_branch_start_ratio", "value"), &OpenWorldTreeGenerationProfile::set_branch_start_ratio);
	ClassDB::bind_method(D_METHOD("get_branch_start_ratio"), &OpenWorldTreeGenerationProfile::get_branch_start_ratio);
	ClassDB::bind_method(D_METHOD("set_branch_end_ratio", "value"), &OpenWorldTreeGenerationProfile::set_branch_end_ratio);
	ClassDB::bind_method(D_METHOD("get_branch_end_ratio"), &OpenWorldTreeGenerationProfile::get_branch_end_ratio);
	ClassDB::bind_method(D_METHOD("set_branch_interval", "value"), &OpenWorldTreeGenerationProfile::set_branch_interval);
	ClassDB::bind_method(D_METHOD("get_branch_interval"), &OpenWorldTreeGenerationProfile::get_branch_interval);
	ClassDB::bind_method(D_METHOD("set_phyllotaxy_angle_degrees", "value"), &OpenWorldTreeGenerationProfile::set_phyllotaxy_angle_degrees);
	ClassDB::bind_method(D_METHOD("get_phyllotaxy_angle_degrees"), &OpenWorldTreeGenerationProfile::get_phyllotaxy_angle_degrees);
	ClassDB::bind_method(D_METHOD("set_branch_elevation_degrees", "value"), &OpenWorldTreeGenerationProfile::set_branch_elevation_degrees);
	ClassDB::bind_method(D_METHOD("get_branch_elevation_degrees"), &OpenWorldTreeGenerationProfile::get_branch_elevation_degrees);
	ClassDB::bind_method(D_METHOD("set_branch_length_min", "value"), &OpenWorldTreeGenerationProfile::set_branch_length_min);
	ClassDB::bind_method(D_METHOD("get_branch_length_min"), &OpenWorldTreeGenerationProfile::get_branch_length_min);
	ClassDB::bind_method(D_METHOD("set_branch_length_max", "value"), &OpenWorldTreeGenerationProfile::set_branch_length_max);
	ClassDB::bind_method(D_METHOD("get_branch_length_max"), &OpenWorldTreeGenerationProfile::get_branch_length_max);
	ClassDB::bind_method(D_METHOD("set_branch_segments", "value"), &OpenWorldTreeGenerationProfile::set_branch_segments);
	ClassDB::bind_method(D_METHOD("get_branch_segments"), &OpenWorldTreeGenerationProfile::get_branch_segments);
	ClassDB::bind_method(D_METHOD("set_branch_base_radius_scale", "value"), &OpenWorldTreeGenerationProfile::set_branch_base_radius_scale);
	ClassDB::bind_method(D_METHOD("get_branch_base_radius_scale"), &OpenWorldTreeGenerationProfile::get_branch_base_radius_scale);
	ClassDB::bind_method(D_METHOD("set_branch_bend", "value"), &OpenWorldTreeGenerationProfile::set_branch_bend);
	ClassDB::bind_method(D_METHOD("get_branch_bend"), &OpenWorldTreeGenerationProfile::get_branch_bend);

	ClassDB::bind_method(D_METHOD("set_branch_droop", "value"), &OpenWorldTreeGenerationProfile::set_branch_droop);
	ClassDB::bind_method(D_METHOD("get_branch_droop"), &OpenWorldTreeGenerationProfile::get_branch_droop);
	ClassDB::bind_method(D_METHOD("set_secondary_branch_count", "value"), &OpenWorldTreeGenerationProfile::set_secondary_branch_count);
	ClassDB::bind_method(D_METHOD("get_secondary_branch_count"), &OpenWorldTreeGenerationProfile::get_secondary_branch_count);
	ClassDB::bind_method(D_METHOD("set_secondary_branch_scale", "value"), &OpenWorldTreeGenerationProfile::set_secondary_branch_scale);
	ClassDB::bind_method(D_METHOD("get_secondary_branch_scale"), &OpenWorldTreeGenerationProfile::get_secondary_branch_scale);
	ClassDB::bind_method(D_METHOD("set_canopy_blob_count", "value"), &OpenWorldTreeGenerationProfile::set_canopy_blob_count);
	ClassDB::bind_method(D_METHOD("get_canopy_blob_count"), &OpenWorldTreeGenerationProfile::get_canopy_blob_count);
	ClassDB::bind_method(D_METHOD("set_canopy_radius_min", "value"), &OpenWorldTreeGenerationProfile::set_canopy_radius_min);
	ClassDB::bind_method(D_METHOD("get_canopy_radius_min"), &OpenWorldTreeGenerationProfile::get_canopy_radius_min);
	ClassDB::bind_method(D_METHOD("set_canopy_radius_max", "value"), &OpenWorldTreeGenerationProfile::set_canopy_radius_max);
	ClassDB::bind_method(D_METHOD("get_canopy_radius_max"), &OpenWorldTreeGenerationProfile::get_canopy_radius_max);
	ClassDB::bind_method(D_METHOD("set_canopy_vertical_scale", "value"), &OpenWorldTreeGenerationProfile::set_canopy_vertical_scale);
	ClassDB::bind_method(D_METHOD("get_canopy_vertical_scale"), &OpenWorldTreeGenerationProfile::get_canopy_vertical_scale);
	ClassDB::bind_method(D_METHOD("set_canopy_position_jitter", "value"), &OpenWorldTreeGenerationProfile::set_canopy_position_jitter);
	ClassDB::bind_method(D_METHOD("get_canopy_position_jitter"), &OpenWorldTreeGenerationProfile::get_canopy_position_jitter);
	ClassDB::bind_method(D_METHOD("set_canopy_roughness", "value"), &OpenWorldTreeGenerationProfile::set_canopy_roughness);
	ClassDB::bind_method(D_METHOD("get_canopy_roughness"), &OpenWorldTreeGenerationProfile::get_canopy_roughness);

	ClassDB::bind_method(D_METHOD("set_root_flare_scale", "value"), &OpenWorldTreeGenerationProfile::set_root_flare_scale);
	ClassDB::bind_method(D_METHOD("get_root_flare_scale"), &OpenWorldTreeGenerationProfile::get_root_flare_scale);
	ClassDB::bind_method(D_METHOD("set_root_height", "value"), &OpenWorldTreeGenerationProfile::set_root_height);
	ClassDB::bind_method(D_METHOD("get_root_height"), &OpenWorldTreeGenerationProfile::get_root_height);
	ClassDB::bind_method(D_METHOD("set_root_count", "value"), &OpenWorldTreeGenerationProfile::set_root_count);
	ClassDB::bind_method(D_METHOD("get_root_count"), &OpenWorldTreeGenerationProfile::get_root_count);
	ClassDB::bind_method(D_METHOD("set_root_length", "value"), &OpenWorldTreeGenerationProfile::set_root_length);
	ClassDB::bind_method(D_METHOD("get_root_length"), &OpenWorldTreeGenerationProfile::get_root_length);
	ClassDB::bind_method(D_METHOD("set_palm_frond_count", "value"), &OpenWorldTreeGenerationProfile::set_palm_frond_count);
	ClassDB::bind_method(D_METHOD("get_palm_frond_count"), &OpenWorldTreeGenerationProfile::get_palm_frond_count);
	ClassDB::bind_method(D_METHOD("set_palm_frond_length", "value"), &OpenWorldTreeGenerationProfile::set_palm_frond_length);
	ClassDB::bind_method(D_METHOD("get_palm_frond_length"), &OpenWorldTreeGenerationProfile::get_palm_frond_length);
	ClassDB::bind_method(D_METHOD("set_palm_frond_width", "value"), &OpenWorldTreeGenerationProfile::set_palm_frond_width);
	ClassDB::bind_method(D_METHOD("get_palm_frond_width"), &OpenWorldTreeGenerationProfile::get_palm_frond_width);
	ClassDB::bind_method(D_METHOD("set_palm_frond_droop", "value"), &OpenWorldTreeGenerationProfile::set_palm_frond_droop);
	ClassDB::bind_method(D_METHOD("get_palm_frond_droop"), &OpenWorldTreeGenerationProfile::get_palm_frond_droop);

	ADD_GROUP("Architecture", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "archetype", PROPERTY_HINT_ENUM, "Temperate Broadleaf,Tropical Broadleaf,Umbrella,Conifer,Palm,Mangrove"), "set_archetype", "get_archetype");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "crown_shape", PROPERTY_HINT_ENUM, "Auto,Round,Umbrella,Conical,Tiered"), "set_crown_shape", "get_crown_shape");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "root_style", PROPERTY_HINT_ENUM, "Auto,None,Flare,Buttress,Prop"), "set_root_style", "get_root_style");
	ADD_GROUP("Trunk", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tree_height", PROPERTY_HINT_RANGE, "0.1,100,0.1,or_greater,suffix:m"), "set_tree_height", "get_tree_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trunk_segments", PROPERTY_HINT_RANGE, "2,64,1"), "set_trunk_segments", "get_trunk_segments");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trunk_base_radius", PROPERTY_HINT_RANGE, "0.01,10,0.01,or_greater,suffix:m"), "set_trunk_base_radius", "get_trunk_base_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trunk_tip_radius", PROPERTY_HINT_RANGE, "0.001,10,0.001,or_greater,suffix:m"), "set_trunk_tip_radius", "get_trunk_tip_radius");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trunk_radial_sides", PROPERTY_HINT_RANGE, "3,32,1"), "set_trunk_radial_sides", "get_trunk_radial_sides");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trunk_bend", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:m"), "set_trunk_bend", "get_trunk_bend");

	ADD_GROUP("Branches", "branch_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_start_ratio", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_branch_start_ratio", "get_branch_start_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_end_ratio", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_branch_end_ratio", "get_branch_end_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_interval", PROPERTY_HINT_RANGE, "0.05,10,0.01,or_greater,suffix:m"), "set_branch_interval", "get_branch_interval");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phyllotaxy_angle_degrees", PROPERTY_HINT_RANGE, "0,360,0.1,degrees"), "set_phyllotaxy_angle_degrees", "get_phyllotaxy_angle_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_elevation_degrees", PROPERTY_HINT_RANGE, "-89,89,0.1,degrees"), "set_branch_elevation_degrees", "get_branch_elevation_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_length_min", PROPERTY_HINT_RANGE, "0.05,100,0.05,or_greater,suffix:m"), "set_branch_length_min", "get_branch_length_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_length_max", PROPERTY_HINT_RANGE, "0.05,100,0.05,or_greater,suffix:m"), "set_branch_length_max", "get_branch_length_max");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "branch_segments", PROPERTY_HINT_RANGE, "1,32,1"), "set_branch_segments", "get_branch_segments");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_base_radius_scale", PROPERTY_HINT_RANGE, "0.01,1,0.01"), "set_branch_base_radius_scale", "get_branch_base_radius_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_bend", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:m"), "set_branch_bend", "get_branch_bend");

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "branch_droop", PROPERTY_HINT_RANGE, "0,10,0.01,suffix:m"), "set_branch_droop", "get_branch_droop");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "secondary_branch_count", PROPERTY_HINT_RANGE, "0,8,1"), "set_secondary_branch_count", "get_secondary_branch_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "secondary_branch_scale", PROPERTY_HINT_RANGE, "0.1,1,0.01"), "set_secondary_branch_scale", "get_secondary_branch_scale");
	ADD_GROUP("Canopy", "canopy_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "canopy_blob_count", PROPERTY_HINT_RANGE, "0,256,1"), "set_canopy_blob_count", "get_canopy_blob_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "canopy_radius_min", PROPERTY_HINT_RANGE, "0.05,100,0.05,or_greater,suffix:m"), "set_canopy_radius_min", "get_canopy_radius_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "canopy_radius_max", PROPERTY_HINT_RANGE, "0.05,100,0.05,or_greater,suffix:m"), "set_canopy_radius_max", "get_canopy_radius_max");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "canopy_vertical_scale", PROPERTY_HINT_RANGE, "0.05,4,0.01"), "set_canopy_vertical_scale", "get_canopy_vertical_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "canopy_position_jitter", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:m"), "set_canopy_position_jitter", "get_canopy_position_jitter");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "canopy_roughness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_canopy_roughness", "get_canopy_roughness");
	ADD_GROUP("Roots", "root_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "root_flare_scale", PROPERTY_HINT_RANGE, "1,8,0.05"), "set_root_flare_scale", "get_root_flare_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "root_height", PROPERTY_HINT_RANGE, "0.05,20,0.05,suffix:m"), "set_root_height", "get_root_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "root_count", PROPERTY_HINT_RANGE, "0,16,1"), "set_root_count", "get_root_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "root_length", PROPERTY_HINT_RANGE, "0.05,20,0.05,suffix:m"), "set_root_length", "get_root_length");

	ADD_GROUP("Palm Fronds", "palm_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "palm_frond_count", PROPERTY_HINT_RANGE, "3,64,1"), "set_palm_frond_count", "get_palm_frond_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "palm_frond_length", PROPERTY_HINT_RANGE, "0.1,30,0.05,suffix:m"), "set_palm_frond_length", "get_palm_frond_length");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "palm_frond_width", PROPERTY_HINT_RANGE, "0.02,10,0.01,suffix:m"), "set_palm_frond_width", "get_palm_frond_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "palm_frond_droop", PROPERTY_HINT_RANGE, "0,10,0.01,suffix:m"), "set_palm_frond_droop", "get_palm_frond_droop");
}

