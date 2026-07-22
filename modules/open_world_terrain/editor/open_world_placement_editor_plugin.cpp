/**************************************************************************/
/*  open_world_placement_editor_plugin.cpp                                */
/**************************************************************************/

#include "open_world_placement_editor_plugin.h"

#include "modules/simple_terrain/simple_terrain_3d.h"

#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/main/viewport.h"

void OpenWorldPlacementEditorPlugin::_select_mode_pressed() {
	apply_mode = false;
	_clear_cursor();
	_update_toolbar();
}

void OpenWorldPlacementEditorPlugin::_apply_mode_pressed() {
	apply_mode = placement != nullptr;
	_update_toolbar();
}

void OpenWorldPlacementEditorPlugin::_presets_pressed() {
	if (preset_dock != nullptr) {
		EditorDockManager::get_singleton()->focus_dock(preset_dock);
	}
}

void OpenWorldPlacementEditorPlugin::_update_toolbar() {
	if (toolbar == nullptr || select_mode_button == nullptr || apply_mode_button == nullptr || overlay_panel == nullptr) {
		return;
	}
	const bool has_placement = placement != nullptr;
	toolbar->set_visible(has_placement);
	select_mode_button->set_disabled(!has_placement);
	apply_mode_button->set_disabled(!has_placement);
	select_mode_button->set_pressed_no_signal(has_placement && !apply_mode);
	apply_mode_button->set_pressed_no_signal(has_placement && apply_mode);
	overlay_panel->set_visible(has_placement && apply_mode);
	_update_overlay();
}

void OpenWorldPlacementEditorPlugin::_attach_overlay() {
	Node3DEditorViewport *viewport = Node3DEditor::get_singleton()->get_editor_viewport(0);
	if (viewport == nullptr || overlay_panel->get_parent() != nullptr) {
		return;
	}
	Control *surface = viewport->get_surface();
	if (surface == nullptr) {
		return;
	}
	surface->add_child(overlay_panel);
	overlay_panel->move_to_front();
}

void OpenWorldPlacementEditorPlugin::_detach_overlay() {
	if (overlay_panel != nullptr && overlay_panel->get_parent() != nullptr) {
		overlay_panel->get_parent()->remove_child(overlay_panel);
	}
}

void OpenWorldPlacementEditorPlugin::_update_overlay() {
	if (status_label == nullptr || preview_label == nullptr) {
		return;
	}
	if (placement == nullptr) {
		status_label->set_text(TTRC("No OpenWorldPlacement3D selected."));
		preview_label->set_text(String());
		return;
	}
	const Ref<OpenWorldPlacementPreset> preset = placement->get_active_preset();
	if (preset.is_null()) {
		status_label->set_text(TTRC("Assign an active placement preset."));
	} else {
		const String title = preset->get_display_name().is_empty() ? preset->get_stable_id() : preset->get_display_name();
		status_label->set_text(vformat(TTR("Apply: %s  |  seed %d  |  requested %d"), title, placement->get_default_seed(), preset->get_requested_object_count()));
	}
	if (last_preview_report.is_empty()) {
		preview_label->set_text(TTRC("Click to replace the footprint region (flat plane if no terrain)."));
	} else if ((bool)last_preview_report.get("success", false)) {
		preview_label->set_text(vformat(TTR("Preview accepted %d / replace %d"), (int)last_preview_report.get("accepted_count", 0), (int)last_preview_report.get("replacement_count", 0)));
	} else {
		PackedStringArray codes = last_preview_report.get("error_codes", PackedStringArray());
		preview_label->set_text(codes.is_empty() ? TTRC("Preview failed.") : vformat(TTR("Preview failed: %s"), String(", ").join(codes)));
	}
}

SimpleTerrain3D *OpenWorldPlacementEditorPlugin::_resolve_terrain() const {
	if (placement == nullptr || !placement->is_inside_tree()) {
		return nullptr;
	}
	Node *node = placement->get_node_or_null(placement->get_terrain_path());
	return Object::cast_to<SimpleTerrain3D>(node);
}

