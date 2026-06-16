/**************************************************************************/
/*  open_world_terrain_editor_plugin.cpp                                  */
/**************************************************************************/

#include "open_world_terrain_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"

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

void OpenWorldTerrainSettingsDock::_create_default_layers() {
	if (terrain == nullptr) {
		return;
	}

	Array layers;

	Ref<OpenWorldTerrainLayer> low;
	low.instantiate();
	low->set_layer_name(TTR("Low"));
	low->set_albedo_texture(terrain->get_low_texture());
	low->set_normal_texture(terrain->get_low_normal_texture());
	low->set_roughness_texture(terrain->get_low_roughness_texture());
	low->set_ao_texture(terrain->get_low_ao_texture());
	low->set_parallax_texture(terrain->get_low_parallax_texture());
	low->set_tint_color(terrain->get_low_color());
	low->set_texture_scale(terrain->get_low_texture_scale());
	low->set_roughness(terrain->get_low_roughness());
	layers.push_back(low);

	Ref<OpenWorldTerrainLayer> mid;
	mid.instantiate();
	mid->set_layer_name(TTR("Mid"));
	mid->set_albedo_texture(terrain->get_mid_texture());
	mid->set_normal_texture(terrain->get_mid_normal_texture());
	mid->set_roughness_texture(terrain->get_mid_roughness_texture());
	mid->set_ao_texture(terrain->get_mid_ao_texture());
	mid->set_parallax_texture(terrain->get_mid_parallax_texture());
	mid->set_tint_color(terrain->get_mid_color());
	mid->set_texture_scale(terrain->get_mid_texture_scale());
	mid->set_roughness(terrain->get_mid_roughness());
	layers.push_back(mid);

	Ref<OpenWorldTerrainLayer> high;
	high.instantiate();
	high->set_layer_name(TTR("High"));
	high->set_albedo_texture(terrain->get_high_texture());
	high->set_normal_texture(terrain->get_high_normal_texture());
	high->set_roughness_texture(terrain->get_high_roughness_texture());
	high->set_ao_texture(terrain->get_high_ao_texture());
	high->set_parallax_texture(terrain->get_high_parallax_texture());
	high->set_tint_color(terrain->get_high_color());
	high->set_texture_scale(terrain->get_high_texture_scale());
	high->set_roughness(terrain->get_high_roughness());
	layers.push_back(high);

	terrain->set_terrain_layers(layers);
	if (suppress_inspector_plugin != nullptr) {
		*suppress_inspector_plugin = true;
	}
	settings_inspector->edit(nullptr);
	settings_inspector->edit(terrain);
	if (suppress_inspector_plugin != nullptr) {
		*suppress_inspector_plugin = false;
	}
}

void OpenWorldTerrainSettingsDock::set_suppress_inspector_plugin_flag(bool *p_suppress) {
	suppress_inspector_plugin = p_suppress;
}

void OpenWorldTerrainSettingsDock::edit(OpenWorldTerrain3D *p_terrain) {
	terrain = p_terrain;
	create_default_layers_button->set_disabled(terrain == nullptr);

	if (suppress_inspector_plugin != nullptr) {
		*suppress_inspector_plugin = true;
	}
	settings_inspector->edit(nullptr);
	settings_inspector->edit(terrain);
	if (suppress_inspector_plugin != nullptr) {
		*suppress_inspector_plugin = false;
	}
}

OpenWorldTerrainSettingsDock::OpenWorldTerrainSettingsDock() {
	set_name(TTRC("OpenWorld Terrain Settings"));
	set_title(TTRC("OpenWorld Terrain Settings"));
	set_icon_name("Tools");
	set_default_slot(EditorDock::DOCK_SLOT_BOTTOM);
	set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	set_custom_minimum_size(Size2(640, 260) * EDSCALE);

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(root);

	HBoxContainer *top_bar = memnew(HBoxContainer);
	root->add_child(top_bar);

	create_default_layers_button = memnew(Button);
	create_default_layers_button->set_text(TTRC("Create Low/Mid/High"));
	create_default_layers_button->set_tooltip_text(TTRC("Create three layer resources from the current legacy material slots."));
	create_default_layers_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainSettingsDock::_create_default_layers));
	top_bar->add_child(create_default_layers_button);

	settings_inspector = memnew(EditorInspector);
	settings_inspector->set_custom_minimum_size(Size2(760, 520) * EDSCALE);
	settings_inspector->set_use_wide_editors(true);
	settings_inspector->set_use_folding(true);
	settings_inspector->set_hide_script(true);
	settings_inspector->set_mark_unsaved(true);
	settings_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(settings_inspector);
}

