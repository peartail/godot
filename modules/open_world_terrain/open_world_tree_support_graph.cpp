/**************************************************************************/
/*  open_world_tree_support_graph.cpp                                     */
/**************************************************************************/
#include "open_world_tree_support_graph.h"
#include "core/object/class_db.h"

void OpenWorldTreeSupportGraph::set_paths(const Array &p_value) { paths = p_value; emit_changed(); }
void OpenWorldTreeSupportGraph::set_radii(const Array &p_value) { radii = p_value; emit_changed(); }
void OpenWorldTreeSupportGraph::set_parent_paths(const PackedInt32Array &p_value) { parent_paths = p_value; emit_changed(); }
void OpenWorldTreeSupportGraph::set_attachment_ratios(const PackedFloat32Array &p_value) { attachment_ratios = p_value; emit_changed(); }
void OpenWorldTreeSupportGraph::clear() { paths.clear(); radii.clear(); parent_paths.clear(); attachment_ratios.clear(); emit_changed(); }
void OpenWorldTreeSupportGraph::add_path(const PackedVector3Array &p_points, const PackedFloat32Array &p_radii, int p_parent_path, real_t p_attachment_ratio) {
	ERR_FAIL_COND_MSG(p_points.size() != p_radii.size(), "Tree support points and radii must have equal sizes.");
	paths.push_back(p_points); radii.push_back(p_radii); parent_paths.push_back(p_parent_path); attachment_ratios.push_back(CLAMP(p_attachment_ratio, (real_t)0.0, (real_t)1.0)); emit_changed();
}
PackedVector3Array OpenWorldTreeSupportGraph::get_path_points(int p_index) const { ERR_FAIL_INDEX_V(p_index, paths.size(), PackedVector3Array()); return paths[p_index]; }
PackedFloat32Array OpenWorldTreeSupportGraph::get_path_radii(int p_index) const { ERR_FAIL_INDEX_V(p_index, radii.size(), PackedFloat32Array()); return radii[p_index]; }
void OpenWorldTreeSupportGraph::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_paths", "paths"), &OpenWorldTreeSupportGraph::set_paths); ClassDB::bind_method(D_METHOD("get_paths"), &OpenWorldTreeSupportGraph::get_paths);
	ClassDB::bind_method(D_METHOD("set_radii", "radii"), &OpenWorldTreeSupportGraph::set_radii); ClassDB::bind_method(D_METHOD("get_radii"), &OpenWorldTreeSupportGraph::get_radii);
	ClassDB::bind_method(D_METHOD("set_parent_paths", "parents"), &OpenWorldTreeSupportGraph::set_parent_paths); ClassDB::bind_method(D_METHOD("get_parent_paths"), &OpenWorldTreeSupportGraph::get_parent_paths);
	ClassDB::bind_method(D_METHOD("set_attachment_ratios", "ratios"), &OpenWorldTreeSupportGraph::set_attachment_ratios); ClassDB::bind_method(D_METHOD("get_attachment_ratios"), &OpenWorldTreeSupportGraph::get_attachment_ratios);
	ClassDB::bind_method(D_METHOD("clear"), &OpenWorldTreeSupportGraph::clear);
	ClassDB::bind_method(D_METHOD("add_path", "points", "radii", "parent_path", "attachment_ratio"), &OpenWorldTreeSupportGraph::add_path, DEFVAL(-1), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("get_path_count"), &OpenWorldTreeSupportGraph::get_path_count); ClassDB::bind_method(D_METHOD("get_path_points", "index"), &OpenWorldTreeSupportGraph::get_path_points); ClassDB::bind_method(D_METHOD("get_path_radii", "index"), &OpenWorldTreeSupportGraph::get_path_radii);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "paths", PROPERTY_HINT_ARRAY_TYPE, "PackedVector3Array"), "set_paths", "get_paths"); ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "radii", PROPERTY_HINT_ARRAY_TYPE, "PackedFloat32Array"), "set_radii", "get_radii");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "parent_paths"), "set_parent_paths", "get_parent_paths"); ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "attachment_ratios"), "set_attachment_ratios", "get_attachment_ratios");
}