Dictionary OpenWorldPlacementEditorPlugin::_get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const {
	if (p_camera == nullptr) {
		return Dictionary();
	}
	const Vector3 ray_origin = p_camera->project_ray_origin(p_mouse_position);
	const Vector3 ray_direction = p_camera->project_ray_normal(p_mouse_position);

	SimpleTerrain3D *terrain = _resolve_terrain();
	if (terrain != nullptr) {
		Dictionary terrain_hit = terrain->get_brush_hit(ray_origin, ray_direction);
		if (!terrain_hit.is_empty() && terrain_hit.has("position")) {
			return terrain_hit;
		}
	}

	// Flat-plane fallback when terrain_path is empty or the ray misses terrain.
	const real_t plane_y = placement != nullptr ? placement->get_global_position().y : 0.0;
	if (Math::is_zero_approx(ray_direction.y)) {
		return Dictionary();
	}
	const real_t t = (plane_y - ray_origin.y) / ray_direction.y;
	if (t < 0.0) {
		return Dictionary();
	}
	Dictionary hit;
	hit["position"] = ray_origin + ray_direction * t;
	hit["normal"] = Vector3(0.0, 1.0, 0.0);
	return hit;
}

void OpenWorldPlacementEditorPlugin::_restore_placement_snapshot(Object *p_placement_object, const Ref<OpenWorldPlacementData> &p_snapshot) {
	OpenWorldPlacement3D *node = Object::cast_to<OpenWorldPlacement3D>(p_placement_object);
	if (node == nullptr) {
		return;
	}
	Ref<OpenWorldPlacementData> data = node->get_placement_data();
	if (data.is_null()) {
		data.instantiate();
		node->set_placement_data(data);
	}
	data->assign_from(p_snapshot);
	node->rebuild_generated();
}

void OpenWorldPlacementEditorPlugin::_apply_at_mouse(Camera3D *p_camera, const Vector2 &p_mouse_position) {
	if (placement == nullptr || placement->get_active_preset().is_null()) {
		WARN_PRINT("World Placement apply requires OpenWorldPlacement3D with an active preset.");
		return;
	}
	const Dictionary hit = _get_hit(p_camera, p_mouse_position);
	if (hit.is_empty() || !hit.has("position")) {
		return;
	}
	const Vector3 world_position = hit["position"];

	Ref<OpenWorldPlacementData> placement_data_ref = placement->get_placement_data();
	if (placement_data_ref.is_null()) {
		placement_data_ref.instantiate();
		placement->set_placement_data(placement_data_ref);
	}
	Ref<OpenWorldPlacementData> before_snapshot;
	before_snapshot.instantiate();
	before_snapshot->assign_from(placement_data_ref);

	const Dictionary report = placement->apply_placement(world_position, Ref<OpenWorldPlacementPreset>(), 0);
	last_preview_report = report;
	_update_overlay();
	if (!(bool)report.get("success", false)) {
		return;
	}

	Ref<OpenWorldPlacementData> after_snapshot;
	after_snapshot.instantiate();
	after_snapshot->assign_from(placement->get_placement_data());

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Apply World Placement"));
	undo_redo->add_do_method(this, "_restore_placement_snapshot", placement, after_snapshot);
	undo_redo->add_undo_method(this, "_restore_placement_snapshot", placement, before_snapshot);
	undo_redo->add_do_reference(after_snapshot.ptr());
	undo_redo->add_undo_reference(before_snapshot.ptr());
	undo_redo->commit_action(false);
}

