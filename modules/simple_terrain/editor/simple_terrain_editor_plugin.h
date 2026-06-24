/**************************************************************************/
/*  simple_terrain_editor_plugin.h                                               */
/**************************************************************************/

#pragma once

#include "../simple_world_object_profile.h"
#include "../simple_world_placement_data.h"
#include "../simple_world_placement_library.h"

#include "editor/docks/editor_dock.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"
#include "core/templates/hash_map.h"
#include "../simple_terrain_3d.h"

class Button;
class ButtonGroup;
class Control;
class EditorFileDialog;
class EditorResourcePicker;
class EditorSpinSlider;
class HBoxContainer;
class ItemList;
class LineEdit;
class OptionButton;
class PanelContainer;
class VBoxContainer;

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

class SimpleWorldPlacementDock : public EditorDock {
	GDCLASS(SimpleWorldPlacementDock, EditorDock);

	SimpleTerrain3D *terrain = nullptr;
	Ref<SimpleWorldPlacementLibrary> connected_library;
	Ref<SimpleWorldObjectProfile> connected_profile;
	ObjectID selected_profile_object_id;
	bool updating = false;

	EditorResourcePicker *library_picker = nullptr;
	EditorResourcePicker *data_picker = nullptr;
	Button *new_library_button = nullptr;
	Button *new_data_button = nullptr;
	LineEdit *search_edit = nullptr;
	OptionButton *category_filter = nullptr;
	ItemList *profile_list = nullptr;
	Button *add_profile_button = nullptr;
	Button *add_scene_button = nullptr;
	Button *duplicate_profile_button = nullptr;
	Button *remove_profile_button = nullptr;
	EditorInspector *profile_inspector = nullptr;
	EditorFileDialog *scene_file_dialog = nullptr;

	void _library_resource_changed(const Ref<Resource> &p_resource);
	void _data_resource_changed(const Ref<Resource> &p_resource);
	void _new_library_pressed();
	void _new_data_pressed();
	void _add_profile_pressed();
	void _add_scene_pressed();
	void _scene_file_selected(const String &p_path);
	void _duplicate_profile_pressed();
	void _remove_profile_pressed();
	void _profile_list_item_selected(int p_index);
	void _profile_list_item_activated(int p_index);
	void _search_text_changed(const String &p_text);
	void _category_selected(int p_index);
	void _editor_selection_changed();
	void _library_changed();
	void _profile_changed();
	void _refresh_after_resource_undo();
	void _refresh_after_resource_undo_deferred();

	void _set_library_with_undo(const Ref<SimpleWorldPlacementLibrary> &p_library, const String &p_action_name);
	void _set_data_with_undo(const Ref<SimpleWorldPlacementData> &p_data, const String &p_action_name);
	void _set_profiles_with_undo(const Array &p_before, const Array &p_after, const String &p_action_name);
	void _connect_library(const Ref<SimpleWorldPlacementLibrary> &p_library);
	void _connect_profile(const Ref<SimpleWorldObjectProfile> &p_profile);
	void _refresh_resource_pickers();
	void _refresh_profile_list();
	void _refresh_profile_inspector();
	void _update_controls();
	void _debug_log(const String &p_message) const;
	void _debug_log_state(const String &p_context) const;
	void _save_resource_if_file_backed(const Ref<Resource> &p_resource) const;
	void _save_current_resources_if_file_backed() const;
	SimpleTerrain3D *_get_selected_terrain() const;
	bool _sync_selected_terrain();
	Vector<String> _get_scene_paths_from_drag_data(const Variant &p_data) const;
	bool _has_scene_files_in_drag_data(const Variant &p_data) const;
	void _add_scene_paths(const Vector<String> &p_paths, const String &p_action_name);
	String _make_unique_profile_id(const String &p_base_id) const;
	Ref<SimpleWorldObjectProfile> _get_selected_profile() const;

protected:
	static void _bind_methods();

public:
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;
	bool can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
	void drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);
	Ref<SimpleWorldObjectProfile> get_selected_profile() const;
	void edit(SimpleTerrain3D *p_terrain);

	SimpleWorldPlacementDock();
};

class SimpleTerrainInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(SimpleTerrainInspectorPlugin, EditorInspectorPlugin);

	SimpleWorldPlacementDock *placement_dock = nullptr;

	void _open_world_objects(Object *p_object);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_end(Object *p_object) override;
	void set_placement_dock(SimpleWorldPlacementDock *p_dock);
};

