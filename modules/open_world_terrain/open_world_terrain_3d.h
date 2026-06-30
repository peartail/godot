/**************************************************************************/
/*  open_world_terrain_3d.h                                               */
/**************************************************************************/

#pragma once

#include "open_world_terrain_data.h"
#include "open_world_terrain_layer.h"

#include "core/templates/hash_set.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/texture.h"

class OpenWorldTerrain3D : public MeshInstance3D {
	GDCLASS(OpenWorldTerrain3D, MeshInstance3D);

public:
	enum BrushOperation {
		BRUSH_RAISE,
		BRUSH_LOWER,
		BRUSH_SMOOTH,
		BRUSH_FLATTEN,
	};

	enum SplatDebugMode {
		SPLAT_DEBUG_NONE,
		SPLAT_DEBUG_HEIGHT,
		SPLAT_DEBUG_SLOPE,
		SPLAT_DEBUG_WEIGHTS,
		SPLAT_DEBUG_PAINTED_LAYERS,
	};

	enum LayerPaintTarget {
		LAYER_PAINT_LOW,
		LAYER_PAINT_MID,
		LAYER_PAINT_HIGH,
	};

	enum LayerBlendMode {
		LAYER_BLEND_NORMAL,
		LAYER_BLEND_MULTIPLY,
		LAYER_BLEND_DARKEN,
		LAYER_BLEND_LIGHTEN,
	};

private:
	struct TerrainTile {
		Vector2i cell;
		int quad_width = 0;
		int quad_height = 0;
		Ref<ArrayMesh> mesh;
		Ref<Image> height_image;
		Ref<ImageTexture> height_texture;
		Ref<Image> layer_image;
		Ref<ImageTexture> layer_texture;
		Ref<ShaderMaterial> material;
		RID instance;
	};

	Ref<OpenWorldTerrainData> terrain_data;
	Ref<Shader> displacement_shader;
	Ref<Material> terrain_material;
	Vector<TerrainTile> tiles;
	HashSet<Vector2i> active_grid_cell_set;
	PackedVector2Array active_grid_cells;
	Array terrain_layers;

	int patch_resolution = 128;
	int tile_size = 256;
	bool use_builtin_displacement_material = true;
	Ref<Texture2D> low_texture;
	Ref<Texture2D> mid_texture;
	Ref<Texture2D> high_texture;
	Ref<Texture2D> low_normal_texture;
	Ref<Texture2D> mid_normal_texture;
	Ref<Texture2D> high_normal_texture;
	Ref<Texture2D> low_roughness_texture;
	Ref<Texture2D> mid_roughness_texture;
	Ref<Texture2D> high_roughness_texture;
	Ref<Texture2D> low_ao_texture;
	Ref<Texture2D> mid_ao_texture;
	Ref<Texture2D> high_ao_texture;
	Ref<Texture2D> low_parallax_texture;
	Ref<Texture2D> mid_parallax_texture;
	Ref<Texture2D> high_parallax_texture;
	Ref<Texture2D> macro_variation_texture;
	Color low_color = Color(0.22, 0.38, 0.18);
	Color mid_color = Color(0.42, 0.34, 0.22);
	Color high_color = Color(0.78, 0.78, 0.72);
	real_t low_height = 0.28;
	real_t high_height = 0.68;
	real_t blend_width = 0.12;
	real_t texture_scale = 0.08;
	real_t low_texture_scale = 0.08;
	real_t mid_texture_scale = 0.08;
	real_t high_texture_scale = 0.08;
	real_t triplanar_sharpness = 4.0;
	real_t slope_start = 0.45;
	real_t slope_end = 0.85;
	real_t slope_high_strength = 0.65;
	real_t low_roughness = 0.92;
	real_t mid_roughness = 0.92;
	real_t high_roughness = 0.92;
	real_t ao_strength = 1.0;
	bool parallax_enabled = false;
	real_t parallax_scale = 0.03;
	bool parallax_flip = false;
	real_t parallax_fade_start = 120.0;
	real_t parallax_fade_end = 300.0;
	bool anti_tiling_enabled = false;
	real_t anti_tiling_strength = 0.15;
	real_t macro_variation_scale = 0.0025;
	real_t macro_variation_strength = 0.25;
	SplatDebugMode splat_debug_mode = SPLAT_DEBUG_NONE;
	real_t flatten_height = 0.5;
	real_t brush_falloff = 1.0;
	bool show_debug_gizmo = true;

