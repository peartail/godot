/**************************************************************************/
/*  resource_view.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "snapshot_view.h"

#include "scene/gui/tree.h"

class SnapshotResourceView : public SnapshotView {
	GDCLASS(SnapshotResourceView, SnapshotView);

	Tree *resource_tree = nullptr;
	HashMap<TreeItem *, SnapshotDataObject *> item_data_map;

	String _resource_key(SnapshotDataObject *p_obj) const;
	void _insert_resources(GameStateSnapshot *p_snapshot, const String &p_snapshot_name, const HashSet<String> &p_other_keys, bool p_diff_mode);
	void _resource_selected();

public:
	SnapshotResourceView();
	virtual void show_snapshot(GameStateSnapshot *p_data, GameStateSnapshot *p_diff_data) override;
	virtual void clear_snapshot() override;
};