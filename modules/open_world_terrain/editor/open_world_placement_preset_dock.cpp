/**************************************************************************/
/*  open_world_placement_preset_dock.cpp                                  */
/**************************************************************************/

#include "open_world_placement_preset_dock.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/templates/hash_set.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/split_container.h"

void OpenWorldPlacementPresetDock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_refresh_after_resource_undo"), &OpenWorldPlacementPresetDock::_refresh_after_resource_undo);
}

void OpenWorldPlacementPresetDock::_refresh_after_resource_undo() {
	_refresh_preset_picker();
	_refresh_entry_list();
	_refresh_inspectors();
	_refresh_validation();
	_update_controls();
}

String OpenWorldPlacementPresetDock::_make_unique_entry_id(const String &p_base_id) const {
	if (connected_preset.is_null()) {
		return p_base_id;
	}
	const String base = p_base_id.is_empty() ? String("entry") : p_base_id;
	HashSet<String> used;
	for (int i = 0; i < connected_preset->get_entry_count(); i++) {
		Ref<OpenWorldPlacementEntry> entry = connected_preset->get_entry(i);
		if (entry.is_valid()) {
			used.insert(entry->get_stable_id());
		}
	}
	if (!used.has(base)) {
		return base;
	}
	for (int suffix = 2; suffix < 10000; suffix++) {
		const String candidate = vformat("%s_%d", base, suffix);
		if (!used.has(candidate)) {
			return candidate;
		}
	}
	return vformat("%s_%d", base, OS::get_singleton()->get_ticks_usec());
}

Ref<OpenWorldPlacementEntry> OpenWorldPlacementPresetDock::_create_default_entry(OpenWorldPlacementEntry::ContentKind p_kind) const {
	Ref<OpenWorldPlacementEntry> entry;
	entry.instantiate();
	entry->set_content_kind(p_kind);
	switch (p_kind) {
		case OpenWorldPlacementEntry::CONTENT_TREE: {
			entry->set_stable_id(_make_unique_entry_id("tree"));
			Ref<OpenWorldTreeGenerationProfile> profile;
			profile.instantiate();
			entry->set_tree_profile(profile);
		} break;
		case OpenWorldPlacementEntry::CONTENT_ROCK: {
			entry->set_stable_id(_make_unique_entry_id("rock"));
			Ref<OpenWorldRockGenerationProfile> profile;
			profile.instantiate();
			Ref<OpenWorldRockGenerationRequest> request;
			request.instantiate();
			request->set_profile(profile);
			entry->set_rock_request_template(request);
		} break;
		case OpenWorldPlacementEntry::CONTENT_VINE: {
			entry->set_stable_id(_make_unique_entry_id("bramble"));
			Ref<OpenWorldVineGenerationProfile> profile;
			profile.instantiate();
			Ref<OpenWorldVineGenerationRequest> request;
			request.instantiate();
			request->set_mode(OpenWorldVineGenerationRequest::MODE_BRAMBLE);
			request->set_profile(profile);
			entry->set_vine_request_template(request);
		} break;
	}
	return entry;
}

void OpenWorldPlacementPresetDock::_set_preset_with_undo(const Ref<OpenWorldPlacementPreset> &p_preset, const String &p_action_name) {
	if (placement == nullptr) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(placement, "active_preset", p_preset);
	undo_redo->add_undo_property(placement, "active_preset", placement->get_active_preset());
	undo_redo->commit_action();
	_connect_preset(placement->get_active_preset());
	_refresh_preset_picker();
	_refresh_entry_list();
	_refresh_inspectors();
	_refresh_validation();
	_update_controls();
}

void OpenWorldPlacementPresetDock::_set_entries_with_undo(const Array &p_before, const Array &p_after, const String &p_action_name) {
	if (connected_preset.is_null()) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_method(connected_preset.ptr(), "set_entries", p_after);
	undo_redo->add_undo_method(connected_preset.ptr(), "set_entries", p_before);
	undo_redo->add_do_reference(connected_preset.ptr());
	undo_redo->add_undo_reference(connected_preset.ptr());
	undo_redo->commit_action();
	_refresh_entry_list();
	_refresh_inspectors();
	_refresh_validation();
}

