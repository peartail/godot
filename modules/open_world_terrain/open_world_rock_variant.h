/**************************************************************************/
/*  open_world_rock_variant.h                                             */
/**************************************************************************/
#pragma once

#include "open_world_rock_generation_request.h"
#include "core/io/resource.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/mesh.h"

class OpenWorldRockVariant : public Resource {
	GDCLASS(OpenWorldRockVariant, Resource);
	RES_BASE_EXTENSION("owrockvariant");

public:
	enum GameplayState { STATE_INTACT, STATE_DAMAGED, STATE_DEPLETED };

private:
	String variant_name = "Rock Variant";
	OpenWorldRockGenerationRequest::RockMode source_mode = OpenWorldRockGenerationRequest::MODE_BOULDER;
	int source_seed = 0;
	Ref<Mesh> lod_meshes[3];
	Ref<Shape3D> collision_shape;
	PackedVector3Array collision_points;
	real_t lod1_distance = 20.0;
	real_t lod2_distance = 45.0;
	real_t max_distance = 120.0;
	AABB local_bounds;
	String stable_id;
	PackedStringArray tags;
	GameplayState gameplay_state = STATE_INTACT;
	Ref<OpenWorldRockVariant> damaged_variant;
	Ref<OpenWorldRockVariant> depleted_variant;

protected:
	static void _bind_methods();

public:
	void set_variant_name(const String &p_value); String get_variant_name() const { return variant_name; }
	void set_source_mode(OpenWorldRockGenerationRequest::RockMode p_value); OpenWorldRockGenerationRequest::RockMode get_source_mode() const { return source_mode; }
	void set_source_seed(int p_value); int get_source_seed() const { return source_seed; }
	void set_lod0_mesh(const Ref<Mesh> &p_value); Ref<Mesh> get_lod0_mesh() const { return lod_meshes[0]; }
	void set_lod1_mesh(const Ref<Mesh> &p_value); Ref<Mesh> get_lod1_mesh() const { return lod_meshes[1]; }
	void set_lod2_mesh(const Ref<Mesh> &p_value); Ref<Mesh> get_lod2_mesh() const { return lod_meshes[2]; }
	Ref<Mesh> get_lod_mesh(int p_lod) const;
	void set_collision_shape(const Ref<Shape3D> &p_value); Ref<Shape3D> get_collision_shape() const { return collision_shape; }
	void set_collision_points(const PackedVector3Array &p_value); PackedVector3Array get_collision_points() const { return collision_points; }
	void set_lod1_distance(real_t p_value); real_t get_lod1_distance() const { return lod1_distance; }
	void set_lod2_distance(real_t p_value); real_t get_lod2_distance() const { return lod2_distance; }
	void set_max_distance(real_t p_value); real_t get_max_distance() const { return max_distance; }
	int get_lod_index_for_distance(real_t p_distance) const;
	void set_local_bounds(const AABB &p_value); AABB get_local_bounds() const { return local_bounds; }
	void set_stable_id(const String &p_value); String get_stable_id() const { return stable_id; }
	void set_tags(const PackedStringArray &p_value); PackedStringArray get_tags() const { return tags; }
	void set_gameplay_state(GameplayState p_value); GameplayState get_gameplay_state() const { return gameplay_state; }
	void set_damaged_variant(const Ref<OpenWorldRockVariant> &p_value); Ref<OpenWorldRockVariant> get_damaged_variant() const { return damaged_variant; }
	void set_depleted_variant(const Ref<OpenWorldRockVariant> &p_value); Ref<OpenWorldRockVariant> get_depleted_variant() const { return depleted_variant; }
	Ref<OpenWorldRockVariant> get_variant_for_state(GameplayState p_state) const;
};
VARIANT_ENUM_CAST(OpenWorldRockVariant::GameplayState);
