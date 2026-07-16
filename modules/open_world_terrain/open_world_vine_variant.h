/**************************************************************************/
/*  open_world_vine_variant.h                                             */
/**************************************************************************/
#pragma once
#include "open_world_vine_generation_request.h"
#include "core/io/resource.h"
#include "scene/resources/mesh.h"

class OpenWorldVineVariant : public Resource {
	GDCLASS(OpenWorldVineVariant, Resource);
	RES_BASE_EXTENSION("owvinevariant");
public:
	enum SupportLostPolicy { SUPPORT_KEEP, SUPPORT_HIDE, SUPPORT_DETACH };
private:
	String variant_name = "Vine Variant";
	int source_seed = 0;
	OpenWorldVineGenerationRequest::VineMode source_mode = OpenWorldVineGenerationRequest::MODE_CREEPING;
	Ref<Mesh> lod_meshes[3];
	real_t lod1_distance = 18.0;
	real_t lod2_distance = 42.0;
	real_t max_distance = 90.0;
	int support_stable_id = 0;
	SupportLostPolicy support_lost_policy = SUPPORT_HIDE;
protected:
	static void _bind_methods();
public:
	void set_variant_name(const String &p_value); String get_variant_name() const { return variant_name; }
	void set_source_seed(int p_value); int get_source_seed() const { return source_seed; }
	void set_source_mode(OpenWorldVineGenerationRequest::VineMode p_value); OpenWorldVineGenerationRequest::VineMode get_source_mode() const { return source_mode; }
	void set_lod0_mesh(const Ref<Mesh> &p_value); Ref<Mesh> get_lod0_mesh() const { return lod_meshes[0]; }
	void set_lod1_mesh(const Ref<Mesh> &p_value); Ref<Mesh> get_lod1_mesh() const { return lod_meshes[1]; }
	void set_lod2_mesh(const Ref<Mesh> &p_value); Ref<Mesh> get_lod2_mesh() const { return lod_meshes[2]; }
	Ref<Mesh> get_lod_mesh(int p_lod) const;
	void set_lod1_distance(real_t p_value); real_t get_lod1_distance() const { return lod1_distance; }
	void set_lod2_distance(real_t p_value); real_t get_lod2_distance() const { return lod2_distance; }
	void set_max_distance(real_t p_value); real_t get_max_distance() const { return max_distance; }
	int get_lod_index_for_distance(real_t p_distance) const;
	void set_support_stable_id(int p_value); int get_support_stable_id() const { return support_stable_id; }
	void set_support_lost_policy(SupportLostPolicy p_value); SupportLostPolicy get_support_lost_policy() const { return support_lost_policy; }
};
VARIANT_ENUM_CAST(OpenWorldVineVariant::SupportLostPolicy);
