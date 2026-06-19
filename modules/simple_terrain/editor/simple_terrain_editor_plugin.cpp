/**************************************************************************/
/*  simple_terrain_editor_plugin.cpp                                             */
/**************************************************************************/

#include "simple_terrain_editor_plugin.h"

#include "core/io/resource_loader.h"
#include "core/math/random_pcg.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"
#include "core/templates/hash_set.h"
#include "editor/editor_data.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/navigation/navigation_obstacle_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/main/window.h"

void SimpleWorldPlacementDock::_library_resource_changed(const Ref<Resource> &p_resource) {
	_debug_log(vformat("library_resource_changed resource=%s updating=%s", p_resource.is_valid() ? p_resource->get_path() : String("<null>"), updating ? "true" : "false"));
	if (updating || !_sync_selected_terrain()) {
		_debug_log("library_resource_changed ignored");
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library = p_resource;
	_set_library_with_undo(library, TTR("Set World Placement Library"));
}

void SimpleWorldPlacementDock::_data_resource_changed(const Ref<Resource> &p_resource) {
	_debug_log(vformat("data_resource_changed resource=%s updating=%s", p_resource.is_valid() ? p_resource->get_path() : String("<null>"), updating ? "true" : "false"));
	if (updating || !_sync_selected_terrain()) {
		_debug_log("data_resource_changed ignored");
		return;
	}
	Ref<SimpleWorldPlacementData> placement_data = p_resource;
	_set_data_with_undo(placement_data, TTR("Set World Placement Data"));
}

void SimpleWorldPlacementDock::_new_library_pressed() {
	if (!_sync_selected_terrain()) {
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library;
	library.instantiate();
	_set_library_with_undo(library, TTR("Create World Placement Library"));
}

void SimpleWorldPlacementDock::_new_data_pressed() {
	if (!_sync_selected_terrain()) {
		return;
	}
	Ref<SimpleWorldPlacementData> placement_data;
	placement_data.instantiate();
	placement_data->set_terrain_path(terrain->get_path());
	placement_data->set_library(terrain->get_world_placement_library());
	_set_data_with_undo(placement_data, TTR("Create World Placement Data"));
}

void SimpleWorldPlacementDock::_add_profile_pressed() {
	_debug_log_state("add_profile_pressed:begin");
	if (!_sync_selected_terrain()) {
		_debug_log("add_profile_pressed ignored: no terrain");
		return;
	}

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	if (library.is_null()) {
		_debug_log("add_profile_pressed: creating transient library");
		library.instantiate();
	}

	int index = 1;
	String id;
	do {
		id = vformat("profile_%d", index++);
	} while (library->has_profile_id(id));

	Ref<SimpleWorldObjectProfile> profile;
	profile.instantiate();
	profile->set_id(id);
	profile->set_display_name(vformat("Profile %d", index - 1));

	Array before = library->get_profiles().duplicate();
	Array after = before.duplicate();
	after.push_back(profile);
	selected_profile_object_id = profile->get_instance_id();
	_debug_log(vformat("add_profile_pressed profile id=%s before=%d after=%d selected=%d", id, before.size(), after.size(), (uint64_t)selected_profile_object_id));

	if (terrain->get_world_placement_library().is_null()) {
		library->set_profiles(after);
		_set_library_with_undo(library, TTR("Add World Object Profile"));
	} else {
		_set_profiles_with_undo(before, after, TTR("Add World Object Profile"));
	}
	_debug_log_state("add_profile_pressed:end");
}

void SimpleWorldPlacementDock::_add_scene_pressed() {
	if (!_sync_selected_terrain()) {
		return;
	}
	scene_file_dialog->popup_file_dialog();
}

void SimpleWorldPlacementDock::_scene_file_selected(const String &p_path) {
	_debug_log(vformat("scene_file_selected path=%s", p_path));
	Vector<String> paths;
	paths.push_back(p_path);
	_add_scene_paths(paths, TTR("Add Scene to World Placement Library"));
}

void SimpleWorldPlacementDock::_duplicate_profile_pressed() {
	if (!_sync_selected_terrain()) {
		return;
	}
	Ref<SimpleWorldObjectProfile> source = _get_selected_profile();
	if (terrain == nullptr || terrain->get_world_placement_library().is_null() || source.is_null()) {
		return;
	}

	Ref<SimpleWorldObjectProfile> duplicate;
	duplicate.instantiate();
	duplicate->set_id(_make_unique_profile_id(source->get_id().is_empty() ? "profile" : source->get_id()));
	duplicate->set_display_name(source->get_display_name().is_empty() ? duplicate->get_id() : vformat("%s Copy", source->get_display_name()));
	duplicate->set_category(source->get_category());
	duplicate->set_scene(source->get_scene());
	duplicate->set_preview_icon(source->get_preview_icon());
	duplicate->set_placement_type(source->get_placement_type());
	duplicate->set_collision_radius(source->get_collision_radius());
	duplicate->set_spacing(source->get_spacing());
	duplicate->set_density(source->get_density());
	duplicate->set_min_scale(source->get_min_scale());
	duplicate->set_max_scale(source->get_max_scale());
	duplicate->set_random_yaw(source->is_random_yaw_enabled());
	duplicate->set_align_to_terrain_normal(source->is_aligning_to_terrain_normal());
	duplicate->set_slope_min_degrees(source->get_slope_min_degrees());
	duplicate->set_slope_max_degrees(source->get_slope_max_degrees());
	duplicate->set_height_min(source->get_height_min());
	duplicate->set_height_max(source->get_height_max());
	duplicate->set_surface_offset(source->get_surface_offset());
	duplicate->set_tags(source->get_tags());
	duplicate->set_navigation_obstacle_mode(source->get_navigation_obstacle_mode());
	duplicate->set_navigation_obstacle_radius(source->get_navigation_obstacle_radius());
	duplicate->set_navigation_obstacle_height(source->get_navigation_obstacle_height());
	duplicate->set_navigation_obstacle_carve(source->get_navigation_obstacle_carve());
	duplicate->set_navigation_avoidance_layers(source->get_navigation_avoidance_layers());

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	Array before = library->get_profiles().duplicate();
	Array after = before.duplicate();
	after.push_back(duplicate);
	selected_profile_object_id = duplicate->get_instance_id();
	_set_profiles_with_undo(before, after, TTR("Duplicate World Object Profile"));
}

void SimpleWorldPlacementDock::_remove_profile_pressed() {
	if (!_sync_selected_terrain() || terrain->get_world_placement_library().is_null()) {
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	Ref<SimpleWorldObjectProfile> profile = _get_selected_profile();
	if (profile.is_null()) {
		return;
	}

	int index = -1;
	const Array profiles = library->get_profiles();
	for (int i = 0; i < profiles.size(); i++) {
		Ref<SimpleWorldObjectProfile> candidate = profiles[i];
		if (candidate == profile) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		return;
	}

	Array before = library->get_profiles().duplicate();
	Array after = before.duplicate();
	after.remove_at(index);
	selected_profile_object_id = ObjectID();
	_set_profiles_with_undo(before, after, TTR("Remove World Object Profile"));
}

void SimpleWorldPlacementDock::_profile_list_item_selected(int p_index) {
	ERR_FAIL_INDEX(p_index, profile_list->get_item_count());
	const uint64_t profile_id = profile_list->get_item_metadata(p_index);
	selected_profile_object_id = ObjectID(profile_id);
	_debug_log(vformat("profile_list_item_selected index=%d selected=%d", p_index, profile_id));
	_refresh_profile_inspector();
	_update_controls();
}

void SimpleWorldPlacementDock::_search_text_changed(const String &p_text) {
	_refresh_profile_list();
}

void SimpleWorldPlacementDock::_category_selected(int p_index) {
	_refresh_profile_list();
}

void SimpleWorldPlacementDock::_editor_selection_changed() {
	SimpleTerrain3D *selected_terrain = _get_selected_terrain();
	if (selected_terrain != nullptr || terrain != nullptr) {
		edit(selected_terrain);
		return;
	}

	_update_controls();
}

void SimpleWorldPlacementDock::_library_changed() {
	_debug_log_state("library_changed");
	_refresh_profile_list();
	_refresh_profile_inspector();
	_update_controls();
}

void SimpleWorldPlacementDock::_profile_changed() {
	_debug_log_state("profile_changed");
	_refresh_profile_list();
	_update_controls();
	_save_current_resources_if_file_backed();
}

void SimpleWorldPlacementDock::_refresh_after_resource_undo() {
	_debug_log_state("refresh_after_resource_undo:begin");
	_refresh_resource_pickers();
	_connect_library(terrain != nullptr ? terrain->get_world_placement_library() : Ref<SimpleWorldPlacementLibrary>());
	_refresh_profile_list();
	_refresh_profile_inspector();
	_update_controls();
	_save_current_resources_if_file_backed();
	profile_list->queue_redraw();
	profile_inspector->queue_redraw();
	_debug_log_state("refresh_after_resource_undo:end");
}

void SimpleWorldPlacementDock::_refresh_after_resource_undo_deferred() {
	_debug_log("refresh_after_resource_undo_deferred scheduled");
	call_deferred(SNAME("_refresh_after_resource_undo"));
}

void SimpleWorldPlacementDock::_set_library_with_undo(const Ref<SimpleWorldPlacementLibrary> &p_library, const String &p_action_name) {
	_debug_log(vformat("set_library_with_undo action=%s incoming=%s current=%s", p_action_name, p_library.is_valid() ? p_library->get_path() : String("<null>"), terrain != nullptr && terrain->get_world_placement_library().is_valid() ? terrain->get_world_placement_library()->get_path() : String("<null>")));
	if (terrain == nullptr || terrain->get_world_placement_library() == p_library) {
		_debug_log("set_library_with_undo ignored");
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(terrain, "world_placement_library", p_library);
	undo_redo->add_do_method(this, "_refresh_after_resource_undo");
	undo_redo->add_undo_property(terrain, "world_placement_library", terrain->get_world_placement_library());
	undo_redo->add_undo_method(this, "_refresh_after_resource_undo");
	undo_redo->commit_action();
	terrain->set_world_placement_library(p_library);
	_refresh_after_resource_undo();
	_refresh_after_resource_undo_deferred();
}

void SimpleWorldPlacementDock::_set_data_with_undo(const Ref<SimpleWorldPlacementData> &p_data, const String &p_action_name) {
	if (terrain == nullptr || terrain->get_world_placement_data() == p_data) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(terrain, "world_placement_data", p_data);
	undo_redo->add_do_method(this, "_refresh_after_resource_undo");
	undo_redo->add_undo_property(terrain, "world_placement_data", terrain->get_world_placement_data());
	undo_redo->add_undo_method(this, "_refresh_after_resource_undo");
	undo_redo->commit_action();
	terrain->set_world_placement_data(p_data);
	_refresh_after_resource_undo();
	_refresh_after_resource_undo_deferred();
}

void SimpleWorldPlacementDock::_set_profiles_with_undo(const Array &p_before, const Array &p_after, const String &p_action_name) {
	_debug_log(vformat("set_profiles_with_undo action=%s before=%d after=%d", p_action_name, p_before.size(), p_after.size()));
	if (terrain == nullptr || terrain->get_world_placement_library().is_null()) {
		_debug_log("set_profiles_with_undo ignored: no terrain/library");
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	Array before = p_before.duplicate();
	Array after = p_after.duplicate();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(library.ptr(), "profiles", after);
	undo_redo->add_do_method(this, "_refresh_after_resource_undo");
	undo_redo->add_undo_property(library.ptr(), "profiles", before);
	undo_redo->add_undo_method(this, "_refresh_after_resource_undo");
	undo_redo->commit_action();
	library->set_profiles(after);
	_refresh_after_resource_undo();
	_refresh_after_resource_undo_deferred();
}

void SimpleWorldPlacementDock::_connect_library(const Ref<SimpleWorldPlacementLibrary> &p_library) {
	if (connected_library == p_library) {
		return;
	}
	if (connected_library.is_valid()) {
		connected_library->disconnect_changed(callable_mp(this, &SimpleWorldPlacementDock::_library_changed));
	}
	connected_library = p_library;
	if (connected_library.is_valid()) {
		connected_library->connect_changed(callable_mp(this, &SimpleWorldPlacementDock::_library_changed));
	}
}

void SimpleWorldPlacementDock::_connect_profile(const Ref<SimpleWorldObjectProfile> &p_profile) {
	if (connected_profile == p_profile) {
		return;
	}
	if (connected_profile.is_valid()) {
		connected_profile->disconnect_changed(callable_mp(this, &SimpleWorldPlacementDock::_profile_changed));
	}
	connected_profile = p_profile;
	if (connected_profile.is_valid()) {
		connected_profile->connect_changed(callable_mp(this, &SimpleWorldPlacementDock::_profile_changed));
	}
}

void SimpleWorldPlacementDock::_refresh_resource_pickers() {
	updating = true;
	library_picker->set_resource_owner(terrain);
	library_picker->set_property_path("world_placement_library");
	data_picker->set_resource_owner(terrain);
	data_picker->set_property_path("world_placement_data");
	Ref<Resource> library_resource = terrain != nullptr ? Ref<Resource>(terrain->get_world_placement_library()) : Ref<Resource>();
	Ref<Resource> data_resource = terrain != nullptr ? Ref<Resource>(terrain->get_world_placement_data()) : Ref<Resource>();
	library_picker->set_edited_resource(library_resource);
	data_picker->set_edited_resource(data_resource);
	updating = false;
}

void SimpleWorldPlacementDock::_refresh_profile_list() {
	const ObjectID previous_selection = selected_profile_object_id;
	const String selected_category = category_filter->get_selected() > 0 ? category_filter->get_item_text(category_filter->get_selected()) : String();
	profile_list->clear();
	category_filter->clear();
	category_filter->add_item(TTRC("All"), 0);

	if (terrain == nullptr || terrain->get_world_placement_library().is_null()) {
		selected_profile_object_id = ObjectID();
		_debug_log("refresh_profile_list empty: no terrain/library");
		return;
	}

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	const Array profiles = library->get_profiles();
	Vector<String> categories;
	for (int i = 0; i < profiles.size(); i++) {
		Ref<SimpleWorldObjectProfile> profile = profiles[i];
		if (profile.is_valid() && !profile->get_category().is_empty() && categories.find(profile->get_category()) < 0) {
			categories.push_back(profile->get_category());
		}
	}

	for (int i = 0; i < categories.size(); i++) {
		category_filter->add_item(categories[i], i + 1);
		if (categories[i] == selected_category) {
			category_filter->select(i + 1);
		}
	}

	const String search_text = search_edit->get_text().strip_edges().to_lower();
	const String active_category = category_filter->get_selected() > 0 ? category_filter->get_item_text(category_filter->get_selected()) : String();
	int selected_item = -1;
	for (int i = 0; i < profiles.size(); i++) {
		Ref<SimpleWorldObjectProfile> profile = profiles[i];
		if (profile.is_null()) {
			continue;
		}
		if (!active_category.is_empty() && profile->get_category() != active_category) {
			continue;
		}
		const String label = profile->get_display_name().is_empty() ? profile->get_id() : profile->get_display_name();
		const String haystack = vformat("%s %s %s", label, profile->get_id(), profile->get_category()).to_lower();
		if (!search_text.is_empty() && !haystack.contains(search_text)) {
			continue;
		}

		const int item_index = profile_list->add_item(label.is_empty() ? TTRC("Unnamed Profile") : label);
		profile_list->set_item_metadata(item_index, (uint64_t)profile->get_instance_id());
		if (profile->get_instance_id() == previous_selection) {
			selected_item = item_index;
		}
	}

	if (selected_item >= 0) {
		profile_list->select(selected_item);
		selected_profile_object_id = previous_selection;
	} else if (profile_list->get_item_count() > 0) {
		profile_list->select(0);
		selected_profile_object_id = ObjectID((uint64_t)profile_list->get_item_metadata(0));
	} else {
		selected_profile_object_id = ObjectID();
	}
	_debug_log(vformat("refresh_profile_list profiles=%d visible=%d previous_selected=%d selected=%d search='%s' category='%s'", profiles.size(), profile_list->get_item_count(), (uint64_t)previous_selection, (uint64_t)selected_profile_object_id, search_text, active_category));
}

void SimpleWorldPlacementDock::_refresh_profile_inspector() {
	Ref<SimpleWorldObjectProfile> profile = _get_selected_profile();
	_connect_profile(profile);
	_debug_log(vformat("refresh_profile_inspector profile=%s selected=%d", profile.is_valid() ? profile->get_id() : String("<null>"), (uint64_t)selected_profile_object_id));
	profile_inspector->edit(nullptr);
	profile_inspector->edit(profile.ptr());
}

void SimpleWorldPlacementDock::_update_controls() {
	const bool has_terrain = terrain != nullptr;
	const bool has_library = has_terrain && terrain->get_world_placement_library().is_valid();
	const bool has_profile = _get_selected_profile().is_valid();
	library_picker->set_editable(has_terrain);
	data_picker->set_editable(has_terrain);
	new_library_button->set_disabled(!has_terrain);
	new_data_button->set_disabled(!has_terrain);
	search_edit->set_editable(has_library);
	category_filter->set_disabled(!has_library);
	add_profile_button->set_disabled(!has_terrain);
	add_scene_button->set_disabled(!has_terrain);
	duplicate_profile_button->set_disabled(!has_profile);
	remove_profile_button->set_disabled(!has_profile);
}

void SimpleWorldPlacementDock::_save_resource_if_file_backed(const Ref<Resource> &p_resource) const {
	if (p_resource.is_null() || !p_resource->get_path().is_resource_file()) {
		_debug_log(vformat("save skipped resource=%s", p_resource.is_valid() ? p_resource->get_path() : String("<null>")));
		return;
	}
	_debug_log(vformat("saving resource=%s", p_resource->get_path()));
	EditorNode::get_singleton()->save_resource(p_resource);
}

void SimpleWorldPlacementDock::_debug_log(const String &p_message) const {
	print_verbose(vformat("[SimpleWorldObjects] %s", p_message));
}

void SimpleWorldPlacementDock::_debug_log_state(const String &p_context) const {
	const bool has_terrain = terrain != nullptr;
	Ref<SimpleWorldPlacementLibrary> library = has_terrain ? terrain->get_world_placement_library() : Ref<SimpleWorldPlacementLibrary>();
	const int profile_count = library.is_valid() ? library->get_profile_count() : -1;
	const int visible_count = profile_list != nullptr ? profile_list->get_item_count() : -1;
	_debug_log(vformat("%s terrain=%s library=%s library_valid=%s profiles=%d visible=%d selected=%d",
			p_context,
			has_terrain ? String(terrain->get_name()) : String("<null>"),
			library.is_valid() ? library->get_path() : String("<null>"),
			library.is_valid() ? "true" : "false",
			profile_count,
			visible_count,
			(uint64_t)selected_profile_object_id));
}

void SimpleWorldPlacementDock::_save_current_resources_if_file_backed() const {
	if (terrain == nullptr) {
		return;
	}
	_save_resource_if_file_backed(terrain->get_world_placement_library());
	_save_resource_if_file_backed(terrain->get_world_placement_data());
}

SimpleTerrain3D *SimpleWorldPlacementDock::_get_selected_terrain() const {
	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	if (selection == nullptr) {
		return nullptr;
	}

	List<Node *> selected_nodes = selection->get_top_selected_node_list();
	for (Node *node : selected_nodes) {
		SimpleTerrain3D *selected_terrain = Object::cast_to<SimpleTerrain3D>(node);
		if (selected_terrain != nullptr) {
			return selected_terrain;
		}
	}

	selected_nodes = selection->get_full_selected_node_list();
	for (Node *node : selected_nodes) {
		SimpleTerrain3D *selected_terrain = Object::cast_to<SimpleTerrain3D>(node);
		if (selected_terrain != nullptr) {
			return selected_terrain;
		}
	}

	return nullptr;
}

bool SimpleWorldPlacementDock::_sync_selected_terrain() {
	SimpleTerrain3D *selected_terrain = _get_selected_terrain();
	if (selected_terrain != nullptr && selected_terrain != terrain) {
		_debug_log(vformat("sync_selected_terrain %s -> %s", terrain != nullptr ? String(terrain->get_name()) : String("<null>"), String(selected_terrain->get_name())));
		edit(selected_terrain);
	}
	return terrain != nullptr;
}

Vector<String> SimpleWorldPlacementDock::_get_scene_paths_from_drag_data(const Variant &p_data) const {
	Vector<String> scene_paths;
	if (p_data.get_type() != Variant::DICTIONARY) {
		return scene_paths;
	}

	Dictionary drag_data = p_data;
	const String type = drag_data.get("type", "");
	if ((type == "files" || type == "files_and_dirs") && drag_data.has("files")) {
		const Vector<String> files = drag_data["files"];
		for (const String &path : files) {
			if (path.ends_with("/")) {
				continue;
			}
			const String resource_type = ResourceLoader::get_resource_type(path);
			if (ClassDB::is_parent_class(resource_type, "PackedScene") || path.get_extension().to_lower() == "tscn" || path.get_extension().to_lower() == "scn") {
				scene_paths.push_back(path);
			}
		}
		return scene_paths;
	}

	if (type == "resource" && drag_data.has("resource")) {
		Ref<PackedScene> scene = drag_data["resource"];
		if (scene.is_valid() && scene->get_path().is_resource_file()) {
			scene_paths.push_back(scene->get_path());
		}
	}

	return scene_paths;
}

bool SimpleWorldPlacementDock::_has_scene_files_in_drag_data(const Variant &p_data) const {
	if (terrain == nullptr && _get_selected_terrain() == nullptr) {
		return false;
	}
	return !_get_scene_paths_from_drag_data(p_data).is_empty();
}

void SimpleWorldPlacementDock::_add_scene_paths(const Vector<String> &p_paths, const String &p_action_name) {
	_debug_log(vformat("add_scene_paths begin paths=%d action=%s", p_paths.size(), p_action_name));
	if (!_sync_selected_terrain() || p_paths.is_empty()) {
		_debug_log("add_scene_paths ignored: no terrain or empty paths");
		return;
	}

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	const bool created_library = library.is_null();
	if (created_library) {
		library.instantiate();
	}

	Array before = library->get_profiles().duplicate();
	Array after = before.duplicate();
	HashSet<String> used_ids;
	for (int i = 0; i < after.size(); i++) {
		Ref<SimpleWorldObjectProfile> existing_profile = after[i];
		if (existing_profile.is_valid()) {
			used_ids.insert(existing_profile->get_id());
		}
	}

	for (const String &path : p_paths) {
		_debug_log(vformat("add_scene_paths candidate=%s", path));
		if (path.ends_with("/")) {
			continue;
		}
		const String resource_type = ResourceLoader::get_resource_type(path);
		if (!ClassDB::is_parent_class(resource_type, "PackedScene") && path.get_extension().to_lower() != "tscn" && path.get_extension().to_lower() != "scn") {
			_debug_log(vformat("add_scene_paths skipped non-scene path=%s type=%s", path, resource_type));
			continue;
		}

		Error err = OK;
		Ref<PackedScene> scene = ResourceLoader::load(path, "PackedScene", ResourceFormatLoader::CACHE_MODE_REUSE, &err);
		if (err != OK || scene.is_null()) {
			WARN_PRINT(vformat("Failed to load PackedScene for world placement profile: %s", path));
			continue;
		}

		const String scene_name = path.get_file().get_basename();
		String base_id = scene_name.to_snake_case();
		if (base_id.is_empty()) {
			base_id = "scene_profile";
		}

		String id = base_id;
		int index = 2;
		while (used_ids.has(id)) {
			id = vformat("%s_%d", base_id, index++);
		}
		used_ids.insert(id);

		Ref<SimpleWorldObjectProfile> profile;
		profile.instantiate();
		profile->set_id(id);
		profile->set_display_name(scene_name.capitalize());
		profile->set_category(path.get_base_dir().get_file().capitalize());
		profile->set_scene(scene);

		after.push_back(profile);
		selected_profile_object_id = profile->get_instance_id();
		_debug_log(vformat("add_scene_paths added id=%s scene=%s selected=%d", id, path, (uint64_t)selected_profile_object_id));
	}

	if (after.size() == before.size()) {
		_debug_log(vformat("add_scene_paths no profiles added before=%d after=%d", before.size(), after.size()));
		return;
	}

	if (created_library) {
		library->set_profiles(after);
		_set_library_with_undo(library, p_action_name);
	} else {
		_set_profiles_with_undo(before, after, p_action_name);
	}
	_debug_log_state("add_scene_paths:end");
}

String SimpleWorldPlacementDock::_make_unique_profile_id(const String &p_base_id) const {
	if (terrain == nullptr || terrain->get_world_placement_library().is_null()) {
		return p_base_id;
	}
	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	String candidate = p_base_id + "_copy";
	if (!library->has_profile_id(candidate)) {
		return candidate;
	}
	int index = 2;
	while (library->has_profile_id(vformat("%s_copy_%d", p_base_id, index))) {
		index++;
	}
	return vformat("%s_copy_%d", p_base_id, index);
}

Ref<SimpleWorldObjectProfile> SimpleWorldPlacementDock::_get_selected_profile() const {
	if (terrain == nullptr || terrain->get_world_placement_library().is_null() || selected_profile_object_id == ObjectID()) {
		return Ref<SimpleWorldObjectProfile>();
	}
	const Array profiles = terrain->get_world_placement_library()->get_profiles();
	for (int i = 0; i < profiles.size(); i++) {
		Ref<SimpleWorldObjectProfile> profile = profiles[i];
		if (profile.is_valid() && profile->get_instance_id() == selected_profile_object_id) {
			return profile;
		}
	}
	return Ref<SimpleWorldObjectProfile>();
}

Ref<SimpleWorldObjectProfile> SimpleWorldPlacementDock::get_selected_profile() const {
	return _get_selected_profile();
}

void SimpleWorldPlacementDock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_refresh_after_resource_undo"), &SimpleWorldPlacementDock::_refresh_after_resource_undo);
}

bool SimpleWorldPlacementDock::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (_has_scene_files_in_drag_data(p_data)) {
		return true;
	}
	return EditorDock::can_drop_data(p_point, p_data);
}

void SimpleWorldPlacementDock::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (!_has_scene_files_in_drag_data(p_data)) {
		EditorDock::drop_data(p_point, p_data);
		return;
	}

	Dictionary drag_data = p_data;
	_add_scene_paths(_get_scene_paths_from_drag_data(drag_data), TTR("Drop Scenes into World Placement Library"));
}

bool SimpleWorldPlacementDock::can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	return _has_scene_files_in_drag_data(p_data);
}

void SimpleWorldPlacementDock::drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	if (!_has_scene_files_in_drag_data(p_data)) {
		return;
	}

	Dictionary drag_data = p_data;
	_add_scene_paths(_get_scene_paths_from_drag_data(drag_data), TTR("Drop Scenes into World Placement Library"));
}

