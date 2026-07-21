/**************************************************************************/
/*  open_world_rock_topology_data.cpp                                     */
/**************************************************************************/
#include "open_world_rock_topology_data.h"

#include "core/object/class_db.h"

#define DATA_SETTER(type, name) void OpenWorldRockTopologyData::set_##name(type p_value) { name = p_value; emit_changed(); }
DATA_SETTER(const PackedVector3Array &, source_points);
DATA_SETTER(const PackedVector3Array &, hull_vertices);
DATA_SETTER(const PackedInt32Array &, hull_indices);
DATA_SETTER(const PackedInt32Array &, face_groups);
DATA_SETTER(real_t, base_plane);
DATA_SETTER(const AABB &, local_bounds);
DATA_SETTER(int, source_seed);
DATA_SETTER(const String &, topology_hash);
#undef DATA_SETTER

int OpenWorldRockTopologyData::get_base_contact_count(real_t p_epsilon) const {
	int count = 0;
	for (const Vector3 &point : hull_vertices) if (Math::abs(point.y - base_plane) <= p_epsilon) count++;
	return count;
}

void OpenWorldRockTopologyData::_bind_methods() {
#define DATA_BIND(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldRockTopologyData::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldRockTopologyData::get_##name)
	DATA_BIND(source_points); DATA_BIND(hull_vertices); DATA_BIND(hull_indices); DATA_BIND(face_groups); DATA_BIND(base_plane); DATA_BIND(local_bounds); DATA_BIND(source_seed); DATA_BIND(topology_hash);
#undef DATA_BIND
	ClassDB::bind_method(D_METHOD("get_triangle_count"), &OpenWorldRockTopologyData::get_triangle_count);
	ClassDB::bind_method(D_METHOD("get_base_contact_count", "epsilon"), &OpenWorldRockTopologyData::get_base_contact_count, DEFVAL(0.0001));
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "source_points"), "set_source_points", "get_source_points");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "hull_vertices"), "set_hull_vertices", "get_hull_vertices");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "hull_indices"), "set_hull_indices", "get_hull_indices");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "face_groups"), "set_face_groups", "get_face_groups");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_plane"), "set_base_plane", "get_base_plane");
	ADD_PROPERTY(PropertyInfo(Variant::AABB, "local_bounds"), "set_local_bounds", "get_local_bounds");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "source_seed"), "set_source_seed", "get_source_seed");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "topology_hash"), "set_topology_hash", "get_topology_hash");
}
