/**************************************************************************/
/*  simple_terrain_3d.h                                                          */
/**************************************************************************/

#pragma once

#include "simple_terrain_data.h"
#include "simple_world_placement_data.h"
#include "simple_world_placement_library.h"
#include "core/object/object_id.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/navigation_mesh.h"
#include "scene/resources/texture.h"

class SimpleNavigationBlocker3D;
class NavigationObstacle3D;
class NavigationMeshSourceGeometryData3D;

class SimpleTerrain3D : public MeshInstance3D {
	GDCLASS(SimpleTerrain3D, MeshInstance3D);

public:
	enum BrushOperation {
		BRUSH_RAISE,
		BRUSH_LOWER,
		BRUSH_SMOOTH,
		BRUSH_FLATTEN,
	};

	enum NavigationBuildMode {
		NAVIGATION_BUILD_TRIANGLES,
		NAVIGATION_BUILD_QUADS,
		NAVIGATION_BUILD_MERGED_RECTS,
		NAVIGATION_BUILD_BAKED,
	};

private:
	struct TerrainChunk {
		// Chunk coordinates are measured in terrain quad indices, not world
		// units. quad_width/quad_depth can be smaller than chunk_size at the
		// terrain edge. The mesh itself stores local-space vertex positions.
		int origin_x = 0;
		int origin_z = 0;
		int quad_width = 0;
		int quad_depth = 0;
		Ref<ArrayMesh> mesh;
		RID instance;
		Ref<NavigationMesh> navigation_mesh;
		RID navigation_region;
	};

	Ref<SimpleTerrainData> simple_terrain_data;
	Ref<SimpleWorldPlacementLibrary> world_placement_library;
	Ref<SimpleWorldPlacementData> world_placement_data;

	// The terrain is rendered by internal RenderingServer instances rather
	// than the inherited MeshInstance3D::mesh. This gives the renderer a
	// separate RID per chunk, allowing frustum culling to happen per chunk.
	Vector<TerrainChunk> chunks;
	int chunk_size = 32;
	real_t flat_height = 0.0;
	real_t random_height_scale = 6.0;
	real_t random_frequency = 0.04;
	int random_octaves = 4;
	int random_seed = 1;
	bool syncing_data = false;
	bool show_chunk_gizmos = false;
	bool navigation_enabled = false;
	uint32_t navigation_layers = 1;
	real_t navigation_max_slope = 45.0;
	NavigationBuildMode navigation_build_mode = NAVIGATION_BUILD_QUADS;
	Ref<NavigationMesh> navigation_baked_mesh;
	RID navigation_baked_region;
	bool navigation_bake_dirty = true;
	bool navigation_dynamic_enabled = false;
	uint32_t navigation_dynamic_layers = 2;
	Ref<NavigationMesh> navigation_dynamic_baked_mesh;
	RID navigation_dynamic_baked_region;
	bool navigation_dynamic_bake_dirty = true;
	Vector<ObjectID> dynamic_navigation_blockers;
	real_t navigation_quad_max_normal_angle = 5.0;
	real_t navigation_quad_planar_tolerance = 0.05;
	real_t navigation_merge_max_normal_angle = 8.0;
	real_t navigation_merge_max_height_delta = 0.25;
	real_t navigation_merge_planar_tolerance = 0.15;
	int navigation_merge_max_rect_size = 8;
	bool navigation_debug_visible = true;
	bool navigation_debug_runtime_obstacles_visible = true;
	Vector<NavigationObstacle3D *> runtime_navigation_obstacles;

	// Material priority is:
	// 1. explicit terrain_material
	// 2. generated height-based triplanar ShaderMaterial
	// 3. inherited GeometryInstance3D material_override
	// The internal chunk instances receive the chosen material as an override.
	Ref<Material> terrain_material;
	bool use_builtin_triplanar_material = false;
	Ref<Shader> builtin_triplanar_shader;
	Ref<ShaderMaterial> builtin_triplanar_material;
	Ref<Texture2D> triplanar_low_texture;
	Ref<Texture2D> triplanar_mid_texture;
	Ref<Texture2D> triplanar_high_texture;
	Color triplanar_low_color = Color(0.45, 0.34, 0.22);
	Color triplanar_mid_color = Color(0.26, 0.45, 0.18);
	Color triplanar_high_color = Color(0.55, 0.55, 0.52);
	real_t triplanar_low_height = 1.5;
	real_t triplanar_high_height = 8.0;
	real_t triplanar_blend_width = 2.0;
	real_t triplanar_texture_scale = 0.08;
	real_t triplanar_blend_sharpness = 4.0;

#ifdef DEBUG_ENABLED
	RID navigation_debug_instance;
	Ref<ArrayMesh> navigation_debug_mesh;
#endif // DEBUG_ENABLED

	// SimpleTerrainData emits "changed" when edited from the inspector, scripts, or
	// undo/redo. syncing_data suppresses recursive rebuilds while SimpleTerrain3D is
	// already mutating the resource.
	void _simple_terrain_data_changed();
	void _ensure_data();