void SimpleWorldPlacementDock::edit(SimpleTerrain3D *p_terrain) {
	terrain = p_terrain;
	_connect_library(terrain != nullptr ? terrain->get_world_placement_library() : Ref<SimpleWorldPlacementLibrary>());
	if (terrain == nullptr) {
		selected_profile_object_id = ObjectID();
	}
	_refresh_resource_pickers();
	_refresh_profile_list();
	_refresh_profile_inspector();
	_update_controls();
}

SimpleWorldPlacementDock::SimpleWorldPlacementDock() {
	set_name(TTRC("Simple World Objects"));
	set_title(TTRC("Simple World Objects"));
	set_icon_name("Node3D");
	set_default_slot(EditorDock::DOCK_SLOT_RIGHT_UL);
	set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	set_custom_minimum_size(Size2(520, 360) * EDSCALE);

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root->set_tooltip_text(TTRC("Drop scene files here to add them to the placement library."));
	SET_DRAG_FORWARDING_CD(root, SimpleWorldPlacementDock);
	add_child(root);

	HBoxContainer *library_row = memnew(HBoxContainer);
	root->add_child(library_row);
	Label *library_label = memnew(Label);
	library_label->set_text(TTRC("Placement Library"));
	library_label->set_custom_minimum_size(Size2(140, 0) * EDSCALE);
	library_row->add_child(library_label);
	library_picker = memnew(EditorResourcePicker);
	library_picker->set_base_type("SimpleWorldPlacementLibrary");
	library_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	library_picker->connect("resource_changed", callable_mp(this, &SimpleWorldPlacementDock::_library_resource_changed));
	library_row->add_child(library_picker);
	new_library_button = memnew(Button);
	new_library_button->set_text(TTRC("New"));
	new_library_button->set_tooltip_text(TTRC("Create and assign a new placement library."));
	new_library_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_new_library_pressed));
	library_row->add_child(new_library_button);

	HBoxContainer *data_row = memnew(HBoxContainer);
	root->add_child(data_row);
	Label *data_label = memnew(Label);
	data_label->set_text(TTRC("Placement Data"));
	data_label->set_custom_minimum_size(Size2(140, 0) * EDSCALE);
	data_row->add_child(data_label);
	data_picker = memnew(EditorResourcePicker);
	data_picker->set_base_type("SimpleWorldPlacementData");
	data_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	data_picker->connect("resource_changed", callable_mp(this, &SimpleWorldPlacementDock::_data_resource_changed));
	data_row->add_child(data_picker);
	new_data_button = memnew(Button);
	new_data_button->set_text(TTRC("New"));
	new_data_button->set_tooltip_text(TTRC("Create and assign a new placement data resource."));
	new_data_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_new_data_pressed));
	data_row->add_child(new_data_button);

	HSplitContainer *split = memnew(HSplitContainer);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	SET_DRAG_FORWARDING_CD(split, SimpleWorldPlacementDock);
	root->add_child(split);

	VBoxContainer *list_column = memnew(VBoxContainer);
	list_column->set_custom_minimum_size(Size2(200, 280) * EDSCALE);
	SET_DRAG_FORWARDING_CD(list_column, SimpleWorldPlacementDock);
	split->add_child(list_column);

	HBoxContainer *filter_row = memnew(HBoxContainer);
	filter_row->add_theme_constant_override("separation", 4 * EDSCALE);
	list_column->add_child(filter_row);

	search_edit = memnew(LineEdit);
	search_edit->set_placeholder(TTRC("Search profiles"));
	search_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	search_edit->connect(SceneStringName(text_changed), callable_mp(this, &SimpleWorldPlacementDock::_search_text_changed));
	filter_row->add_child(search_edit);

	category_filter = memnew(OptionButton);
	category_filter->set_tooltip_text(TTRC("Filter profiles by category."));
	category_filter->set_custom_minimum_size(Size2(90, 0) * EDSCALE);
	category_filter->connect(SceneStringName(item_selected), callable_mp(this, &SimpleWorldPlacementDock::_category_selected));
	filter_row->add_child(category_filter);
	SET_DRAG_FORWARDING_CD(filter_row, SimpleWorldPlacementDock);

	add_profile_button = memnew(Button);
	add_profile_button->set_text(TTRC("Add"));
	add_profile_button->set_tooltip_text(TTRC("Add an object profile. Creates a placement library first if needed."));
	add_profile_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_add_profile_pressed));
	filter_row->add_child(add_profile_button);

	add_scene_button = memnew(Button);
	add_scene_button->set_text(TTRC("Add Scene"));
	add_scene_button->set_tooltip_text(TTRC("Choose a scene file and add it as an object profile."));
	add_scene_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_add_scene_pressed));
	filter_row->add_child(add_scene_button);

	duplicate_profile_button = memnew(Button);
	duplicate_profile_button->set_text(TTRC("Duplicate"));
	duplicate_profile_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_duplicate_profile_pressed));
	filter_row->add_child(duplicate_profile_button);

	remove_profile_button = memnew(Button);
	remove_profile_button->set_text(TTRC("Remove"));
	remove_profile_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_remove_profile_pressed));
	filter_row->add_child(remove_profile_button);

	profile_list = memnew(ItemList);
	profile_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	profile_list->set_tooltip_text(TTRC("Drop scene files here to add them to the placement library."));
	profile_list->connect(SceneStringName(item_selected), callable_mp(this, &SimpleWorldPlacementDock::_profile_list_item_selected));
	SET_DRAG_FORWARDING_CD(profile_list, SimpleWorldPlacementDock);
	list_column->add_child(profile_list);

	profile_inspector = memnew(EditorInspector);
	profile_inspector->set_use_wide_editors(true);
	profile_inspector->set_use_folding(true);
	profile_inspector->set_hide_script(true);
	profile_inspector->set_mark_unsaved(true);
	profile_inspector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	profile_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	SET_DRAG_FORWARDING_CD(profile_inspector, SimpleWorldPlacementDock);
	split->add_child(profile_inspector);

	scene_file_dialog = memnew(EditorFileDialog);
	scene_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	scene_file_dialog->set_access(FileDialog::ACCESS_RESOURCES);
	scene_file_dialog->add_filter("*.tscn", TTRC("Text Scene"));
	scene_file_dialog->add_filter("*.scn", TTRC("Binary Scene"));
	scene_file_dialog->connect("file_selected", callable_mp(this, &SimpleWorldPlacementDock::_scene_file_selected));
	add_child(scene_file_dialog);

	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	if (selection != nullptr) {
		selection->connect("selection_changed", callable_mp(this, &SimpleWorldPlacementDock::_editor_selection_changed));
	}

	_update_controls();
}

