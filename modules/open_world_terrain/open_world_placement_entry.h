/**************************************************************************/
/*  open_world_placement_entry.h                                    */
/**************************************************************************/

#pragma once

#include "open_world_rock_generation_request.h"
#include "open_world_tree_generation_profile.h"
#include "open_world_vine_generation_request.h"

#include "core/io/resource.h"
#include "scene/resources/material.h"

class OpenWorldPlacementEntry : public Resource {
	GDCLASS(OpenWorldPlacementEntry, Resource);
	RES_BASE_EXTENSION("owplacemententry");

public:
	enum ContentKind {
		CONTENT_TREE,
		CONTENT_VINE,
		CONTENT_ROCK,
	};

private:
	String stable_id;
	ContentKind content_kind = CONTENT_TREE;
	bool enabled = true;
	real_t weight = 1.0;
	Vector3 min_scale = Vector3(1.0, 1.0, 1.0);
	Vector3 max_scale = Vector3(1.0, 1.0, 1.0);
	bool random_yaw = true;
	bool align_to_surface_normal = false;
	real_t surface_offset = 0.0;
	real_t minimum_spacing_override = 0.0;
	Ref<OpenWorldTreeGenerationProfile> tree_profile;
	Ref<OpenWorldVineGenerationRequest> vine_request_template;
	Ref<OpenWorldRockGenerationRequest> rock_request_template;
	Ref<Material> trunk_material;
	Ref<Material> foliage_material;
	Ref<Material> stem_material;
	Ref<Material> preview_material;

protected:
	static void _bind_methods();

public:
	void set_stable_id(String p_value);
	String get_stable_id() const { return stable_id; }
	void set_content_kind(ContentKind p_value);
	ContentKind get_content_kind() const { return content_kind; }
	void set_enabled(bool p_value);
	bool is_enabled() const { return enabled; }
	void set_weight(real_t p_value);
	real_t get_weight() const { return weight; }
	void set_min_scale(Vector3 p_value);
	Vector3 get_min_scale() const { return min_scale; }
	void set_max_scale(Vector3 p_value);
	Vector3 get_max_scale() const { return max_scale; }
	void set_random_yaw(bool p_value);
	bool is_random_yaw_enabled() const { return random_yaw; }
	void set_align_to_surface_normal(bool p_value);
	bool is_aligning_to_surface_normal() const { return align_to_surface_normal; }
	void set_surface_offset(real_t p_value);
	real_t get_surface_offset() const { return surface_offset; }
	void set_minimum_spacing_override(real_t p_value);
	real_t get_minimum_spacing_override() const { return minimum_spacing_override; }
	void set_tree_profile(const Ref<OpenWorldTreeGenerationProfile> &p_value);
	Ref<OpenWorldTreeGenerationProfile> get_tree_profile() const { return tree_profile; }
	void set_vine_request_template(const Ref<OpenWorldVineGenerationRequest> &p_value);
	Ref<OpenWorldVineGenerationRequest> get_vine_request_template() const { return vine_request_template; }
	void set_rock_request_template(const Ref<OpenWorldRockGenerationRequest> &p_value);
	Ref<OpenWorldRockGenerationRequest> get_rock_request_template() const { return rock_request_template; }
	void set_trunk_material(const Ref<Material> &p_value);
	Ref<Material> get_trunk_material() const { return trunk_material; }
	void set_foliage_material(const Ref<Material> &p_value);
	Ref<Material> get_foliage_material() const { return foliage_material; }
	void set_stem_material(const Ref<Material> &p_value);
	Ref<Material> get_stem_material() const { return stem_material; }
	void set_preview_material(const Ref<Material> &p_value);
	Ref<Material> get_preview_material() const { return preview_material; }

	Dictionary validate_entry() const;
};

VARIANT_ENUM_CAST(OpenWorldPlacementEntry::ContentKind);

