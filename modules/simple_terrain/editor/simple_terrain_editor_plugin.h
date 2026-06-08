/**************************************************************************/
/*  simple_terrain_editor_plugin.h                                               */
/**************************************************************************/

#pragma once

#include "editor/plugins/editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"
#include "core/templates/hash_map.h"
#include "../simple_terrain_3d.h"

class Button;
class EditorSpinSlider;
class HBoxContainer;
class OptionButton;

class SimpleTerrain3DGizmoPlugin : public EditorNode3DGizmoPlugin {
	GDCLASS(SimpleTerrain3DGizmoPlugin, EditorNode3DGizmoPlugin);

public:
	// The gizmo is intentionally thin: SimpleTerrain3D owns the debug line data, while
	// the plugin only adapts it to the editor's 3D gizmo drawing API.
	bool has_gizmo(Node3D *p_spatial) override;
	String get_gizmo_name() const override;
	int get_priority() const override;
	void redraw(EditorNode3DGizmo *p_gizmo) override;

	SimpleTerrain3DGizmoPlugin();
};

class SimpleTerrainEditorPlugin : public EditorPlugin {
	GDCLASS(SimpleTerrainEditorPlugin, EditorPlugin);

	// The toolbar is shown only while a SimpleTerrain3D node is selected. It lives in
	// the 3D editor menu so terrain painting feels like a viewport mode instead
	// of a generic inspector operation.
	HBoxContainer *toolbar = nullptr;
	Button *mode_button = nullptr;
	OptionButton *operation_button = nullptr;
	EditorSpinSlider *radius_slider = nullptr;
	EditorSpinSlider *strength_slider = nullptr;
	Button *flat_button = nullptr;
	Button *random_button = nullptr;
	Ref<SimpleTerrain3DGizmoPlugin> gizmo_plugin;

	SimpleTerrain3D *terrain = nullptr;
	bool terrain_mode = false;
	bool painting = false;

	// Brush strokes can generate many height edits. Store only changed indices
	// and their before/after values so releasing the mouse does not copy the
	// whole height field for undo.
	bool has_last_brush_position = false;
	Vector3 last_brush_position;
	HashMap<int, int> stroke_index_map;
	PackedInt32Array stroke_indices;
	PackedFloat32Array stroke_before_values;
	PackedFloat32Array stroke_after_values;

	// UI callbacks keep editor behavior separate from the runtime SimpleTerrain3D API.
	// This separation makes the runtime class usable in exported games and keeps
	// the editor plugin easier to replace or port to GDExtension later.
	void _mode_toggled(bool p_pressed);
	void _operation_selected(int p_index);
	void _flat_pressed();
	void _random_pressed();
	void _update_toolbar();
	void _apply_brush(const Vector3 &p_world_position);
	void _record_brush_delta(const Dictionary &p_delta);
	Dictionary _get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
	void _commit_height_undo(const String &p_action_name, const PackedFloat32Array &p_before_heights);
	void _commit_stroke_undo();

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "Simple Terrain"; }
	virtual bool handles(Object *p_object) const override;
	virtual void edit(Object *p_object) override;
	virtual void clear() override;
	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;

	SimpleTerrainEditorPlugin();
};
