/**************************************************************************/
/*  simple_terrain_editor_plugin.cpp                                             */
/**************************************************************************/

#include "simple_terrain_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/themes/editor_scale.h"
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
	if (updating || terrain == nullptr) {
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library = p_resource;
	_set_library_with_undo(library, TTR("Set World Placement Library"));
}

void SimpleWorldPlacementDock::_data_resource_changed(const Ref<Resource> &p_resource) {
	if (updating || terrain == nullptr) {
		return;
	}
	Ref<SimpleWorldPlacementData> placement_data = p_resource;
	_set_data_with_undo(placement_data, TTR("Set World Placement Data"));
}

void SimpleWorldPlacementDock::_new_library_pressed() {
	if (terrain == nullptr) {
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library;
	library.instantiate();
	_set_library_with_undo(library, TTR("Create World Placement Library"));
}

void SimpleWorldPlacementDock::_new_data_pressed() {
	if (terrain == nullptr) {
		return;
	}
	Ref<SimpleWorldPlacementData> placement_data;
	placement_data.instantiate();
	placement_data->set_terrain_path(terrain->get_path());
	placement_data->set_library(terrain->get_world_placement_library());
	_set_data_with_undo(placement_data, TTR("Create World Placement Data"));
}

void SimpleWorldPlacementDock::_add_profile_pressed() {
	if (terrain == nullptr || terrain->get_world_placement_library().is_null()) {
		return;
	}

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	int index = 1;
	String id;
	do {
		id = vformat("profile_%d", index++);
	} while (library->has_profile_id(id));

	Ref<SimpleWorldObjectProfile> profile;
	profile.instantiate();
	profile->set_id(id);
	profile->set_display_name(vformat("Profile %d", index - 1));

	Array before = library->get_profiles();
	Array after = before;
	after.push_back(profile);
	selected_profile_object_id = profile->get_instance_id();
	_set_profiles_with_undo(before, after, TTR("Add World Object Profile"));
}

void SimpleWorldPlacementDock::_duplicate_profile_pressed() {
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

	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	Array before = library->get_profiles();
	Array after = before;
	after.push_back(duplicate);
	selected_profile_object_id = duplicate->get_instance_id();
	_set_profiles_with_undo(before, after, TTR("Duplicate World Object Profile"));
}

void SimpleWorldPlacementDock::_remove_profile_pressed() {
	if (terrain == nullptr || terrain->get_world_placement_library().is_null()) {
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

	Array before = library->get_profiles();
	Array after = before;
	after.remove_at(index);
	selected_profile_object_id = ObjectID();
	_set_profiles_with_undo(before, after, TTR("Remove World Object Profile"));
}

void SimpleWorldPlacementDock::_profile_list_item_selected(int p_index) {
	ERR_FAIL_INDEX(p_index, profile_list->get_item_count());
	const uint64_t profile_id = profile_list->get_item_metadata(p_index);
	selected_profile_object_id = ObjectID(profile_id);
	_refresh_profile_inspector();
	_update_controls();
}

void SimpleWorldPlacementDock::_search_text_changed(const String &p_text) {
	_refresh_profile_list();
}

void SimpleWorldPlacementDock::_category_selected(int p_index) {
	_refresh_profile_list();
}

void SimpleWorldPlacementDock::_library_changed() {
	_refresh_profile_list();
	_refresh_profile_inspector();
	_update_controls();
}

void SimpleWorldPlacementDock::_profile_changed() {
	_refresh_profile_list();
	_update_controls();
}

void SimpleWorldPlacementDock::_refresh_after_resource_undo() {
	_refresh_resource_pickers();
	_connect_library(terrain != nullptr ? terrain->get_world_placement_library() : Ref<SimpleWorldPlacementLibrary>());
	_refresh_profile_list();
	_refresh_profile_inspector();
	_update_controls();
}

void SimpleWorldPlacementDock::_set_library_with_undo(const Ref<SimpleWorldPlacementLibrary> &p_library, const String &p_action_name) {
	if (terrain == nullptr || terrain->get_world_placement_library() == p_library) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(terrain, "world_placement_library", p_library);
	undo_redo->add_do_method(this, "_refresh_after_resource_undo");
	undo_redo->add_undo_property(terrain, "world_placement_library", terrain->get_world_placement_library());
	undo_redo->add_undo_method(this, "_refresh_after_resource_undo");
	undo_redo->commit_action();
	_refresh_after_resource_undo();
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
	_refresh_after_resource_undo();
}

void SimpleWorldPlacementDock::_set_profiles_with_undo(const Array &p_before, const Array &p_after, const String &p_action_name) {
	if (terrain == nullptr || terrain->get_world_placement_library().is_null()) {
		return;
	}
	Ref<SimpleWorldPlacementLibrary> library = terrain->get_world_placement_library();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action_name);
	undo_redo->add_do_property(library.ptr(), "profiles", p_after);
	undo_redo->add_do_method(this, "_refresh_after_resource_undo");
	undo_redo->add_undo_property(library.ptr(), "profiles", p_before);
	undo_redo->add_undo_method(this, "_refresh_after_resource_undo");
	undo_redo->commit_action();
	_refresh_after_resource_undo();
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
}

void SimpleWorldPlacementDock::_refresh_profile_inspector() {
	Ref<SimpleWorldObjectProfile> profile = _get_selected_profile();
	_connect_profile(profile);
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
	add_profile_button->set_disabled(!has_library);
	duplicate_profile_button->set_disabled(!has_profile);
	remove_profile_button->set_disabled(!has_profile);
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

void SimpleWorldPlacementDock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_refresh_after_resource_undo"), &SimpleWorldPlacementDock::_refresh_after_resource_undo);
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
	root->add_child(split);

	VBoxContainer *list_column = memnew(VBoxContainer);
	list_column->set_custom_minimum_size(Size2(200, 280) * EDSCALE);
	split->add_child(list_column);

	search_edit = memnew(LineEdit);
	search_edit->set_placeholder(TTRC("Search profiles"));
	search_edit->connect(SceneStringName(text_changed), callable_mp(this, &SimpleWorldPlacementDock::_search_text_changed));
	list_column->add_child(search_edit);

	category_filter = memnew(OptionButton);
	category_filter->set_tooltip_text(TTRC("Filter profiles by category."));
	category_filter->connect(SceneStringName(item_selected), callable_mp(this, &SimpleWorldPlacementDock::_category_selected));
	list_column->add_child(category_filter);

	profile_list = memnew(ItemList);
	profile_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	profile_list->connect(SceneStringName(item_selected), callable_mp(this, &SimpleWorldPlacementDock::_profile_list_item_selected));
	list_column->add_child(profile_list);

	HBoxContainer *profile_button_row = memnew(HBoxContainer);
	list_column->add_child(profile_button_row);
	add_profile_button = memnew(Button);
	add_profile_button->set_text(TTRC("Add"));
	add_profile_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_add_profile_pressed));
	profile_button_row->add_child(add_profile_button);
	duplicate_profile_button = memnew(Button);
	duplicate_profile_button->set_text(TTRC("Duplicate"));
	duplicate_profile_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_duplicate_profile_pressed));
	profile_button_row->add_child(duplicate_profile_button);
	remove_profile_button = memnew(Button);
	remove_profile_button->set_text(TTRC("Remove"));
	remove_profile_button->connect(SceneStringName(pressed), callable_mp(this, &SimpleWorldPlacementDock::_remove_profile_pressed));
	profile_button_row->add_child(remove_profile_button);

	profile_inspector = memnew(EditorInspector);
	profile_inspector->set_use_wide_editors(true);
	profile_inspector->set_use_folding(true);
	profile_inspector->set_hide_script(true);
	profile_inspector->set_mark_unsaved(true);
	profile_inspector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	profile_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	split->add_child(profile_inspector);

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

	if (terrain_node == nullptr || !terrain_node->is_showing_chunk_gizmos()) {
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
	painting = false;
	has_last_brush_position = false;
	_clear_cursor_preview();
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::_edit_mode_pressed() {
	terrain_mode = terrain != nullptr;
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
}

void SimpleTerrainEditorPlugin::_random_pressed() {
	if (terrain == nullptr) {
		return;
	}
	const PackedFloat32Array before_heights = terrain->get_height_data();
	terrain->randomize_seed();
	_commit_height_undo(TTR("Generate Random Terrain"), before_heights);
}

void SimpleTerrainEditorPlugin::_update_toolbar() {
	const bool has_terrain = terrain != nullptr;
	// The toolbar remains allocated for the lifetime of the plugin, but it is
	// only visible and interactive when a SimpleTerrain3D node is actively selected.
	toolbar->set_visible(has_terrain);
	brush_overlay_panel->set_visible(has_terrain && terrain_mode);
	select_mode_button->set_disabled(!has_terrain);
	edit_mode_button->set_disabled(!has_terrain);
	select_mode_button->set_pressed_no_signal(!terrain_mode && has_terrain);
	edit_mode_button->set_pressed_no_signal(terrain_mode && has_terrain);
	operation_button->set_disabled(!terrain_mode);
	radius_slider->set_read_only(!terrain_mode);
	strength_slider->set_read_only(!terrain_mode);
	flat_button->set_disabled(!has_terrain);
	random_button->set_disabled(!has_terrain);
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

void SimpleTerrainEditorPlugin::_detach_brush_overlay() {
	if (brush_overlay_panel->get_parent() != nullptr) {
		brush_overlay_panel->get_parent()->remove_child(brush_overlay_panel);
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
			set_input_event_forwarding_always_enabled();
			set_force_draw_over_forwarding_enabled();
			select_mode_button->set_button_icon(select_mode_button->get_editor_theme_icon(SNAME("ToolSelect")));
			edit_mode_button->set_button_icon(edit_mode_button->get_editor_theme_icon(SNAME("Edit")));
			_update_toolbar();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_detach_brush_overlay();
			remove_dock(placement_dock);
			remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			remove_node_3d_gizmo_plugin(gizmo_plugin);
			remove_inspector_plugin(inspector_plugin);
		} break;
	}
}

bool SimpleTerrainEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<SimpleTerrain3D>(p_object) != nullptr;
}

void SimpleTerrainEditorPlugin::edit(Object *p_object) {
	terrain = Object::cast_to<SimpleTerrain3D>(p_object);
	if (terrain == nullptr) {
		terrain_mode = false;
		painting = false;
		_clear_cursor_preview();
	}
	placement_dock->edit(terrain);
	_update_toolbar();
}

void SimpleTerrainEditorPlugin::clear() {
	terrain = nullptr;
	terrain_mode = false;
	painting = false;
	_clear_cursor_preview();
	placement_dock->edit(nullptr);
	_update_toolbar();
}

EditorPlugin::AfterGUIInput SimpleTerrainEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
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
}
