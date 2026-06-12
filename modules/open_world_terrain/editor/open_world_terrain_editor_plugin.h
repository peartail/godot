/**************************************************************************/
/*  open_world_terrain_editor_plugin.h                                    */
/**************************************************************************/

#pragma once

#include "../open_world_terrain_3d.h"

#include "core/templates/hash_map.h"
#include "editor/docks/editor_dock.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"

class Button;
class ButtonGroup;
class Camera3D;
class Control;
class EditorSpinSlider;
class HBoxContainer;
class OptionButton;

class OpenWorldTerrain3DGizmoPlugin : public EditorNode3DGizmoPlugin {
	GDCLASS(OpenWorldTerrain3DGizmoPlugin, EditorNode3DGizmoPlugin);

public:
	bool has_gizmo(Node3D *p_spatial) override;
	String get_gizmo_name() const override;
	int get_priority() const override;
	void redraw(EditorNode3DGizmo *p_gizmo) override;

	OpenWorldTerrain3DGizmoPlugin();
};

class OpenWorldTerrainSettingsDock : public EditorDock {
	GDCLASS(OpenWorldTerrainSettingsDock, EditorDock);

	OpenWorldTerrain3D *terrain = nullptr;
	Button *create_default_layers_button = nullptr;
	EditorInspector *settings_inspector = nullptr;
	bool *suppress_inspector_plugin = nullptr;

	void _create_default_layers();

public:
	void set_suppress_inspector_plugin_flag(bool *p_suppress);
	void edit(OpenWorldTerrain3D *p_terrain);

	OpenWorldTerrainSettingsDock();
};

class OpenWorldTerrainInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(OpenWorldTerrainInspectorPlugin, EditorInspectorPlugin);

	OpenWorldTerrainSettingsDock *settings_dock = nullptr;
	bool suppress_settings_inspector_plugin = false;

	void _open_settings(Object *p_object);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide = false) override;
	virtual void parse_end(Object *p_object) override;

	void set_settings_dock(OpenWorldTerrainSettingsDock *p_dock);
	OpenWorldTerrainInspectorPlugin();
};

class OpenWorldTerrainEditorPlugin : public EditorPlugin {
	GDCLASS(OpenWorldTerrainEditorPlugin, EditorPlugin);

	HBoxContainer *toolbar = nullptr;
	Button *select_mode_button = nullptr;
	Button *edit_mode_button = nullptr;
	Ref<ButtonGroup> mode_button_group;
	OptionButton *paint_mode_button = nullptr;
	OptionButton *operation_button = nullptr;
	OptionButton *layer_button = nullptr;
	OptionButton *layer_blend_button = nullptr;
	EditorSpinSlider *radius_slider = nullptr;
	EditorSpinSlider *strength_slider = nullptr;
	EditorSpinSlider *layer_alpha_slider = nullptr;
	EditorSpinSlider *falloff_slider = nullptr;
	EditorSpinSlider *spacing_slider = nullptr;
	EditorSpinSlider *flatten_slider = nullptr;
	Button *pick_flatten_button = nullptr;
	Button *flat_button = nullptr;
	Button *random_button = nullptr;
	Button *rebuild_button = nullptr;
	Ref<OpenWorldTerrain3DGizmoPlugin> gizmo_plugin;
	Ref<OpenWorldTerrainInspectorPlugin> inspector_plugin;
	OpenWorldTerrainSettingsDock *settings_dock = nullptr;

	OpenWorldTerrain3D *terrain = nullptr;
	bool terrain_mode = false;
	bool painting = false;
	bool has_last_brush_position = false;
	bool picking_flatten_height = false;
	bool has_cursor_hit = false;
	bool has_pending_create_cell = false;
	Vector3 last_brush_position;
	Vector2i pending_create_cell;
	Vector3 pending_create_world_position;
	Rect2 pending_create_button_rect;
	ObjectID last_view_camera_id;
	PackedVector2Array cursor_points;
	Color cursor_color;
	int random_seed = 1;

	HashMap<int, int> stroke_index_map;
	PackedInt32Array stroke_indices;
	PackedFloat32Array stroke_before_values;
	PackedFloat32Array stroke_after_values;
	PackedColorArray stroke_before_layers;
	PackedColorArray stroke_after_layers;

	void _select_mode_pressed();
	void _edit_mode_pressed();
	void _paint_mode_selected(int p_index);
	void _flat_pressed();
	void _random_pressed();
	void _rebuild_pressed();
	void _pick_flatten_toggled(bool p_pressed);
	void _update_toolbar();
	real_t _get_brush_spacing() const;
	void _apply_brush(const Vector3 &p_world_position);
	void _record_brush_delta(const Dictionary &p_delta);
	void _record_layer_delta(const Dictionary &p_delta);
	Dictionary _get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
	bool _get_grid_cell_at_mouse(Camera3D *p_camera, const Vector2 &p_mouse_position, Vector2i &r_cell, Vector3 &r_world_position) const;
	void _set_pending_create_cell(const Vector2i &p_cell, const Vector3 &p_world_position);
	void _clear_pending_create_cell();
	void _create_pending_grid_cell();
	real_t _sample_normalized_height_nearest(const Vector3 &p_local_position) const;
	Color _get_cursor_color() const;
	void _update_cursor_preview(Camera3D *p_camera, const Dictionary &p_hit);
	void _clear_cursor_preview();
	void _draw_over_viewport(Control *p_overlay);
	void _commit_full_height_undo(const String &p_action_name, const PackedFloat32Array &p_before_heights);
	void _commit_stroke_undo();

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "OpenWorld Terrain"; }
	virtual bool handles(Object *p_object) const override;
	virtual void edit(Object *p_object) override;
	virtual void clear() override;
	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	virtual void forward_3d_draw_over_viewport(Control *p_overlay) override;
	virtual void forward_3d_force_draw_over_viewport(Control *p_overlay) override;

	OpenWorldTerrainEditorPlugin();
};
