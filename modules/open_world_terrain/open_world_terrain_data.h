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
	real_t height_scale = 128.0;
	PackedFloat32Array height_data;

	_FORCE_INLINE_ int _get_required_height_count() const { return heightmap_resolution * heightmap_resolution; }
	void _ensure_height_data_size(bool p_clear_existing);

protected:
	static void _bind_methods();

public:
	void set_heightmap_resolution(int p_resolution);
	int get_heightmap_resolution() const { return heightmap_resolution; }

	void set_world_size(real_t p_world_size);
	real_t get_world_size() const { return world_size; }

	void set_height_scale(real_t p_height_scale);
	real_t get_height_scale() const { return height_scale; }

	void set_height_data(const PackedFloat32Array &p_height_data);
	PackedFloat32Array get_height_data() const { return height_data; }
	PackedFloat32Array &get_mutable_height_data() { return height_data; }
	void notify_height_data_changed();

	void resize(int p_resolution, real_t p_world_size, bool p_clear_existing = false);
	void fill_flat(real_t p_normalized_height = 0.0);
	int get_height_index(int p_x, int p_y) const;
	real_t get_height(int p_x, int p_y) const;
	void set_height(int p_x, int p_y, real_t p_height);

	// Builds a single-channel 32-bit float image from the normalized height data.
	// OpenWorldTerrain3D uploads this image as a texture, then the vertex shader
	// samples it for displacement. Keeping this conversion in the resource makes
	// the storage contract visible to scripts and future streaming code.
	Ref<Image> create_height_image() const;

	OpenWorldTerrainData();
};