	void _terrain_data_changed();
	void _terrain_layer_changed();
	void _ensure_data();
	Vector3 _get_local_height_position(int p_x, int p_y) const;
	real_t _sample_height_nearest(int p_x, int p_y) const;
	real_t _sample_height_bilinear(real_t p_x, real_t p_y) const;
	real_t _sample_tile_height_nearest(const Vector2i &p_cell, int p_x, int p_y) const;
	real_t _sample_tile_height_bilinear(const Vector2i &p_cell, real_t p_x, real_t p_y) const;
	real_t _get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_y) const;
	real_t _sample_value_noise(real_t p_x, real_t p_y, int p_seed) const;
	Ref<ArrayMesh> _build_tile_mesh() const;
	Ref<Image> _build_tile_height_image(const Vector2i &p_cell) const;
	Ref<Image> _build_tile_layer_image(const Vector2i &p_cell) const;
	void _clear_tiles();
	void _sync_tile_instances();
	void _update_tile_visibility();
	void _sync_tile_materials();
	void _rebuild_tiles();
	bool _is_grid_cell_active(const Vector2i &p_cell) const;
	bool _is_quad_origin_active(int p_origin_x, int p_origin_y) const;
	Vector2i _get_grid_cell_for_local_position(const Vector3 &p_local_position) const;
	Vector2i _get_tile_cell_for_local_position(const Vector3 &p_local_position) const;
	void _rebuild_height_textures();
	void _rebuild_layer_textures();
	void _refresh_height_texture_region(const Vector2i &p_cell, int p_min_x, int p_min_y, int p_max_x, int p_max_y);
	void _refresh_layer_texture_region(const Vector2i &p_cell, int p_min_x, int p_min_y, int p_max_x, int p_max_y);
	void _update_tile_material(TerrainTile &r_tile);
	void _update_materials();
	void _sync_material_layers_from_resources();
	static String _get_builtin_displacement_shader_code();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_terrain_data(const Ref<OpenWorldTerrainData> &p_terrain_data);
	Ref<OpenWorldTerrainData> get_terrain_data() const { return terrain_data; }

	void set_patch_resolution(int p_patch_resolution);
	int get_patch_resolution() const { return patch_resolution; }
	void set_tile_size(int p_tile_size);
	int get_tile_size() const { return tile_size; }
	void set_tile_world_size(real_t p_tile_world_size);
	real_t get_tile_world_size() const;
	void set_tile_resolution(int p_tile_resolution);
	int get_tile_resolution() const;
	void set_active_grid_cells(const PackedVector2Array &p_cells);
	PackedVector2Array get_active_grid_cells() const { return active_grid_cells; }
	bool has_grid_cell(const Vector2i &p_cell) const;
	void create_grid_cell(const Vector2i &p_cell);
	void remove_grid_cell(const Vector2i &p_cell);

	void set_world_size(real_t p_world_size);
	real_t get_world_size() const;

	void set_height_scale(real_t p_height_scale);
	real_t get_height_scale() const;

	void set_use_builtin_displacement_material(bool p_use);
	bool is_using_builtin_displacement_material() const { return use_builtin_displacement_material; }

	void set_terrain_material(const Ref<Material> &p_material);
	Ref<Material> get_terrain_material() const { return terrain_material; }

	void set_terrain_layers(const Array &p_layers);
	Array get_terrain_layers() const { return terrain_layers; }

	void set_low_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_low_texture() const { return low_texture; }
	void set_mid_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_mid_texture() const { return mid_texture; }
	void set_high_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_high_texture() const { return high_texture; }
	void set_low_normal_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_low_normal_texture() const { return low_normal_texture; }
	void set_mid_normal_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_mid_normal_texture() const { return mid_normal_texture; }
	void set_high_normal_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_high_normal_texture() const { return high_normal_texture; }
	void set_low_roughness_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_low_roughness_texture() const { return low_roughness_texture; }
	void set_mid_roughness_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_mid_roughness_texture() const { return mid_roughness_texture; }
	void set_high_roughness_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_high_roughness_texture() const { return high_roughness_texture; }
	void set_low_ao_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_low_ao_texture() const { return low_ao_texture; }
	void set_mid_ao_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_mid_ao_texture() const { return mid_ao_texture; }
	void set_high_ao_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_high_ao_texture() const { return high_ao_texture; }
	void set_low_parallax_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_low_parallax_texture() const { return low_parallax_texture; }
	void set_mid_parallax_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_mid_parallax_texture() const { return mid_parallax_texture; }
	void set_high_parallax_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_high_parallax_texture() const { return high_parallax_texture; }
	void set_macro_variation_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_macro_variation_texture() const { return macro_variation_texture; }
	void set_low_color(const Color &p_color);
	Color get_low_color() const { return low_color; }
	void set_mid_color(const Color &p_color);
	Color get_mid_color() const { return mid_color; }
	void set_high_color(const Color &p_color);
	Color get_high_color() const { return high_color; }
	void set_low_height(real_t p_height);
	real_t get_low_height() const { return low_height; }
	void set_high_height(real_t p_height);
	real_t get_high_height() const { return high_height; }
	void set_blend_width(real_t p_width);
	real_t get_blend_width() const { return blend_width; }
	void set_texture_scale(real_t p_scale);
	real_t get_texture_scale() const { return texture_scale; }
	void set_low_texture_scale(real_t p_scale);
	real_t get_low_texture_scale() const { return low_texture_scale; }
	void set_mid_texture_scale(real_t p_scale);
	real_t get_mid_texture_scale() const { return mid_texture_scale; }
	void set_high_texture_scale(real_t p_scale);
	real_t get_high_texture_scale() const { return high_texture_scale; }
	void set_triplanar_sharpness(real_t p_sharpness);
	real_t get_triplanar_sharpness() const { return triplanar_sharpness; }
	void set_slope_start(real_t p_slope);
	real_t get_slope_start() const { return slope_start; }
	void set_slope_end(real_t p_slope);
	real_t get_slope_end() const { return slope_end; }
	void set_slope_high_strength(real_t p_strength);
	real_t get_slope_high_strength() const { return slope_high_strength; }
	void set_low_roughness(real_t p_roughness);
	real_t get_low_roughness() const { return low_roughness; }
	void set_mid_roughness(real_t p_roughness);
	real_t get_mid_roughness() const { return mid_roughness; }
	void set_high_roughness(real_t p_roughness);
	real_t get_high_roughness() const { return high_roughness; }
	void set_ao_strength(real_t p_strength);
	real_t get_ao_strength() const { return ao_strength; }
	void set_parallax_enabled(bool p_enabled);
	bool is_parallax_enabled() const { return parallax_enabled; }
	void set_parallax_scale(real_t p_scale);
	real_t get_parallax_scale() const { return parallax_scale; }
	void set_parallax_flip(bool p_flip);
	bool is_parallax_flipped() const { return parallax_flip; }
	void set_parallax_fade_start(real_t p_distance);
	real_t get_parallax_fade_start() const { return parallax_fade_start; }
	void set_parallax_fade_end(real_t p_distance);
	real_t get_parallax_fade_end() const { return parallax_fade_end; }
	void set_anti_tiling_enabled(bool p_enabled);
	bool is_anti_tiling_enabled() const { return anti_tiling_enabled; }
	void set_anti_tiling_strength(real_t p_strength);
	real_t get_anti_tiling_strength() const { return anti_tiling_strength; }
	void set_macro_variation_scale(real_t p_scale);
	real_t get_macro_variation_scale() const { return macro_variation_scale; }
	void set_macro_variation_strength(real_t p_strength);
	real_t get_macro_variation_strength() const { return macro_variation_strength; }
	void set_splat_debug_mode(SplatDebugMode p_mode);
	SplatDebugMode get_splat_debug_mode() const { return splat_debug_mode; }
	void set_flatten_height(real_t p_height);
	real_t get_flatten_height() const { return flatten_height; }
	void set_brush_falloff(real_t p_falloff);
	real_t get_brush_falloff() const { return brush_falloff; }
	void set_show_debug_gizmo(bool p_show);
	bool is_showing_debug_gizmo() const { return show_debug_gizmo; }

	void reset_flat_terrain(real_t p_normalized_height = 0.0);
	void generate_random_terrain(int p_seed = 1, real_t p_amplitude = 1.0, real_t p_frequency = 0.025, int p_octaves = 4);
	void rebuild();
	void apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation);
	Dictionary apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation);
	void apply_height_patch(const PackedInt32Array &p_indices, const PackedFloat32Array &p_heights);
	void apply_layer_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, LayerPaintTarget p_target, real_t p_alpha = 1.0, LayerBlendMode p_blend_mode = LAYER_BLEND_NORMAL);
	Dictionary apply_layer_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, LayerPaintTarget p_target, real_t p_alpha = 1.0, LayerBlendMode p_blend_mode = LAYER_BLEND_NORMAL);
	void apply_layer_patch(const PackedInt32Array &p_indices, const PackedColorArray &p_layers);
	Dictionary get_brush_hit(const Vector3 &p_ray_origin, const Vector3 &p_ray_direction) const;
	PackedVector3Array get_debug_lines() const;
	Ref<Texture2D> get_height_texture() const;
	virtual AABB get_aabb() const override;
	virtual PackedStringArray get_configuration_warnings() const override;

	OpenWorldTerrain3D();
	~OpenWorldTerrain3D();
};

VARIANT_ENUM_CAST(OpenWorldTerrain3D::BrushOperation);
VARIANT_ENUM_CAST(OpenWorldTerrain3D::SplatDebugMode);
VARIANT_ENUM_CAST(OpenWorldTerrain3D::LayerPaintTarget);
VARIANT_ENUM_CAST(OpenWorldTerrain3D::LayerBlendMode);
