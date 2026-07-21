/**************************************************************************/
/*  open_world_rock_placement_data.h                                      */
/**************************************************************************/
#pragma once

#include "core/io/resource.h"

class OpenWorldRockPlacementData : public Resource {
	GDCLASS(OpenWorldRockPlacementData, Resource);
	RES_BASE_EXTENSION("owrockplacements");

	PackedVector3Array positions;
	PackedVector3Array rotations;
	PackedVector3Array scales;
	PackedInt32Array variant_indices;
	PackedStringArray stable_ids;

protected:
	static void _bind_methods();

public:
	void set_positions(const PackedVector3Array &p_value); PackedVector3Array get_positions() const { return positions; }
	void set_rotations(const PackedVector3Array &p_value); PackedVector3Array get_rotations() const { return rotations; }
	void set_scales(const PackedVector3Array &p_value); PackedVector3Array get_scales() const { return scales; }
	void set_variant_indices(const PackedInt32Array &p_value); PackedInt32Array get_variant_indices() const { return variant_indices; }
	void set_stable_ids(const PackedStringArray &p_value); PackedStringArray get_stable_ids() const { return stable_ids; }
	void clear();
	void add_placement(const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, int p_variant_index, const String &p_stable_id);
	int get_placement_count() const { return positions.size(); }
	Array get_placements_in_cell(const Vector2i &p_cell, real_t p_cell_size) const;
	Dictionary validate_placements() const;
};
