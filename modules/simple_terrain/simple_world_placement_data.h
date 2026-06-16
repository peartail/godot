/**************************************************************************/
/*  simple_world_placement_data.h                                         */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "core/string/node_path.h"

class SimpleWorldPlacementLibrary;

class SimpleWorldPlacementData : public Resource {
	GDCLASS(SimpleWorldPlacementData, Resource);
	RES_BASE_EXTENSION("swplacementdata");

	NodePath terrain_path;
	Ref<SimpleWorldPlacementLibrary> library;
	PackedStringArray profile_ids;
	PackedVector3Array positions;
	PackedVector3Array rotations;
	PackedVector3Array scales;
	PackedVector3Array terrain_normals;
	PackedInt32Array seeds;
	PackedVector2Array chunk_coords;

	void _ensure_record_arrays_size();

protected:
	static void _bind_methods();

public:
	void set_terrain_path(const NodePath &p_terrain_path);
	NodePath get_terrain_path() const { return terrain_path; }

	void set_library(const Ref<SimpleWorldPlacementLibrary> &p_library);
	Ref<SimpleWorldPlacementLibrary> get_library() const { return library; }

	void set_profile_ids(const PackedStringArray &p_profile_ids);
	PackedStringArray get_profile_ids() const { return profile_ids; }

	void set_positions(const PackedVector3Array &p_positions);
	PackedVector3Array get_positions() const { return positions; }

	void set_rotations(const PackedVector3Array &p_rotations);
	PackedVector3Array get_rotations() const { return rotations; }

	void set_scales(const PackedVector3Array &p_scales);
	PackedVector3Array get_scales() const { return scales; }

	void set_terrain_normals(const PackedVector3Array &p_terrain_normals);
	PackedVector3Array get_terrain_normals() const { return terrain_normals; }

	void set_seeds(const PackedInt32Array &p_seeds);
	PackedInt32Array get_seeds() const { return seeds; }

	void set_chunk_coords(const PackedVector2Array &p_chunk_coords);
	PackedVector2Array get_chunk_coords() const { return chunk_coords; }

	int get_placement_count() const { return profile_ids.size(); }
	Dictionary get_placement(int p_index) const;
	void add_placement(const String &p_profile_id, const Vector3 &p_position, const Vector3 &p_rotation, const Vector3 &p_scale, const Vector3 &p_terrain_normal, int p_seed = 0, const Vector2 &p_chunk_coord = Vector2());
	void remove_placement_at(int p_index);
	void clear_placements();
};
