/**************************************************************************/
/*  open_world_placement_editor_plugin.h                                  */
/**************************************************************************/

#pragma once

#include "../open_world_placement_3d.h"
#include "../open_world_placement_data.h"
#include "../open_world_placement_preset.h"

#include "open_world_placement_preset_dock.h"

#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_inspector.h"
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

class OpenWorldPlacementContextMenuPlugin : public EditorContextMenuPlugin {
	GDCLASS(OpenWorldPlacementContextMenuPlugin, EditorContextMenuPlugin);

	void _rebuild_generated(const Variant &p_arg);
	void _clear_generated(const Variant &p_arg);

protected:
	static void _bind_methods();

public:
	virtual void get_options(const OptionsData &p_data) override;
};

class OpenWorldPlacementInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(OpenWorldPlacementInspectorPlugin, EditorInspectorPlugin);

	void _rebuild_generated(Object *p_object);
	void _clear_generated(Object *p_object);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_end(Object *p_object) override;
};

class OpenWorldPlacementEditorPlugin : public EditorPlugin {
	GDCLASS(OpenWorldPlacementEditorPlugin, EditorPlugin);

	HBoxContainer *toolbar = nullptr;
	PanelContainer *overlay_panel = nullptr;
	VBoxContainer *overlay_vbox = nullptr;
	Button *select_mode_button = nullptr;
	Button *apply_mode_button = nullptr;
	Button *presets_button = nullptr;
	Button *confirm_apply_button = nullptr;
	Button *cancel_pending_button = nullptr;
	OpenWorldPlacementPresetDock *preset_dock = nullptr;
	Ref<ButtonGroup> mode_button_group;
	Ref<OpenWorldPlacementContextMenuPlugin> context_menu_plugin;
	Ref<OpenWorldPlacementInspectorPlugin> inspector_plugin;
	Label *status_label = nullptr;
	Label *preview_label = nullptr;

	OpenWorldPlacement3D *placement = nullptr;
	bool apply_mode = false;
	bool pending_confirm = false;
	bool has_cursor_hit = false;
	Vector3 cursor_center;
	Vector3 pending_center;
	real_t pending_ray_origin_y = OpenWorldPlacement3D::VERTICAL_RAY_ORIGIN_AUTO;
	PackedVector2Array cursor_points;
	Dictionary last_preview_report;

	void _select_mode_pressed();
	void _apply_mode_pressed();
	void _presets_pressed();
	void _confirm_apply_pressed();
	void _cancel_pending_pressed();
	void _update_toolbar();
	void _attach_overlay();
	void _detach_overlay();
	void _update_overlay();
	void _set_pending(bool p_pending);
	SimpleTerrain3D *_find_terrain_for_center(const Vector3 &p_world_center, Camera3D *p_camera) const;
	Dictionary _get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
	void _lock_pending_at_mouse(Camera3D *p_camera, const Vector2 &p_mouse_position);
	void _confirm_pending_apply(Camera3D *p_camera);
	void _apply_at_world_position(const Vector3 &p_world_position, real_t p_vertical_ray_origin_y);
	void _update_cursor(Camera3D *p_camera, const Dictionary &p_hit, bool p_locked_style);
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