void OpenWorldPlacementEditorPlugin::_update_cursor(Camera3D *p_camera, const Dictionary &p_hit) {
	cursor_points.clear();
	has_cursor_hit = false;
	if (p_camera == nullptr || placement == nullptr || placement->get_active_preset().is_null() || p_hit.is_empty() || !p_hit.has("position")) {
		update_overlays();
		return;
	}
	const Ref<OpenWorldPlacementPreset> preset = placement->get_active_preset();
	SimpleTerrain3D *terrain = _resolve_terrain();
	cursor_center = p_hit["position"];

	const Vector2 size = preset->get_size();
	const real_t yaw = Math::deg_to_rad(preset->get_yaw_degrees());
	const real_t cos_y = Math::cos(yaw);
	const real_t sin_y = Math::sin(yaw);
	const int segments = 48;
	for (int i = 0; i <= segments; i++) {
		const real_t t = (real_t)i / (real_t)segments;
		Vector2 local;
		switch (preset->get_shape()) {
			case OpenWorldPlacementPreset::SHAPE_RECTANGLE: {
				const real_t hx = size.x * 0.5;
				const real_t hz = size.y * 0.5;
				const int side = MIN(3, (i * 4) / segments);
				const int side_segments = MAX(1, segments / 4);
				const real_t u = (real_t)(i % side_segments) / (real_t)side_segments;
				if (side == 0) {
					local = Vector2(Math::lerp(-hx, hx, u), -hz);
				} else if (side == 1) {
					local = Vector2(hx, Math::lerp(-hz, hz, u));
				} else if (side == 2) {
					local = Vector2(Math::lerp(hx, -hx, u), hz);
				} else {
					local = Vector2(-hx, Math::lerp(hz, -hz, u));
				}
			} break;
			case OpenWorldPlacementPreset::SHAPE_ELLIPSE: {
				const real_t angle = t * Math::TAU;
				local = Vector2(Math::cos(angle) * size.x * 0.5, Math::sin(angle) * size.y * 0.5);
			} break;
			case OpenWorldPlacementPreset::SHAPE_CIRCLE:
			default: {
				const real_t angle = t * Math::TAU;
				const real_t radius = size.x * 0.5;
				local = Vector2(Math::cos(angle) * radius, Math::sin(angle) * radius);
			} break;
		}
		const Vector2 rotated = Vector2(local.x * cos_y - local.y * sin_y, local.x * sin_y + local.y * cos_y);
		Vector3 world = Vector3(cursor_center.x + rotated.x, cursor_center.y, cursor_center.z + rotated.y);
		if (terrain != nullptr) {
			const Dictionary sample = terrain->sample_surface_at_world_xz(world);
			if ((bool)sample.get("success", false)) {
				world = sample["position"];
				world.y += 0.05;
			}
		}
		if (p_camera->is_position_behind(world)) {
			cursor_points.clear();
			update_overlays();
			return;
		}
		cursor_points.push_back(p_camera->unproject_position(world));
	}
	has_cursor_hit = cursor_points.size() > 1;
	update_overlays();
}

void OpenWorldPlacementEditorPlugin::_clear_cursor() {
	if (!has_cursor_hit && cursor_points.is_empty()) {
		return;
	}
	has_cursor_hit = false;
	cursor_points.clear();
	update_overlays();
}

void OpenWorldPlacementEditorPlugin::_draw_over_viewport(Control *p_overlay) {
	if (!apply_mode || !has_cursor_hit || cursor_points.size() < 2 || p_overlay == nullptr) {
		return;
	}
	p_overlay->draw_polyline(cursor_points, Color(0.2, 0.85, 0.55, 0.9), 2.0 * EDSCALE, true);
}

void OpenWorldPlacementEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			preset_dock = memnew(OpenWorldPlacementPresetDock);
			add_dock(preset_dock);
			preset_dock->close();
			add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			_attach_overlay();
			set_input_event_forwarding_always_enabled();
			set_force_draw_over_forwarding_enabled();
			select_mode_button->set_button_icon(select_mode_button->get_editor_theme_icon(SNAME("ToolSelect")));
			apply_mode_button->set_button_icon(apply_mode_button->get_editor_theme_icon(SNAME("MeshInstance3D")));
			_update_toolbar();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_detach_overlay();
			remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			if (preset_dock != nullptr) {
				remove_dock(preset_dock);
				preset_dock = nullptr;
			}
		} break;
	}
}

void OpenWorldPlacementEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_restore_placement_snapshot", "placement", "snapshot"), &OpenWorldPlacementEditorPlugin::_restore_placement_snapshot);
}

