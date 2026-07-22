/**************************************************************************/
/*  open_world_placement_preset.h                                   */
/**************************************************************************/

#pragma once

#include "open_world_placement_entry.h"

#include "core/io/resource.h"

class OpenWorldPlacementPreset : public Resource {
	GDCLASS(OpenWorldPlacementPreset, Resource);
	RES_BASE_EXTENSION("owplacementpreset");

public:
	enum PlacementShape {
		SHAPE_CIRCLE,
		SHAPE_RECTANGLE,
		SHAPE_ELLIPSE,
	};

private:
	String stable_id;
	String display_name;
	PlacementShape shape = SHAPE_CIRCLE;
	Vector2 size = Vector2(20.0, 20.0);
	real_t yaw_degrees = 0.0;
	real_t density_per_100_square_meters = 5.0;
	real_t minimum_spacing = 2.0;
	real_t height_min = -1000000.0;
	real_t height_max = 1000000.0;
	real_t slope_min_degrees = 0.0;
	real_t slope_max_degrees = 60.0;
	int max_objects_per_operation = 500;
	Array entries;

protected:
	static void _bind_methods();

public:
	void set_stable_id(String p_value);
	String get_stable_id() const { return stable_id; }
	void set_display_name(String p_value);
	String get_display_name() const { return display_name; }
	void set_shape(PlacementShape p_value);
	PlacementShape get_shape() const { return shape; }
	void set_size(Vector2 p_value);
	Vector2 get_size() const { return size; }
	void set_yaw_degrees(real_t p_value);
	real_t get_yaw_degrees() const { return yaw_degrees; }
	void set_density_per_100_square_meters(real_t p_value);
	real_t get_density_per_100_square_meters() const { return density_per_100_square_meters; }
	void set_minimum_spacing(real_t p_value);
	real_t get_minimum_spacing() const { return minimum_spacing; }
	void set_height_min(real_t p_value);
	real_t get_height_min() const { return height_min; }
	void set_height_max(real_t p_value);
	real_t get_height_max() const { return height_max; }
	void set_slope_min_degrees(real_t p_value);
	real_t get_slope_min_degrees() const { return slope_min_degrees; }
	void set_slope_max_degrees(real_t p_value);
	real_t get_slope_max_degrees() const { return slope_max_degrees; }
	void set_max_objects_per_operation(int p_value);
	int get_max_objects_per_operation() const { return max_objects_per_operation; }
	void set_entries(const Array &p_value);
	Array get_entries() const { return entries; }
	void add_entry(const Ref<OpenWorldPlacementEntry> &p_entry);
	void remove_entry_at(int p_index);
	int get_entry_count() const { return entries.size(); }
	Ref<OpenWorldPlacementEntry> get_entry(int p_index) const;

	real_t get_footprint_area() const;
	int get_requested_object_count() const;
	Dictionary validate_preset() const;
};

VARIANT_ENUM_CAST(OpenWorldPlacementPreset::PlacementShape);

