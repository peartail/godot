/**************************************************************************/
/*  resource_view.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "resource_view.h"

#include "editor/editor_node.h"

String SnapshotResourceView::_resource_key(SnapshotDataObject *p_obj) const {
	String path = p_obj->extra_debug_data.has("resource_path") ? (String)p_obj->extra_debug_data["resource_path"] : String();
	String type = p_obj->extra_debug_data.has("resource_type") ? (String)p_obj->extra_debug_data["resource_type"] : p_obj->type_name;
	if (path.is_empty()) {
		path = "<runtime>:" + itos((uint64_t)p_obj->remote_object_id);
	}
	return type + "|" + path;
}

void SnapshotResourceView::_insert_resources(GameStateSnapshot *p_snapshot, const String &p_snapshot_name, const HashSet<String> &p_other_keys, bool p_diff_mode) {
	TreeItem *root = resource_tree->get_root();
	for (const KeyValue<ObjectID, SnapshotDataObject *> &pair : p_snapshot->objects) {
		SnapshotDataObject *obj = pair.value;
		if (!obj->extra_debug_data.has("resource_type")) {
			continue;
		}

		String key = _resource_key(obj);
		if (p_diff_mode && p_other_keys.has(key)) {
			continue;
		}

		TreeItem *item = resource_tree->create_item(root);
		item_data_map[item] = obj;
		String path = obj->extra_debug_data.has("resource_path") ? (String)obj->extra_debug_data["resource_path"] : String();
		String name = obj->extra_debug_data.has("resource_name") ? (String)obj->extra_debug_data["resource_name"] : String();
		item->set_text(0, p_snapshot_name);
		item->set_text(1, obj->extra_debug_data.has("resource_type") ? (String)obj->extra_debug_data["resource_type"] : obj->type_name);
		item->set_text(2, path.is_empty() ? TTRC("<runtime>") : path);
		item->set_text(3, name);
		item->set_text(4, obj->extra_debug_data.has("ref_count") ? itos((uint64_t)obj->extra_debug_data["ref_count"]) : String());
		item->set_text(5, obj->extra_debug_data.has("mesh_surface_count") ? itos((int)obj->extra_debug_data["mesh_surface_count"]) : String());
		item->set_text(6, obj->extra_debug_data.has("mesh_vertex_count") ? itos((int)obj->extra_debug_data["mesh_vertex_count"]) : String());
		item->set_text(7, obj->extra_debug_data.has("mesh_index_count") ? itos((int)obj->extra_debug_data["mesh_index_count"]) : String());
	}
}

void SnapshotResourceView::_resource_selected() {
	TreeItem *selected = resource_tree->get_selected();
	if (selected == nullptr || !item_data_map.has(selected)) {
		return;
	}
	EditorNode::get_singleton()->push_item(item_data_map[selected]);
}

SnapshotResourceView::SnapshotResourceView() {
	set_name(TTRC("Resources"));
	set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	resource_tree = memnew(Tree);
	resource_tree->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	resource_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	resource_tree->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	resource_tree->set_hide_root(true);
	resource_tree->set_columns(8);
	resource_tree->set_column_titles_visible(true);
	const char *titles[8] = { "Snapshot", "Type", "Path", "Name", "Refs", "Surfaces", "Vertices", "Indices" };
	for (int i = 0; i < 8; i++) {
		resource_tree->set_column_title(i, String(titles[i]));
	}
	resource_tree->connect(SceneStringName(item_selected), callable_mp(this, &SnapshotResourceView::_resource_selected));
	add_child(resource_tree);
}

void SnapshotResourceView::show_snapshot(GameStateSnapshot *p_data, GameStateSnapshot *p_diff_data) {
	SnapshotView::show_snapshot(p_data, p_diff_data);
	resource_tree->clear();
	resource_tree->create_item();
	item_data_map.clear();

	HashSet<String> diff_keys;
	if (diff_data != nullptr) {
		for (const KeyValue<ObjectID, SnapshotDataObject *> &pair : diff_data->objects) {
			if (pair.value->extra_debug_data.has("resource_type")) {
				diff_keys.insert(_resource_key(pair.value));
			}
		}
	}
	_insert_resources(snapshot_data, diff_data == nullptr ? TTR("Snapshot") : TTR("Snapshot A Added/Changed"), diff_keys, diff_data != nullptr);

	if (diff_data != nullptr) {
		HashSet<String> snapshot_keys;
		for (const KeyValue<ObjectID, SnapshotDataObject *> &pair : snapshot_data->objects) {
			if (pair.value->extra_debug_data.has("resource_type")) {
				snapshot_keys.insert(_resource_key(pair.value));
			}
		}
		_insert_resources(diff_data, TTR("Snapshot B Removed/Changed"), snapshot_keys, true);
	}
}

void SnapshotResourceView::clear_snapshot() {
	SnapshotView::clear_snapshot();
	resource_tree->clear();
	resource_tree->create_item();
	item_data_map.clear();
}