bool OpenWorldTerrainInspectorPlugin::can_handle(Object *p_object) {
	return !suppress_settings_inspector_plugin && Object::cast_to<OpenWorldTerrain3D>(p_object) != nullptr;
}

bool OpenWorldTerrainInspectorPlugin::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide) {
	if (!Object::cast_to<OpenWorldTerrain3D>(p_object)) {
		return false;
	}

	return true;
}

void OpenWorldTerrainInspectorPlugin::_open_settings(Object *p_object) {
	OpenWorldTerrain3D *terrain_node = Object::cast_to<OpenWorldTerrain3D>(p_object);
	if (terrain_node == nullptr || settings_dock == nullptr) {
		return;
	}
	settings_dock->edit(terrain_node);
	EditorDockManager::get_singleton()->focus_dock(settings_dock);
}

void OpenWorldTerrainInspectorPlugin::parse_end(Object *p_object) {
	if (!Object::cast_to<OpenWorldTerrain3D>(p_object)) {
		return;
	}

	Button *button = memnew(EditorInspectorActionButton(TTRC("Settings..."), SNAME("Tools")));
	button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	button->set_custom_minimum_size(Size2(160, 0) * EDSCALE);
	button->set_tooltip_text(TTRC("Open the OpenWorld terrain settings dock."));
	button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainInspectorPlugin::_open_settings).bind(p_object), CONNECT_DEFERRED);
	add_custom_control(button);
}

void OpenWorldTerrainInspectorPlugin::set_settings_dock(OpenWorldTerrainSettingsDock *p_dock) {
	settings_dock = p_dock;
	if (settings_dock != nullptr) {
		settings_dock->set_suppress_inspector_plugin_flag(&suppress_settings_inspector_plugin);
	}
}

OpenWorldTerrainInspectorPlugin::OpenWorldTerrainInspectorPlugin() {
}

void OpenWorldTerrainEditorPlugin::_select_mode_pressed() {
	terrain_mode = false;
	painting = false;
	picking_flatten_height = false;
	_clear_pending_create_cell();
	_clear_cursor_preview();
	_update_toolbar();
}

void OpenWorldTerrainEditorPlugin::_edit_mode_pressed() {
	terrain_mode = terrain != nullptr;
	_update_toolbar();
}

