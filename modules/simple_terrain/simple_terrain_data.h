/**************************************************************************/
/*  simple_terrain_data.h                                                        */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class SimpleTerrainData : public Resource {
	GDCLASS(SimpleTerrainData, Resource);
	RES_BASE_EXTENSION("simpleterraindata");

	// SimpleTerrainData stores only the editable height field. Rendering, chunking,
	// brush logic, material selection, and editor tools intentionally live in
	// SimpleTerrain3D/SimpleTerrainEditorPlugin so this resource can stay serializable,
	// reusable, and cheap to duplicate.
	int grid_size = 64;
	real_t cell_size = 1.0;

	// Heights are stored in row-major order:
	// index = z * (grid_size + 1) + x.
	// The array is exposed through ClassDB so tools, GDScript, C#, undo/redo,
	// and saved .simpleterraindata resources all share the same storage contract.
	PackedFloat32Array height_data;

	// A grid with N quads needs N + 1 vertices on each axis.
	_FORCE_INLINE_ int _get_required_height_count() const { return (grid_size + 1) * (grid_size + 1); }

protected:
	static void _bind_methods();

public:
	void set_grid_size(int p_grid_size);
	int get_grid_size() const { return grid_size; }

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const { return cell_size; }

	void set_height_data(const PackedFloat32Array &p_height_data);
	PackedFloat32Array get_height_data() const { return height_data; }

	// Internal fast path for SimpleTerrain3D brush editing. Scripts should use the
	// property setter so array size validation and change notification happen in
	// one safe public call.
	PackedFloat32Array &get_mutable_height_data() { return height_data; }
	void notify_height_data_changed();

	// Normalizes height_data after any external mutation. If p_clear_existing is
	// true, every height is reset to 0; otherwise the overlapping array prefix is
	// preserved for simple inspector-driven size changes.
	void ensure_height_data_size(bool p_clear_existing = false);
	void resize(int p_grid_size, real_t p_cell_size, bool p_clear_existing = false);
	void fill_flat(real_t p_height = 0.0);

	int get_vertex_count() const { return grid_size + 1; }
	int get_height_index(int p_x, int p_z) const;
	real_t get_height(int p_x, int p_z) const;
	void set_height(int p_x, int p_z, real_t p_height);

	SimpleTerrainData();
};
