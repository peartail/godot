/**************************************************************************/
/*  open_world_vine_generation_request.h                                  */
/**************************************************************************/

#pragma once

#include "open_world_vine_generation_profile.h"

#include "core/io/resource.h"

class OpenWorldVineGenerationRequest : public Resource {
	GDCLASS(OpenWorldVineGenerationRequest, Resource);
	RES_BASE_EXTENSION("owvinerequest");

public:
	enum VineMode {
		MODE_CREEPING,
		MODE_CLIMBING,
		MODE_HANGING,
		MODE_TREE_WRAP,
	};

private:
	VineMode mode = MODE_CREEPING;
	int seed = 1207;
	Ref<OpenWorldVineGenerationProfile> profile;
	Vector3 start_position;
	Vector3 start_direction = Vector3::FORWARD;
	Vector3 target_position;
	bool target_enabled = false;
	real_t desired_length = 6.0;
	NodePath support_path;
	int support_stable_id = 0;
	int branch_budget = 2;
	PackedVector3Array explicit_anchors;
	PackedVector3Array explicit_normals;
	void _profile_changed();

protected:
	static void _bind_methods();

public:
	void set_mode(VineMode p_mode);
	VineMode get_mode() const { return mode; }
	void set_seed(int p_seed);
	int get_seed() const { return seed; }
	void set_profile(const Ref<OpenWorldVineGenerationProfile> &p_profile);
	Ref<OpenWorldVineGenerationProfile> get_profile() const { return profile; }
	void set_start_position(const Vector3 &p_value);
	Vector3 get_start_position() const { return start_position; }
	void set_start_direction(const Vector3 &p_value);
	Vector3 get_start_direction() const { return start_direction; }
	void set_target_position(const Vector3 &p_value);
	Vector3 get_target_position() const { return target_position; }
	void set_target_enabled(bool p_enabled);
	bool is_target_enabled() const { return target_enabled; }
	void set_desired_length(real_t p_value);
	real_t get_desired_length() const { return desired_length; }
	void set_support_path(const NodePath &p_path);
	NodePath get_support_path() const { return support_path; }
	void set_support_stable_id(int p_id);
	int get_support_stable_id() const { return support_stable_id; }
	void set_branch_budget(int p_value);
	int get_branch_budget() const { return branch_budget; }
	void set_explicit_anchors(const PackedVector3Array &p_points);
	PackedVector3Array get_explicit_anchors() const { return explicit_anchors; }
	void set_explicit_normals(const PackedVector3Array &p_normals);
	PackedVector3Array get_explicit_normals() const { return explicit_normals; }
	~OpenWorldVineGenerationRequest();
};

VARIANT_ENUM_CAST(OpenWorldVineGenerationRequest::VineMode);
