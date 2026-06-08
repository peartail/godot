/**************************************************************************/
/*  open_world_terrain_editor_plugin.cpp                                  */
/**************************************************************************/

#include "open_world_terrain_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/option_button.h"

bool OpenWorldTerrain3DGizmoPlugin::has_gizmo(Node3D *p_spatial) {
	return Object::cast_to<OpenWorldTerrain3D>(p_spatial) != nullptr;
}

String OpenWorldTerrain3DGizmoPlugin::get_gizmo_name() const {
	return "OpenWorldTerrain3D";
}

int OpenWorldTerrain3DGizmoPlugin::get_priority() const {
	return -1;
}

void OpenWorldTerrain3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	OpenWorldTerrain3D *terrain_node = Object::cast_to<OpenWorldTerrain3D>(p_gizmo->get_node_3d());
	p_gizmo->clear();

	if (terrain_node == nullptr || !terrain_node->is_showing_debug_gizmo()) {
		return;
	}

	const PackedVector3Array lines = terrain_node->get_debug_lines();
	if (!lines.is_empty()) {
		p_gizmo->add_lines(lines, get_material("open_world_bounds", p_gizmo));
	}
}

OpenWorldTerrain3DGizmoPlugin::OpenWorldTerrain3DGizmoPlugin() {
	create_material("open_world_bounds", Color(0.95, 0.65, 0.12, 0.9), false, true);
}

void OpenWorldTerrainEditorPlugin::_mode_toggled(bool p_pressed) {
	terrain_mode = p_pressed && terrain != nullptr;
	_update_toolbar();
}

void OpenWorldTerrainEditorPlugin::_flat_pressed() {
	if (terrain == nullptr || terrain->get_terrain_data().is_null()) {
		return;
	}

	const PackedFloat32Array before_heights = terrain->get_terrain_data()->get_height_data();
	terrain->reset_flat_terrain(flatten_slider->get_value());
	_commit_full_height_undo(TTR("Reset OpenWorld Terrain"), before_heights);
}

void OpenWorldTerrainEditorPlugin::_random_pressed() {
	if (terrain == nullptr || terrain->get_terrain_data().is_null()) {
		return;
	}

	const PackedFloat32Array before_heights = terrain->get_terrain_data()->get_height_data();
	terrain->generate_random_terrain(random_seed++, 1.0, 0.025, 4);
	_commit_full_height_undo(TTR("Generate OpenWorld Terrain"), before_heights);
}

void OpenWorldTerrainEditorPlugin::_rebuild_pressed() {
	if (terrain != nullptr) {
		terrain->rebuild();
	}
}

void OpenWorldTerrainEditorPlugin::_update_toolbar() {
	const bool has_terrain = terrain != nullptr;
	toolbar->set_visible(has_terrain);
	mode_button->set_disabled(!has_terrain);
	mode_button->set_pressed_no_signal(terrain_mode && has_terrain);
	operation_button->set_disabled(!terrain_mode);
	radius_slider->set_read_only(!terrain_mode);
	strength_slider->set_read_only(!terrain_mode);
	flatten_slider->set_read_only(!terrain_mode);
	flat_button->set_disabled(!has_terrain);
	random_button->set_disabled(!has_terrain);
	rebuild_button->set_disabled(!has_terrain);
}

void OpenWorldTerrainEditorPlugin::_apply_brush(const Vector3 &p_world_position) {
	if (terrain == nullptr || terrain->get_terrain_data().is_null()) {
		return;
	}

	const real_t radius = radius_slider->get_value();
	const Ref<OpenWorldTerrainData> terrain_data = terrain->get_terrain_data();
	const real_t texel_world_size = terrain_data->get_world_size() / MAX(1.0, (real_t)terrain_data->get_heightmap_resolution() - 1.0);
	const real_t min_spacing = MAX(texel_world_size * 0.5, radius * 0.05);
	if (has_last_brush_position && last_brush_position.distance_to(p_world_position) < min_spacing) {
		return;
	}

	last_brush_position = p_world_position;
	has_last_brush_position = true;
	terrain->set_flatten_height(flatten_slider->get_value());
	const OpenWorldTerrain3D::BrushOperation operation = (OpenWorldTerrain3D::BrushOperation)operation_button->get_selected_id();
	_record_brush_delta(terrain->apply_brush_with_delta(p_world_position, radius, strength_slider->get_value(), operation));
}

