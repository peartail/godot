/**************************************************************************/
/*  simple_terrain_editor_plugin.cpp                                             */
/**************************************************************************/

#include "simple_terrain_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/option_button.h"
#include "scene/main/window.h"

bool SimpleTerrain3DGizmoPlugin::has_gizmo(Node3D *p_spatial) {
	return Object::cast_to<SimpleTerrain3D>(p_spatial) != nullptr;
}

String SimpleTerrain3DGizmoPlugin::get_gizmo_name() const {
	return "SimpleTerrain3D";
}

int SimpleTerrain3DGizmoPlugin::get_priority() const {
	return -1;
}

void SimpleTerrain3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	SimpleTerrain3D *terrain_node = Object::cast_to<SimpleTerrain3D>(p_gizmo->get_node_3d());
	p_gizmo->clear();

	if (terrain_node == nullptr || !terrain_node->is_showing_chunk_gizmos()) {
		return;
	}

	// The debug lines are produced by SimpleTerrain3D so the gizmo does not need to
	// know about TerrainChunk internals. This also makes the same data available
	// to scripts or future diagnostic views.
	const PackedVector3Array lines = terrain_node->get_chunk_debug_lines();
	if (lines.is_empty()) {
		return;
	}

	p_gizmo->add_lines(lines, get_material("terrain_chunk_lines", p_gizmo));
}

SimpleTerrain3DGizmoPlugin::SimpleTerrain3DGizmoPlugin() {
	create_material("terrain_chunk_lines", Color(0.1, 0.85, 1.0, 0.85), false, true);
}

void SimpleTerrainEditorPlugin::_mode_toggled(bool p_pressed) {
	terrain_mode = p_pressed && terrain != nullptr;
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_operation_selected(int p_index) {
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_flat_pressed() {
	if (terrain == nullptr) {
		return;
	}
	// Buttons use the same height_data undo path as brush strokes so Flat,
	// Random, paint, undo, and redo all exercise one SimpleTerrain3D public API.
	const PackedFloat32Array before_heights = terrain->get_height_data();
	terrain->reset_flat_terrain();
	_commit_height_undo(TTR("Reset Terrain"), before_heights);
}

void SimpleTerrainEditorPlugin::_random_pressed() {
	if (terrain == nullptr) {
		return;
	}
	const PackedFloat32Array before_heights = terrain->get_height_data();
	terrain->randomize_seed();
	_commit_height_undo(TTR("Generate Random Terrain"), before_heights);
}

void SimpleTerrainEditorPlugin::_update_toolbar() {
	const bool has_terrain = terrain != nullptr;
	// The toolbar remains allocated for the lifetime of the plugin, but it is
	// only visible and interactive when a SimpleTerrain3D node is actively selected.
	toolbar->set_visible(has_terrain);
	mode_button->set_disabled(!has_terrain);
	mode_button->set_pressed_no_signal(terrain_mode && has_terrain);
	operation_button->set_disabled(!terrain_mode);
	radius_slider->set_read_only(!terrain_mode);
	strength_slider->set_read_only(!terrain_mode);
	flat_button->set_disabled(!has_terrain);
	random_button->set_disabled(!has_terrain);
}

void SimpleTerrainEditorPlugin::_apply_brush(const Vector3 &p_world_position) {
	if (terrain == nullptr) {
		return;
	}
	const real_t radius = radius_slider->get_value();

	// Mouse motion events can arrive at sub-cell distances. Skipping tiny moves
	// reduces redundant chunk uploads without changing the visible stroke shape.
	const real_t min_spacing = MAX(terrain->get_cell_size() * 0.5, radius * 0.05);
	if (has_last_brush_position && last_brush_position.distance_to(p_world_position) < min_spacing) {
		return;
	}
	last_brush_position = p_world_position;
	has_last_brush_position = true;
	const SimpleTerrain3D::BrushOperation operation = (SimpleTerrain3D::BrushOperation)operation_button->get_selected_id();
	_record_brush_delta(terrain->apply_brush_with_delta(p_world_position, radius, strength_slider->get_value(), operation));
}

void SimpleTerrainEditorPlugin::_record_brush_delta(const Dictionary &p_delta) {
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
			// If the same vertex is painted multiple times in one stroke, keep the
			// original before value and update only the final redo value.
			stroke_after_values.set(E->value, after_values[i]);
			continue;
		}
		stroke_index_map.insert(index, stroke_indices.size());
		stroke_indices.push_back(index);
		stroke_before_values.push_back(before_values[i]);
		stroke_after_values.push_back(after_values[i]);
	}
}

Dictionary SimpleTerrainEditorPlugin::_get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const {
	if (terrain == nullptr || p_camera == nullptr) {
		return Dictionary();
	}
	// The editor owns viewport-to-ray conversion. SimpleTerrain3D owns terrain picking
	// so the same hit-test can be reused by scripts or future tools.
	const Vector3 ray_origin = p_camera->project_ray_origin(p_mouse_position);
	const Vector3 ray_direction = p_camera->project_ray_normal(p_mouse_position);
	return terrain->get_brush_hit(ray_origin, ray_direction);
}

