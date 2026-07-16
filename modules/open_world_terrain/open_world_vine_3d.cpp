/**************************************************************************/
/*  open_world_vine_3d.cpp                                                */
/**************************************************************************/
#include "open_world_vine_3d.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"

void OpenWorldVine3D::_variant_changed() { _update_lod(); }
Camera3D *OpenWorldVine3D::_resolve_camera() const { if (!is_inside_tree()) return nullptr; if (!camera_path.is_empty()) return Object::cast_to<Camera3D>(get_node_or_null(camera_path)); Viewport *viewport = get_viewport(); return viewport ? viewport->get_camera_3d() : nullptr; }
void OpenWorldVine3D::_update_lod() { if (variant.is_null()) { current_lod = -1; set_mesh(Ref<Mesh>()); return; } Camera3D *camera = _resolve_camera(); real_t distance = camera ? camera->get_global_position().distance_to(get_global_position()) : 0.0; int lod = variant->get_lod_index_for_distance(distance); if (lod == current_lod) return; current_lod = lod; set_visible(lod >= 0); set_mesh(lod >= 0 ? variant->get_lod_mesh(lod) : Ref<Mesh>()); }
void OpenWorldVine3D::set_variant(const Ref<OpenWorldVineVariant> &p_variant) { if (variant == p_variant) return; if (variant.is_valid()) variant->disconnect_changed(callable_mp(this, &OpenWorldVine3D::_variant_changed)); variant = p_variant; if (variant.is_valid()) variant->connect_changed(callable_mp(this, &OpenWorldVine3D::_variant_changed)); current_lod = -1; _update_lod(); }
void OpenWorldVine3D::set_camera_path(const NodePath &p_path) { camera_path = p_path; force_lod_update(); }
void OpenWorldVine3D::set_lod_update_interval(real_t p_value) { lod_update_interval = MAX((real_t)0.01, p_value); }
void OpenWorldVine3D::force_lod_update() { current_lod = -1; _update_lod(); }
void OpenWorldVine3D::notify_support_lost() { if (variant.is_null()) return; if (variant->get_support_lost_policy() == OpenWorldVineVariant::SUPPORT_HIDE) hide(); else if (variant->get_support_lost_policy() == OpenWorldVineVariant::SUPPORT_DETACH) emit_signal(SNAME("support_detach_requested")); }
void OpenWorldVine3D::_notification(int p_what) { if (p_what == NOTIFICATION_ENTER_TREE) { set_process(true); force_lod_update(); } else if (p_what == NOTIFICATION_PROCESS) { lod_elapsed += get_process_delta_time(); if (lod_elapsed >= lod_update_interval) { lod_elapsed = 0.0; _update_lod(); } } }
OpenWorldVine3D::~OpenWorldVine3D() { if (variant.is_valid()) variant->disconnect_changed(callable_mp(this, &OpenWorldVine3D::_variant_changed)); }
void OpenWorldVine3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_variant", "variant"), &OpenWorldVine3D::set_variant); ClassDB::bind_method(D_METHOD("get_variant"), &OpenWorldVine3D::get_variant);
	ClassDB::bind_method(D_METHOD("set_camera_path", "path"), &OpenWorldVine3D::set_camera_path); ClassDB::bind_method(D_METHOD("get_camera_path"), &OpenWorldVine3D::get_camera_path);
	ClassDB::bind_method(D_METHOD("set_lod_update_interval", "interval"), &OpenWorldVine3D::set_lod_update_interval); ClassDB::bind_method(D_METHOD("get_lod_update_interval"), &OpenWorldVine3D::get_lod_update_interval);
	ClassDB::bind_method(D_METHOD("force_lod_update"), &OpenWorldVine3D::force_lod_update); ClassDB::bind_method(D_METHOD("get_current_lod"), &OpenWorldVine3D::get_current_lod); ClassDB::bind_method(D_METHOD("notify_support_lost"), &OpenWorldVine3D::notify_support_lost);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "variant", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldVineVariant"), "set_variant", "get_variant"); ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "camera_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera3D"), "set_camera_path", "get_camera_path"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod_update_interval", PROPERTY_HINT_RANGE, "0.01,10,0.01,suffix:s"), "set_lod_update_interval", "get_lod_update_interval");
	ADD_SIGNAL(MethodInfo("support_detach_requested"));
}
