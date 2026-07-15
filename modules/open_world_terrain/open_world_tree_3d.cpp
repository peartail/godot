/**************************************************************************/
/*  open_world_tree_3d.cpp                                                */
/**************************************************************************/

#include "open_world_tree_3d.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/world_3d.h"
#include "servers/rendering/rendering_server.h"

void OpenWorldTree3D::_source_changed() {
	cached_bucket_indices.clear();
	update_gizmos();
	if (!is_inside_tree()) {
		return;
	}
	_rebuild_render_buckets();
	_update_lods(true);
}

void OpenWorldTree3D::_clear_render_buckets() {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < render_buckets.size(); i++) {
		if (render_buckets[i].instance.is_valid()) {
			rs->free_rid(render_buckets[i].instance);
			render_buckets.write[i].instance = RID();
		}
		render_buckets.write[i].multimesh.unref();
	}
	render_buckets.clear();
}

void OpenWorldTree3D::_rebuild_render_buckets() {
	_clear_render_buckets();
	cached_bucket_indices.clear();

	if (species.is_null()) {
		return;
	}

	const int variant_count = species->get_variant_count();
	render_buckets.resize(variant_count * 3);
	RenderingServer *rs = RenderingServer::get_singleton();

	for (int variant_index = 0; variant_index < variant_count; variant_index++) {
		Ref<OpenWorldTreeVariant> variant = species->get_variant(variant_index);
		if (variant.is_null()) {
			continue;
		}
		for (int lod_index = 0; lod_index < 3; lod_index++) {
			const int bucket_index = variant_index * 3 + lod_index;
			RenderBucket &bucket = render_buckets.write[bucket_index];
			bucket.variant_index = variant_index;
			bucket.lod_index = lod_index;
			Ref<Mesh> mesh = variant->get_lod_mesh(lod_index);
			if (mesh.is_null()) {
				continue;
			}

			bucket.multimesh.instantiate();
			bucket.multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
			bucket.multimesh->set_use_colors(true);
			bucket.multimesh->set_use_custom_data(true);
			bucket.multimesh->set_mesh(mesh);
			bucket.multimesh->set_instance_count(0);

			bucket.instance = rs->instance_create();
			rs->instance_set_base(bucket.instance, bucket.multimesh->get_rid());
			rs->instance_set_layer_mask(bucket.instance, visibility_layer);
			rs->instance_geometry_set_material_override(bucket.instance, variant->get_material_override().is_valid() ? variant->get_material_override()->get_rid() : RID());
		}
	}

	_sync_bucket_scenarios();
	_sync_bucket_transforms();
	_sync_bucket_visibility();
}

void OpenWorldTree3D::_sync_bucket_scenarios() {
	RID scenario;
	if (is_inside_world() && rendering_enabled && get_world_3d().is_valid()) {
		scenario = get_world_3d()->get_scenario();
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < render_buckets.size(); i++) {
		if (render_buckets[i].instance.is_valid()) {
			rs->instance_set_scenario(render_buckets[i].instance, scenario);
		}
	}
}

void OpenWorldTree3D::_sync_bucket_transforms() {
	const Transform3D global_transform = is_inside_tree() ? get_global_transform() : Transform3D();
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < render_buckets.size(); i++) {
		if (render_buckets[i].instance.is_valid()) {
			rs->instance_set_transform(render_buckets[i].instance, global_transform);
		}
	}
}

void OpenWorldTree3D::_sync_bucket_visibility() {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < render_buckets.size(); i++) {
		if (render_buckets[i].instance.is_valid()) {
			rs->instance_set_visible(render_buckets[i].instance, rendering_enabled);
		}
	}
}

Camera3D *OpenWorldTree3D::_resolve_camera() const {
	if (!camera_path.is_empty()) {
		Node *camera_node = get_node_or_null(camera_path);
		Camera3D *camera = Object::cast_to<Camera3D>(camera_node);
		if (camera != nullptr) {
			return camera;
		}
	}
	Viewport *viewport = get_viewport();
	return viewport != nullptr ? viewport->get_camera_3d() : nullptr;
}

int OpenWorldTree3D::_resolve_variant_index(int p_instance_index, int p_requested_index, int p_seed) const {
	const int variant_count = species.is_valid() ? species->get_variant_count() : 0;
	if (p_requested_index >= 0 && p_requested_index < variant_count) {
		return p_requested_index;
	}
	const int seed = p_seed != 0 ? p_seed : p_instance_index + 1;
	return species.is_valid() ? species->get_variant_index_for_seed(seed) : -1;
}

