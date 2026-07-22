/**************************************************************************/
/*  open_world_placement_preset_dock.h                                    */
/**************************************************************************/

#pragma once

#include "../open_world_placement_3d.h"
#include "../open_world_placement_entry.h"
#include "../open_world_placement_preset.h"

#include "editor/docks/editor_dock.h"
#include "editor/inspector/editor_inspector.h"

class Button;
class EditorResourcePicker;
class HSplitContainer;
class ItemList;
class Label;
class VBoxContainer;

class OpenWorldPlacementPresetDock : public EditorDock {
	GDCLASS(OpenWorldPlacementPresetDock, EditorDock);

	OpenWorldPlacement3D *placement = nullptr;
	Ref<OpenWorldPlacementPreset> connected_preset;
	Ref<OpenWorldPlacementEntry> connected_entry;
	int selected_entry_index = -1;
	bool updating = false;

	EditorResourcePicker *preset_picker = nullptr;
	Button *new_preset_button = nullptr;
	Button *add_entry_button = nullptr;
	Button *duplicate_entry_button = nullptr;
	Button *remove_entry_button = nullptr;
	ItemList *entry_list = nullptr;
	Label *validation_label = nullptr;
	EditorInspector *preset_inspector = nullptr;
	EditorInspector *entry_inspector = nullptr;

	void _preset_resource_changed(const Ref<Resource> &p_resource);
	void _new_preset_pressed();
	void _add_entry_pressed();
	void _duplicate_entry_pressed();
	void _remove_entry_pressed();
	void _entry_list_item_selected(int p_index);
	void _preset_changed();
	void _entry_changed();
	void _connect_preset(const Ref<OpenWorldPlacementPreset> &p_preset);
	void _connect_entry(const Ref<OpenWorldPlacementEntry> &p_entry);
	void _refresh_preset_picker();
	void _refresh_entry_list();
	void _refresh_inspectors();
	void _refresh_validation();
	void _update_controls();
	void _set_preset_with_undo(const Ref<OpenWorldPlacementPreset> &p_preset, const String &p_action_name);
	void _set_entries_with_undo(const Array &p_before, const Array &p_after, const String &p_action_name);
	String _make_unique_entry_id(const String &p_base_id) const;
	Ref<OpenWorldPlacementEntry> _create_default_entry(OpenWorldPlacementEntry::ContentKind p_kind) const;

protected:
	static void _bind_methods();
	void _refresh_after_resource_undo();

public:
	void edit(OpenWorldPlacement3D *p_placement);

	OpenWorldPlacementPresetDock();
};
