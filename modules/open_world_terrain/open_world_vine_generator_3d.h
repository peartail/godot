/**************************************************************************/
/*  open_world_vine_generator_3d.h                                        */
/**************************************************************************/
#pragma once

#include "open_world_vine_generation_request.h"
#include "open_world_vine_path_data.h"
#include "open_world_vine_variant.h"

#include "scene/3d/mesh_instance_3d.h"

class OpenWorldVineGenerator3D : public MeshInstance3D {
	GDCLASS(OpenWorldVineGenerator3D, MeshInstance3D);
public:
	enum PreviewLOD { PREVIEW_LOD0, PREVIEW_LOD1, PREVIEW_LOD2 };
private:
	Ref<OpenWorldVineGenerationRequest> generation_request;
	Ref<OpenWorldVinePathData> generated_path;
	Ref<ArrayMesh> generated_lod_meshes[3];
	Ref<Material> stem_material;
	Ref<Material> foliage_material;
	Ref<ShaderMaterial> wind_stem_material;
	Ref<ShaderMaterial> wind_foliage_material;
	Dictionary generation_report;
	bool auto_generate = true;
	bool wind_enabled = false;
	PreviewLOD preview_lod = PREVIEW_LOD0;
	real_t lod1_distance = 18.0;
	real_t lod2_distance = 42.0;
	real_t max_distance = 90.0;

	void _request_changed();
	void _update_preview_mesh();
	void _update_materials();
	Ref<ArrayMesh> _generate_lod_mesh(int p_lod) const;
	bool _project_to_support(Node *p_support, const Vector3 &p_origin, const Vector3 &p_direction, real_t p_distance, Vector3 &r_position, Vector3 &r_normal) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_generation_request(const Ref<OpenWorldVineGenerationRequest> &p_request);
	Ref<OpenWorldVineGenerationRequest> get_generation_request() const { return generation_request; }
	void set_auto_generate(bool p_enabled);
	bool is_auto_generate() const { return auto_generate; }
	void set_stem_material(const Ref<Material> &p_material);
	Ref<Material> get_stem_material() const { return stem_material; }
	void set_foliage_material(const Ref<Material> &p_material);
	Ref<Material> get_foliage_material() const { return foliage_material; }
	void set_wind_enabled(bool p_enabled);
	bool is_wind_enabled() const { return wind_enabled; }
	void set_preview_lod(PreviewLOD p_lod);
	PreviewLOD get_preview_lod() const { return preview_lod; }
	void set_lod1_distance(real_t p_value);
	real_t get_lod1_distance() const { return lod1_distance; }
	void set_lod2_distance(real_t p_value);
	real_t get_lod2_distance() const { return lod2_distance; }
	void set_max_distance(real_t p_value);
	real_t get_max_distance() const { return max_distance; }

	Dictionary validate_request() const;
	Ref<OpenWorldVinePathData> generate_path();
	void generate_vine();
	void randomize_seed();
	void clear_generated_vine();
	Ref<OpenWorldVinePathData> get_generated_path() const { return generated_path; }
	Ref<ArrayMesh> get_generated_lod_mesh(int p_lod) const;
	Dictionary get_lod_statistics(int p_lod) const;
	Dictionary get_generation_report() const { return generation_report; }
	Ref<OpenWorldVineVariant> create_baked_variant() const;

	OpenWorldVineGenerator3D();
	~OpenWorldVineGenerator3D();
};

VARIANT_ENUM_CAST(OpenWorldVineGenerator3D::PreviewLOD);