void OpenWorldPlacementPresetDock::_connect_preset(const Ref<OpenWorldPlacementPreset> &p_preset) {
	if (connected_preset.is_valid()) {
		connected_preset->disconnect_changed(callable_mp(this, &OpenWorldPlacementPresetDock::_preset_changed));
	}
	connected_preset = p_preset;
	if (connected_preset.is_valid()) {
		connected_preset->connect_changed(callable_mp(this, &OpenWorldPlacementPresetDock::_preset_changed));
	}
}

void OpenWorldPlacementPresetDock::_connect_entry(const Ref<OpenWorldPlacementEntry> &p_entry) {
	if (connected_entry.is_valid()) {
		connected_entry->disconnect_changed(callable_mp(this, &OpenWorldPlacementPresetDock::_entry_changed));
	}
	connected_entry = p_entry;
	if (connected_entry.is_valid()) {
		connected_entry->connect_changed(callable_mp(this, &OpenWorldPlacementPresetDock::_entry_changed));
	}
}

void OpenWorldPlacementPresetDock::_preset_changed() {
	if (updating) {
		return;
	}
	_refresh_entry_list();
	_refresh_validation();
}

void OpenWorldPlacementPresetDock::_entry_changed() {
	if (updating) {
		return;
	}
	_refresh_entry_list();
	_refresh_validation();
}

void OpenWorldPlacementPresetDock::_preset_resource_changed(const Ref<Resource> &p_resource) {
	if (updating || placement == nullptr) {
		return;
	}
	Ref<OpenWorldPlacementPreset> preset = p_resource;
	if (preset == placement->get_active_preset()) {
		_connect_preset(preset);
		_refresh_entry_list();
		_refresh_inspectors();
		_refresh_validation();
		return;
	}
	_set_preset_with_undo(preset, TTR("Assign Placement Preset"));
}

void OpenWorldPlacementPresetDock::_new_preset_pressed() {
	if (placement == nullptr) {
		return;
	}
	Ref<OpenWorldPlacementPreset> preset;
	preset.instantiate();
	preset->set_stable_id("new-preset");
	preset->set_display_name(TTR("New Preset"));
	preset->add_entry(_create_default_entry(OpenWorldPlacementEntry::CONTENT_TREE));
	_set_preset_with_undo(preset, TTR("Create Placement Preset"));
}

void OpenWorldPlacementPresetDock::_add_entry_pressed() {
	if (connected_preset.is_null()) {
		return;
	}
	const Array before = connected_preset->get_entries();
	Array after = before.duplicate();
	after.push_back(_create_default_entry(OpenWorldPlacementEntry::CONTENT_TREE));
	_set_entries_with_undo(before, after, TTR("Add Placement Entry"));
	selected_entry_index = after.size() - 1;
	entry_list->select(selected_entry_index);
	_refresh_inspectors();
}

void OpenWorldPlacementPresetDock::_duplicate_entry_pressed() {
	if (connected_preset.is_null() || selected_entry_index < 0 || selected_entry_index >= connected_preset->get_entry_count()) {
		return;
	}
	Ref<OpenWorldPlacementEntry> source = connected_preset->get_entry(selected_entry_index);
	if (source.is_null()) {
		return;
	}
	Ref<OpenWorldPlacementEntry> copy = source->duplicate(true);
	if (copy.is_null()) {
		return;
	}
	copy->set_stable_id(_make_unique_entry_id(source->get_stable_id() + "_copy"));
	const Array before = connected_preset->get_entries();
	Array after = before.duplicate();
	after.push_back(copy);
	_set_entries_with_undo(before, after, TTR("Duplicate Placement Entry"));
	selected_entry_index = after.size() - 1;
	entry_list->select(selected_entry_index);
	_refresh_inspectors();
}

