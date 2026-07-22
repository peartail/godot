/**************************************************************************/
/*  open_world_placement_data.h                                           */
/**************************************************************************/

#pragma once

#include "open_world_placement_entry.h"

#include "core/io/resource.h"

class OpenWorldPlacementData : public Resource {
	GDCLASS(OpenWorldPlacementData, Resource);
	RES_BASE_EXTENSION("owplacementdata");

	PackedStringArray stable_ids;
	Array source_entries;
	PackedVector3Array positions;
	PackedVector3Array rotations;
	PackedVector3Array scales;
	PackedVector3Array terrain_normals;
	PackedInt32Array seeds;
	PackedFloat32Array spacing_radii;



protected:
	static void _bind_methods();

public:
	void set_stable_ids(const PackedStringArray &p_value);
	PackedStringArray get_stable_ids() const { return stable_ids; }
	void set_source_entries(const Array &p_value);
	Array get_source_entries() const { return source_entries; }
	void set_positions(const PackedVector3Array &p_value);
	PackedVector3Array get_positions() const { return positions; }
	void set_rotations(const PackedVector3Array &p_value);
	PackedVector3Array get_rotations() const { return rotations; }
	void set_scales(const PackedVector3Array &p_value);
	PackedVector3Array get_scales() const { return scales; }
	void set_terrain_normals(const PackedVector3Array &p_value);
	PackedVector3Array get_terrain_normals() const { return terrain_normals; }
	void set_seeds(const PackedInt32Array &p_value);
	PackedInt32Array get_seeds() const { return seeds; }
	void set_spacing_radii(const PackedFloat32Array &p_value);
	PackedFloat32Array get_spacing_radii() const { return spacing_radii; }

	int get_placement_count() const { return stable_ids.size(); }
	Dictionary get_placement(int p_index) const;
	void add_placement(const String &p_stable_id, const Ref<OpenWorldPlacementEntry> &p_entry, const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, const Vector3 &p_terrain_normal, int p_seed, real_t p_spacing_radius);
	void remove_placement_at(int p_index);
	int find_placement(const String &p_stable_id) const;
	void clear_placements();
	void assign_from(const Ref<OpenWorldPlacementData> &p_other);
	Dictionary validate_data() const;
};