void OpenWorldTree3D::_update_lods(bool p_force) {
	if (!rendering_enabled || species.is_null() || placement_data.is_null()) {
		return;
	}

	const int expected_bucket_count = species->get_variant_count() * 3;
	if (render_buckets.size() != expected_bucket_count) {
		_rebuild_render_buckets();
	}

	const PackedVector3Array positions = placement_data->get_positions();
	const PackedVector3Array rotations = placement_data->get_rotations();
	const PackedVector3Array scales = placement_data->get_scales();
	const PackedInt32Array seeds = placement_data->get_seeds();
	const PackedInt32Array variant_indices = placement_data->get_variant_indices();
	const PackedByteArray enabled = placement_data->get_enabled();
	const PackedColorArray colors = placement_data->get_colors();
	const PackedColorArray custom_data = placement_data->get_custom_data();
	const int instance_count = positions.size();

	Camera3D *camera = _resolve_camera();
	Vector3 local_camera_position;
	const bool has_camera = camera != nullptr;
	if (has_camera) {
		local_camera_position = get_global_transform().affine_inverse().xform(camera->get_global_position());
	}

	Vector<int> new_bucket_indices;
	new_bucket_indices.resize(instance_count);
	for (int i = 0; i < instance_count; i++) {
		new_bucket_indices.write[i] = -1;
		if (i >= enabled.size() || enabled[i] == 0) {
			continue;
		}

		const int requested_variant = i < variant_indices.size() ? variant_indices[i] : -1;
		const int seed = i < seeds.size() ? seeds[i] : 0;
		const int variant_index = _resolve_variant_index(i, requested_variant, seed);
		if (variant_index < 0) {
			continue;
		}
		Ref<OpenWorldTreeVariant> variant = species->get_variant(variant_index);
		if (variant.is_null()) {
			continue;
		}
		const real_t distance = has_camera ? positions[i].distance_to(local_camera_position) : 0.0;
		const int lod_index = variant->get_lod_index_for_distance(distance);
		if (lod_index >= 0) {
			new_bucket_indices.write[i] = variant_index * 3 + lod_index;
		}
	}

	bool assignments_changed = p_force || cached_bucket_indices.size() != new_bucket_indices.size();
	if (!assignments_changed) {
		for (int i = 0; i < new_bucket_indices.size(); i++) {
			if (cached_bucket_indices[i] != new_bucket_indices[i]) {
				assignments_changed = true;
				break;
			}
		}
	}
	if (!assignments_changed) {
		return;
	}
	cached_bucket_indices = new_bucket_indices;

	Vector<Vector<int>> bucket_members;
	bucket_members.resize(render_buckets.size());
	for (int i = 0; i < instance_count; i++) {
		const int bucket_index = new_bucket_indices[i];
		if (bucket_index >= 0 && bucket_index < bucket_members.size()) {
			bucket_members.write[bucket_index].push_back(i);
		}
	}

	for (int bucket_index = 0; bucket_index < render_buckets.size(); bucket_index++) {
		RenderBucket &bucket = render_buckets.write[bucket_index];
		if (bucket.multimesh.is_null()) {
			continue;
		}
		const Vector<int> &members = bucket_members[bucket_index];
		bucket.multimesh->set_instance_count(members.size());
		for (int member_index = 0; member_index < members.size(); member_index++) {
			const int source_index = members[member_index];
			Transform3D transform(Basis::from_euler(source_index < rotations.size() ? rotations[source_index] : Vector3()), positions[source_index]);
			transform.basis.scale(source_index < scales.size() ? scales[source_index] : Vector3(1.0, 1.0, 1.0));
			bucket.multimesh->set_instance_transform(member_index, transform);
			bucket.multimesh->set_instance_color(member_index, source_index < colors.size() ? colors[source_index] : Color(1.0, 1.0, 1.0, 1.0));
			bucket.multimesh->set_instance_custom_data(member_index, source_index < custom_data.size() ? custom_data[source_index] : Color());
		}
	}
}

