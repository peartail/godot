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

private:
	Ref<OpenWorldTerrainData> terrain_data;
	Ref<ImageTexture> height_texture;
	Ref<Shader> displacement_shader;
	Ref<ShaderMaterial> displacement_material;
	Ref<Material> terrain_material;

	int patch_resolution = 128;
	bool use_builtin_displacement_material = true;
	Color low_color = Color(0.22, 0.38, 0.18);
	Color mid_color = Color(0.42, 0.34, 0.22);
	Color high_color = Color(0.78, 0.78, 0.72);
	real_t flatten_height = 0.5;
	bool show_debug_gizmo = true;

	void _terrain_data_changed();
	void _ensure_data();
	Vector3 _get_local_height_position(int p_x, int p_y) const;
	real_t _get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_y) const;
	real_t _sample_value_noise(real_t p_x, real_t p_y, int p_seed) const;
	void _rebuild_patch_mesh();
	void _rebuild_height_texture();
	void _update_material();
	static String _get_builtin_displacement_shader_code();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_terrain_data(const Ref<OpenWorldTerrainData> &p_terrain_data);
	Ref<OpenWorldTerrainData> get_terrain_data() const { return terrain_data; }

	void set_patch_resolution(int p_patch_resolution);
	int get_patch_resolution() const { return patch_resolution; }

	void set_world_size(real_t p_world_size);
	real_t get_world_size() const;

	void set_height_scale(real_t p_height_scale);
	real_t get_height_scale() const;

	void set_use_builtin_displacement_material(bool p_use);
	bool is_using_builtin_displacement_material() const { return use_builtin_displacement_material; }

	void set_terrain_material(const Ref<Material> &p_material);
	Ref<Material> get_terrain_material() const { return terrain_material; }

	void set_low_color(const Color &p_color);
	Color get_low_color() const { return low_color; }
	void set_mid_color(const Color &p_color);
	Color get_mid_color() const { return mid_color; }
	void set_high_color(const Color &p_color);
	Color get_high_color() const { return high_color; }
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
};

VARIANT_ENUM_CAST(OpenWorldTerrain3D::BrushOperation);
