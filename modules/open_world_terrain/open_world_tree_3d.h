/**************************************************************************/
/*  open_world_tree_3d.h                                                  */
/**************************************************************************/

#pragma once

#include "open_world_tree_placement_data.h"
#include "open_world_tree_species.h"

#include "scene/3d/node_3d.h"
#include "scene/resources/multimesh.h"

class Camera3D;

class OpenWorldTree3D : public Node3D {
	GDCLASS(OpenWorldTree3D, Node3D);

	struct RenderBucket {
		int variant_index = -1;
		int lod_index = -1;
		Ref<MultiMesh> multimesh;
		RID instance;
	};

	Ref<OpenWorldTreeSpecies> species;
	Ref<OpenWorldTreePlacementData> placement_data;
	NodePath camera_path;
	real_t lod_update_interval = 0.25;
	real_t lod_update_time = 0.0;
	uint32_t visibility_layer = 1;
	bool rendering_enabled = true;

	Vector<RenderBucket> render_buckets;
	Vector<int> cached_bucket_indices;

	void _source_changed();
	void _clear_render_buckets();
	void _rebuild_render_buckets();
	void _sync_bucket_scenarios();
	void _sync_bucket_transforms();
	void _sync_bucket_visibility();
	void _update_lods(bool p_force);
	Camera3D *_resolve_camera() const;
	int _resolve_variant_index(int p_instance_index, int p_requested_index, int p_seed) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	OpenWorldTree3D();

	void set_species(const Ref<OpenWorldTreeSpecies> &p_species);
	Ref<OpenWorldTreeSpecies> get_species() const { return species; }
	void set_placement_data(const Ref<OpenWorldTreePlacementData> &p_data);
	Ref<OpenWorldTreePlacementData> get_placement_data() const { return placement_data; }

	void set_camera_path(const NodePath &p_path);
	NodePath get_camera_path() const { return camera_path; }
	void set_lod_update_interval(real_t p_interval);
	real_t get_lod_update_interval() const { return lod_update_interval; }
	void set_visibility_layer(uint32_t p_layer);
	uint32_t get_visibility_layer() const { return visibility_layer; }
	void set_rendering_enabled(bool p_enabled);
	bool is_rendering_enabled() const { return rendering_enabled; }

	void rebuild_tree_instances();
	void force_lod_update();
	int get_rendered_instance_count() const;
	int get_closest_instance_id(const Vector3 &p_global_position, real_t p_max_distance) const;
	Transform3D get_instance_global_transform(int p_instance_id) const;
	void set_tree_enabled(int p_instance_id, bool p_enabled);

	~OpenWorldTree3D();
};
