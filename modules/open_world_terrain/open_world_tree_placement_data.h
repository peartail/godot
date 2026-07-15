/**************************************************************************/
/*  open_world_tree_placement_data.h                                      */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class OpenWorldTreePlacementData : public Resource {
	GDCLASS(OpenWorldTreePlacementData, Resource);
	RES_BASE_EXTENSION("owtreeplacements");

	PackedVector3Array positions;
	PackedVector3Array rotations;
	PackedVector3Array scales;
	PackedInt32Array seeds;
	PackedInt32Array variant_indices;
	PackedInt32Array instance_ids;
	PackedByteArray enabled;
	PackedColorArray colors;
	PackedColorArray custom_data;

	void _ensure_record_arrays_size();
	int _get_next_instance_id() const;

protected:
	static void _bind_methods();

public:
	void set_positions(const PackedVector3Array &p_positions);
	PackedVector3Array get_positions() const { return positions; }
	void set_rotations(const PackedVector3Array &p_rotations);
	PackedVector3Array get_rotations() const { return rotations; }
	void set_scales(const PackedVector3Array &p_scales);
	PackedVector3Array get_scales() const { return scales; }
	void set_seeds(const PackedInt32Array &p_seeds);
	PackedInt32Array get_seeds() const { return seeds; }
	void set_variant_indices(const PackedInt32Array &p_indices);
	PackedInt32Array get_variant_indices() const { return variant_indices; }
	void set_instance_ids(const PackedInt32Array &p_ids);
	PackedInt32Array get_instance_ids() const { return instance_ids; }
	void set_enabled(const PackedByteArray &p_enabled);
	PackedByteArray get_enabled() const { return enabled; }
	void set_colors(const PackedColorArray &p_colors);
	PackedColorArray get_colors() const { return colors; }
	void set_custom_data(const PackedColorArray &p_custom_data);
	PackedColorArray get_custom_data() const { return custom_data; }

	int get_instance_count() const { return positions.size(); }
	Dictionary get_instance(int p_index) const;
	Transform3D get_instance_transform(int p_index) const;
	int find_index_by_id(int p_instance_id) const;
	int add_tree(const Vector3 &p_position, const Vector3 &p_rotation = Vector3(), const Vector3 &p_scale = Vector3(1.0, 1.0, 1.0), int p_seed = 0, int p_variant_index = -1, const Color &p_color = Color(1.0, 1.0, 1.0, 1.0), const Color &p_custom_data = Color());
	void remove_tree_at(int p_index);
	void set_instance_enabled(int p_index, bool p_enabled);
	bool is_instance_enabled(int p_index) const;
	void clear_trees();
};