void OpenWorldTree3D::set_species(const Ref<OpenWorldTreeSpecies> &p_species) {
	if (species == p_species) {
		return;
	}
	if (species.is_valid()) {
		species->disconnect_changed(callable_mp(this, &OpenWorldTree3D::_source_changed));
	}
	species = p_species;
	if (species.is_valid()) {
		species->connect_changed(callable_mp(this, &OpenWorldTree3D::_source_changed));
	}
	_source_changed();
}

void OpenWorldTree3D::set_placement_data(const Ref<OpenWorldTreePlacementData> &p_data) {
	if (placement_data == p_data) {
		return;
	}
	if (placement_data.is_valid()) {
		placement_data->disconnect_changed(callable_mp(this, &OpenWorldTree3D::_source_changed));
	}
	placement_data = p_data;
	if (placement_data.is_valid()) {
		placement_data->connect_changed(callable_mp(this, &OpenWorldTree3D::_source_changed));
	}
	_source_changed();
}

void OpenWorldTree3D::set_camera_path(const NodePath &p_path) {
	if (camera_path == p_path) {
		return;
	}
	camera_path = p_path;
	force_lod_update();
}

void OpenWorldTree3D::set_lod_update_interval(real_t p_interval) {
	lod_update_interval = MAX((real_t)0.0, p_interval);
}

void OpenWorldTree3D::set_visibility_layer(uint32_t p_layer) {
	if (visibility_layer == p_layer) {
		return;
	}
	visibility_layer = p_layer;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < render_buckets.size(); i++) {
		if (render_buckets[i].instance.is_valid()) {
			rs->instance_set_layer_mask(render_buckets[i].instance, visibility_layer);
		}
	}
}

void OpenWorldTree3D::set_rendering_enabled(bool p_enabled) {
	if (rendering_enabled == p_enabled) {
		return;
	}
	rendering_enabled = p_enabled;
	_sync_bucket_scenarios();
	_sync_bucket_visibility();
	if (rendering_enabled) {
		force_lod_update();
	}
}

void OpenWorldTree3D::rebuild_tree_instances() {
	_rebuild_render_buckets();
	_update_lods(true);
}

void OpenWorldTree3D::force_lod_update() {
	lod_update_time = 0.0;
	cached_bucket_indices.clear();
	if (is_inside_tree()) {
		_update_lods(true);
	}
}

int OpenWorldTree3D::get_rendered_instance_count() const {
	int count = 0;
	for (int i = 0; i < render_buckets.size(); i++) {
		if (render_buckets[i].multimesh.is_valid()) {
			count += render_buckets[i].multimesh->get_instance_count();
		}
	}
	return count;
}

int OpenWorldTree3D::get_closest_instance_id(const Vector3 &p_global_position, real_t p_max_distance) const {
	if (placement_data.is_null() || p_max_distance < 0.0) {
		return -1;
	}
	const Vector3 local_position = get_global_transform().affine_inverse().xform(p_global_position);
	const PackedVector3Array positions = placement_data->get_positions();
	const PackedInt32Array ids = placement_data->get_instance_ids();
	const PackedByteArray enabled = placement_data->get_enabled();
	real_t closest_distance_squared = p_max_distance * p_max_distance;
	int closest_id = -1;
	for (int i = 0; i < positions.size(); i++) {
		if (i >= enabled.size() || enabled[i] == 0 || i >= ids.size()) {
			continue;
		}
		const real_t distance_squared = positions[i].distance_squared_to(local_position);
		if (distance_squared <= closest_distance_squared) {
			closest_distance_squared = distance_squared;
			closest_id = ids[i];
		}
	}
	return closest_id;
}

Transform3D OpenWorldTree3D::get_instance_global_transform(int p_instance_id) const {
	ERR_FAIL_COND_V(placement_data.is_null(), Transform3D());
	const int index = placement_data->find_index_by_id(p_instance_id);
	ERR_FAIL_COND_V_MSG(index < 0, Transform3D(), "Unknown tree instance ID.");
	return get_global_transform() * placement_data->get_instance_transform(index);
}

void OpenWorldTree3D::set_tree_enabled(int p_instance_id, bool p_enabled) {
	ERR_FAIL_COND(placement_data.is_null());
	const int index = placement_data->find_index_by_id(p_instance_id);
	ERR_FAIL_COND_MSG(index < 0, "Unknown tree instance ID.");
	placement_data->set_instance_enabled(index, p_enabled);
}