	// Height-space helpers. X/Z parameters use terrain vertex coordinates unless
	// a method explicitly says it receives a world or local position.
	Vector3 _get_vertex_position(int p_x, int p_z) const;
	Vector3 _get_vertex_normal(int p_x, int p_z) const;
	real_t _sample_nearest_height(real_t p_center_x, real_t p_center_z) const;
	real_t _get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_z) const;
	real_t _sample_value_noise(real_t p_x, real_t p_z, int p_seed) const;

	// Mesh rebuild helpers. A full rebuild recreates the chunk list; brush
	// strokes call _rebuild_chunks_for_region() so only touched chunks are
	// uploaded again.
	Ref<ArrayMesh> _build_chunk_mesh(int p_origin_x, int p_origin_z, int p_quad_width, int p_quad_depth) const;
	Ref<NavigationMesh> _build_chunk_navigation_mesh(int p_origin_x, int p_origin_z, int p_quad_width, int p_quad_depth) const;
	Ref<NavigationMeshSourceGeometryData3D> _build_bake_source_geometry(bool p_include_runtime_obstacles) const;
	void _mark_navigation_bake_dirty();
	void _mark_navigation_dynamic_bake_dirty();
	void _navigation_bake_finished();
	void _navigation_dynamic_bake_finished();
	void _world_placement_navigation_source_changed();
	void _clear_runtime_navigation_obstacles();
	void _sync_runtime_navigation_obstacles();
#ifdef DEBUG_ENABLED
	void _navigation_debug_changed();
	void _update_navigation_debug_mesh();