class SimpleTerrainEditorPlugin : public EditorPlugin {
	GDCLASS(SimpleTerrainEditorPlugin, EditorPlugin);

	// The toolbar is shown only while a SimpleTerrain3D node is selected. It lives in
	// the 3D editor menu so terrain painting feels like a viewport mode instead
	// of a generic inspector operation.
	HBoxContainer *toolbar = nullptr;
	PanelContainer *brush_overlay_panel = nullptr;
	PanelContainer *placement_overlay_panel = nullptr;
	VBoxContainer *brush_options_vbox = nullptr;
	VBoxContainer *placement_options_vbox = nullptr;
	Button *select_mode_button = nullptr;
	Button *edit_mode_button = nullptr;
	Button *placement_mode_button = nullptr;
	Ref<ButtonGroup> mode_button_group;
	OptionButton *operation_button = nullptr;
	Label *placement_status_label = nullptr;
	Label *placement_library_label = nullptr;
	EditorSpinSlider *radius_slider = nullptr;
	EditorSpinSlider *strength_slider = nullptr;
	Button *flat_button = nullptr;
	Button *random_button = nullptr;
	Button *bake_navigation_button = nullptr;
	Button *bake_dynamic_navigation_button = nullptr;
	Ref<SimpleTerrain3DGizmoPlugin> gizmo_plugin;
	Ref<SimpleTerrainInspectorPlugin> inspector_plugin;
	SimpleWorldPlacementDock *placement_dock = nullptr;

	SimpleTerrain3D *terrain = nullptr;
	bool terrain_mode = false;
	bool placement_mode = false;
	bool painting = false;
	bool has_cursor_hit = false;

	// Brush strokes can generate many height edits. Store only changed indices
	// and their before/after values so releasing the mouse does not copy the
	// whole height field for undo.
	bool has_last_brush_position = false;
	Vector3 last_brush_position;
	PackedVector2Array cursor_points;
	Color cursor_color;
	HashMap<int, int> stroke_index_map;
	PackedInt32Array stroke_indices;
	PackedFloat32Array stroke_before_values;
	PackedFloat32Array stroke_after_values;

	// UI callbacks keep editor behavior separate from the runtime SimpleTerrain3D API.
	// This separation makes the runtime class usable in exported games and keeps
	// the editor plugin easier to replace or port to GDExtension later.
	void _select_mode_pressed();
	void _edit_mode_pressed();
	void _placement_mode_pressed();
	void _operation_selected(int p_index);
	void _flat_pressed();
	void _random_pressed();
	void _bake_navigation_pressed();
	void _bake_dynamic_navigation_pressed();
	void _update_toolbar();
	void _update_placement_overlay();
	Node3D *_get_or_create_placement_root(EditorUndoRedoManager *p_undo_redo);
	void _place_selected_profile(Camera3D *p_camera, const Vector2 &p_mouse_position);
	void _set_placement_arrays(SimpleWorldPlacementData *p_data, const PackedStringArray &p_profile_ids, const PackedVector3Array &p_positions, const PackedVector3Array &p_rotations, const PackedVector3Array &p_scales, const PackedVector3Array &p_normals, const PackedInt32Array &p_seeds, const PackedVector2Array &p_chunk_coords);
	void _attach_brush_overlay();
	void _detach_brush_overlay();
	void _attach_placement_overlay();
	void _detach_placement_overlay();
	void _apply_brush(const Vector3 &p_world_position);
	void _record_brush_delta(const Dictionary &p_delta);
	Dictionary _get_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
	Dictionary _get_cursor_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const;
	Color _get_cursor_color() const;
	void _update_cursor_preview(Camera3D *p_camera, const Dictionary &p_hit);
	void _clear_cursor_preview();
	void _draw_over_viewport(Control *p_overlay);
	void _commit_height_undo(const String &p_action_name, const PackedFloat32Array &p_before_heights);
	void _commit_stroke_undo();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "Simple Terrain"; }
	virtual bool handles(Object *p_object) const override;
	virtual void edit(Object *p_object) override;
	virtual void clear() override;
	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	virtual void forward_3d_draw_over_viewport(Control *p_overlay) override;
	virtual void forward_3d_force_draw_over_viewport(Control *p_overlay) override;

	SimpleTerrainEditorPlugin();
};
