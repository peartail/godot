/**************************************************************************/
/*  open_world_terrain_editor_plugin.h                                    */
/**************************************************************************/

#pragma once

#include "../open_world_terrain_3d.h"

#include "core/templates/hash_map.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"

class Button;
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

class OpenWorldTerrainEditorPlugin : public EditorPlugin {
	GDCLASS(OpenWorldTerrainEditorPlugin, EditorPlugin);

	HBoxContainer *toolbar = nullptr;
	Button *mode_button = nullptr;
	OptionButton *operation_button = nullptr;
	EditorSpinSlider *radius_slider = nullptr;
	EditorSpinSlider *strength_slider = nullptr;
	EditorSpinSlider *flatten_slider = nullptr;
	Button *flat_button = nullptr;
	Button *random_button = nullptr;
	Button *rebuild_button = nullptr;
	Ref<OpenWorldTerrain3DGizmoPlugin> gizmo_plugin;

	OpenWorldTerrain3D *terrain = nullptr;
	bool terrain_mode = false;
	bool painting = false;
	bool has_last_brush_position = false;
	Vector3 last_brush_position;
	int random_seed = 1;

	HashMap<int, int> stroke_index_map;
	PackedInt32Array stroke_indices;
	PackedFloat32Array stroke_before_values;
	PackedFloat32Array stroke_after_values;

	void _mode_toggled(bool p_pressed);
	void _flat_pressed();
	void _random_pressed();
	void _rebuild_pressed();
	void _update_toolbar();
	void _apply_brush(const Vector3 &p_world_position);
	void _record_brush_delta(const Dictionary &p_delta);
	Dictionary _get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
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

	OpenWorldTerrainEditorPlugin();
};