#endif // DEBUG_ENABLED
	void _clear_chunks();
	void _sync_chunk_instances();
	void _sync_chunk_materials();
	void _sync_chunk_navigation();
	void _sync_dynamic_navigation_region(RID p_navigation_map, const Transform3D &p_global_transform);
	void _rebuild_chunks_for_region(int p_min_x, int p_min_z, int p_max_x, int p_max_z);
	Ref<Material> _get_active_chunk_material();
	void _update_builtin_triplanar_material();
	static String _get_builtin_triplanar_shader_code();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Data ownership. If no SimpleTerrainData is assigned, SimpleTerrain3D lazily creates a
	// flat resource when it needs to render or edit.
	void set_simple_terrain_data(const Ref<SimpleTerrainData> &p_simple_terrain_data);
	Ref<SimpleTerrainData> get_simple_terrain_data() const { return simple_terrain_data; }
	void set_world_placement_library(const Ref<SimpleWorldPlacementLibrary> &p_library);
	Ref<SimpleWorldPlacementLibrary> get_world_placement_library() const { return world_placement_library; }
	void set_world_placement_data(const Ref<SimpleWorldPlacementData> &p_data);
	Ref<SimpleWorldPlacementData> get_world_placement_data() const { return world_placement_data; }

	// Grid controls proxy to SimpleTerrainData so the node remains convenient in the
	// Inspector while the data can still be saved as a separate resource.
	void set_grid_size(int p_grid_size);
	int get_grid_size() const;
	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const;
	void set_chunk_size(int p_chunk_size);
	int get_chunk_size() const { return chunk_size; }
	void set_show_chunk_gizmos(bool p_show);
	bool is_showing_chunk_gizmos() const { return show_chunk_gizmos; }
	void set_navigation_enabled(bool p_enabled);
	bool is_navigation_enabled() const { return navigation_enabled; }
	void set_navigation_layers(uint32_t p_layers);
	uint32_t get_navigation_layers() const { return navigation_layers; }
	void set_navigation_max_slope(real_t p_slope);
	real_t get_navigation_max_slope() const { return navigation_max_slope; }
	void set_navigation_build_mode(NavigationBuildMode p_mode);
	NavigationBuildMode get_navigation_build_mode() const { return navigation_build_mode; }
	void set_navigation_baked_mesh(const Ref<NavigationMesh> &p_navigation_mesh);
	Ref<NavigationMesh> get_navigation_baked_mesh() const { return navigation_baked_mesh; }
	bool is_navigation_bake_dirty() const { return navigation_bake_dirty; }
	void set_navigation_dynamic_enabled(bool p_enabled);
	bool is_navigation_dynamic_enabled() const { return navigation_dynamic_enabled; }
	void set_navigation_dynamic_layers(uint32_t p_layers);
	uint32_t get_navigation_dynamic_layers() const { return navigation_dynamic_layers; }
	void set_navigation_dynamic_baked_mesh(const Ref<NavigationMesh> &p_navigation_mesh);
	Ref<NavigationMesh> get_navigation_dynamic_baked_mesh() const { return navigation_dynamic_baked_mesh; }
	bool is_navigation_dynamic_bake_dirty() const { return navigation_dynamic_bake_dirty; }
	void mark_dynamic_navigation_bake_dirty();
	void set_navigation_quad_max_normal_angle(real_t p_angle);
	real_t get_navigation_quad_max_normal_angle() const { return navigation_quad_max_normal_angle; }
	void set_navigation_quad_planar_tolerance(real_t p_tolerance);
	real_t get_navigation_quad_planar_tolerance() const { return navigation_quad_planar_tolerance; }
	void set_navigation_merge_max_normal_angle(real_t p_angle);
	real_t get_navigation_merge_max_normal_angle() const { return navigation_merge_max_normal_angle; }
	void set_navigation_merge_max_height_delta(real_t p_height_delta);
	real_t get_navigation_merge_max_height_delta() const { return navigation_merge_max_height_delta; }
	void set_navigation_merge_planar_tolerance(real_t p_tolerance);
	real_t get_navigation_merge_planar_tolerance() const { return navigation_merge_planar_tolerance; }
	void set_navigation_merge_max_rect_size(int p_size);
	int get_navigation_merge_max_rect_size() const { return navigation_merge_max_rect_size; }
	void set_navigation_debug_visible(bool p_visible);
	bool is_navigation_debug_visible() const { return navigation_debug_visible; }
	void set_navigation_debug_runtime_obstacles_visible(bool p_visible);
	bool is_navigation_debug_runtime_obstacles_visible() const { return navigation_debug_runtime_obstacles_visible; }
	void set_height_data(const PackedFloat32Array &p_height_data);
	PackedFloat32Array get_height_data() const;

	// Runtime material API. These properties are safe for scripts to change at
	// runtime; they update existing chunk instances without rebuilding geometry.
	void set_terrain_material(const Ref<Material> &p_material);
	Ref<Material> get_terrain_material() const { return terrain_material; }
	void set_use_builtin_triplanar_material(bool p_use);
	bool is_using_builtin_triplanar_material() const { return use_builtin_triplanar_material; }
	void set_triplanar_low_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_triplanar_low_texture() const { return triplanar_low_texture; }
	void set_triplanar_mid_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_triplanar_mid_texture() const { return triplanar_mid_texture; }
	void set_triplanar_high_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_triplanar_high_texture() const { return triplanar_high_texture; }
	void set_triplanar_low_color(const Color &p_color);
	Color get_triplanar_low_color() const { return triplanar_low_color; }
	void set_triplanar_mid_color(const Color &p_color);
	Color get_triplanar_mid_color() const { return triplanar_mid_color; }
	void set_triplanar_high_color(const Color &p_color);
	Color get_triplanar_high_color() const { return triplanar_high_color; }
	void set_triplanar_low_height(real_t p_height);
	real_t get_triplanar_low_height() const { return triplanar_low_height; }
	void set_triplanar_high_height(real_t p_height);
	real_t get_triplanar_high_height() const { return triplanar_high_height; }
	void set_triplanar_blend_width(real_t p_width);
	real_t get_triplanar_blend_width() const { return triplanar_blend_width; }
	void set_triplanar_texture_scale(real_t p_scale);
	real_t get_triplanar_texture_scale() const { return triplanar_texture_scale; }
	void set_triplanar_blend_sharpness(real_t p_sharpness);
	real_t get_triplanar_blend_sharpness() const { return triplanar_blend_sharpness; }

	// Generation settings. The node starts flat by default; random generation is
	// opt-in through generate_random_terrain() or randomize_seed().
	void set_flat_height(real_t p_flat_height);
	real_t get_flat_height() const { return flat_height; }
	void set_random_height_scale(real_t p_random_height_scale);
	real_t get_random_height_scale() const { return random_height_scale; }
	void set_random_frequency(real_t p_random_frequency);
	real_t get_random_frequency() const { return random_frequency; }
	void set_random_octaves(int p_random_octaves);
	int get_random_octaves() const { return random_octaves; }
	void set_random_seed(int p_random_seed);
	int get_random_seed() const { return random_seed; }

	void reset_flat_terrain();
	void generate_random_terrain();
	void randomize_seed();
	void rebuild_mesh();
	void rebuild_navigation();
	void bake_navigation(bool p_on_thread = false);
	void bake_dynamic_navigation(bool p_on_thread = false);
	void register_dynamic_navigation_blocker(SimpleNavigationBlocker3D *p_blocker);
	void unregister_dynamic_navigation_blocker(SimpleNavigationBlocker3D *p_blocker);
	void clear_dynamic_navigation_blockers();

	// Editing and diagnostics. apply_brush() is runtime-safe; the editor plugin
	// builds undo/redo and viewport input behavior on top of this low-level API.
	void apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation);
	Dictionary apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation);
	void apply_height_patch(const PackedInt32Array &p_indices, const PackedFloat32Array &p_heights);
	Dictionary get_brush_hit(const Vector3 &p_ray_origin, const Vector3 &p_ray_direction) const;
	PackedVector3Array get_chunk_debug_lines() const;
	virtual Ref<TriangleMesh> generate_triangle_mesh() const override;
	virtual AABB get_aabb() const override;
	virtual PackedStringArray get_configuration_warnings() const override;

	SimpleTerrain3D();
	~SimpleTerrain3D();
};

VARIANT_ENUM_CAST(SimpleTerrain3D::BrushOperation);
VARIANT_ENUM_CAST(SimpleTerrain3D::NavigationBuildMode);
