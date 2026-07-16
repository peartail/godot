/**************************************************************************/
/*  open_world_vine_3d.h                                                  */
/**************************************************************************/
#pragma once
#include "open_world_vine_variant.h"
#include "scene/3d/mesh_instance_3d.h"

class Camera3D;

class OpenWorldVine3D : public MeshInstance3D {
	GDCLASS(OpenWorldVine3D, MeshInstance3D);
	Ref<OpenWorldVineVariant> variant;
	NodePath camera_path;
	real_t lod_update_interval = 0.25;
	real_t lod_elapsed = 0.0;
	int current_lod = -1;
	void _variant_changed();
	Camera3D *_resolve_camera() const;
	void _update_lod();
protected:
	static void _bind_methods();
	void _notification(int p_what);
public:
	void set_variant(const Ref<OpenWorldVineVariant> &p_variant);
	Ref<OpenWorldVineVariant> get_variant() const { return variant; }
	void set_camera_path(const NodePath &p_path);
	NodePath get_camera_path() const { return camera_path; }
	void set_lod_update_interval(real_t p_value);
	real_t get_lod_update_interval() const { return lod_update_interval; }
	void force_lod_update();
	int get_current_lod() const { return current_lod; }
	void notify_support_lost();
	~OpenWorldVine3D();
};
