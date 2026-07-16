/**************************************************************************/
/*  open_world_vine_path_data.h                                           */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class OpenWorldVinePathData : public Resource {
	GDCLASS(OpenWorldVinePathData, Resource);
	RES_BASE_EXTENSION("owvinepath");

	Array paths;
	Array normals;
	Array attached_flags;
	PackedInt32Array parent_paths;
	PackedInt32Array support_segments;

protected:
	static void _bind_methods();

public:
	void set_paths(const Array &p_paths);
	Array get_paths() const { return paths; }
	void set_normals(const Array &p_normals);
	Array get_normals() const { return normals; }
	void set_attached_flags(const Array &p_flags);
	Array get_attached_flags() const { return attached_flags; }
	void set_parent_paths(const PackedInt32Array &p_parents);
	PackedInt32Array get_parent_paths() const { return parent_paths; }
	void set_support_segments(const PackedInt32Array &p_segments);
	PackedInt32Array get_support_segments() const { return support_segments; }

	void clear();
	void add_path(const PackedVector3Array &p_points, const PackedVector3Array &p_normals = PackedVector3Array(), const PackedByteArray &p_attached = PackedByteArray(), int p_parent_path = -1, int p_support_segment = -1);
	int get_path_count() const { return paths.size(); }
	PackedVector3Array get_path_points(int p_index) const;
	PackedVector3Array get_path_normals(int p_index) const;
	PackedByteArray get_path_attached_flags(int p_index) const;
	real_t get_total_length() const;
};