void OpenWorldTree3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_rebuild_render_buckets();
		} break;
		case NOTIFICATION_ENTER_WORLD: {
			_sync_bucket_scenarios();
			_update_lods(true);
		} break;
		case NOTIFICATION_EXIT_WORLD: {
			RenderingServer *rs = RenderingServer::get_singleton();
			for (int i = 0; i < render_buckets.size(); i++) {
				if (render_buckets[i].instance.is_valid()) {
					rs->instance_set_scenario(render_buckets[i].instance, RID());
				}
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_clear_render_buckets();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			_sync_bucket_transforms();
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (!rendering_enabled) {
				break;
			}
			lod_update_time += get_process_delta_time();
			if (lod_update_interval <= 0.0 || lod_update_time >= lod_update_interval) {
				lod_update_time = 0.0;
				_update_lods(false);
			}
		} break;
	}
}

OpenWorldTree3D::OpenWorldTree3D() {
	set_notify_transform(true);
	set_process_internal(true);
}

OpenWorldTree3D::~OpenWorldTree3D() {
	if (species.is_valid()) {
		species->disconnect_changed(callable_mp(this, &OpenWorldTree3D::_source_changed));
	}
	if (placement_data.is_valid()) {
		placement_data->disconnect_changed(callable_mp(this, &OpenWorldTree3D::_source_changed));
	}
	_clear_render_buckets();
}

void OpenWorldTree3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_species", "species"), &OpenWorldTree3D::set_species);
	ClassDB::bind_method(D_METHOD("get_species"), &OpenWorldTree3D::get_species);
	ClassDB::bind_method(D_METHOD("set_placement_data", "placement_data"), &OpenWorldTree3D::set_placement_data);
	ClassDB::bind_method(D_METHOD("get_placement_data"), &OpenWorldTree3D::get_placement_data);
	ClassDB::bind_method(D_METHOD("set_camera_path", "camera_path"), &OpenWorldTree3D::set_camera_path);
	ClassDB::bind_method(D_METHOD("get_camera_path"), &OpenWorldTree3D::get_camera_path);
	ClassDB::bind_method(D_METHOD("set_lod_update_interval", "interval"), &OpenWorldTree3D::set_lod_update_interval);
	ClassDB::bind_method(D_METHOD("get_lod_update_interval"), &OpenWorldTree3D::get_lod_update_interval);
	ClassDB::bind_method(D_METHOD("set_visibility_layer", "layer"), &OpenWorldTree3D::set_visibility_layer);
	ClassDB::bind_method(D_METHOD("get_visibility_layer"), &OpenWorldTree3D::get_visibility_layer);
	ClassDB::bind_method(D_METHOD("set_rendering_enabled", "enabled"), &OpenWorldTree3D::set_rendering_enabled);
	ClassDB::bind_method(D_METHOD("is_rendering_enabled"), &OpenWorldTree3D::is_rendering_enabled);
	ClassDB::bind_method(D_METHOD("rebuild_tree_instances"), &OpenWorldTree3D::rebuild_tree_instances);
	ClassDB::bind_method(D_METHOD("force_lod_update"), &OpenWorldTree3D::force_lod_update);
	ClassDB::bind_method(D_METHOD("get_rendered_instance_count"), &OpenWorldTree3D::get_rendered_instance_count);
	ClassDB::bind_method(D_METHOD("get_closest_instance_id", "global_position", "max_distance"), &OpenWorldTree3D::get_closest_instance_id);
	ClassDB::bind_method(D_METHOD("get_instance_global_transform", "instance_id"), &OpenWorldTree3D::get_instance_global_transform);
	ClassDB::bind_method(D_METHOD("set_tree_enabled", "instance_id", "enabled"), &OpenWorldTree3D::set_tree_enabled);

	ADD_GROUP("Tree Sources", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "species", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldTreeSpecies"), "set_species", "get_species");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "placement_data", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldTreePlacementData"), "set_placement_data", "get_placement_data");
	ADD_GROUP("LOD", "");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "camera_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera3D"), "set_camera_path", "get_camera_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod_update_interval", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,suffix:s"), "set_lod_update_interval", "get_lod_update_interval");
	ADD_GROUP("Rendering", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rendering_enabled"), "set_rendering_enabled", "is_rendering_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "visibility_layer", PROPERTY_HINT_LAYERS_3D_RENDER), "set_visibility_layer", "get_visibility_layer");
}
