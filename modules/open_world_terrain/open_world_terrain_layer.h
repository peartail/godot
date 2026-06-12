/**************************************************************************/
/*  open_world_terrain_layer.h                                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "scene/resources/texture.h"

class OpenWorldTerrainLayer : public Resource {
	GDCLASS(OpenWorldTerrainLayer, Resource);
	RES_BASE_EXTENSION("owterrainlayer");

public:
	enum GenerationRole {
		GENERATION_ROLE_HEIGHT, // Natural terrain class, such as mountain, plain, or cliff.
		GENERATION_ROLE_SURFACE, // Surface-only layer, such as road or dirt path.
		GENERATION_ROLE_EXCLUDED, // Reserved/manual layer ignored by terrain generation.
	};

	enum TerrainFeature {
		FEATURE_GENERIC,
		FEATURE_MOUNTAIN,
		FEATURE_PLAIN,
		FEATURE_CLIFF,
		FEATURE_ROAD,
		FEATURE_DIRT_PATH,
		FEATURE_GRASS,
		FEATURE_ROCK,
	};

private:
	String layer_name = "Terrain Layer";
	GenerationRole generation_role = GENERATION_ROLE_HEIGHT;
	TerrainFeature terrain_feature = FEATURE_GENERIC;
	bool material_enabled = true;
	Ref<Texture2D> albedo_texture;
	Ref<Texture2D> normal_texture;
	Ref<Texture2D> roughness_texture;
	Ref<Texture2D> ao_texture;
	Ref<Texture2D> parallax_texture;
	Color tint_color = Color(1.0, 1.0, 1.0);
	real_t texture_scale = 0.08;
	real_t roughness = 0.92;

protected:
	static void _bind_methods();

public:
	void set_layer_name(const String &p_name);
	String get_layer_name() const { return layer_name; }

	void set_generation_role(GenerationRole p_role);
	GenerationRole get_generation_role() const { return generation_role; }

	void set_terrain_feature(TerrainFeature p_feature);
	TerrainFeature get_terrain_feature() const { return terrain_feature; }

	void set_material_enabled(bool p_enabled);
	bool is_material_enabled() const { return material_enabled; }

	void set_albedo_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_albedo_texture() const { return albedo_texture; }

	void set_normal_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_normal_texture() const { return normal_texture; }

	void set_roughness_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_roughness_texture() const { return roughness_texture; }

	void set_ao_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_ao_texture() const { return ao_texture; }

	void set_parallax_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_parallax_texture() const { return parallax_texture; }

	void set_tint_color(const Color &p_color);
	Color get_tint_color() const { return tint_color; }

	void set_texture_scale(real_t p_scale);
	real_t get_texture_scale() const { return texture_scale; }

	void set_roughness(real_t p_roughness);
	real_t get_roughness() const { return roughness; }
};

VARIANT_ENUM_CAST(OpenWorldTerrainLayer::GenerationRole);
VARIANT_ENUM_CAST(OpenWorldTerrainLayer::TerrainFeature);