void OpenWorldPlacementPresetDock::_remove_entry_pressed() {
	if (connected_preset.is_null() || selected_entry_index < 0 || selected_entry_index >= connected_preset->get_entry_count()) {
		return;
	}
	const Array before = connected_preset->get_entries();
	Array after = before.duplicate();
	after.remove_at(selected_entry_index);
	_set_entries_with_undo(before, after, TTR("Remove Placement Entry"));
	selected_entry_index = -1;
	_refresh_inspectors();
}

void OpenWorldPlacementPresetDock::_entry_list_item_selected(int p_index) {
	selected_entry_index = p_index;
	_refresh_inspectors();
}

void OpenWorldPlacementPresetDock::_refresh_preset_picker() {
	updating = true;
	if (placement != nullptr) {
		preset_picker->set_edited_resource(placement->get_active_preset());
	}
	updating = false;
}

void OpenWorldPlacementPresetDock::_refresh_entry_list() {
	updating = true;
	entry_list->clear();
	if (connected_preset.is_valid()) {
		for (int i = 0; i < connected_preset->get_entry_count(); i++) {
			Ref<OpenWorldPlacementEntry> entry = connected_preset->get_entry(i);
			if (entry.is_null()) {
				entry_list->add_item(vformat(TTR("Entry %d (missing)"), i));
				continue;
			}
			String kind;
			switch (entry->get_content_kind()) {
				case OpenWorldPlacementEntry::CONTENT_VINE:
					kind = TTR("Vine");
					break;
				case OpenWorldPlacementEntry::CONTENT_ROCK:
					kind = TTR("Rock");
					break;
				case OpenWorldPlacementEntry::CONTENT_TREE:
				default:
					kind = TTR("Tree");
					break;
			}
			const String entry_title = entry->get_stable_id().is_empty() ? vformat(TTR("%s %d"), kind, i + 1) : entry->get_stable_id();
			const String label = entry->is_enabled() ? vformat("%s  (w=%.2f)", entry_title, entry->get_weight()) : vformat("%s  [%s]", entry_title, TTR("disabled"));
			entry_list->add_item(label);
		}
	}
	if (selected_entry_index >= 0 && selected_entry_index < entry_list->get_item_count()) {
		entry_list->select(selected_entry_index);
	} else {
		selected_entry_index = -1;
	}
	updating = false;
}

void OpenWorldPlacementPresetDock::_refresh_inspectors() {
	Ref<OpenWorldPlacementEntry> entry;
	if (connected_preset.is_valid() && selected_entry_index >= 0 && selected_entry_index < connected_preset->get_entry_count()) {
		entry = connected_preset->get_entry(selected_entry_index);
	}
	_connect_entry(entry);
	if (entry.is_valid()) {
		preset_inspector->edit(nullptr);
		entry_inspector->edit(entry.ptr());
	} else if (connected_preset.is_valid()) {
		entry_inspector->edit(nullptr);
		preset_inspector->edit(connected_preset.ptr());
	} else {
		preset_inspector->edit(nullptr);
		entry_inspector->edit(nullptr);
	}
}

void OpenWorldPlacementPresetDock::_refresh_validation() {
	if (connected_preset.is_null()) {
		validation_label->set_text(TTRC("Assign or create a placement preset."));
		return;
	}
	const Dictionary report = connected_preset->validate_preset();
	if ((bool)report.get("success", false)) {
		validation_label->set_text(vformat(TTR("Valid preset — requested %d / max %d"), (int)report.get("requested_count", 0), (int)report.get("max_objects_per_operation", 0)));
	} else {
		PackedStringArray codes = report.get("error_codes", PackedStringArray());
		validation_label->set_text(codes.is_empty() ? TTRC("Preset validation failed.") : vformat(TTR("Invalid: %s"), String(", ").join(codes)));
	}
}

void OpenWorldPlacementPresetDock::_update_controls() {
	const bool has_placement = placement != nullptr;
	const bool has_preset = connected_preset.is_valid();
	preset_picker->set_editable(has_placement);
	new_preset_button->set_disabled(!has_placement);
	add_entry_button->set_disabled(!has_preset);
	duplicate_entry_button->set_disabled(!has_preset || selected_entry_index < 0);
	remove_entry_button->set_disabled(!has_preset || selected_entry_index < 0);
}

