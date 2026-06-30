/**************************************************************************/
/*  simple_terrain_data.h                                                        */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class SimpleTerrainData : public Resource {
	GDCLASS(SimpleTerrainData, Resource);
	RES_BASE_EXTENSION("simpleterraindata");

	// SimpleTerrainData stores only tile-based height fields. Rendering, chunking,
	// brush logic, material selection, and editor tools intentionally live in
	// SimpleTerrain3D/SimpleTerrainEditorPlugin so this resource can stay serializable,
	// reusable, and cheap to duplicate.
	real_t cell_size = 1.0;
	int tile_size = 128;

	// Tile heights are stored in row-major order per tile:
	// index = z * (tile_size + 1) + x. Each created_tile_cells entry owns the
	// PackedFloat32Array at the same index in tile_height_data.
	PackedVector2Array created_tile_cells;
	Array tile_height_data;

	// A tile with N quads needs N + 1 vertices on each axis.
	_FORCE_INLINE_ int _get_required_tile_height_count() const { return (tile_size + 1) * (tile_size + 1); }
	int _get_tile_array_index(const Vector2i &p_cell) const;
	PackedFloat32Array _make_flat_tile_height_data(real_t p_height = 0.0) const;
	void _ensure_tile_height_data_size(bool p_clear_existing);

protected:
	static void _bind_methods();

public:
	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const { return cell_size; }

	void set_tile_size(int p_tile_size);
	int get_tile_size() const { return tile_size; }

	void set_created_tile_cells(const PackedVector2Array &p_cells);
	PackedVector2Array get_created_tile_cells() const { return created_tile_cells; }
	void set_tile_height_data_array(const Array &p_tile_height_data);
	Array get_tile_height_data_array() const { return tile_height_data; }
	void notify_height_data_changed();
	void fill_flat(real_t p_height = 0.0);

	int get_tile_vertex_count() const { return tile_size + 1; }
	bool has_tile(const Vector2i &p_cell) const;
	void create_tile(const Vector2i &p_cell);
	void remove_tile(const Vector2i &p_cell);
	PackedFloat32Array get_tile_height_data(const Vector2i &p_cell) const;
	void set_tile_height_data(const Vector2i &p_cell, const PackedFloat32Array &p_height_data);
	void set_tile_height_data_no_notify(const Vector2i &p_cell, const PackedFloat32Array &p_height_data);
	int get_tile_height_index(int p_x, int p_z) const;
	real_t get_tile_height(const Vector2i &p_cell, int p_x, int p_z) const;
	void set_tile_height(const Vector2i &p_cell, int p_x, int p_z, real_t p_height);

	SimpleTerrainData();
};
