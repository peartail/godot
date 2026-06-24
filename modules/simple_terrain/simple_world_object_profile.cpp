/**************************************************************************/
/*  simple_world_object_profile.cpp                                       */
/**************************************************************************/

#include "simple_world_object_profile.h"

#include "core/object/class_db.h"

void SimpleWorldObjectProfile::set_id(const String &p_id) {
	if (id == p_id) {
		return;
	}
	id = p_id;
	emit_changed();
}

void SimpleWorldObjectProfile::set_display_name(const String &p_display_name) {
	if (display_name == p_display_name) {
		return;
	}
	display_name = p_display_name;
	emit_changed();
}

void SimpleWorldObjectProfile::set_category(const String &p_category) {
	if (category == p_category) {
		return;
	}
	category = p_category;
	emit_changed();
}

void SimpleWorldObjectProfile::set_scene(const Ref<PackedScene> &p_scene) {
	if (scene == p_scene) {
		return;
	}
	scene = p_scene;
	emit_changed();
}

void SimpleWorldObjectProfile::set_preview_icon(const Ref<Texture2D> &p_preview_icon) {
	if (preview_icon == p_preview_icon) {
		return;
	}
	preview_icon = p_preview_icon;
	emit_changed();
}

void SimpleWorldObjectProfile::set_placement_type(PlacementType p_placement_type) {
	if (placement_type == p_placement_type) {
		return;
	}
	placement_type = p_placement_type;
	emit_changed();
}

void SimpleWorldObjectProfile::set_collision_radius(real_t p_collision_radius) {
	const real_t new_radius = MAX((real_t)0.0, p_collision_radius);
	if (Math::is_equal_approx(collision_radius, new_radius)) {
		return;
	}
	collision_radius = new_radius;
	emit_changed();
}

void SimpleWorldObjectProfile::set_spacing(real_t p_spacing) {
	const real_t new_spacing = MAX((real_t)0.0, p_spacing);
	if (Math::is_equal_approx(spacing, new_spacing)) {
		return;
	}
	spacing = new_spacing;
	emit_changed();
}

void SimpleWorldObjectProfile::set_density(real_t p_density) {
	const real_t new_density = MAX((real_t)0.0, p_density);
	if (Math::is_equal_approx(density, new_density)) {
		return;
	}
	density = new_density;
	emit_changed();
}

void SimpleWorldObjectProfile::set_min_scale(const Vector3 &p_min_scale) {
	if (min_scale.is_equal_approx(p_min_scale)) {
		return;
	}
	min_scale = p_min_scale;
	emit_changed();
}

void SimpleWorldObjectProfile::set_max_scale(const Vector3 &p_max_scale) {
	if (max_scale.is_equal_approx(p_max_scale)) {
		return;
	}
	max_scale = p_max_scale;
	emit_changed();
}

void SimpleWorldObjectProfile::set_random_yaw(bool p_random_yaw) {
	if (random_yaw == p_random_yaw) {
		return;
	}
	random_yaw = p_random_yaw;
	emit_changed();
}

void SimpleWorldObjectProfile::set_align_to_terrain_normal(bool p_align_to_terrain_normal) {
	if (align_to_terrain_normal == p_align_to_terrain_normal) {
		return;
	}
	align_to_terrain_normal = p_align_to_terrain_normal;
	emit_changed();
}

void SimpleWorldObjectProfile::set_slope_min_degrees(real_t p_slope_min_degrees) {
	const real_t new_slope = CLAMP(p_slope_min_degrees, (real_t)0.0, (real_t)90.0);
	if (Math::is_equal_approx(slope_min_degrees, new_slope)) {
		return;
	}
	slope_min_degrees = new_slope;
	emit_changed();
}

void SimpleWorldObjectProfile::set_slope_max_degrees(real_t p_slope_max_degrees) {
	const real_t new_slope = CLAMP(p_slope_max_degrees, (real_t)0.0, (real_t)90.0);
	if (Math::is_equal_approx(slope_max_degrees, new_slope)) {
		return;
	}
	slope_max_degrees = new_slope;
	emit_changed();
}

void SimpleWorldObjectProfile::set_height_min(real_t p_height_min) {
	if (Math::is_equal_approx(height_min, p_height_min)) {
		return;
	}
	height_min = p_height_min;
	emit_changed();
}