void OpenWorldTerrainEditorPlugin::_paint_mode_selected(int p_index) {
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

void OpenWorldTerrainEditorPlugin::_pick_flatten_toggled(bool p_pressed) {
	picking_flatten_height = p_pressed && terrain_mode && terrain != nullptr;
	if (pick_flatten_button != nullptr) {
		pick_flatten_button->set_pressed_no_signal(picking_flatten_height);
	}
}

void OpenWorldTerrainEditorPlugin::_update_toolbar() {
	const bool has_terrain = terrain != nullptr;
	toolbar->set_visible(has_terrain);
	brush_overlay_panel->set_visible(has_terrain && terrain_mode);
	select_mode_button->set_disabled(!has_terrain);
	edit_mode_button->set_disabled(!has_terrain);
	select_mode_button->set_pressed_no_signal(!terrain_mode && has_terrain);
	edit_mode_button->set_pressed_no_signal(terrain_mode && has_terrain);
	paint_mode_button->set_disabled(!terrain_mode);
	const bool layer_mode = terrain_mode && paint_mode_button->get_selected_id() == 1;
	operation_button->set_disabled(!terrain_mode);
	operation_button->set_visible(!layer_mode);
	if (has_terrain) {
		const int selected_layer = layer_button->get_selected_id();
		layer_button->clear();
		const Array terrain_layers = terrain->get_terrain_layers();
		int material_layer_count = 0;
		for (int i = 0; i < terrain_layers.size() && material_layer_count < 3; i++) {
			Ref<OpenWorldTerrainLayer> layer = terrain_layers[i];
			if (layer.is_null() || !layer->is_material_enabled()) {
				continue;
			}
			const String label = layer->get_layer_name().is_empty() ? vformat("Layer %d", material_layer_count) : layer->get_layer_name();
			layer_button->add_item(label, material_layer_count);
			material_layer_count++;
		}
		if (material_layer_count == 0) {
			layer_button->add_item(TTRC("Low"), OpenWorldTerrain3D::LAYER_PAINT_LOW);
			layer_button->add_item(TTRC("Mid"), OpenWorldTerrain3D::LAYER_PAINT_MID);
			layer_button->add_item(TTRC("High"), OpenWorldTerrain3D::LAYER_PAINT_HIGH);
		}
		if (selected_layer >= 0) {
			for (int i = 0; i < layer_button->get_item_count(); i++) {
				if (layer_button->get_item_id(i) == selected_layer) {
					layer_button->select(i);
					break;
				}
			}
		}
	}
	layer_button->set_disabled(!terrain_mode);
	layer_button->set_visible(layer_mode);
	layer_blend_button->set_disabled(!terrain_mode);
	layer_blend_button->set_visible(layer_mode);
	radius_slider->set_read_only(!terrain_mode);
	strength_slider->set_read_only(!terrain_mode);
	layer_alpha_slider->set_read_only(!terrain_mode);
	layer_alpha_slider->set_visible(layer_mode);
	falloff_slider->set_read_only(!terrain_mode);
	spacing_slider->set_read_only(!terrain_mode);
	flatten_slider->set_read_only(!terrain_mode);
	pick_flatten_button->set_disabled(!terrain_mode);
	pick_flatten_button->set_pressed_no_signal(picking_flatten_height && terrain_mode);
	flat_button->set_disabled(!has_terrain);
	random_button->set_disabled(!has_terrain);
	rebuild_button->set_disabled(!has_terrain);
}

void OpenWorldTerrainEditorPlugin::_attach_brush_overlay() {
	Node3DEditorViewport *viewport = Node3DEditor::get_singleton()->get_editor_viewport(0);
	if (viewport == nullptr || brush_overlay_panel->get_parent() != nullptr) {
		return;
	}

	Control *surface = viewport->get_surface();
	if (surface == nullptr) {
		return;
	}

	surface->add_child(brush_overlay_panel);
	brush_overlay_panel->move_to_front();
}

void OpenWorldTerrainEditorPlugin::_detach_brush_overlay() {
	if (brush_overlay_panel->get_parent() != nullptr) {
		brush_overlay_panel->get_parent()->remove_child(brush_overlay_panel);
	}
}

real_t OpenWorldTerrainEditorPlugin::_get_brush_spacing() const {
	return spacing_slider != nullptr ? spacing_slider->get_value() : 0.05;
}

void OpenWorldTerrainEditorPlugin::_apply_brush(const Vector3 &p_world_position) {
	if (terrain == nullptr || terrain->get_terrain_data().is_null()) {
		return;
	}

	const real_t radius = radius_slider->get_value();
	const Ref<OpenWorldTerrainData> terrain_data = terrain->get_terrain_data();
	const real_t texel_world_size = terrain_data->get_tile_world_size() / MAX(1.0, (real_t)terrain_data->get_tile_resolution() - 1.0);
	const real_t min_spacing = MAX(texel_world_size * 0.5, radius * _get_brush_spacing());
	if (has_last_brush_position && last_brush_position.distance_to(p_world_position) < min_spacing) {
		return;
	}

	last_brush_position = p_world_position;
	has_last_brush_position = true;
	terrain->set_flatten_height(flatten_slider->get_value());
	terrain->set_brush_falloff(falloff_slider->get_value());
	if (paint_mode_button->get_selected_id() == 1) {
		const OpenWorldTerrain3D::LayerPaintTarget target = (OpenWorldTerrain3D::LayerPaintTarget)layer_button->get_selected_id();
		const OpenWorldTerrain3D::LayerBlendMode blend_mode = (OpenWorldTerrain3D::LayerBlendMode)layer_blend_button->get_selected_id();
		_record_layer_delta(terrain->apply_layer_brush_with_delta(p_world_position, radius, strength_slider->get_value(), target, layer_alpha_slider->get_value(), blend_mode));
	} else {
		const OpenWorldTerrain3D::BrushOperation operation = (OpenWorldTerrain3D::BrushOperation)operation_button->get_selected_id();
		_record_brush_delta(terrain->apply_brush_with_delta(p_world_position, radius, strength_slider->get_value(), operation));
	}
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

void OpenWorldTerrainEditorPlugin::_record_layer_delta(const Dictionary &p_delta) {
	if (p_delta.is_empty()) {
		return;
	}

	const PackedInt32Array indices = p_delta["indices"];
	const PackedColorArray before_values = p_delta["before"];
	const PackedColorArray after_values = p_delta["after"];
	ERR_FAIL_COND(indices.size() != before_values.size());
	ERR_FAIL_COND(indices.size() != after_values.size());

	for (int i = 0; i < indices.size(); i++) {
		const int index = indices[i];
		HashMap<int, int>::Iterator E = stroke_index_map.find(index);
		if (E) {
			stroke_after_layers.set(E->value, after_values[i]);
			continue;
		}

		stroke_index_map.insert(index, stroke_indices.size());
		stroke_indices.push_back(index);
		stroke_before_layers.push_back(before_values[i]);
		stroke_after_layers.push_back(after_values[i]);
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

bool OpenWorldTerrainEditorPlugin::_get_grid_cell_at_mouse(Camera3D *p_camera, const Vector2 &p_mouse_position, Vector2i &r_cell, Vector3 &r_world_position) const {
	if (terrain == nullptr || terrain->get_terrain_data().is_null() || p_camera == nullptr) {
		return false;
	}

	const Transform3D inverse_transform = terrain->get_global_transform().affine_inverse();
	const Vector3 local_origin = inverse_transform.xform(p_camera->project_ray_origin(p_mouse_position));
	const Vector3 local_direction = inverse_transform.basis.xform(p_camera->project_ray_normal(p_mouse_position)).normalized();
	if (Math::is_zero_approx(local_direction.y)) {
		return false;
	}

	const real_t t = -local_origin.y / local_direction.y;
	if (t < 0.0) {
		return false;
	}

	const Ref<OpenWorldTerrainData> terrain_data = terrain->get_terrain_data();
	const Vector3 local_position = local_origin + local_direction * t;
	const real_t tile_world_size = terrain_data->get_tile_world_size();
	if (tile_world_size <= 0.0) {
		return false;
	}

	const int cell_x = Math::floor(local_position.x / tile_world_size);
	const int cell_y = Math::floor(local_position.z / tile_world_size);
	const Vector3 local_center(
			((real_t)cell_x + 0.5) * tile_world_size,
			0.0,
			((real_t)cell_y + 0.5) * tile_world_size);

	r_cell = Vector2i(cell_x, cell_y);
	r_world_position = terrain->get_global_transform().xform(local_center);
	return true;
}

void OpenWorldTerrainEditorPlugin::_set_pending_create_cell(const Vector2i &p_cell, const Vector3 &p_world_position) {
	pending_create_cell = p_cell;
	pending_create_world_position = p_world_position;
	has_pending_create_cell = true;
	update_overlays();
}

void OpenWorldTerrainEditorPlugin::_clear_pending_create_cell() {
	if (!has_pending_create_cell) {
		return;
	}
	has_pending_create_cell = false;
	pending_create_button_rect = Rect2();
	update_overlays();
}

void OpenWorldTerrainEditorPlugin::_create_pending_grid_cell() {
	if (!has_pending_create_cell || terrain == nullptr) {
		return;
	}
	if (terrain->has_grid_cell(pending_create_cell)) {
		_clear_pending_create_cell();
		return;
	}
	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Create OpenWorld Terrain Tile"));
	undo_redo->add_do_method(terrain, "create_grid_cell", pending_create_cell);
	undo_redo->add_undo_method(terrain, "remove_grid_cell", pending_create_cell);
	undo_redo->commit_action();
	_clear_pending_create_cell();
}

real_t OpenWorldTerrainEditorPlugin::_sample_normalized_height_nearest(const Vector3 &p_local_position) const {
	if (terrain == nullptr || terrain->get_terrain_data().is_null()) {
		return 0.0;
	}

	const Ref<OpenWorldTerrainData> terrain_data = terrain->get_terrain_data();
	const int resolution = terrain_data->get_tile_resolution();
	if (resolution < 1) {
		return 0.0;
	}

	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const Vector2i cell(Math::floor(p_local_position.x / tile_world_size), Math::floor(p_local_position.z / tile_world_size));
	if (!terrain_data->has_tile(cell)) {
		return 0.0;
	}
	const real_t texel_world_size = tile_world_size / MAX((real_t)1.0, (real_t)resolution - 1.0);
	const int x = CLAMP(Math::round((p_local_position.x - (real_t)cell.x * tile_world_size) / texel_world_size), 0, resolution - 1);
	const int y = CLAMP(Math::round((p_local_position.z - (real_t)cell.y * tile_world_size) / texel_world_size), 0, resolution - 1);
	return terrain_data->get_tile_height(cell, x, y);
}

Color OpenWorldTerrainEditorPlugin::_get_cursor_color() const {
	if (paint_mode_button != nullptr && paint_mode_button->get_selected_id() == 1) {
		switch ((OpenWorldTerrain3D::LayerPaintTarget)layer_button->get_selected_id()) {
			case OpenWorldTerrain3D::LAYER_PAINT_LOW:
				return Color(0.2, 0.85, 0.25, 0.9);
			case OpenWorldTerrain3D::LAYER_PAINT_MID:
				return Color(0.75, 0.55, 0.25, 0.9);
			case OpenWorldTerrain3D::LAYER_PAINT_HIGH:
				return Color(0.85, 0.85, 0.9, 0.9);
		}
	}

	switch ((OpenWorldTerrain3D::BrushOperation)operation_button->get_selected_id()) {
		case OpenWorldTerrain3D::BRUSH_RAISE:
			return Color(0.2, 1.0, 0.35, 0.9);
		case OpenWorldTerrain3D::BRUSH_LOWER:
			return Color(1.0, 0.25, 0.2, 0.9);
		case OpenWorldTerrain3D::BRUSH_SMOOTH:
			return Color(0.25, 0.55, 1.0, 0.9);
		case OpenWorldTerrain3D::BRUSH_FLATTEN:
			return Color(1.0, 0.85, 0.2, 0.9);
	}
	return Color(1.0, 1.0, 1.0, 0.9);
}

void OpenWorldTerrainEditorPlugin::_update_cursor_preview(Camera3D *p_camera, const Dictionary &p_hit) {
	cursor_points.clear();
	has_cursor_hit = false;
	if (!terrain_mode || terrain == nullptr || p_camera == nullptr || p_hit.is_empty()) {
		update_overlays();
		return;
	}

	const Ref<OpenWorldTerrainData> terrain_data = terrain->get_terrain_data();
	if (terrain_data.is_null()) {
		update_overlays();
		return;
	}

	const Vector3 local_center = p_hit["local_position"];
	const real_t radius = radius_slider->get_value();
	const real_t tile_world_size = terrain_data->get_tile_world_size();
	const int resolution = terrain_data->get_tile_resolution();
	const real_t texel_world_size = tile_world_size / MAX((real_t)1.0, (real_t)resolution - 1.0);
	const Transform3D terrain_transform = terrain->get_global_transform();
	const int segments = 64;

	for (int i = 0; i <= segments; i++) {
		const real_t angle = Math::PI * 2.0 * (real_t)i / (real_t)segments;
		const real_t local_x = local_center.x + Math::cos(angle) * radius;
		const real_t local_z = local_center.z + Math::sin(angle) * radius;
		const Vector2i cell(Math::floor(local_x / tile_world_size), Math::floor(local_z / tile_world_size));
		if (!terrain_data->has_tile(cell)) {
			cursor_points.clear();
			update_overlays();
			return;
		}
		const int height_x = CLAMP(Math::round((local_x - (real_t)cell.x * tile_world_size) / texel_world_size), 0, resolution - 1);
		const int height_y = CLAMP(Math::round((local_z - (real_t)cell.y * tile_world_size) / texel_world_size), 0, resolution - 1);
		const real_t local_y = terrain_data->get_tile_height(cell, height_x, height_y) * terrain_data->get_height_scale() + 0.05;
		const Vector3 world_point = terrain_transform.xform(Vector3(local_x, local_y, local_z));
		if (p_camera->is_position_behind(world_point)) {
			cursor_points.clear();
			update_overlays();
			return;
		}
		cursor_points.push_back(p_camera->unproject_position(world_point));
	}

	cursor_color = _get_cursor_color();
	has_cursor_hit = cursor_points.size() > 1;
	update_overlays();
}

void OpenWorldTerrainEditorPlugin::_clear_cursor_preview() {
	if (!has_cursor_hit && cursor_points.is_empty()) {
		return;
	}
	has_cursor_hit = false;
	cursor_points.clear();
	update_overlays();
}

void OpenWorldTerrainEditorPlugin::_draw_over_viewport(Control *p_overlay) {
	if (!terrain_mode) {
		return;
	}

	if (has_pending_create_cell && terrain != nullptr) {
		Camera3D *camera = Object::cast_to<Camera3D>(ObjectDB::get_instance(last_view_camera_id));
		if (camera == nullptr) {
			camera = Object::cast_to<Camera3D>(p_overlay->get_viewport()->get_camera_3d());
		}
		if (camera != nullptr && !camera->is_position_behind(pending_create_world_position)) {
			const Vector2 screen_position = camera->unproject_position(pending_create_world_position);
			const Size2 button_size = Size2(112, 34) * EDSCALE;
			pending_create_button_rect = Rect2((screen_position - button_size * 0.5).round(), button_size);
			p_overlay->draw_rect(pending_create_button_rect.grow(2 * EDSCALE), Color(0, 0, 0, 0.7), true);
			p_overlay->draw_rect(pending_create_button_rect, Color(0.14, 0.58, 0.95, 0.96), true);
			p_overlay->draw_rect(pending_create_button_rect, Color(1, 1, 1, 0.85), false, Math::round(1 * EDSCALE));

			const Ref<Font> font = p_overlay->get_theme_font(SceneStringName(font), SNAME("Button"));
			const int font_size = p_overlay->get_theme_font_size(SceneStringName(font_size), SNAME("Button"));
			const String label = TTR("[Create]");
			const Size2 text_size = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
			const Vector2 text_position = pending_create_button_rect.position + Vector2((pending_create_button_rect.size.x - text_size.x) * 0.5, (pending_create_button_rect.size.y + text_size.y) * 0.5 - 4 * EDSCALE);
			p_overlay->draw_string(font, text_position, label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1, 1, 1));
		}
	}

	if (has_cursor_hit && cursor_points.size() >= 2) {
		for (int i = 0; i < cursor_points.size() - 1; i++) {
			p_overlay->draw_line(cursor_points[i], cursor_points[i + 1], Color(0, 0, 0, cursor_color.a), Math::round(4 * EDSCALE), true);
		}
		for (int i = 0; i < cursor_points.size() - 1; i++) {
			p_overlay->draw_line(cursor_points[i], cursor_points[i + 1], cursor_color, Math::round(2 * EDSCALE), true);
		}
	}
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
	if (!stroke_after_layers.is_empty()) {
		undo_redo->create_action(TTR("Paint OpenWorld Terrain Layer"));
		undo_redo->add_do_method(terrain, "apply_layer_patch", stroke_indices, stroke_after_layers);
		undo_redo->add_undo_method(terrain, "apply_layer_patch", stroke_indices, stroke_before_layers);
	} else {
		undo_redo->create_action(TTR("Paint OpenWorld Terrain"));
		undo_redo->add_do_method(terrain, "apply_height_patch", stroke_indices, stroke_after_values);
		undo_redo->add_undo_method(terrain, "apply_height_patch", stroke_indices, stroke_before_values);
	}
	undo_redo->commit_action(false);
}

void OpenWorldTerrainEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			add_node_3d_gizmo_plugin(gizmo_plugin);
			add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			_attach_brush_overlay();
			add_dock(settings_dock);
			settings_dock->close();
			set_force_draw_over_forwarding_enabled();
			select_mode_button->set_button_icon(select_mode_button->get_editor_theme_icon(SNAME("ToolSelect")));
			edit_mode_button->set_button_icon(edit_mode_button->get_editor_theme_icon(SNAME("OpenWorldTerrainEdit")));
			_update_toolbar();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_detach_brush_overlay();
			remove_dock(settings_dock);
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
		picking_flatten_height = false;
		_clear_cursor_preview();
	}
	settings_dock->edit(terrain);
	_update_toolbar();
}

void OpenWorldTerrainEditorPlugin::clear() {
	terrain = nullptr;
	terrain_mode = false;
	painting = false;
	picking_flatten_height = false;
	_clear_cursor_preview();
	settings_dock->edit(nullptr);
	_update_toolbar();
}

EditorPlugin::AfterGUIInput OpenWorldTerrainEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!terrain_mode || terrain == nullptr) {
		_clear_cursor_preview();
		return AFTER_GUI_INPUT_PASS;
	}

	if (p_camera != nullptr) {
		last_view_camera_id = p_camera->get_instance_id();
	}

	Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT) {
		const Vector2 mouse_position = mouse_button->get_position();
		if (mouse_button->is_pressed() && has_pending_create_cell && pending_create_button_rect.has_point(mouse_position)) {
			_create_pending_grid_cell();
			return AFTER_GUI_INPUT_STOP;
		}

		const Dictionary hit = _get_hit(p_camera, mouse_button->get_position());
		_update_cursor_preview(p_camera, hit);

		if (mouse_button->is_pressed()) {
			if (hit.is_empty()) {
				Vector2i grid_cell;
				Vector3 grid_world_position;
				if (_get_grid_cell_at_mouse(p_camera, mouse_position, grid_cell, grid_world_position)) {
					if (!terrain->has_grid_cell(grid_cell)) {
						_set_pending_create_cell(grid_cell, grid_world_position);
					} else {
						_clear_pending_create_cell();
					}
				} else {
					_clear_pending_create_cell();
				}
				return AFTER_GUI_INPUT_STOP;
			}

			_clear_pending_create_cell();
			if (picking_flatten_height) {
				const real_t picked_height = _sample_normalized_height_nearest(hit["local_position"]);
				flatten_slider->set_value(picked_height);
				terrain->set_flatten_height(picked_height);
				picking_flatten_height = false;
				pick_flatten_button->set_pressed_no_signal(false);
				return AFTER_GUI_INPUT_STOP;
			}

			painting = true;
			has_last_brush_position = false;
			stroke_index_map.clear();
			stroke_indices = PackedInt32Array();
			stroke_before_values = PackedFloat32Array();
			stroke_after_values = PackedFloat32Array();
			stroke_before_layers = PackedColorArray();
			stroke_after_layers = PackedColorArray();
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
			stroke_before_layers = PackedColorArray();
			stroke_after_layers = PackedColorArray();
			return AFTER_GUI_INPUT_STOP;
		}
		return AFTER_GUI_INPUT_STOP;
	}

	Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid()) {
		const Dictionary hit = _get_hit(p_camera, mouse_motion->get_position());
		_update_cursor_preview(p_camera, hit);
		if (painting && !hit.is_empty()) {
			_apply_brush(hit["position"]);
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

void OpenWorldTerrainEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
}

void OpenWorldTerrainEditorPlugin::forward_3d_force_draw_over_viewport(Control *p_overlay) {
	_draw_over_viewport(p_overlay);
}

OpenWorldTerrainEditorPlugin::OpenWorldTerrainEditorPlugin() {
	gizmo_plugin = Ref<OpenWorldTerrain3DGizmoPlugin>(memnew(OpenWorldTerrain3DGizmoPlugin));
	settings_dock = memnew(OpenWorldTerrainSettingsDock);
	inspector_plugin.instantiate();
	inspector_plugin->set_settings_dock(settings_dock);
	add_inspector_plugin(inspector_plugin);

	toolbar = memnew(HBoxContainer);
	toolbar->hide();

	brush_overlay_panel = memnew(PanelContainer);
	brush_overlay_panel->hide();
	brush_overlay_panel->set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT, Control::PRESET_MODE_MINSIZE, 12 * EDSCALE);
	brush_overlay_panel->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
	brush_overlay_panel->set_custom_minimum_size(Size2(220, 0) * EDSCALE);

	brush_options_vbox = memnew(VBoxContainer);
	brush_options_vbox->add_theme_constant_override("separation", 6 * EDSCALE);
	brush_overlay_panel->add_child(brush_options_vbox);

	mode_button_group.instantiate();

	select_mode_button = memnew(Button);
	select_mode_button->set_toggle_mode(true);
	select_mode_button->set_button_group(mode_button_group);
	select_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	select_mode_button->set_tooltip_text(TTRC("Select scene objects."));
	select_mode_button->set_accessibility_name(TTRC("Select Mode"));
	select_mode_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_select_mode_pressed));
	toolbar->add_child(select_mode_button);

	edit_mode_button = memnew(Button);
	edit_mode_button->set_toggle_mode(true);
	edit_mode_button->set_button_group(mode_button_group);
	edit_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	edit_mode_button->set_tooltip_text(TTRC("Edit OpenWorld terrain."));
	edit_mode_button->set_accessibility_name(TTRC("OpenWorld Terrain Edit Mode"));
	edit_mode_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_edit_mode_pressed));
	toolbar->add_child(edit_mode_button);

	paint_mode_button = memnew(OptionButton);
	paint_mode_button->set_tooltip_text(TTRC("Choose whether the brush edits height or splat layers."));
	paint_mode_button->add_item(TTRC("Height"), 0);
	paint_mode_button->add_item(TTRC("Layer"), 1);
	paint_mode_button->connect(SceneStringName(item_selected), callable_mp(this, &OpenWorldTerrainEditorPlugin::_paint_mode_selected));
	brush_options_vbox->add_child(paint_mode_button);

	operation_button = memnew(OptionButton);
	operation_button->set_tooltip_text(TTRC("Brush operation."));
	operation_button->add_item(TTRC("Raise"), OpenWorldTerrain3D::BRUSH_RAISE);
	operation_button->add_item(TTRC("Lower"), OpenWorldTerrain3D::BRUSH_LOWER);
	operation_button->add_item(TTRC("Smooth"), OpenWorldTerrain3D::BRUSH_SMOOTH);
	operation_button->add_item(TTRC("Flatten"), OpenWorldTerrain3D::BRUSH_FLATTEN);
	brush_options_vbox->add_child(operation_button);

	layer_button = memnew(OptionButton);
	layer_button->set_tooltip_text(TTRC("Layer painted by the brush."));
	layer_button->add_item(TTRC("Low"), OpenWorldTerrain3D::LAYER_PAINT_LOW);
	layer_button->add_item(TTRC("Mid"), OpenWorldTerrain3D::LAYER_PAINT_MID);
	layer_button->add_item(TTRC("High"), OpenWorldTerrain3D::LAYER_PAINT_HIGH);
	layer_button->hide();
	brush_options_vbox->add_child(layer_button);

	layer_blend_button = memnew(OptionButton);
	layer_blend_button->set_tooltip_text(TTRC("Blend mode used when painting layer data."));
	layer_blend_button->add_item(TTRC("Normal"), OpenWorldTerrain3D::LAYER_BLEND_NORMAL);
	layer_blend_button->add_item(TTRC("Multiply"), OpenWorldTerrain3D::LAYER_BLEND_MULTIPLY);
	layer_blend_button->add_item(TTRC("Darkness"), OpenWorldTerrain3D::LAYER_BLEND_DARKEN);
	layer_blend_button->add_item(TTRC("Lighten"), OpenWorldTerrain3D::LAYER_BLEND_LIGHTEN);
	layer_blend_button->hide();
	brush_options_vbox->add_child(layer_blend_button);

	radius_slider = memnew(EditorSpinSlider);
	radius_slider->set_label(TTRC("Radius"));
	radius_slider->set_min(0.1);
	radius_slider->set_max(512.0);
	radius_slider->set_step(0.1);
	radius_slider->set_value(24.0);
	radius_slider->set_custom_minimum_size(Size2(92, 0) * EDSCALE);
	brush_options_vbox->add_child(radius_slider);

	strength_slider = memnew(EditorSpinSlider);
	strength_slider->set_label(TTRC("Strength"));
	strength_slider->set_min(0.001);
	strength_slider->set_max(1.0);
	strength_slider->set_step(0.001);
	strength_slider->set_value(0.05);
	strength_slider->set_custom_minimum_size(Size2(96, 0) * EDSCALE);
	brush_options_vbox->add_child(strength_slider);

	layer_alpha_slider = memnew(EditorSpinSlider);
	layer_alpha_slider->set_label(TTRC("Alpha"));
	layer_alpha_slider->set_min(0.0);
	layer_alpha_slider->set_max(1.0);
	layer_alpha_slider->set_step(0.001);
	layer_alpha_slider->set_value(1.0);
	layer_alpha_slider->set_custom_minimum_size(Size2(92, 0) * EDSCALE);
	layer_alpha_slider->hide();
	brush_options_vbox->add_child(layer_alpha_slider);

	falloff_slider = memnew(EditorSpinSlider);
	falloff_slider->set_label(TTRC("Falloff"));
	falloff_slider->set_min(0.0);
	falloff_slider->set_max(8.0);
	falloff_slider->set_step(0.001);
	falloff_slider->set_value(1.0);
	falloff_slider->set_custom_minimum_size(Size2(96, 0) * EDSCALE);
	brush_options_vbox->add_child(falloff_slider);

	spacing_slider = memnew(EditorSpinSlider);
	spacing_slider->set_label(TTRC("Spacing"));
	spacing_slider->set_min(0.0);
	spacing_slider->set_max(1.0);
	spacing_slider->set_step(0.001);
	spacing_slider->set_value(0.05);
	spacing_slider->set_custom_minimum_size(Size2(96, 0) * EDSCALE);
	brush_options_vbox->add_child(spacing_slider);

	flatten_slider = memnew(EditorSpinSlider);
	flatten_slider->set_label(TTRC("Flatten"));
	flatten_slider->set_min(0.0);
	flatten_slider->set_max(1.0);
	flatten_slider->set_step(0.001);
	flatten_slider->set_value(0.5);
	flatten_slider->set_custom_minimum_size(Size2(96, 0) * EDSCALE);
	brush_options_vbox->add_child(flatten_slider);

	HBoxContainer *terrain_action_row = memnew(HBoxContainer);
	terrain_action_row->add_theme_constant_override("separation", 4 * EDSCALE);
	brush_options_vbox->add_child(terrain_action_row);

	pick_flatten_button = memnew(Button);
	pick_flatten_button->set_text(TTRC("Pick"));
	pick_flatten_button->set_toggle_mode(true);
	pick_flatten_button->set_tooltip_text(TTRC("Pick flatten height from the terrain."));
	pick_flatten_button->connect(SceneStringName(toggled), callable_mp(this, &OpenWorldTerrainEditorPlugin::_pick_flatten_toggled));
	terrain_action_row->add_child(pick_flatten_button);

	flat_button = memnew(Button);
	flat_button->set_text(TTRC("Flat"));
	flat_button->set_tooltip_text(TTRC("Reset selected OpenWorld terrain to the flatten height."));
	flat_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_flat_pressed));
	terrain_action_row->add_child(flat_button);

	random_button = memnew(Button);
	random_button->set_text(TTRC("Random"));
	random_button->set_tooltip_text(TTRC("Generate random OpenWorld terrain on the selected node."));
	random_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_random_pressed));
	terrain_action_row->add_child(random_button);

	rebuild_button = memnew(Button);
	rebuild_button->set_text(TTRC("Rebuild"));
	rebuild_button->set_tooltip_text(TTRC("Rebuild mesh, height texture, and material."));
	rebuild_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldTerrainEditorPlugin::_rebuild_pressed));
	terrain_action_row->add_child(rebuild_button);
}
