/**************************************************************************/
/*  simple_navigation_blocker_3d.cpp                                       */
/**************************************************************************/

#include "simple_navigation_blocker_3d.h"

#include "simple_terrain_3d.h"

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "scene/main/scene_tree.h"

SimpleTerrain3D *SimpleNavigationBlocker3D::_find_terrain_in_tree(Node *p_node) const {
	if (p_node == nullptr) {
		return nullptr;
	}

	SimpleTerrain3D *terrain = Object::cast_to<SimpleTerrain3D>(p_node);
	if (terrain != nullptr) {
		return terrain;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		terrain = _find_terrain_in_tree(p_node->get_child(i));
		if (terrain != nullptr) {
			return terrain;
		}
	}

	return nullptr;
}

SimpleTerrain3D *SimpleNavigationBlocker3D::_find_terrain() const {
	if (!terrain_path.is_empty() && has_node(terrain_path)) {
		return Object::cast_to<SimpleTerrain3D>(get_node(terrain_path));
	}

	Node *node = get_parent();
	while (node != nullptr) {
		SimpleTerrain3D *terrain = Object::cast_to<SimpleTerrain3D>(node);
		if (terrain != nullptr) {
			return terrain;
		}
		node = node->get_parent();
	}

	SceneTree *tree = get_tree();
	if (tree != nullptr && tree->get_current_scene() != nullptr) {
		return _find_terrain_in_tree(tree->get_current_scene());
	}

	return nullptr;
}

void SimpleNavigationBlocker3D::_register_with_terrain() {
	if (!auto_register || registered) {
		return;
	}

	SimpleTerrain3D *terrain = _find_terrain();
	if (terrain != nullptr) {
		register_to_terrain(terrain);
	}
}

void SimpleNavigationBlocker3D::_unregister_from_terrain() {
	if (registered) {
		unregister_from_terrain();
	}
}

void SimpleNavigationBlocker3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_register_with_terrain();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_unregister_from_terrain();
		} break;
	}
}

void SimpleNavigationBlocker3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_terrain_path", "path"), &SimpleNavigationBlocker3D::set_terrain_path);
	ClassDB::bind_method(D_METHOD("get_terrain_path"), &SimpleNavigationBlocker3D::get_terrain_path);
	ClassDB::bind_method(D_METHOD("set_auto_register", "enabled"), &SimpleNavigationBlocker3D::set_auto_register);
	ClassDB::bind_method(D_METHOD("is_auto_registering"), &SimpleNavigationBlocker3D::is_auto_registering);
	ClassDB::bind_method(D_METHOD("set_bake_on_register", "enabled"), &SimpleNavigationBlocker3D::set_bake_on_register);
	ClassDB::bind_method(D_METHOD("is_baking_on_register"), &SimpleNavigationBlocker3D::is_baking_on_register);
	ClassDB::bind_method(D_METHOD("set_bake_on_unregister", "enabled"), &SimpleNavigationBlocker3D::set_bake_on_unregister);
	ClassDB::bind_method(D_METHOD("is_baking_on_unregister"), &SimpleNavigationBlocker3D::is_baking_on_unregister);
	ClassDB::bind_method(D_METHOD("set_shape_source", "source"), &SimpleNavigationBlocker3D::set_shape_source);
	ClassDB::bind_method(D_METHOD("get_shape_source"), &SimpleNavigationBlocker3D::get_shape_source);
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &SimpleNavigationBlocker3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &SimpleNavigationBlocker3D::get_radius);
	ClassDB::bind_method(D_METHOD("set_height", "height"), &SimpleNavigationBlocker3D::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &SimpleNavigationBlocker3D::get_height);
	ClassDB::bind_method(D_METHOD("set_carve", "carve"), &SimpleNavigationBlocker3D::set_carve);
	ClassDB::bind_method(D_METHOD("get_carve"), &SimpleNavigationBlocker3D::get_carve);
	ClassDB::bind_method(D_METHOD("register_to_terrain", "terrain"), &SimpleNavigationBlocker3D::register_to_terrain);
	ClassDB::bind_method(D_METHOD("unregister_from_terrain"), &SimpleNavigationBlocker3D::unregister_from_terrain);
	ClassDB::bind_method(D_METHOD("is_registered"), &SimpleNavigationBlocker3D::is_registered);

	BIND_ENUM_CONSTANT(SHAPE_RADIUS);
	BIND_ENUM_CONSTANT(SHAPE_SCENE_COLLISION);
	BIND_ENUM_CONSTANT(SHAPE_MESH_AABB);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "terrain_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "SimpleTerrain3D"), "set_terrain_path", "get_terrain_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_register"), "set_auto_register", "is_auto_registering");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bake_on_register"), "set_bake_on_register", "is_baking_on_register");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bake_on_unregister"), "set_bake_on_unregister", "is_baking_on_unregister");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "shape_source", PROPERTY_HINT_ENUM, "Radius,Scene Collision,Mesh AABB"), "set_shape_source", "get_shape_source");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "carve"), "set_carve", "get_carve");
}