bool OpenWorldPlacementEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<OpenWorldPlacement3D>(p_object) != nullptr;
}

void OpenWorldPlacementEditorPlugin::edit(Object *p_object) {
	placement = Object::cast_to<OpenWorldPlacement3D>(p_object);
	if (placement == nullptr) {
		apply_mode = false;
		_clear_cursor();
		last_preview_report = Dictionary();
	}
	if (preset_dock != nullptr) {
		preset_dock->edit(placement);
	}
	_update_toolbar();
}

void OpenWorldPlacementEditorPlugin::clear() {
	placement = nullptr;
	apply_mode = false;
	_clear_cursor();
	last_preview_report = Dictionary();
	if (preset_dock != nullptr) {
		preset_dock->edit(nullptr);
	}
	_update_toolbar();
}

EditorPlugin::AfterGUIInput OpenWorldPlacementEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!apply_mode || placement == nullptr) {
		_clear_cursor();
		return AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT && mouse_button->is_pressed() && !mouse_button->is_echo()) {
		_apply_at_mouse(p_camera, mouse_button->get_position());
		return AFTER_GUI_INPUT_STOP;
	}

	Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid()) {
		const Dictionary hit = _get_hit(p_camera, mouse_motion->get_position());
		_update_cursor(p_camera, hit);
		if (!hit.is_empty() && hit.has("position") && placement->get_active_preset().is_valid()) {
			last_preview_report = placement->preview_placement(hit["position"], Ref<OpenWorldPlacementPreset>(), 0);
			_update_overlay();
		}
		return AFTER_GUI_INPUT_PASS;
	}

	return AFTER_GUI_INPUT_PASS;
}

void OpenWorldPlacementEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
	(void)p_overlay;
}

void OpenWorldPlacementEditorPlugin::forward_3d_force_draw_over_viewport(Control *p_overlay) {
	_draw_over_viewport(p_overlay);
}

OpenWorldPlacementEditorPlugin::OpenWorldPlacementEditorPlugin() {
	toolbar = memnew(HBoxContainer);
	toolbar->hide();

	overlay_panel = memnew(PanelContainer);
	overlay_panel->hide();
	overlay_panel->set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT, Control::PRESET_MODE_MINSIZE, 12 * EDSCALE);
	overlay_panel->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
	overlay_panel->set_custom_minimum_size(Size2(260, 0) * EDSCALE);

	overlay_vbox = memnew(VBoxContainer);
	overlay_vbox->add_theme_constant_override("separation", 6 * EDSCALE);
	overlay_panel->add_child(overlay_vbox);

	status_label = memnew(Label);
	status_label->set_text(TTRC("World Placement"));
	status_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	overlay_vbox->add_child(status_label);

	preview_label = memnew(Label);
	preview_label->set_text(TTRC("Click to apply area replacement."));
	preview_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	overlay_vbox->add_child(preview_label);

	mode_button_group.instantiate();

	select_mode_button = memnew(Button);
	select_mode_button->set_toggle_mode(true);
	select_mode_button->set_button_group(mode_button_group);
	select_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	select_mode_button->set_tooltip_text(TTRC("Select mode."));
	select_mode_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementEditorPlugin::_select_mode_pressed));
	toolbar->add_child(select_mode_button);

	apply_mode_button = memnew(Button);
	apply_mode_button->set_toggle_mode(true);
	apply_mode_button->set_button_group(mode_button_group);
	apply_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	apply_mode_button->set_tooltip_text(TTRC("Click terrain to apply World Placement (area replacement)."));
	apply_mode_button->set_accessibility_name(TTRC("World Placement Apply Mode"));
	apply_mode_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementEditorPlugin::_apply_mode_pressed));
	toolbar->add_child(apply_mode_button);

	presets_button = memnew(Button);
	presets_button->set_theme_type_variation(SceneStringName(FlatButton));
	presets_button->set_text(TTRC("Presets"));
	presets_button->set_tooltip_text(TTRC("Open the World Placement preset dock."));
	presets_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementEditorPlugin::_presets_pressed));
	toolbar->add_child(presets_button);
}
