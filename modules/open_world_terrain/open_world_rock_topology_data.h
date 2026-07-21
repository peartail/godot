/**************************************************************************/
/*  open_world_rock_topology_data.h                                       */
/**************************************************************************/
#pragma once

#include "core/io/resource.h"

class OpenWorldRockTopologyData : public Resource {
	GDCLASS(OpenWorldRockTopologyData, Resource);
	RES_BASE_EXTENSION("owrocktopology");

	PackedVector3Array source_points;
	PackedVector3Array hull_vertices;
	PackedInt32Array hull_indices;
	PackedInt32Array face_groups;
	real_t base_plane = 0.0;
	AABB local_bounds;
	int source_seed = 0;
	String topology_hash;

protected:
	static void _bind_methods();

public:
	void set_source_points(const PackedVector3Array &p_value); PackedVector3Array get_source_points() const { return source_points; }
	void set_hull_vertices(const PackedVector3Array &p_value); PackedVector3Array get_hull_vertices() const { return hull_vertices; }
	void set_hull_indices(const PackedInt32Array &p_value); PackedInt32Array get_hull_indices() const { return hull_indices; }
	void set_face_groups(const PackedInt32Array &p_value); PackedInt32Array get_face_groups() const { return face_groups; }
	void set_base_plane(real_t p_value); real_t get_base_plane() const { return base_plane; }
	void set_local_bounds(const AABB &p_value); AABB get_local_bounds() const { return local_bounds; }
	void set_source_seed(int p_value); int get_source_seed() const { return source_seed; }
	void set_topology_hash(const String &p_value); String get_topology_hash() const { return topology_hash; }
	int get_triangle_count() const { return hull_indices.size() / 3; }
	int get_base_contact_count(real_t p_epsilon = 0.0001) const;
};
