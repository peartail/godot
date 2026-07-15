/**************************************************************************/
/*  open_world_tree_variant.h                                             */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"

class OpenWorldTreeVariant : public Resource {
	GDCLASS(OpenWorldTreeVariant, Resource);
	RES_BASE_EXTENSION("owtreevariant");

	String variant_name = "Tree Variant";
	int source_seed = 0;
	Ref<Mesh> lod_meshes[3];
	real_t lod1_distance = 25.0;
	real_t lod2_distance = 55.0;
	real_t max_distance = 120.0;
	Ref<Material> material_override;
	real_t collision_radius = 0.5;
	real_t collision_height = 2.0;

protected:
	static void _bind_methods();

public:
	void set_variant_name(const String &p_name);
	String get_variant_name() const { return variant_name; }

	void set_source_seed(int p_seed);
	int get_source_seed() const { return source_seed; }

	void set_lod0_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_lod0_mesh() const { return lod_meshes[0]; }
	void set_lod1_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_lod1_mesh() const { return lod_meshes[1]; }
	void set_lod2_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_lod2_mesh() const { return lod_meshes[2]; }
	Ref<Mesh> get_lod_mesh(int p_lod) const;

	void set_lod1_distance(real_t p_distance);
	real_t get_lod1_distance() const { return lod1_distance; }
	void set_lod2_distance(real_t p_distance);
	real_t get_lod2_distance() const { return lod2_distance; }
	void set_max_distance(real_t p_distance);
	real_t get_max_distance() const { return max_distance; }

	int get_lod_index_for_distance(real_t p_distance) const;

	void set_material_override(const Ref<Material> &p_material);
	Ref<Material> get_material_override() const { return material_override; }

	void set_collision_radius(real_t p_radius);
	real_t get_collision_radius() const { return collision_radius; }
	void set_collision_height(real_t p_height);
	real_t get_collision_height() const { return collision_height; }
};
