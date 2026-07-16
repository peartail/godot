/**************************************************************************/
/*  open_world_tree_support_graph.h                                       */
/**************************************************************************/
#pragma once
#include "core/io/resource.h"

class OpenWorldTreeSupportGraph : public Resource {
	GDCLASS(OpenWorldTreeSupportGraph, Resource);
	RES_BASE_EXTENSION("owtreesupport");
	Array paths;
	Array radii;
	PackedInt32Array parent_paths;
	PackedFloat32Array attachment_ratios;
protected:
	static void _bind_methods();
public:
	void set_paths(const Array &p_value);
	Array get_paths() const { return paths; }
	void set_radii(const Array &p_value);
	Array get_radii() const { return radii; }
	void set_parent_paths(const PackedInt32Array &p_value);
	PackedInt32Array get_parent_paths() const { return parent_paths; }
	void set_attachment_ratios(const PackedFloat32Array &p_value);
	PackedFloat32Array get_attachment_ratios() const { return attachment_ratios; }
	void clear();
	void add_path(const PackedVector3Array &p_points, const PackedFloat32Array &p_radii, int p_parent_path = -1, real_t p_attachment_ratio = 0.0);
	int get_path_count() const { return paths.size(); }
	PackedVector3Array get_path_points(int p_index) const;
	PackedFloat32Array get_path_radii(int p_index) const;
};
