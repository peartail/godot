/**************************************************************************/
/*  open_world_terrain_data.h                                             */
/**************************************************************************/

#pragma once

#include "core/io/image.h"
#include "core/io/resource.h"
#include "scene/resources/texture.h"

class OpenWorldTerrainData : public Resource {
	GDCLASS(OpenWorldTerrainData, Resource);
	RES_BASE_EXTENSION("owterraindata");

	int heightmap_resolution = 256;
	real_t world_size = 1024.0;
	real_t tile_world_size = 256.0;
	int tile_resolution = 257;
	real_t height_scale = 128.0;
	PackedFloat32Array height_data;
	PackedColorArray layer_data;
	PackedVector2Array created_tile_cells;
	Array tile_height_data;
	Array tile_layer_data;

	_FORCE_INLINE_ int _get_required_height_count() const { return heightmap_resolution * heightmap_resolution; }
	_FORCE_INLINE_ int _get_required_tile_value_count() const { return tile_resolution * tile_resolution; }
	void _ensure_height_data_size(bool p_clear_existing);
	void _ensure_layer_data_size(bool p_clear_existing);
	int _get_tile_array_index(const Vector2i &p_cell) const;
	PackedFloat32Array _make_flat_tile_height_data(real_t p_normalized_height = 0.0) const;
	PackedColorArray _make_empty_tile_layer_data() const;
	void _ensure_tile_data_size(bool p_clear_existing);

protected:
	static void _bind_methods();

public:
	void set_tile_world_size(real_t p_tile_world_size);
	real_t get_tile_world_size() const { return tile_world_size; }

	void set_tile_resolution(int p_tile_resolution);
	int get_tile_resolution() const { return tile_resolution; }

	void set_heightmap_resolution(int p_resolution);
	int get_heightmap_resolution() const { return heightmap_resolution; }

	void set_world_size(real_t p_world_size);
	real_t get_world_size() const { return world_size; }

	void set_height_scale(real_t p_height_scale);
	real_t get_height_scale() const { return height_scale; }

	void set_height_data(const PackedFloat32Array &p_height_data);
	PackedFloat32Array get_height_data() const { return height_data; }
	const PackedFloat32Array &get_height_data_ref() const { return height_data; }
	PackedFloat32Array &get_mutable_height_data() { return height_data; }
	void notify_height_data_changed();

	void set_layer_data(const PackedColorArray &p_layer_data);
	PackedColorArray get_layer_data() const { return layer_data; }
	const PackedColorArray &get_layer_data_ref() const { return layer_data; }
	PackedColorArray &get_mutable_layer_data() { return layer_data; }
	void notify_layer_data_changed();

	void set_created_tile_cells(const PackedVector2Array &p_cells);
	PackedVector2Array get_created_tile_cells() const { return created_tile_cells; }
	void set_tile_height_data_array(const Array &p_tile_height_data);
	Array get_tile_height_data_array() const { return tile_height_data; }
	void set_tile_layer_data_array(const Array &p_tile_layer_data);
	Array get_tile_layer_data_array() const { return tile_layer_data; }

	bool has_tile(const Vector2i &p_cell) const;
	void create_tile(const Vector2i &p_cell);
	void remove_tile(const Vector2i &p_cell);
	PackedFloat32Array get_tile_height_data(const Vector2i &p_cell) const;
	void set_tile_height_data(const Vector2i &p_cell, const PackedFloat32Array &p_height_data);
	void set_tile_height_data_no_notify(const Vector2i &p_cell, const PackedFloat32Array &p_height_data);
	PackedColorArray get_tile_layer_data(const Vector2i &p_cell) const;
	void set_tile_layer_data(const Vector2i &p_cell, const PackedColorArray &p_layer_data);
	void set_tile_layer_data_no_notify(const Vector2i &p_cell, const PackedColorArray &p_layer_data);
	int get_tile_height_index(int p_x, int p_y) const;
	real_t get_tile_height(const Vector2i &p_cell, int p_x, int p_y) const;
	void set_tile_height(const Vector2i &p_cell, int p_x, int p_y, real_t p_height);
	Color get_tile_layer(const Vector2i &p_cell, int p_x, int p_y) const;
	void set_tile_layer(const Vector2i &p_cell, int p_x, int p_y, const Color &p_layer);

	void resize(int p_resolution, real_t p_world_size, bool p_clear_existing = false);
	void fill_flat(real_t p_normalized_height = 0.0);
	int get_height_index(int p_x, int p_y) const;
	real_t get_height(int p_x, int p_y) const;
	void set_height(int p_x, int p_y, real_t p_height);
	Color get_layer(int p_x, int p_y) const;
	void set_layer(int p_x, int p_y, const Color &p_layer);

	// Builds a single-channel 32-bit float image from the normalized height data.
	// OpenWorldTerrain3D uploads this image as a texture, then the vertex shader
	// samples it for displacement. Keeping this conversion in the resource makes
	// the storage contract visible to scripts and future streaming code.
	Ref<Image> create_height_image() const;
	Ref<Image> create_layer_image() const;

	OpenWorldTerrainData();
};