void SimpleWorldObjectProfile::set_height_max(real_t p_height_max) {
	if (Math::is_equal_approx(height_max, p_height_max)) {
		return;
	}
	height_max = p_height_max;
	emit_changed();
}

void SimpleWorldObjectProfile::set_surface_offset(real_t p_surface_offset) {
	if (Math::is_equal_approx(surface_offset, p_surface_offset)) {
		return;
	}
	surface_offset = p_surface_offset;
	emit_changed();
}

void SimpleWorldObjectProfile::set_tags(const PackedStringArray &p_tags) {
	tags = p_tags;
	emit_changed();
}

void SimpleWorldObjectProfile::set_navigation_obstacle_mode(NavigationObstacleMode p_mode) {
	if (navigation_obstacle_mode == p_mode) {
		return;
	}
	navigation_obstacle_mode = p_mode;
	emit_changed();
}

bool SimpleWorldObjectProfile::uses_baked_navigation_obstacle() const {
	return navigation_obstacle_mode == NAVIGATION_OBSTACLE_BAKE_STATIC || navigation_obstacle_mode == NAVIGATION_OBSTACLE_BAKE_AND_RUNTIME;
}

bool SimpleWorldObjectProfile::uses_runtime_navigation_obstacle() const {
	return navigation_obstacle_mode == NAVIGATION_OBSTACLE_RUNTIME_AVOIDANCE || navigation_obstacle_mode == NAVIGATION_OBSTACLE_BAKE_AND_RUNTIME;
}

void SimpleWorldObjectProfile::set_navigation_obstacle_radius(real_t p_radius) {
	const real_t new_radius = MAX((real_t)0.0, p_radius);
	if (Math::is_equal_approx(navigation_obstacle_radius, new_radius)) {
		return;
	}
	navigation_obstacle_radius = new_radius;
	emit_changed();
}

void SimpleWorldObjectProfile::set_navigation_obstacle_height(real_t p_height) {
	const real_t new_height = MAX((real_t)0.0, p_height);
	if (Math::is_equal_approx(navigation_obstacle_height, new_height)) {
		return;
	}
	navigation_obstacle_height = new_height;
	emit_changed();
}

void SimpleWorldObjectProfile::set_navigation_obstacle_carve(bool p_carve) {
	if (navigation_obstacle_carve == p_carve) {
		return;
	}
	navigation_obstacle_carve = p_carve;
	emit_changed();
}

void SimpleWorldObjectProfile::set_navigation_avoidance_layers(uint32_t p_layers) {
	if (navigation_avoidance_layers == p_layers) {
		return;
	}
	navigation_avoidance_layers = p_layers;
	emit_changed();
}

void SimpleWorldObjectProfile::set_navigation_obstacle_shape_source(NavigationObstacleShapeSource p_source) {
	if (navigation_obstacle_shape_source == p_source) {
		return;
	}
	navigation_obstacle_shape_source = p_source;
	emit_changed();
}

void SimpleWorldObjectProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "id"), &SimpleWorldObjectProfile::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &SimpleWorldObjectProfile::get_id);
	ClassDB::bind_method(D_METHOD("set_display_name", "display_name"), &SimpleWorldObjectProfile::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &SimpleWorldObjectProfile::get_display_name);
	ClassDB::bind_method(D_METHOD("set_category", "category"), &SimpleWorldObjectProfile::set_category);
	ClassDB::bind_method(D_METHOD("get_category"), &SimpleWorldObjectProfile::get_category);
	ClassDB::bind_method(D_METHOD("set_scene", "scene"), &SimpleWorldObjectProfile::set_scene);
	ClassDB::bind_method(D_METHOD("get_scene"), &SimpleWorldObjectProfile::get_scene);
	ClassDB::bind_method(D_METHOD("set_preview_icon", "preview_icon"), &SimpleWorldObjectProfile::set_preview_icon);
	ClassDB::bind_method(D_METHOD("get_preview_icon"), &SimpleWorldObjectProfile::get_preview_icon);
	ClassDB::bind_method(D_METHOD("set_placement_type", "placement_type"), &SimpleWorldObjectProfile::set_placement_type);
	ClassDB::bind_method(D_METHOD("get_placement_type"), &SimpleWorldObjectProfile::get_placement_type);
	ClassDB::bind_method(D_METHOD("set_collision_radius", "collision_radius"), &SimpleWorldObjectProfile::set_collision_radius);
	ClassDB::bind_method(D_METHOD("get_collision_radius"), &SimpleWorldObjectProfile::get_collision_radius);
	ClassDB::bind_method(D_METHOD("set_spacing", "spacing"), &SimpleWorldObjectProfile::set_spacing);
	ClassDB::bind_method(D_METHOD("get_spacing"), &SimpleWorldObjectProfile::get_spacing);
	ClassDB::bind_method(D_METHOD("set_density", "density"), &SimpleWorldObjectProfile::set_density);
	ClassDB::bind_method(D_METHOD("get_density"), &SimpleWorldObjectProfile::get_density);
	ClassDB::bind_method(D_METHOD("set_min_scale", "min_scale"), &SimpleWorldObjectProfile::set_min_scale);
	ClassDB::bind_method(D_METHOD("get_min_scale"), &SimpleWorldObjectProfile::get_min_scale);
	ClassDB::bind_method(D_METHOD("set_max_scale", "max_scale"), &SimpleWorldObjectProfile::set_max_scale);
	ClassDB::bind_method(D_METHOD("get_max_scale"), &SimpleWorldObjectProfile::get_max_scale);
	ClassDB::bind_method(D_METHOD("set_random_yaw", "random_yaw"), &SimpleWorldObjectProfile::set_random_yaw);
	ClassDB::bind_method(D_METHOD("is_random_yaw_enabled"), &SimpleWorldObjectProfile::is_random_yaw_enabled);
	ClassDB::bind_method(D_METHOD("set_align_to_terrain_normal", "align_to_terrain_normal"), &SimpleWorldObjectProfile::set_align_to_terrain_normal);
	ClassDB::bind_method(D_METHOD("is_aligning_to_terrain_normal"), &SimpleWorldObjectProfile::is_aligning_to_terrain_normal);
	ClassDB::bind_method(D_METHOD("set_slope_min_degrees", "slope_min_degrees"), &SimpleWorldObjectProfile::set_slope_min_degrees);
	ClassDB::bind_method(D_METHOD("get_slope_min_degrees"), &SimpleWorldObjectProfile::get_slope_min_degrees);
	ClassDB::bind_method(D_METHOD("set_slope_max_degrees", "slope_max_degrees"), &SimpleWorldObjectProfile::set_slope_max_degrees);
	ClassDB::bind_method(D_METHOD("get_slope_max_degrees"), &SimpleWorldObjectProfile::get_slope_max_degrees);
	ClassDB::bind_method(D_METHOD("set_height_min", "height_min"), &SimpleWorldObjectProfile::set_height_min);
	ClassDB::bind_method(D_METHOD("get_height_min"), &SimpleWorldObjectProfile::get_height_min);
	ClassDB::bind_method(D_METHOD("set_height_max", "height_max"), &SimpleWorldObjectProfile::set_height_max);
	ClassDB::bind_method(D_METHOD("get_height_max"), &SimpleWorldObjectProfile::get_height_max);
	ClassDB::bind_method(D_METHOD("set_surface_offset", "surface_offset"), &SimpleWorldObjectProfile::set_surface_offset);
	ClassDB::bind_method(D_METHOD("get_surface_offset"), &SimpleWorldObjectProfile::get_surface_offset);
	ClassDB::bind_method(D_METHOD("set_tags", "tags"), &SimpleWorldObjectProfile::set_tags);
	ClassDB::bind_method(D_METHOD("get_tags"), &SimpleWorldObjectProfile::get_tags);
	ClassDB::bind_method(D_METHOD("set_navigation_obstacle_mode", "mode"), &SimpleWorldObjectProfile::set_navigation_obstacle_mode);
	ClassDB::bind_method(D_METHOD("get_navigation_obstacle_mode"), &SimpleWorldObjectProfile::get_navigation_obstacle_mode);
	ClassDB::bind_method(D_METHOD("uses_baked_navigation_obstacle"), &SimpleWorldObjectProfile::uses_baked_navigation_obstacle);
	ClassDB::bind_method(D_METHOD("uses_runtime_navigation_obstacle"), &SimpleWorldObjectProfile::uses_runtime_navigation_obstacle);
	ClassDB::bind_method(D_METHOD("set_navigation_obstacle_radius", "radius"), &SimpleWorldObjectProfile::set_navigation_obstacle_radius);
	ClassDB::bind_method(D_METHOD("get_navigation_obstacle_radius"), &SimpleWorldObjectProfile::get_navigation_obstacle_radius);
	ClassDB::bind_method(D_METHOD("set_navigation_obstacle_height", "height"), &SimpleWorldObjectProfile::set_navigation_obstacle_height);
	ClassDB::bind_method(D_METHOD("get_navigation_obstacle_height"), &SimpleWorldObjectProfile::get_navigation_obstacle_height);
	ClassDB::bind_method(D_METHOD("set_navigation_obstacle_carve", "carve"), &SimpleWorldObjectProfile::set_navigation_obstacle_carve);
	ClassDB::bind_method(D_METHOD("get_navigation_obstacle_carve"), &SimpleWorldObjectProfile::get_navigation_obstacle_carve);
	ClassDB::bind_method(D_METHOD("set_navigation_avoidance_layers", "layers"), &SimpleWorldObjectProfile::set_navigation_avoidance_layers);
	ClassDB::bind_method(D_METHOD("get_navigation_avoidance_layers"), &SimpleWorldObjectProfile::get_navigation_avoidance_layers);
	ClassDB::bind_method(D_METHOD("set_navigation_obstacle_shape_source", "source"), &SimpleWorldObjectProfile::set_navigation_obstacle_shape_source);
	ClassDB::bind_method(D_METHOD("get_navigation_obstacle_shape_source"), &SimpleWorldObjectProfile::get_navigation_obstacle_shape_source);

	BIND_ENUM_CONSTANT(PLACEMENT_SINGLE);
	BIND_ENUM_CONSTANT(PLACEMENT_BRUSH);
	BIND_ENUM_CONSTANT(PLACEMENT_SCATTER);
	BIND_ENUM_CONSTANT(PLACEMENT_GRASS);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_NONE);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_BAKE_STATIC);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_RUNTIME_AVOIDANCE);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_BAKE_AND_RUNTIME);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_SHAPE_RADIUS);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_SHAPE_SCENE_COLLISION);
	BIND_ENUM_CONSTANT(NAVIGATION_OBSTACLE_SHAPE_MESH_AABB);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "category"), "set_category", "get_category");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_scene", "get_scene");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "preview_icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_preview_icon", "get_preview_icon");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "placement_type", PROPERTY_HINT_ENUM, "Single,Brush,Scatter,Grass"), "set_placement_type", "get_placement_type");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_radius", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_collision_radius", "get_collision_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spacing", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_spacing", "get_spacing");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"), "set_density", "get_density");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "min_scale"), "set_min_scale", "get_min_scale");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "max_scale"), "set_max_scale", "get_max_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_yaw"), "set_random_yaw", "is_random_yaw_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "align_to_terrain_normal"), "set_align_to_terrain_normal", "is_aligning_to_terrain_normal");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_min_degrees", PROPERTY_HINT_RANGE, "0,90,0.1,degrees"), "set_slope_min_degrees", "get_slope_min_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_max_degrees", PROPERTY_HINT_RANGE, "0,90,0.1,degrees"), "set_slope_max_degrees", "get_slope_max_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_min", PROPERTY_HINT_RANGE, "-1000000,1000000,0.01,suffix:m"), "set_height_min", "get_height_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_max", PROPERTY_HINT_RANGE, "-1000000,1000000,0.01,suffix:m"), "set_height_max", "get_height_max");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_offset", PROPERTY_HINT_RANGE, "-1000,1000,0.01,suffix:m"), "set_surface_offset", "get_surface_offset");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tags"), "set_tags", "get_tags");
	ADD_GROUP("Navigation Obstacle", "navigation_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_obstacle_mode", PROPERTY_HINT_ENUM, "None,Bake Static,Runtime Avoidance,Bake And Runtime"), "set_navigation_obstacle_mode", "get_navigation_obstacle_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_obstacle_radius", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_navigation_obstacle_radius", "get_navigation_obstacle_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_obstacle_height", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_navigation_obstacle_height", "get_navigation_obstacle_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "navigation_obstacle_carve"), "set_navigation_obstacle_carve", "get_navigation_obstacle_carve");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_avoidance_layers", PROPERTY_HINT_LAYERS_AVOIDANCE), "set_navigation_avoidance_layers", "get_navigation_avoidance_layers");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_obstacle_shape_source", PROPERTY_HINT_ENUM, "Radius,Scene Collision,Mesh AABB"), "set_navigation_obstacle_shape_source", "get_navigation_obstacle_shape_source");
}