bool SimpleTerrainInspectorPlugin::can_handle(Object *p_object) {
	return Object::cast_to<SimpleTerrain3D>(p_object) != nullptr;
}

void SimpleTerrainInspectorPlugin::_open_world_objects(Object *p_object) {
	SimpleTerrain3D *terrain_node = Object::cast_to<SimpleTerrain3D>(p_object);
	if (terrain_node == nullptr || placement_dock == nullptr) {
		return;
	}
	placement_dock->edit(terrain_node);
	EditorDockManager::get_singleton()->focus_dock(placement_dock);
}

void SimpleTerrainInspectorPlugin::parse_end(Object *p_object) {
	if (!Object::cast_to<SimpleTerrain3D>(p_object)) {
		return;
	}

	Button *button = memnew(EditorInspectorActionButton(TTRC("World Objects..."), SNAME("Node3D")));
	button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	button->set_custom_minimum_size(Size2(160, 0) * EDSCALE);
	button->set_tooltip_text(TTRC("Open the SimpleTerrain world object registration dock."));
	button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainInspectorPlugin::_open_world_objects).bind(p_object), CONNECT_DEFERRED);
	add_custom_control(button);
}

void SimpleTerrainInspectorPlugin::set_placement_dock(SimpleWorldPlacementDock *p_dock) {
	placement_dock = p_dock;
}

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

	if (terrain_node == nullptr) {
		return;
	}

	Ref<TriangleMesh> collision_mesh = terrain_node->generate_triangle_mesh();
	if (collision_mesh.is_valid()) {
		p_gizmo->add_collision_triangles(collision_mesh);
	}

	if (!terrain_node->is_showing_chunk_gizmos()) {
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

void SimpleTerrainEditorPlugin::_select_mode_pressed() {
	terrain_mode = false;
	placement_mode = false;
	painting = false;
	has_last_brush_position = false;
	_clear_cursor_preview();
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_edit_mode_pressed() {
	terrain_mode = terrain != nullptr;
	placement_mode = false;
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_placement_mode_pressed() {
	terrain_mode = false;
	placement_mode = terrain != nullptr;
	painting = false;
	has_last_brush_position = false;
	_clear_cursor_preview();
	if (placement_mode) {
		placement_dock->edit(terrain);
		EditorDockManager::get_singleton()->focus_dock(placement_dock);
	}
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
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_random_pressed() {
	if (terrain == nullptr) {
		return;
	}
	const PackedFloat32Array before_heights = terrain->get_height_data();
	terrain->randomize_seed();
	_commit_height_undo(TTR("Generate Random Terrain"), before_heights);
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_bake_navigation_pressed() {
	if (terrain == nullptr) {
		return;
	}
	terrain->bake_navigation(false);
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_update_toolbar() {
	const bool has_terrain = terrain != nullptr;
	// The toolbar remains allocated for the lifetime of the plugin, but it is
	// only visible and interactive when a SimpleTerrain3D node is actively selected.
	toolbar->set_visible(has_terrain);
	brush_overlay_panel->set_visible(has_terrain && terrain_mode);
	placement_overlay_panel->set_visible(has_terrain && placement_mode);
	select_mode_button->set_disabled(!has_terrain);
	edit_mode_button->set_disabled(!has_terrain);
	placement_mode_button->set_disabled(!has_terrain);
	select_mode_button->set_pressed_no_signal(!terrain_mode && !placement_mode && has_terrain);
	edit_mode_button->set_pressed_no_signal(terrain_mode && has_terrain);
	placement_mode_button->set_pressed_no_signal(placement_mode && has_terrain);
	operation_button->set_disabled(!terrain_mode);
	radius_slider->set_read_only(!terrain_mode);
	strength_slider->set_read_only(!terrain_mode);
	flat_button->set_disabled(!has_terrain);
	random_button->set_disabled(!has_terrain);
	bake_navigation_button->set_disabled(!has_terrain);
	if (has_terrain && terrain->is_navigation_bake_dirty()) {
		bake_navigation_button->set_text(TTRC("Bake Nav *"));
		bake_navigation_button->set_tooltip_text(TTRC("Bake SimpleTerrain navigation mesh. The asterisk means the baked mesh is out of date."));
	} else {
		bake_navigation_button->set_text(TTRC("Bake Nav"));
		bake_navigation_button->set_tooltip_text(TTRC("Bake SimpleTerrain navigation mesh."));
	}
	_update_placement_overlay();
}

void SimpleTerrainEditorPlugin::_update_placement_overlay() {
	if (placement_status_label == nullptr || placement_library_label == nullptr) {
		return;
	}

	if (terrain == nullptr) {
		placement_status_label->set_text(TTRC("No SimpleTerrain3D selected"));
		placement_library_label->set_text(String());
		return;
	}

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	const int profile_count = library.is_valid() ? library->get_profile_count() : 0;
	Ref<SimpleWorldObjectProfile> selected_profile = placement_dock->get_selected_profile();
	const String profile_label = selected_profile.is_valid() ? (selected_profile->get_display_name().is_empty() ? selected_profile->get_id() : selected_profile->get_display_name()) : TTR("None");
	placement_status_label->set_text(vformat(TTR("Placement Mode: %s"), terrain->get_name()));
	placement_library_label->set_text(vformat(TTR("Profiles: %d  Selected: %s"), profile_count, profile_label));
}

Node3D *SimpleTerrainEditorPlugin::_get_or_create_placement_root(EditorUndoRedoManager *p_undo_redo) {
	ERR_FAIL_NULL_V(terrain, nullptr);
	ERR_FAIL_NULL_V(p_undo_redo, nullptr);

	Node3D *root = Object::cast_to<Node3D>(terrain->get_node_or_null(NodePath("SimpleWorldObjects")));
	if (root != nullptr) {
		return root;
	}

	root = memnew(Node3D);
	root->set_name("SimpleWorldObjects");
	Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	Node *owner = edited_scene != nullptr ? edited_scene : terrain->get_owner();
	p_undo_redo->add_do_method(terrain, "add_child", root, true);
	if (owner != nullptr) {
		p_undo_redo->add_do_method(root, "set_owner", owner);
	}
	p_undo_redo->add_undo_method(terrain, "remove_child", root);
	p_undo_redo->add_do_reference(root);
	return root;
}

void SimpleTerrainEditorPlugin::_set_placement_arrays(SimpleWorldPlacementData *p_data, const PackedStringArray &p_profile_ids, const PackedVector3Array &p_positions, const PackedVector3Array &p_rotations, const PackedVector3Array &p_scales, const PackedVector3Array &p_normals, const PackedInt32Array &p_seeds, const PackedVector2Array &p_chunk_coords) {
	ERR_FAIL_NULL(p_data);
	p_data->set_profile_ids(p_profile_ids);
	p_data->set_positions(p_positions);
	p_data->set_rotations(p_rotations);
	p_data->set_scales(p_scales);
	p_data->set_terrain_normals(p_normals);
	p_data->set_seeds(p_seeds);
	p_data->set_chunk_coords(p_chunk_coords);
}

void SimpleTerrainEditorPlugin::_place_selected_profile(Camera3D *p_camera, const Vector2 &p_mouse_position) {
	if (terrain == nullptr || p_camera == nullptr) {
		return;
	}

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	if (library.is_null() || library->get_profile_count() == 0) {
		WARN_PRINT("SimpleTerrain placement requires a placement library with at least one profile.");
		return;
	}

	Ref<SimpleWorldObjectProfile> profile = placement_dock->get_selected_profile();
	if (profile.is_null()) {
		profile = library->get_profile(0);
	}
	if (profile.is_null() || profile->get_scene().is_null() || !profile->get_scene()->can_instantiate()) {
		WARN_PRINT("Selected SimpleTerrain world object profile has no instantiable PackedScene.");
		return;
	}

	const Dictionary hit = _get_hit(p_camera, p_mouse_position);
	if (hit.is_empty()) {
		return;
	}

	Node *instance = profile->get_scene()->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
	Node3D *instance_3d = Object::cast_to<Node3D>(instance);
	if (instance_3d == nullptr) {
		memdelete(instance);
		WARN_PRINT("Selected SimpleTerrain world object scene root must be a Node3D.");
		return;
	}

	const Vector3 local_position = hit["local_position"];
	Vector3 world_position = hit["position"];
	Vector3 normal = Vector3(0.0, 1.0, 0.0);
	if (hit.has("normal")) {
		normal = ((Vector3)hit["normal"]).normalized();
	}
	world_position += normal * profile->get_surface_offset();

	const int seed = Math::rand();
	RandomPCG rng((uint64_t)seed);
	const Vector3 min_scale = profile->get_min_scale();
	const Vector3 max_scale = profile->get_max_scale();
	const Vector3 scale(
			rng.random(MIN(min_scale.x, max_scale.x), MAX(min_scale.x, max_scale.x)),
			rng.random(MIN(min_scale.y, max_scale.y), MAX(min_scale.y, max_scale.y)),
			rng.random(MIN(min_scale.z, max_scale.z), MAX(min_scale.z, max_scale.z)));

	Vector3 rotation;
	if (profile->is_random_yaw_enabled()) {
		rotation.y = rng.random((real_t)0.0, (real_t)Math::TAU);
	}

	const Transform3D terrain_inverse = terrain->get_global_transform().affine_inverse();
	const Vector3 local_normal = terrain_inverse.basis.xform(normal).normalized();
	Transform3D local_transform;
	local_transform.origin = terrain_inverse.xform(world_position);
	if (profile->is_aligning_to_terrain_normal()) {
		Vector3 local_y = local_normal.is_zero_approx() ? Vector3(0.0, 1.0, 0.0) : local_normal;
		Vector3 local_z = Vector3(Math::sin(rotation.y), 0.0, Math::cos(rotation.y));
		local_z = (local_z - local_y * local_y.dot(local_z));
		if (local_z.is_zero_approx()) {
			local_z = local_y.cross(Vector3(1.0, 0.0, 0.0));
		}
		if (local_z.is_zero_approx()) {
			local_z = local_y.cross(Vector3(0.0, 0.0, 1.0));
		}
		local_z.normalize();
		Vector3 local_x = local_y.cross(local_z).normalized();
		local_z = local_x.cross(local_y).normalized();
		local_transform.basis = Basis(local_x, local_y, local_z);
	} else {
		local_transform.basis = Basis::from_euler(rotation);
	}
	local_transform.basis.scale(scale);
	instance_3d->set_transform(local_transform);
	instance_3d->set_name(profile->get_id().is_empty() ? String("WorldObject") : profile->get_id());

	NavigationObstacle3D *runtime_obstacle = nullptr;
	if (profile->uses_runtime_navigation_obstacle() && profile->get_navigation_obstacle_radius() > 0.0) {
		runtime_obstacle = memnew(NavigationObstacle3D);
		runtime_obstacle->set_name("NavigationObstacle3D");
		runtime_obstacle->set_radius(profile->get_navigation_obstacle_radius());
		runtime_obstacle->set_height(profile->get_navigation_obstacle_height());
		runtime_obstacle->set_avoidance_layers(profile->get_navigation_avoidance_layers());
		runtime_obstacle->set_avoidance_enabled(true);
		runtime_obstacle->set_affect_navigation_mesh(false);
		runtime_obstacle->set_carve_navigation_mesh(profile->get_navigation_obstacle_carve());
		instance_3d->add_child(runtime_obstacle);
	}

	Ref<SimpleWorldPlacementData> placement_data = terrain->get_world_placement_data();
	Ref<SimpleWorldPlacementData> old_data = placement_data;
	if (placement_data.is_null()) {
		placement_data.instantiate();
		placement_data->set_terrain_path(terrain->get_path());
		placement_data->set_library(library);
	}

	PackedStringArray before_profile_ids = placement_data->get_profile_ids();
	PackedVector3Array before_positions = placement_data->get_positions();
	PackedVector3Array before_rotations = placement_data->get_rotations();
	PackedVector3Array before_scales = placement_data->get_scales();
	PackedVector3Array before_normals = placement_data->get_terrain_normals();
	PackedInt32Array before_seeds = placement_data->get_seeds();
	PackedVector2Array before_chunk_coords = placement_data->get_chunk_coords();

	PackedStringArray after_profile_ids = before_profile_ids;
	PackedVector3Array after_positions = before_positions;
	PackedVector3Array after_rotations = before_rotations;
	PackedVector3Array after_scales = before_scales;
	PackedVector3Array after_normals = before_normals;
	PackedInt32Array after_seeds = before_seeds;
	PackedVector2Array after_chunk_coords = before_chunk_coords;

	after_profile_ids.push_back(profile->get_id());
	after_positions.push_back(world_position);
	after_rotations.push_back(rotation);
	after_scales.push_back(scale);
	after_normals.push_back(normal);
	after_seeds.push_back(seed);

	Ref<SimpleTerrainData> terrain_data = terrain->get_simple_terrain_data();
	Vector2 chunk_coord;
	if (terrain_data.is_valid()) {
		const real_t half_size = (real_t)terrain_data->get_grid_size() * terrain_data->get_cell_size() * 0.5;
		const real_t chunk_world_size = (real_t)terrain->get_chunk_size() * terrain_data->get_cell_size();
		chunk_coord.x = Math::floor((local_position.x + half_size) / chunk_world_size);
		chunk_coord.y = Math::floor((local_position.z + half_size) / chunk_world_size);
	}
	after_chunk_coords.push_back(chunk_coord);

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Place World Object"));
	Node3D *placement_root = _get_or_create_placement_root(undo_redo);
	undo_redo->add_do_method(placement_root, "add_child", instance_3d, true);
	Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	if (edited_scene != nullptr) {
		undo_redo->add_do_method(instance_3d, "set_owner", edited_scene);
		if (runtime_obstacle != nullptr) {
			undo_redo->add_do_method(runtime_obstacle, "set_owner", edited_scene);
		}
	}
	undo_redo->add_do_property(terrain, "world_placement_data", placement_data);
	undo_redo->add_do_method(this, "_set_placement_arrays", placement_data.ptr(), after_profile_ids, after_positions, after_rotations, after_scales, after_normals, after_seeds, after_chunk_coords);
	undo_redo->add_undo_method(this, "_set_placement_arrays", placement_data.ptr(), before_profile_ids, before_positions, before_rotations, before_scales, before_normals, before_seeds, before_chunk_coords);
	undo_redo->add_undo_property(terrain, "world_placement_data", old_data);
	undo_redo->add_undo_method(placement_root, "remove_child", instance_3d);
	undo_redo->add_do_reference(instance_3d);
	undo_redo->commit_action();

	if (placement_data->get_path().is_resource_file()) {
		EditorNode::get_singleton()->save_resource(placement_data);
	}
	placement_dock->edit(terrain);
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_attach_brush_overlay() {
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

void SimpleTerrainEditorPlugin::_attach_placement_overlay() {
	Node3DEditorViewport *viewport = Node3DEditor::get_singleton()->get_editor_viewport(0);
	if (viewport == nullptr || placement_overlay_panel->get_parent() != nullptr) {
		return;
	}

	Control *surface = viewport->get_surface();
	if (surface == nullptr) {
		return;
	}

	surface->add_child(placement_overlay_panel);
	placement_overlay_panel->move_to_front();
}

void SimpleTerrainEditorPlugin::_detach_brush_overlay() {
	if (brush_overlay_panel->get_parent() != nullptr) {
		brush_overlay_panel->get_parent()->remove_child(brush_overlay_panel);
	}
}

void SimpleTerrainEditorPlugin::_detach_placement_overlay() {
	if (placement_overlay_panel->get_parent() != nullptr) {
		placement_overlay_panel->get_parent()->remove_child(placement_overlay_panel);
	}
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

Dictionary SimpleTerrainEditorPlugin::_get_cursor_hit(Camera3D *p_camera, const Vector2 &p_mouse_position) const {
	Dictionary result;
	if (terrain == nullptr || p_camera == nullptr || terrain->get_simple_terrain_data().is_null()) {
		return result;
	}

	const Ref<SimpleTerrainData> terrain_data = terrain->get_simple_terrain_data();
	const Transform3D inverse_transform = terrain->get_global_transform().affine_inverse();
	const Vector3 local_origin = inverse_transform.xform(p_camera->project_ray_origin(p_mouse_position));
	const Vector3 local_direction = inverse_transform.basis.xform(p_camera->project_ray_normal(p_mouse_position)).normalized();
	if (Math::is_zero_approx(local_direction.y)) {
		return result;
	}

	const real_t t = -local_origin.y / local_direction.y;
	if (t < 0.0) {
		return result;
	}

	Vector3 local_position = local_origin + local_direction * t;
	const real_t half_size = (real_t)terrain_data->get_grid_size() * terrain_data->get_cell_size() * 0.5;
	if (local_position.x < -half_size || local_position.x > half_size || local_position.z < -half_size || local_position.z > half_size) {
		return result;
	}

	const int vertex_count = terrain_data->get_vertex_count();
	const int height_x = CLAMP(Math::round((local_position.x + half_size) / terrain_data->get_cell_size()), 0, vertex_count - 1);
	const int height_z = CLAMP(Math::round((local_position.z + half_size) / terrain_data->get_cell_size()), 0, vertex_count - 1);
	local_position.y = terrain_data->get_height(height_x, height_z);

	result["local_position"] = local_position;
	result["position"] = terrain->get_global_transform().xform(local_position);
	return result;
}

Color SimpleTerrainEditorPlugin::_get_cursor_color() const {
	switch ((SimpleTerrain3D::BrushOperation)operation_button->get_selected_id()) {
		case SimpleTerrain3D::BRUSH_RAISE:
			return Color(0.2, 1.0, 0.35, 0.9);
		case SimpleTerrain3D::BRUSH_LOWER:
			return Color(1.0, 0.25, 0.2, 0.9);
		case SimpleTerrain3D::BRUSH_SMOOTH:
			return Color(0.25, 0.55, 1.0, 0.9);
		case SimpleTerrain3D::BRUSH_FLATTEN:
			return Color(1.0, 0.85, 0.2, 0.9);
	}
	return Color(1.0, 1.0, 1.0, 0.9);
}

void SimpleTerrainEditorPlugin::_update_cursor_preview(Camera3D *p_camera, const Dictionary &p_hit) {
	cursor_points.clear();
	has_cursor_hit = false;
	if (!terrain_mode || terrain == nullptr || p_camera == nullptr || p_hit.is_empty()) {
		update_overlays();
		return;
	}

	const Ref<SimpleTerrainData> terrain_data = terrain->get_simple_terrain_data();
	if (terrain_data.is_null()) {
		update_overlays();
		return;
	}

	const Vector3 local_center = p_hit["local_position"];
	const real_t radius = radius_slider->get_value();
	const int grid_size = terrain_data->get_grid_size();
	const int vertex_count = terrain_data->get_vertex_count();
	const real_t cell_size = terrain_data->get_cell_size();
	const real_t half_size = (real_t)grid_size * cell_size * 0.5;
	const Transform3D terrain_transform = terrain->get_global_transform();
	const int segments = 64;

	for (int i = 0; i <= segments; i++) {
		const real_t angle = Math::TAU * (real_t)i / (real_t)segments;
		const real_t local_x = CLAMP(local_center.x + Math::cos(angle) * radius, -half_size, half_size);
		const real_t local_z = CLAMP(local_center.z + Math::sin(angle) * radius, -half_size, half_size);
		const int height_x = CLAMP(Math::round((local_x + half_size) / cell_size), 0, vertex_count - 1);
		const int height_z = CLAMP(Math::round((local_z + half_size) / cell_size), 0, vertex_count - 1);
		const real_t local_y = terrain_data->get_height(height_x, height_z) + 0.05;
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

void SimpleTerrainEditorPlugin::_clear_cursor_preview() {
	if (!has_cursor_hit && cursor_points.is_empty()) {
		return;
	}
	has_cursor_hit = false;
	cursor_points.clear();
	update_overlays();
}

void SimpleTerrainEditorPlugin::_draw_over_viewport(Control *p_overlay) {
	if (!terrain_mode || !has_cursor_hit || cursor_points.size() < 2) {
		return;
	}

	for (int i = 0; i < cursor_points.size() - 1; i++) {
		p_overlay->draw_line(cursor_points[i], cursor_points[i + 1], Color(0, 0, 0, cursor_color.a), Math::round(4 * EDSCALE), true);
	}
	for (int i = 0; i < cursor_points.size() - 1; i++) {
		p_overlay->draw_line(cursor_points[i], cursor_points[i + 1], cursor_color, Math::round(2 * EDSCALE), true);
	}
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
			add_inspector_plugin(inspector_plugin);
			add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			add_dock(placement_dock);
			placement_dock->close();
			_attach_brush_overlay();
			_attach_placement_overlay();
			set_input_event_forwarding_always_enabled();
			set_force_draw_over_forwarding_enabled();
			select_mode_button->set_button_icon(select_mode_button->get_editor_theme_icon(SNAME("ToolSelect")));
			edit_mode_button->set_button_icon(edit_mode_button->get_editor_theme_icon(SNAME("Edit")));
			placement_mode_button->set_button_icon(placement_mode_button->get_editor_theme_icon(SNAME("Instance")));
			_update_toolbar();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_detach_placement_overlay();
			_detach_brush_overlay();
			remove_dock(placement_dock);
			remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			remove_node_3d_gizmo_plugin(gizmo_plugin);
			remove_inspector_plugin(inspector_plugin);
		} break;
	}
}

void SimpleTerrainEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_set_placement_arrays", "data", "profile_ids", "positions", "rotations", "scales", "normals", "seeds", "chunk_coords"), &SimpleTerrainEditorPlugin::_set_placement_arrays);
}

bool SimpleTerrainEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<SimpleTerrain3D>(p_object) != nullptr;
}

void SimpleTerrainEditorPlugin::edit(Object *p_object) {
	terrain = Object::cast_to<SimpleTerrain3D>(p_object);
	if (terrain == nullptr) {
		terrain_mode = false;
		placement_mode = false;
		painting = false;
		_clear_cursor_preview();
	}
	placement_dock->edit(terrain);
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::clear() {
	terrain = nullptr;
	terrain_mode = false;
	placement_mode = false;
	painting = false;
	_clear_cursor_preview();
	placement_dock->edit(nullptr);
	_update_toolbar();
}

EditorPlugin::AfterGUIInput SimpleTerrainEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (placement_mode) {
		_clear_cursor_preview();
		if (terrain == nullptr) {
			return AFTER_GUI_INPUT_PASS;
		}
		Ref<InputEventMouseButton> mouse_button = p_event;
		if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT && mouse_button->is_pressed()) {
			_place_selected_profile(p_camera, mouse_button->get_position());
			return AFTER_GUI_INPUT_STOP;
		}
		return AFTER_GUI_INPUT_PASS;
	}

	if (!terrain_mode || terrain == nullptr) {
		_clear_cursor_preview();
		return AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT) {
		const Dictionary hit = _get_hit(p_camera, mouse_button->get_position());
		_update_cursor_preview(p_camera, hit.is_empty() ? _get_cursor_hit(p_camera, mouse_button->get_position()) : hit);
		if (mouse_button->is_pressed()) {
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
			_update_toolbar();
			return AFTER_GUI_INPUT_STOP;
		}
	}

	Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid()) {
		const Dictionary hit = _get_hit(p_camera, mouse_motion->get_position());
		_update_cursor_preview(p_camera, hit.is_empty() ? _get_cursor_hit(p_camera, mouse_motion->get_position()) : hit);
		// While painting, consume mouse motion only when it actually hits the
		// terrain. Other viewport behavior can continue when the ray misses.
		if (painting && !hit.is_empty()) {
			_apply_brush(hit["position"]);
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

void SimpleTerrainEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
}

void SimpleTerrainEditorPlugin::forward_3d_force_draw_over_viewport(Control *p_overlay) {
	_draw_over_viewport(p_overlay);
}

SimpleTerrainEditorPlugin::SimpleTerrainEditorPlugin() {
	gizmo_plugin = Ref<SimpleTerrain3DGizmoPlugin>(memnew(SimpleTerrain3DGizmoPlugin));
	placement_dock = memnew(SimpleWorldPlacementDock);
	inspector_plugin.instantiate();
	inspector_plugin->set_placement_dock(placement_dock);

	// Build a compact 3D editor toolbar. It avoids inspector-only workflows so
	// terrain painting feels like an editor mode tied to the selected SimpleTerrain3D.
	toolbar = memnew(HBoxContainer);
	toolbar->hide();

	brush_overlay_panel = memnew(PanelContainer);
	brush_overlay_panel->hide();
	brush_overlay_panel->set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT, Control::PRESET_MODE_MINSIZE, 12 * EDSCALE);
	brush_overlay_panel->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
	brush_overlay_panel->set_custom_minimum_size(Size2(220, 0) * EDSCALE);

	placement_overlay_panel = memnew(PanelContainer);
	placement_overlay_panel->hide();
	placement_overlay_panel->set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT, Control::PRESET_MODE_MINSIZE, 12 * EDSCALE);
	placement_overlay_panel->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
	placement_overlay_panel->set_custom_minimum_size(Size2(240, 0) * EDSCALE);

	brush_options_vbox = memnew(VBoxContainer);
	brush_options_vbox->add_theme_constant_override("separation", 6 * EDSCALE);
	brush_overlay_panel->add_child(brush_options_vbox);

	placement_options_vbox = memnew(VBoxContainer);
	placement_options_vbox->add_theme_constant_override("separation", 6 * EDSCALE);
	placement_overlay_panel->add_child(placement_options_vbox);

	placement_status_label = memnew(Label);
	placement_status_label->set_text(TTRC("Placement Mode"));
	placement_status_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	placement_options_vbox->add_child(placement_status_label);

	placement_library_label = memnew(Label);
	placement_library_label->set_text(TTRC("Profiles: 0"));
	placement_library_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	placement_options_vbox->add_child(placement_library_label);

	mode_button_group.instantiate();

	select_mode_button = memnew(Button);
	select_mode_button->set_toggle_mode(true);
	select_mode_button->set_button_group(mode_button_group);
	select_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	select_mode_button->set_tooltip_text(TTRC("Select scene objects."));
	select_mode_button->set_accessibility_name(TTRC("Select Mode"));
	select_mode_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_select_mode_pressed));
	toolbar->add_child(select_mode_button);

	edit_mode_button = memnew(Button);
	edit_mode_button->set_toggle_mode(true);
	edit_mode_button->set_button_group(mode_button_group);
	edit_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	edit_mode_button->set_tooltip_text(TTRC("Edit SimpleTerrain."));
	edit_mode_button->set_accessibility_name(TTRC("SimpleTerrain Edit Mode"));
	edit_mode_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_edit_mode_pressed));
	toolbar->add_child(edit_mode_button);

	placement_mode_button = memnew(Button);
	placement_mode_button->set_toggle_mode(true);
	placement_mode_button->set_button_group(mode_button_group);
	placement_mode_button->set_theme_type_variation(SceneStringName(FlatButton));
	placement_mode_button->set_tooltip_text(TTRC("Place SimpleTerrain world objects."));
	placement_mode_button->set_accessibility_name(TTRC("SimpleTerrain Placement Mode"));
	placement_mode_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_placement_mode_pressed));
	toolbar->add_child(placement_mode_button);

	operation_button = memnew(OptionButton);
	operation_button->set_tooltip_text(TTRC("Brush operation."));
	operation_button->add_item(TTRC("Raise"), SimpleTerrain3D::BRUSH_RAISE);
	operation_button->add_item(TTRC("Lower"), SimpleTerrain3D::BRUSH_LOWER);
	operation_button->add_item(TTRC("Smooth"), SimpleTerrain3D::BRUSH_SMOOTH);
	operation_button->add_item(TTRC("Flatten"), SimpleTerrain3D::BRUSH_FLATTEN);
	operation_button->connect(SceneStringName(item_selected), callable_mp(this, &SimpleTerrainEditorPlugin::_operation_selected));
	brush_options_vbox->add_child(operation_button);

	radius_slider = memnew(EditorSpinSlider);
	radius_slider->set_label(TTRC("Radius"));
	radius_slider->set_min(0.1);
	radius_slider->set_max(128.0);
	radius_slider->set_step(0.1);
	radius_slider->set_value(4.0);
	radius_slider->set_custom_minimum_size(Size2(100, 0) * EDSCALE);
	brush_options_vbox->add_child(radius_slider);

	strength_slider = memnew(EditorSpinSlider);
	strength_slider->set_label(TTRC("Strength"));
	strength_slider->set_min(0.01);
	strength_slider->set_max(10.0);
	strength_slider->set_step(0.01);
	strength_slider->set_value(0.25);
	strength_slider->set_custom_minimum_size(Size2(110, 0) * EDSCALE);
	brush_options_vbox->add_child(strength_slider);

	HBoxContainer *terrain_action_row = memnew(HBoxContainer);
	terrain_action_row->add_theme_constant_override("separation", 4 * EDSCALE);
	brush_options_vbox->add_child(terrain_action_row);

	flat_button = memnew(Button);
	flat_button->set_text(TTRC("Flat"));
	flat_button->set_tooltip_text(TTRC("Reset selected terrain to a flat heightmap."));
	flat_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_flat_pressed));
	terrain_action_row->add_child(flat_button);

	random_button = memnew(Button);
	random_button->set_text(TTRC("Random"));
	random_button->set_tooltip_text(TTRC("Generate random terrain on the selected terrain."));
	random_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_random_pressed));
	terrain_action_row->add_child(random_button);

	bake_navigation_button = memnew(Button);
	bake_navigation_button->set_text(TTRC("Bake Nav"));
	bake_navigation_button->set_tooltip_text(TTRC("Bake SimpleTerrain navigation mesh."));
	bake_navigation_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleTerrainEditorPlugin::_bake_navigation_pressed));
	terrain_action_row->add_child(bake_navigation_button);
}
