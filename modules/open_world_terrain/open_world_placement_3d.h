/**************************************************************************/
/*  open_world_placement_3d.h                                       */
/**************************************************************************/

#pragma once

#include "open_world_placement_preset.h"
#include "open_world_placement_data.h"

#include "core/math/random_pcg.h"
#include "scene/3d/node_3d.h"

class SimpleTerrain3D;

class OpenWorldPlacement3D : public Node3D {
	GDCLASS(OpenWorldPlacement3D, Node3D);

	struct Candidate {
		Ref<OpenWorldPlacementEntry> entry;
		String stable_id;
		Vector3 position;
		Vector3 rotation;
		Vector3 scale;
		Vector3 normal;
		int seed = 0;
		real_t spacing_radius = 0.0;
	};

	NodePath terrain_path;
	NodePath output_parent_path;
	Ref<OpenWorldPlacementPreset> active_preset;
	Ref<OpenWorldPlacementData> placement_data;
	int default_seed = 1207;
	Dictionary generation_report;

	SimpleTerrain3D *_resolve_terrain() const;
	Node3D *_resolve_output_parent() const;
	Node3D *_get_or_create_generated_root();
	Node3D *_get_generated_root() const;
	bool _contains_world_xz(const Vector3 &p_position, const Vector3 &p_center, const Ref<OpenWorldPlacementPreset> &p_preset) const;
	Vector2 _sample_footprint(RandomPCG &r_random, const Ref<OpenWorldPlacementPreset> &p_preset) const;
	Ref<OpenWorldPlacementEntry> _select_entry(RandomPCG &r_random, const Ref<OpenWorldPlacementPreset> &p_preset) const;
	Dictionary _solve_candidates(const Vector3 &p_world_position, const Ref<OpenWorldPlacementPreset> &p_preset, int p_seed, Vector<Candidate> &r_candidates, Vector<int> &r_replaced_indices) const;
	Node3D *_instantiate_candidate(const Candidate &p_candidate) const;
	Node3D *_instantiate_record(int p_index) const;
	void _delete_generated_by_id(const String &p_stable_id);
	void _assign_scene_owner(Node *p_node, Node3D *p_output_parent) const;

protected:
	static void _bind_methods();

public:
	void set_terrain_path(const NodePath &p_value);
	NodePath get_terrain_path() const { return terrain_path; }
	void set_output_parent_path(const NodePath &p_value);
	NodePath get_output_parent_path() const { return output_parent_path; }
	void set_active_preset(const Ref<OpenWorldPlacementPreset> &p_value);
	Ref<OpenWorldPlacementPreset> get_active_preset() const { return active_preset; }
	void set_placement_data(const Ref<OpenWorldPlacementData> &p_value);
	Ref<OpenWorldPlacementData> get_placement_data() const { return placement_data; }
	void set_default_seed(int p_value);
	int get_default_seed() const { return default_seed; }

	Dictionary preview_placement(const Vector3 &p_world_position, const Ref<OpenWorldPlacementPreset> &p_preset = Ref<OpenWorldPlacementPreset>(), int p_seed = 0) const;
	Dictionary apply_placement(const Vector3 &p_world_position, const Ref<OpenWorldPlacementPreset> &p_preset = Ref<OpenWorldPlacementPreset>(), int p_seed = 0);
	Dictionary rebuild_generated();
	Dictionary clear_generated();
	Dictionary clear_placements();
	Dictionary get_generation_report() const { return generation_report; }
};

