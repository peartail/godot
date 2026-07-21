/**************************************************************************/
/*  open_world_rock_generation_request.h                                  */
/**************************************************************************/
#pragma once

#include "open_world_rock_generation_profile.h"

class OpenWorldRockGenerationRequest : public Resource {
	GDCLASS(OpenWorldRockGenerationRequest, Resource);
	RES_BASE_EXTENSION("owrockrequest");

public:
	enum RockMode { MODE_BOULDER, MODE_SLAB, MODE_SHARD };

private:
	RockMode mode = MODE_BOULDER;
	int seed = 1207;
	Ref<OpenWorldRockGenerationProfile> profile;
	Vector3 size = Vector3(2.0, 1.5, 2.0);
	Vector3 primary_axis = Vector3::UP;
	PackedVector3Array explicit_points;
	String stable_id;
	PackedStringArray tags;
	void _profile_changed();

protected:
	static void _bind_methods();

public:
	void set_mode(RockMode p_value); RockMode get_mode() const { return mode; }
	void set_seed(int p_value); int get_seed() const { return seed; }
	void set_profile(const Ref<OpenWorldRockGenerationProfile> &p_value); Ref<OpenWorldRockGenerationProfile> get_profile() const { return profile; }
	void set_size(const Vector3 &p_value); Vector3 get_size() const { return size; }
	void set_primary_axis(const Vector3 &p_value); Vector3 get_primary_axis() const { return primary_axis; }
	void set_explicit_points(const PackedVector3Array &p_value); PackedVector3Array get_explicit_points() const { return explicit_points; }
	void set_stable_id(const String &p_value); String get_stable_id() const { return stable_id; }
	void set_tags(const PackedStringArray &p_value); PackedStringArray get_tags() const { return tags; }
	~OpenWorldRockGenerationRequest();
};
VARIANT_ENUM_CAST(OpenWorldRockGenerationRequest::RockMode);
