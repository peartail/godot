/**************************************************************************/
/*  open_world_terrain_3d.h                                               */
/**************************************************************************/

#pragma once

#include "open_world_terrain_data.h"

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
	};

private:
	struct TerrainTile {
		int origin_x = 0;
		int origin_y = 0;
		int quad_width = 0;
		int quad_height = 0;
		Ref<ArrayMesh> mesh;
		Ref<Image> height_image;
		Ref<ImageTexture> height_texture;
		Ref<ShaderMaterial> material;
		RID instance;
	};

	Ref<OpenWorldTerrainData> terrain_data;
	Ref<Shader> displacement_shader;
	Ref<Material> terrain_material;
	Vector<TerrainTile> tiles;

	int patch_resolution = 128;
	int tile_size = 256;
	bool use_builtin_displacement_material = true;
	Ref<Texture2D> low_texture;
	Ref<Texture2D> mid_texture;
	Ref<Texture2D> high_texture;
	Color low_color = Color(0.22, 0.38, 0.18);
	Color mid_color = Color(0.42, 0.34, 0.22);
	Color high_color = Color(0.78, 0.78, 0.72);
	real_t low_height = 0.25;
	real_t high_height = 0.7;
	real_t blend_width = 0.15;
	real_t texture_scale = 0.08;
	real_t low_texture_scale = 0.08;
	real_t mid_texture_scale = 0.08;
	real_t high_texture_scale = 0.08;
	real_t triplanar_sharpness = 4.0;
	real_t slope_start = 0.35;
	real_t slope_end = 0.75;
	real_t slope_high_strength = 1.0;
	SplatDebugMode splat_debug_mode = SPLAT_DEBUG_NONE;
	real_t flatten_height = 0.5;
	bool show_debug_gizmo = true;

	void _terrain_data_changed();
	void _ensure_data();
	Vector3 _get_local_height_position(int p_x, int p_y) const;
	real_t _sample_height_nearest(int p_x, int p_y) const;
	real_t _sample_height_bilinear(real_t p_x, real_t p_y) const;
	real_t _get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_y) const;
	real_t _sample_value_noise(real_t p_x, real_t p_y, int p_seed) const;
	Ref<ArrayMesh> _build_tile_mesh(int p_origin_x, int p_origin_y, int p_quad_width, int p_quad_height) const;
	Ref<Image> _build_tile_height_image(int p_origin_x, int p_origin_y, int p_quad_width, int p_quad_height) const;
	void _clear_tiles();
	void _sync_tile_instances();
	void _sync_tile_materials();
	void _rebuild_tiles();
	void _rebuild_height_textures();
	void _refresh_height_texture_region(int p_min_x, int p_min_y, int p_max_x, int p_max_y);
	void _update_tile_material(TerrainTile &r_tile);
	void _update_materials();
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

	void set_world_size(real_t p_world_size);
	real_t get_world_size() const;

	void set_height_scale(real_t p_height_scale);
	real_t get_height_scale() const;

	void set_use_builtin_displacement_material(bool p_use);
	bool is_using_builtin_displacement_material() const { return use_builtin_displacement_material; }

	void set_terrain_material(const Ref<Material> &p_material);
	Ref<Material> get_terrain_material() const { return terrain_material; }

	void set_low_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_low_texture() const { return low_texture; }
	void set_mid_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_mid_texture() const { return mid_texture; }
	void set_high_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_high_texture() const { return high_texture; }
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
	void set_splat_debug_mode(SplatDebugMode p_mode);
	SplatDebugMode get_splat_debug_mode() const { return splat_debug_mode; }
	void set_flatten_height(real_t p_height);
	real_t get_flatten_height() const { return flatten_height; }
	void set_show_debug_gizmo(bool p_show);
	bool is_showing_debug_gizmo() const { return show_debug_gizmo; }

	void reset_flat_terrain(real_t p_normalized_height = 0.0);
	void generate_random_terrain(int p_seed = 1, real_t p_amplitude = 1.0, real_t p_frequency = 0.025, int p_octaves = 4);
	void rebuild();
	void apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation);
	Dictionary apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation);
	void apply_height_patch(const PackedInt32Array &p_indices, const PackedFloat32Array &p_heights);
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
