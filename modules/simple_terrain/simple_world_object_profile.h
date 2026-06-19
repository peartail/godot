/**************************************************************************/
/*  simple_world_object_profile.h                                         */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"

class SimpleWorldObjectProfile : public Resource {
	GDCLASS(SimpleWorldObjectProfile, Resource);
	RES_BASE_EXTENSION("swobjectprofile");

public:
	enum PlacementType {
		PLACEMENT_SINGLE,
		PLACEMENT_BRUSH,
		PLACEMENT_SCATTER,
		PLACEMENT_GRASS,
	};

	enum NavigationObstacleMode {
		NAVIGATION_OBSTACLE_NONE,
		NAVIGATION_OBSTACLE_BAKE_STATIC,
		NAVIGATION_OBSTACLE_RUNTIME_AVOIDANCE,
		NAVIGATION_OBSTACLE_BAKE_AND_RUNTIME,
	};

private:
	String id;
	String display_name;
	String category;
	Ref<PackedScene> scene;
	Ref<Texture2D> preview_icon;
	PlacementType placement_type = PLACEMENT_SINGLE;
	real_t collision_radius = 0.5;
	real_t spacing = 1.0;
	real_t density = 1.0;
	Vector3 min_scale = Vector3(1.0, 1.0, 1.0);
	Vector3 max_scale = Vector3(1.0, 1.0, 1.0);
	bool random_yaw = true;
	bool align_to_terrain_normal = false;
	real_t slope_min_degrees = 0.0;
	real_t slope_max_degrees = 45.0;
	real_t height_min = -1000000.0;
	real_t height_max = 1000000.0;
	real_t surface_offset = 0.0;
	PackedStringArray tags;
	NavigationObstacleMode navigation_obstacle_mode = NAVIGATION_OBSTACLE_NONE;
	real_t navigation_obstacle_radius = 0.5;
	real_t navigation_obstacle_height = 2.0;
	bool navigation_obstacle_carve = false;
	uint32_t navigation_avoidance_layers = 1;

protected:
	static void _bind_methods();

public:
	void set_id(const String &p_id);
	String get_id() const { return id; }

	void set_display_name(const String &p_display_name);
	String get_display_name() const { return display_name; }

	void set_category(const String &p_category);
	String get_category() const { return category; }

	void set_scene(const Ref<PackedScene> &p_scene);
	Ref<PackedScene> get_scene() const { return scene; }

	void set_preview_icon(const Ref<Texture2D> &p_preview_icon);
	Ref<Texture2D> get_preview_icon() const { return preview_icon; }

	void set_placement_type(PlacementType p_placement_type);
	PlacementType get_placement_type() const { return placement_type; }

	void set_collision_radius(real_t p_collision_radius);
	real_t get_collision_radius() const { return collision_radius; }

	void set_spacing(real_t p_spacing);
	real_t get_spacing() const { return spacing; }

	void set_density(real_t p_density);
	real_t get_density() const { return density; }

	void set_min_scale(const Vector3 &p_min_scale);
	Vector3 get_min_scale() const { return min_scale; }

	void set_max_scale(const Vector3 &p_max_scale);
	Vector3 get_max_scale() const { return max_scale; }

	void set_random_yaw(bool p_random_yaw);
	bool is_random_yaw_enabled() const { return random_yaw; }

	void set_align_to_terrain_normal(bool p_align_to_terrain_normal);
	bool is_aligning_to_terrain_normal() const { return align_to_terrain_normal; }

	void set_slope_min_degrees(real_t p_slope_min_degrees);
	real_t get_slope_min_degrees() const { return slope_min_degrees; }

	void set_slope_max_degrees(real_t p_slope_max_degrees);
	real_t get_slope_max_degrees() const { return slope_max_degrees; }

	void set_height_min(real_t p_height_min);
	real_t get_height_min() const { return height_min; }

	void set_height_max(real_t p_height_max);
	real_t get_height_max() const { return height_max; }

	void set_surface_offset(real_t p_surface_offset);
	real_t get_surface_offset() const { return surface_offset; }

	void set_tags(const PackedStringArray &p_tags);
	PackedStringArray get_tags() const { return tags; }

	void set_navigation_obstacle_mode(NavigationObstacleMode p_mode);
	NavigationObstacleMode get_navigation_obstacle_mode() const { return navigation_obstacle_mode; }
	bool uses_baked_navigation_obstacle() const;
	bool uses_runtime_navigation_obstacle() const;

	void set_navigation_obstacle_radius(real_t p_radius);
	real_t get_navigation_obstacle_radius() const { return navigation_obstacle_radius; }
	void set_navigation_obstacle_height(real_t p_height);
	real_t get_navigation_obstacle_height() const { return navigation_obstacle_height; }
	void set_navigation_obstacle_carve(bool p_carve);
	bool get_navigation_obstacle_carve() const { return navigation_obstacle_carve; }
	void set_navigation_avoidance_layers(uint32_t p_layers);
	uint32_t get_navigation_avoidance_layers() const { return navigation_avoidance_layers; }
};

VARIANT_ENUM_CAST(SimpleWorldObjectProfile::PlacementType);
VARIANT_ENUM_CAST(SimpleWorldObjectProfile::NavigationObstacleMode);