void SimpleNavigationBlocker3D::set_terrain_path(const NodePath &p_path) {
	if (terrain_path == p_path) {
		return;
	}
	if (registered) {
		unregister_from_terrain();
	}
	terrain_path = p_path;
	if (is_inside_tree()) {
		_register_with_terrain();
	}
}

void SimpleNavigationBlocker3D::set_auto_register(bool p_enabled) {
	if (auto_register == p_enabled) {
		return;
	}
	auto_register = p_enabled;
	if (!auto_register) {
		_unregister_from_terrain();
	} else if (is_inside_tree()) {
		_register_with_terrain();
	}
}

void SimpleNavigationBlocker3D::set_bake_on_register(bool p_enabled) {
	bake_on_register = p_enabled;
}

void SimpleNavigationBlocker3D::set_bake_on_unregister(bool p_enabled) {
	bake_on_unregister = p_enabled;
}

void SimpleNavigationBlocker3D::set_shape_source(ShapeSource p_source) {
	if (shape_source == p_source) {
		return;
	}
	shape_source = p_source;
	update_gizmos();
	if (registered) {
		SimpleTerrain3D *terrain = ObjectDB::get_instance<SimpleTerrain3D>(registered_terrain_id);
		if (terrain != nullptr) {
			terrain->mark_dynamic_navigation_bake_dirty();
		}
	}
}

void SimpleNavigationBlocker3D::set_radius(real_t p_radius) {
	const real_t new_radius = MAX((real_t)0.0, p_radius);
	if (Math::is_equal_approx(radius, new_radius)) {
		return;
	}
	radius = new_radius;
	update_gizmos();
	if (registered) {
		SimpleTerrain3D *terrain = ObjectDB::get_instance<SimpleTerrain3D>(registered_terrain_id);
		if (terrain != nullptr) {
			terrain->mark_dynamic_navigation_bake_dirty();
		}
	}
}

void SimpleNavigationBlocker3D::set_height(real_t p_height) {
	const real_t new_height = MAX((real_t)0.0, p_height);
	if (Math::is_equal_approx(height, new_height)) {
		return;
	}
	height = new_height;
	update_gizmos();
	if (registered) {
		SimpleTerrain3D *terrain = ObjectDB::get_instance<SimpleTerrain3D>(registered_terrain_id);
		if (terrain != nullptr) {
			terrain->mark_dynamic_navigation_bake_dirty();
		}
	}
}

void SimpleNavigationBlocker3D::set_carve(bool p_carve) {
	if (carve == p_carve) {
		return;
	}
	carve = p_carve;
	if (registered) {
		SimpleTerrain3D *terrain = ObjectDB::get_instance<SimpleTerrain3D>(registered_terrain_id);
		if (terrain != nullptr) {
			terrain->mark_dynamic_navigation_bake_dirty();
		}
	}
}

void SimpleNavigationBlocker3D::register_to_terrain(SimpleTerrain3D *p_terrain) {
	ERR_FAIL_NULL(p_terrain);
	if (registered && registered_terrain_id == p_terrain->get_instance_id()) {
		return;
	}
	if (registered) {
		unregister_from_terrain();
	}
	p_terrain->register_dynamic_navigation_blocker(this);
	registered = true;
	registered_terrain_id = p_terrain->get_instance_id();
	if (bake_on_register) {
		p_terrain->bake_dynamic_navigation(true);
	}
}

void SimpleNavigationBlocker3D::unregister_from_terrain() {
	if (!registered) {
		return;
	}

	SimpleTerrain3D *terrain = ObjectDB::get_instance<SimpleTerrain3D>(registered_terrain_id);
	if (terrain != nullptr) {
		terrain->unregister_dynamic_navigation_blocker(this);
		if (bake_on_unregister) {
			terrain->bake_dynamic_navigation(true);
		}
	}

	registered = false;
	registered_terrain_id = ObjectID();
}
