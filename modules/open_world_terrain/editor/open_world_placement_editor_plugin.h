/**************************************************************************/
/*  open_world_placement_editor_plugin.h                                  */
/**************************************************************************/

#pragma once

#include "../open_world_placement_3d.h"
#include "../open_world_placement_data.h"
#include "../open_world_placement_preset.h"

#include "open_world_placement_preset_dock.h"

#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"

class Button;
class ButtonGroup;
class Camera3D;
class Control;
class Label;
class PanelContainer;
class SimpleTerrain3D;
class VBoxContainer;

class OpenWorldPlacementEditorPlugin : public EditorPlugin {
	GDCLASS(OpenWorldPlacementEditorPlugin, EditorPlugin);

	HBoxContainer *toolbar = nullptr;
	PanelContainer *overlay_panel = nullptr;
	VBoxContainer *overlay_vbox = nullptr;
	Button *select_mode_button = nullptr;
	Button *apply_mode_button = nullptr;
	Button *presets_button = nullptr;
	OpenWorldPlacementPresetDock *preset_dock = nullptr;
	Ref<ButtonGroup> mode_button_group;
	Label *status_label = nullptr;
	Label *preview_label = nullptr;

	OpenWorldPlacement3D *placement = nullptr;
	bool apply_mode = false;
	bool has_cursor_hit = false;
	Vector3 cursor_center;
	PackedVector2Array cursor_points;
	Dictionary last_preview_report;

	void _select_mode_pressed();
	void _apply_mode_pressed();
	void _presets_pressed();
	void _update_toolbar();
	void _attach_overlay();
	void _detach_overlay();
	void _update_overlay();
	SimpleTerrain3D *_resolve_terrain() const;
	Dictionary _get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
	void _apply_at_mouse(Camera3D *p_camera, const Vector2 &p_mouse_position);
	void _update_cursor(Camera3D *p_camera, const Dictionary &p_hit);
	void _clear_cursor();
	void _draw_over_viewport(Control *p_overlay);
	void _restore_placement_snapshot(Object *p_placement, const Ref<OpenWorldPlacementData> &p_snapshot);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "World Placement"; }
	virtual bool handles(Object *p_object) const override;
	virtual void edit(Object *p_object) override;
	virtual void clear() override;
	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	virtual void forward_3d_draw_over_viewport(Control *p_overlay) override;
	virtual void forward_3d_force_draw_over_viewport(Control *p_overlay) override;

	OpenWorldPlacementEditorPlugin();
};
