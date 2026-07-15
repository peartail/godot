/**************************************************************************/
/*  open_world_tree_generator_3d.h                                        */
/**************************************************************************/

#pragma once

#include "open_world_tree_generation_profile.h"
#include "open_world_tree_variant.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/material.h"

class OpenWorldTreeGenerator3D : public MeshInstance3D {
	GDCLASS(OpenWorldTreeGenerator3D, MeshInstance3D);

	Ref<OpenWorldTreeGenerationProfile> generation_profile;
	Ref<Material> trunk_material;
	Ref<Material> foliage_material;
	Ref<ArrayMesh> generated_mesh;
	int seed = 1207;
	bool auto_generate = true;

	void _profile_changed();
	void _apply_materials();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_generation_profile(const Ref<OpenWorldTreeGenerationProfile> &p_profile);
	Ref<OpenWorldTreeGenerationProfile> get_generation_profile() const { return generation_profile; }
	void set_seed(int p_seed);
	int get_seed() const { return seed; }
	void set_auto_generate(bool p_enabled);
	bool is_auto_generate() const { return auto_generate; }
	void set_trunk_material(const Ref<Material> &p_material);
	Ref<Material> get_trunk_material() const { return trunk_material; }
	void set_foliage_material(const Ref<Material> &p_material);
	Ref<Material> get_foliage_material() const { return foliage_material; }

	void generate_tree();
	void randomize_seed();
	void clear_generated_tree();
	Ref<ArrayMesh> get_generated_mesh() const { return generated_mesh; }
	Ref<OpenWorldTreeVariant> create_baked_variant() const;

	OpenWorldTreeGenerator3D();
	~OpenWorldTreeGenerator3D();
};

