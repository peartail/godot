/**************************************************************************/
/*  simple_world_placement_3d.cpp                                         */
/**************************************************************************/

#include "simple_world_placement_3d.h"

#include "simple_world_object_profile.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

void SimpleWorldPlacement3D::_placement_source_changed() {
	if (rebuilding) {
		return;
	}
	if (!is_inside_tree()) {
		rebuild_pending = true;
		return;
	}
	rebuild_placements();
}

void SimpleWorldPlacement3D::_clear_generated_children() {
	while (get_child_count() > 0) {
		Node *child = get_child(0);
		remove_child(child);
		memdelete(child);
	}
}

void SimpleWorldPlacement3D::_rebuild_instances() {
	_clear_generated_children();

	if (world_placement_library.is_null() || world_placement_data.is_null()) {
		return;
	}

	const PackedStringArray profile_ids = world_placement_data->get_profile_ids();
	const PackedVector3Array positions = world_placement_data->get_positions();
	const PackedVector3Array rotations = world_placement_data->get_rotations();
	const PackedVector3Array scales = world_placement_data->get_scales();
	const int placement_count = MIN(profile_ids.size(), MIN(positions.size(), MIN(rotations.size(), scales.size())));

	Node *owner_node = get_owner();

	for (int i = 0; i < placement_count; i++) {
		Ref<SimpleWorldObjectProfile> profile = world_placement_library->get_profile_by_id(profile_ids[i]);
		if (profile.is_null() || profile->get_scene().is_null() || !profile->get_scene()->can_instantiate()) {
			continue;
		}

		Node *instance = profile->get_scene()->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
		Node3D *instance_3d = Object::cast_to<Node3D>(instance);
		if (instance_3d == nullptr) {
			if (instance != nullptr) {
				memdelete(instance);
			}
			continue;
		}

		Transform3D transform;
		transform.origin = get_global_transform().affine_inverse().xform(positions[i]);
		transform.basis = Basis::from_euler(rotations[i]);
		transform.basis.scale(scales[i]);
		instance_3d->set_transform(transform);
		instance_3d->set_name(profile->get_id().is_empty() ? String("WorldObject") : profile->get_id());
		add_child(instance_3d, false);
		if (owner_node != nullptr) {
			instance_3d->set_owner(owner_node);
		}
	}
}

void SimpleWorldPlacement3D::rebuild_placements() {
	if (!is_inside_tree()) {
		rebuild_pending = true;
		return;
	}

	rebuilding = true;
	rebuild_pending = false;
	_rebuild_instances();
	rebuilding = false;
}

void SimpleWorldPlacement3D::set_world_placement_library(const Ref<SimpleWorldPlacementLibrary> &p_library) {
	if (world_placement_library == p_library) {
		return;
	}
	if (world_placement_library.is_valid()) {
		world_placement_library->disconnect_changed(callable_mp(this, &SimpleWorldPlacement3D::_placement_source_changed));
	}
	world_placement_library = p_library;
	if (world_placement_library.is_valid()) {
		world_placement_library->connect_changed(callable_mp(this, &SimpleWorldPlacement3D::_placement_source_changed));
	}
	notify_property_list_changed();
	_placement_source_changed();
}

void SimpleWorldPlacement3D::set_world_placement_data(const Ref<SimpleWorldPlacementData> &p_data) {
	if (world_placement_data == p_data) {
		return;
	}
	if (world_placement_data.is_valid()) {
		world_placement_data->disconnect_changed(callable_mp(this, &SimpleWorldPlacement3D::_placement_source_changed));
	}
	world_placement_data = p_data;
	if (world_placement_data.is_valid()) {
		world_placement_data->connect_changed(callable_mp(this, &SimpleWorldPlacement3D::_placement_source_changed));
	}
	notify_property_list_changed();
	_placement_source_changed();
}

void SimpleWorldPlacement3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			rebuild_placements();
		} break;
	}
}

void SimpleWorldPlacement3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_world_placement_library", "library"), &SimpleWorldPlacement3D::set_world_placement_library);
	ClassDB::bind_method(D_METHOD("get_world_placement_library"), &SimpleWorldPlacement3D::get_world_placement_library);
	ClassDB::bind_method(D_METHOD("set_world_placement_data", "data"), &SimpleWorldPlacement3D::set_world_placement_data);
	ClassDB::bind_method(D_METHOD("get_world_placement_data"), &SimpleWorldPlacement3D::get_world_placement_data);
	ClassDB::bind_method(D_METHOD("rebuild_placements"), &SimpleWorldPlacement3D::rebuild_placements);

	ADD_GROUP("World Placement", "world_placement_");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_placement_library", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementLibrary"), "set_world_placement_library", "get_world_placement_library");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_placement_data", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementData"), "set_world_placement_data", "get_world_placement_data");
}