void OpenWorldTerrainEditorPlugin::_record_brush_delta(const Dictionary &p_delta) {
	if (p_delta.is_empty()) {
		return;
	}

	const PackedInt32Array indices = p_delta["indices"];
	const PackedFloat32Array before_values = p_delta["before"];
	const PackedFloat32Array after_values = p_delta["after"];
	ERR_FAIL_COND(indices.size() != before_values.size());
	ERR_FAIL_COND(indices.size() != after_values.size());

	for (int i = 0; i < indices.size(); i++) {
		const int index = indices[i];
		HashMap<int, int>::Iterator E = stroke_index_map.find(index);
		if (E) {
			stroke_after_values.set(E->value, after_values[i]);
			continue;
		}

		stroke_index_map.insert(index, stroke_indices.size());
		stroke_indices.push_back(index);
		stroke_before_values.push_back(before_values[i]);
		stroke_after_values.push_back(after_values[i]);
	}
}

Dictionary OpenWorldTerrainEditorPlugin::_get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const {
	if (terrain == nullptr || p_camera == nullptr) {
		return Dictionary();
	}

	const Vector3 ray_origin = p_camera->project_ray_origin(p_mouse_position);
	const Vector3 ray_direction = p_camera->project_ray_normal(p_mouse_position);
	return terrain->get_brush_hit(ray_origin, ray_direction);
}

void OpenWorldTerrainEditorPlugin::_commit_full_height_undo(const String &p_action_name, const PackedFloat32Array &p_before_heights) {
	if (terrain == nullptr || terrain->get_terrain_data().is_null()) {
		return;
	}

	const Ref<OpenWorldTerrainData> terrain_data = terrain->get_terrain_data();
	const PackedFloat32Array after_heights = terrain_data->get_height_data();
	if (after_heights == p_before_heights) {
		return;
	}

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(terrain_data.ptr(), "height_data", after_heights);
	undo_redo->add_do_method(terrain, "rebuild");
	undo_redo->add_undo_property(terrain_data.ptr(), "height_data", p_before_heights);
	undo_redo->add_undo_method(terrain, "rebuild");
	undo_redo->commit_action(false);
}

void OpenWorldTerrainEditorPlugin::_commit_stroke_undo() {
	if (terrain == nullptr || stroke_indices.is_empty()) {
		return;
	}

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Paint OpenWorld Terrain"));
	undo_redo->add_do_method(terrain, "apply_height_patch", stroke_indices, stroke_after_values);
	undo_redo->add_undo_method(terrain, "apply_height_patch", stroke_indices, stroke_before_values);
	undo_redo->commit_action(false);
}

void OpenWorldTerrainEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			add_node_3d_gizmo_plugin(gizmo_plugin);
			add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			_update_toolbar();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			remove_node_3d_gizmo_plugin(gizmo_plugin);
		} break;
	}
}

bool OpenWorldTerrainEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<OpenWorldTerrain3D>(p_object) != nullptr;
}

void OpenWorldTerrainEditorPlugin::edit(Object *p_object) {
	terrain = Object::cast_to<OpenWorldTerrain3D>(p_object);
	if (terrain == nullptr) {
		terrain_mode = false;
		painting = false;
	}
	_update_toolbar();
}

void OpenWorldTerrainEditorPlugin::clear() {
	terrain = nullptr;
	terrain_mode = false;
	painting = false;
	_update_toolbar();
}

