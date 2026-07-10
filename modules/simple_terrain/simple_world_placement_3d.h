/**************************************************************************/
/*  simple_world_placement_3d.h                                           */
/**************************************************************************/

#pragma once

#include "simple_world_placement_data.h"
#include "simple_world_placement_library.h"

#include "scene/3d/node_3d.h"

class SimpleWorldPlacement3D : public Node3D {
	GDCLASS(SimpleWorldPlacement3D, Node3D);

	Ref<SimpleWorldPlacementLibrary> world_placement_library;
	Ref<SimpleWorldPlacementData> world_placement_data;
	bool rebuilding = false;
	bool rebuild_pending = false;

	void _placement_source_changed();
	void _clear_generated_children();
	void _rebuild_instances();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_world_placement_library(const Ref<SimpleWorldPlacementLibrary> &p_library);
	Ref<SimpleWorldPlacementLibrary> get_world_placement_library() const { return world_placement_library; }

	void set_world_placement_data(const Ref<SimpleWorldPlacementData> &p_data);
	Ref<SimpleWorldPlacementData> get_world_placement_data() const { return world_placement_data; }

	void rebuild_placements();
};
