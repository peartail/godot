/**************************************************************************/
/*  open_world_tree_generator_3d.h                                        */
/**************************************************************************/

#pragma once

#include "open_world_tree_generation_profile.h"
#include "open_world_tree_variant.h"
#include "open_world_tree_support_graph.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/material.h"


class OpenWorldTreeGenerator3D : public MeshInstance3D {
	GDCLASS(OpenWorldTreeGenerator3D, MeshInstance3D);

public:
	enum PreviewLOD {
		PREVIEW_LOD0,
		PREVIEW_LOD1,
		PREVIEW_LOD2,
	};

private:
	Ref<OpenWorldTreeGenerationProfile> generation_profile;
	Ref<Material> trunk_material;
	Ref<Material> foliage_material;
	Ref<ArrayMesh> generated_mesh;
	Ref<ArrayMesh> generated_lod_meshes[3];
	Ref<ShaderMaterial> wind_trunk_material;
	Ref<ShaderMaterial> wind_foliage_material;
	Ref<OpenWorldTreeSupportGraph> generated_support_graph;
	int seed = 1207;
	bool auto_generate = true;
	bool generate_lod1 = true;
	bool generate_lod2 = true;
	real_t lod1_quality = 0.55;
	real_t lod2_quality = 0.18;
	real_t lod1_distance = 25.0;
	real_t lod2_distance = 55.0;
	real_t max_distance = 120.0;
	PreviewLOD preview_lod = PREVIEW_LOD0;
	bool wind_enabled = false;
	real_t wind_strength = 0.22;
	real_t wind_speed = 1.0;
	real_t wind_gust_strength = 0.35;
	Vector2 wind_direction = Vector2(1.0, 0.35);
	Color wind_trunk_color = Color(0.32, 0.16, 0.07);
	Color wind_foliage_color = Color(0.2, 0.65, 0.16);

	void _profile_changed();
	void _apply_materials();
	void _update_preview_mesh();
	void _update_wind_materials();
	Ref<ArrayMesh> _generate_lod_mesh(int p_lod, Ref<OpenWorldTreeSupportGraph> *r_support_graph = nullptr) const;

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

	void set_generate_lod1(bool p_enabled);
	bool is_generate_lod1_enabled() const { return generate_lod1; }
	void set_generate_lod2(bool p_enabled);
	bool is_generate_lod2_enabled() const { return generate_lod2; }
	void set_lod1_quality(real_t p_quality);
	real_t get_lod1_quality() const { return lod1_quality; }
	void set_lod2_quality(real_t p_quality);
	real_t get_lod2_quality() const { return lod2_quality; }
	void set_lod1_distance(real_t p_distance);
	real_t get_lod1_distance() const { return lod1_distance; }
	void set_lod2_distance(real_t p_distance);
	real_t get_lod2_distance() const { return lod2_distance; }
	void set_max_distance(real_t p_distance);
	real_t get_max_distance() const { return max_distance; }
	void set_preview_lod(PreviewLOD p_lod);
	PreviewLOD get_preview_lod() const { return preview_lod; }

	void set_wind_enabled(bool p_enabled);
	bool is_wind_enabled() const { return wind_enabled; }
	void set_wind_strength(real_t p_strength);
	real_t get_wind_strength() const { return wind_strength; }
	void set_wind_speed(real_t p_speed);
	real_t get_wind_speed() const { return wind_speed; }
	void set_wind_gust_strength(real_t p_strength);
	real_t get_wind_gust_strength() const { return wind_gust_strength; }
	void set_wind_direction(const Vector2 &p_direction);
	Vector2 get_wind_direction() const { return wind_direction; }
	void set_wind_trunk_color(const Color &p_color);
	Color get_wind_trunk_color() const { return wind_trunk_color; }
	void set_wind_foliage_color(const Color &p_color);
	Color get_wind_foliage_color() const { return wind_foliage_color; }

	void generate_tree();
	void randomize_seed();
	void clear_generated_tree();
	Ref<ArrayMesh> get_generated_mesh() const { return generated_mesh; }
	Ref<ArrayMesh> get_generated_lod_mesh(int p_lod) const;
	Dictionary get_lod_statistics(int p_lod) const;
	Ref<OpenWorldTreeSupportGraph> get_generated_support_graph() const { return generated_support_graph; }
	Ref<OpenWorldTreeVariant> create_baked_variant() const;

	OpenWorldTreeGenerator3D();
	~OpenWorldTreeGenerator3D();
};

VARIANT_ENUM_CAST(OpenWorldTreeGenerator3D::PreviewLOD);