EditorPlugin::AfterGUIInput OpenWorldTerrainEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!terrain_mode || terrain == nullptr) {
		return AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT) {
		if (mouse_button->is_pressed()) {
			const Dictionary hit = _get_hit(p_camera, mouse_button->get_position());
			if (hit.is_empty()) {
				return AFTER_GUI_INPUT_PASS;
			}

			painting = true;
			has_last_brush_position = false;
			stroke_index_map.clear();
			stroke_indices = PackedInt32Array();
			stroke_before_values = PackedFloat32Array();
			stroke_after_values = PackedFloat32Array();
			_apply_brush(hit["position"]);
			return AFTER_GUI_INPUT_STOP;
		}

		if (painting) {
			painting = false;
			has_last_brush_position = false;
			_commit_stroke_undo();
			stroke_index_map.clear();
			stroke_indices = PackedInt32Array();
			stroke_before_values = PackedFloat32Array();
			stroke_after_values = PackedFloat32Array();
			return AFTER_GUI_INPUT_STOP;
		}
	}

	Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid() && painting) {
		const Dictionary hit = _get_hit(p_camera, mouse_motion->get_position());
		if (!hit.is_empty()) {
			_apply_brush(hit["position"]);
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

OpenWorldTerrainEditorPlugin::OpenWorldTerrainEditorPlugin() {
	gizmo_plugin = Ref<OpenWorldTerrain3DGizmoPlugin>(memnew(OpenWorldTerrain3DGizmoPlugin));

	toolbar = memnew(HBoxContainer);
	toolbar->hide();

	mode_button = memnew(Button);
	mode_button->set_text(TTRC("OpenWorld"));
	mode_button->set_toggle_mode(true);
	mode_button->set_tooltip_text(TTRC("Toggle OpenWorld terrain editing mode."));
	mode_button->connect(SceneStringName(toggled), callable_mp(this, &OpenWorldTerrainEditorPlugin::_mode_toggled));
	toolbar->add_child(mode_button);

	operation_button = memnew(OptionButton);
	operation_button->set_tooltip_text(TTRC("Brush operation."));
	operation_button->add_item(TTRC("Raise"), OpenWorldTerrain3D::BRUSH_RAISE);
	operation_button->add_item(TTRC("Lower"), OpenWorldTerrain3D::BRUSH_LOWER);
	operation_button->add_item(TTRC("Smooth"), OpenWorldTerrain3D::BRUSH_SMOOTH);
	operation_button->add_item(TTRC("Flatten"), OpenWorldTerrain3D::BRUSH_FLATTEN);
	toolbar->add_child(operation_button);

	radius_slider = memnew(EditorSpinSlider);
	radius_slider->set_label(TTRC("Radius"));
	radius_slider->set_min(0.1);
	radius_slider->set_max(512.0);
	radius_slider->set_step(0.1);
	radius_slider->set_value(24.0);
	radius_slider->set_custom_minimum_size(Size2(110, 0) * EDSCALE);
	toolbar->add_child(radius_slider);

	strength_slider = memnew(EditorSpinSlider);
	strength_slider->set_label(TTRC("Strength"));
	strength_slider->set_min(0.001);
	strength_slider->set_max(1.0);
	strength_slider->set_step(0.001);
	strength_slider->set_value(0.05);
	strength_slider->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	toolbar->add_child(strength_slider);

	flatten_slider = memnew(EditorSpinSlider);
	flatten_slider->set_label(TTRC("Flatten"));
	flatten_slider->set_min(0.0);
	flatten_slider->set_max(1.0);
	flatten_slider->set_step(0.001);
	flatten_slider->set_value(0.5);
	flatten_slider->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	toolbar->add_child(flatten_slider);

	flat_button = memnew(Button);
	flat_button->set_text(TTRC("Flat"));
	flat_button->set_tooltip_text(TTRC("Reset selected OpenWorld terrain to the flatten height."));
	flat_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_flat_pressed));
	toolbar->add_child(flat_button);

	random_button = memnew(Button);
	random_button->set_text(TTRC("Random"));
	random_button->set_tooltip_text(TTRC("Generate random OpenWorld terrain on the selected node."));
	random_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_random_pressed));
	toolbar->add_child(random_button);

	rebuild_button = memnew(Button);
	rebuild_button->set_text(TTRC("Rebuild"));
	rebuild_button->set_tooltip_text(TTRC("Rebuild mesh, height texture, and material."));
	rebuild_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_rebuild_pressed));
	toolbar->add_child(rebuild_button);
}
