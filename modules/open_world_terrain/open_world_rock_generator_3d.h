/**************************************************************************/
/*  open_world_rock_generator_3d.h                                        */
/**************************************************************************/
#pragma once

#include "open_world_rock_generation_request.h"
#include "open_world_rock_topology_data.h"
#include "open_world_rock_variant.h"

#include "scene/3d/mesh_instance_3d.h"

class OpenWorldRockGenerator3D : public MeshInstance3D {
	GDCLASS(OpenWorldRockGenerator3D, MeshInstance3D);

public:
	enum PreviewLOD { PREVIEW_LOD0, PREVIEW_LOD1, PREVIEW_LOD2 };

private:
	Ref<OpenWorldRockGenerationRequest> generation_request;
	Ref<OpenWorldRockTopologyData> generated_topology;
	Ref<OpenWorldRockTopologyData> generated_lod_topologies[3];
	Ref<ArrayMesh> generated_lod_meshes[3];
	Ref<Shape3D> generated_collision_shape;
	PackedVector3Array generated_collision_points;
	Ref<Material> preview_material;
	Dictionary generation_report;
	bool auto_generate = true;
	PreviewLOD preview_lod = PREVIEW_LOD0;
	real_t lod1_distance = 20.0;
	real_t lod2_distance = 45.0;
	real_t max_distance = 120.0;

	void _request_changed();
	PackedVector3Array _generate_source_points() const;
	PackedVector3Array _subset_points(const PackedVector3Array &p_points, int p_target) const;
	Ref<OpenWorldRockTopologyData> _build_topology(const PackedVector3Array &p_points, int p_seed) const;
	Ref<ArrayMesh> _build_mesh(const Ref<OpenWorldRockTopologyData> &p_topology) const;
	void _update_preview_mesh();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_generation_request(const Ref<OpenWorldRockGenerationRequest> &p_value);
	Ref<OpenWorldRockGenerationRequest> get_generation_request() const { return generation_request; }
	void set_auto_generate(bool p_value); bool is_auto_generate() const { return auto_generate; }
	void set_preview_lod(PreviewLOD p_value); PreviewLOD get_preview_lod() const { return preview_lod; }
	void set_preview_material(const Ref<Material> &p_value); Ref<Material> get_preview_material() const { return preview_material; }
	void set_lod1_distance(real_t p_value); real_t get_lod1_distance() const { return lod1_distance; }
	void set_lod2_distance(real_t p_value); real_t get_lod2_distance() const { return lod2_distance; }
	void set_max_distance(real_t p_value); real_t get_max_distance() const { return max_distance; }

	Dictionary validate_request() const;
	Ref<OpenWorldRockTopologyData> generate_topology();
	void generate_rock();
	void clear_generated_rock();
	Ref<OpenWorldRockTopologyData> get_generated_topology() const { return generated_topology; }
	Ref<ArrayMesh> get_generated_lod_mesh(int p_lod) const;
	Ref<Shape3D> get_generated_collision_shape() const { return generated_collision_shape; }
	PackedVector3Array get_generated_collision_points() const { return generated_collision_points; }
	Dictionary get_lod_statistics(int p_lod) const;
	Dictionary get_generation_report() const { return generation_report; }
	Ref<OpenWorldRockVariant> create_baked_variant() const;

	OpenWorldRockGenerator3D();
	~OpenWorldRockGenerator3D();
};
VARIANT_ENUM_CAST(OpenWorldRockGenerator3D::PreviewLOD);