void OpenWorldPlacementPresetDock::edit(OpenWorldPlacement3D *p_placement) {
	placement = p_placement;
	selected_entry_index = -1;
	_connect_preset(placement != nullptr ? placement->get_active_preset() : Ref<OpenWorldPlacementPreset>());
	_connect_entry(Ref<OpenWorldPlacementEntry>());
	_refresh_preset_picker();
	_refresh_entry_list();
	_refresh_inspectors();
	_refresh_validation();
	_update_controls();
}

OpenWorldPlacementPresetDock::OpenWorldPlacementPresetDock() {
	set_name(TTRC("World Placement Presets"));

	VBoxContainer *root = memnew(VBoxContainer);
	add_child(root);

	Label *preset_heading = memnew(Label);
	preset_heading->set_text(TTRC("Active Preset"));
	root->add_child(preset_heading);

	HBoxContainer *preset_row = memnew(HBoxContainer);
	root->add_child(preset_row);

	preset_picker = memnew(EditorResourcePicker);
	preset_picker->set_base_type("OpenWorldPlacementPreset");
	preset_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	preset_picker->connect("resource_changed", callable_mp(this, &OpenWorldPlacementPresetDock::_preset_resource_changed));
	preset_row->add_child(preset_picker);

	new_preset_button = memnew(Button);
	new_preset_button->set_text(TTRC("New"));
	new_preset_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementPresetDock::_new_preset_pressed));
	preset_row->add_child(new_preset_button);

	validation_label = memnew(Label);
	validation_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	root->add_child(validation_label);

	Label *entry_heading = memnew(Label);
	entry_heading->set_text(TTRC("Weighted Entries"));
	root->add_child(entry_heading);

	HBoxContainer *entry_buttons = memnew(HBoxContainer);
	add_entry_button = memnew(Button);
	add_entry_button->set_text(TTRC("Add Tree"));
	add_entry_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementPresetDock::_add_entry_pressed));
	entry_buttons->add_child(add_entry_button);

	duplicate_entry_button = memnew(Button);
	duplicate_entry_button->set_text(TTRC("Duplicate"));
	duplicate_entry_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementPresetDock::_duplicate_entry_pressed));
	entry_buttons->add_child(duplicate_entry_button);

	remove_entry_button = memnew(Button);
	remove_entry_button->set_text(TTRC("Remove"));
	remove_entry_button->connect(SceneStringName(pressed), callable_mp(this, &OpenWorldPlacementPresetDock::_remove_entry_pressed));
	entry_buttons->add_child(remove_entry_button);
	root->add_child(entry_buttons);

	HSplitContainer *split = memnew(HSplitContainer);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(split);

	entry_list = memnew(ItemList);
	entry_list->set_custom_minimum_size(Size2(180, 240) * EDSCALE);
	entry_list->connect(SceneStringName(item_selected), callable_mp(this, &OpenWorldPlacementPresetDock::_entry_list_item_selected));
	split->add_child(entry_list);

	VBoxContainer *inspector_column = memnew(VBoxContainer);
	inspector_column->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	split->add_child(inspector_column);

	preset_inspector = memnew(EditorInspector);
	preset_inspector->set_use_wide_editors(true);
	preset_inspector->set_hide_script(true);
	preset_inspector->set_mark_unsaved(true);
	preset_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	inspector_column->add_child(preset_inspector);

	entry_inspector = memnew(EditorInspector);
	entry_inspector->set_use_wide_editors(true);
	entry_inspector->set_hide_script(true);
	entry_inspector->set_mark_unsaved(true);
	entry_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	inspector_column->add_child(entry_inspector);

	if (EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton()) {
		undo_redo->connect("history_changed", callable_mp(this, &OpenWorldPlacementPresetDock::_refresh_after_resource_undo));
	}
}
