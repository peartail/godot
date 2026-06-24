/**************************************************************************/
/*  simple_navigation_blocker_3d.h                                         */
/**************************************************************************/

#pragma once

#include "core/object/object_id.h"
#include "scene/3d/node_3d.h"

class SimpleTerrain3D;

class SimpleNavigationBlocker3D : public Node3D {
	GDCLASS(SimpleNavigationBlocker3D, Node3D);

public:
	enum ShapeSource {
		SHAPE_RADIUS,
		SHAPE_SCENE_COLLISION,
		SHAPE_MESH_AABB,
	};

private:
	NodePath terrain_path;
	bool auto_register = true;
	bool bake_on_register = false;
	bool bake_on_unregister = false;
	ShapeSource shape_source = SHAPE_SCENE_COLLISION;
	real_t radius = 0.5;
	real_t height = 2.0;
	bool carve = false;
	bool registered = false;
	ObjectID registered_terrain_id;

	SimpleTerrain3D *_find_terrain() const;
	SimpleTerrain3D *_find_terrain_in_tree(Node *p_node) const;
	void _register_with_terrain();
	void _unregister_from_terrain();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_terrain_path(const NodePath &p_path);
	NodePath get_terrain_path() const { return terrain_path; }

	void set_auto_register(bool p_enabled);
	bool is_auto_registering() const { return auto_register; }

	void set_bake_on_register(bool p_enabled);
	bool is_baking_on_register() const { return bake_on_register; }

	void set_bake_on_unregister(bool p_enabled);
	bool is_baking_on_unregister() const { return bake_on_unregister; }

	void set_shape_source(ShapeSource p_source);
	ShapeSource get_shape_source() const { return shape_source; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_height(real_t p_height);
	real_t get_height() const { return height; }

	void set_carve(bool p_carve);
	bool get_carve() const { return carve; }

	void register_to_terrain(SimpleTerrain3D *p_terrain);
	void unregister_from_terrain();
	bool is_registered() const { return registered; }
};

VARIANT_ENUM_CAST(SimpleNavigationBlocker3D::ShapeSource);