void SimpleTerrainEditorPlugin::_commit_height_undo(const String &p_action_name, const PackedFloat32Array &p_before_heights) {
	if (terrain == nullptr) {
		return;
	}
	const PackedFloat32Array after_heights = terrain->get_height_data();
	if (after_heights == p_before_heights) {
		return;
	}

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(p_action_name);
	// Height arrays are stored as properties instead of replaying each brush
	// sample. This makes undo deterministic even if brush spacing or algorithms
	// change later.
	undo_redo->add_do_property(terrain, "height_data", after_heights);
	undo_redo->add_do_method(terrain, "rebuild_mesh");
	undo_redo->add_undo_property(terrain, "height_data", p_before_heights);
	undo_redo->add_undo_method(terrain, "rebuild_mesh");
	undo_redo->commit_action();
}

void SimpleTerrainEditorPlugin::_commit_stroke_undo() {
	if (terrain == nullptr || stroke_indices.is_empty()) {
		return;
	}

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Paint Terrain"));
	undo_redo->add_do_method(terrain, "apply_height_patch", stroke_indices, stroke_after_values);
	undo_redo->add_undo_method(terrain, "apply_height_patch", stroke_indices, stroke_before_values);
	// The stroke has already been applied during mouse motion. Record the action
	// without executing the do method again on mouse release.
	undo_redo->commit_action(false);
}

void SimpleTerrainEditorPlugin::_notification(int p_what) {
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

bool SimpleTerrainEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<SimpleTerrain3D>(p_object) != nullptr;
}

void SimpleTerrainEditorPlugin::edit(Object *p_object) {
	terrain = Object::cast_to<SimpleTerrain3D>(p_object);
	if (terrain == nullptr) {
		terrain_mode = false;
		painting = false;
	}
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::clear() {
	terrain = nullptr;
	terrain_mode = false;
	painting = false;
	_update_toolbar();
}

EditorPlugin::AfterGUIInput SimpleTerrainEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
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
			// Start collecting per-vertex deltas. Mouse motion edits until release
			// become one undoable action without copying the full height field.
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
		// While painting, consume mouse motion only when it actually hits the
		// terrain. Other viewport behavior can continue when the ray misses.
		const Dictionary hit = _get_hit(p_camera, mouse_motion->get_position());
		if (!hit.is_empty()) {
			_apply_brush(hit["position"]);
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

SimpleTerrainEditorPlugin::SimpleTerrainEditorPlugin() {
	gizmo_plugin = Ref<SimpleTerrain3DGizmoPlugin>(memnew(SimpleTerrain3DGizmoPlugin));

	// Build a compact 3D editor toolbar. It avoids inspector-only workflows so
	// terrain painting feels like an editor mode tied to the selected SimpleTerrain3D.
	toolbar = memnew(HBoxContainer);
	toolbar->hide();

	mode_button = memnew(Button);
	mode_button->set_text(TTRC("Terrain"));
	mode_button->set_toggle_mode(true);
	mode_button->set_tooltip_text(TTRC("Toggle terrain editing mode."));
	mode_button->connect(SceneStringName(toggled), callable_mp(this, &SimpleTerrainEditorPlugin::_mode_toggled));
	toolbar->add_child(mode_button);

	operation_button = memnew(OptionButton);
	operation_button->set_tooltip_text(TTRC("Brush operation."));
	operation_button->add_item(TTRC("Raise"), SimpleTerrain3D::BRUSH_RAISE);
	operation_button->add_item(TTRC("Lower"), SimpleTerrain3D::BRUSH_LOWER);
	operation_button->add_item(TTRC("Smooth"), SimpleTerrain3D::BRUSH_SMOOTH);
	operation_button->add_item(TTRC("Flatten"), SimpleTerrain3D::BRUSH_FLATTEN);
	operation_button->connect(SceneStringName(item_selected), callable_mp(this, &SimpleTerrainEditorPlugin::_operation_selected));
	toolbar->add_child(operation_button);

	radius_slider = memnew(EditorSpinSlider);
	radius_slider->set_label(TTRC("Radius"));
	radius_slider->set_min(0.1);
	radius_slider->set_max(128.0);
	radius_slider->set_step(0.1);
	radius_slider->set_value(4.0);
	radius_slider->set_custom_minimum_size(Size2(100, 0) * EDSCALE);
	toolbar->add_child(radius_slider);

	strength_slider = memnew(EditorSpinSlider);
	strength_slider->set_label(TTRC("Strength"));
	strength_slider->set_min(0.01);
	strength_slider->set_max(10.0);
	strength_slider->set_step(0.01);
	strength_slider->set_value(0.25);
	strength_slider->set_custom_minimum_size(Size2(110, 0) * EDSCALE);
	toolbar->add_child(strength_slider);

	flat_button = memnew(Button);
	flat_button->set_text(TTRC("Flat"));
	flat_button->set_tooltip_text(TTRC("Reset selected terrain to a flat heightmap."));
	flat_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_flat_pressed));
	toolbar->add_child(flat_button);

	random_button = memnew(Button);
	random_button->set_text(TTRC("Random"));
	random_button->set_tooltip_text(TTRC("Generate random terrain on the selected terrain."));
	random_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_random_pressed));
	toolbar->add_child(random_button);
}
