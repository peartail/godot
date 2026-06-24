/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#ifndef _3D_DISABLED

#include "register_types.h"

#include "modules/simple_terrain/simple_navigation_blocker_3d.h"
#include "modules/simple_terrain/simple_terrain_3d.h"
#include "modules/simple_terrain/simple_terrain_data.h"
#include "modules/simple_terrain/simple_world_object_profile.h"
#include "modules/simple_terrain/simple_world_placement_data.h"
#include "modules/simple_terrain/simple_world_placement_library.h"

#ifdef TOOLS_ENABLED
#include "modules/simple_terrain/editor/simple_terrain_editor_plugin.h"
#endif

#include "core/object/class_db.h"

void initialize_simple_terrain_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Runtime classes are registered at scene level so projects can create
		// SimpleTerrainData resources and SimpleTerrain3D nodes from scripts, scenes, and the
		// editor class database.
		GDREGISTER_CLASS(SimpleTerrainData);
		GDREGISTER_CLASS(SimpleTerrain3D);
		GDREGISTER_CLASS(SimpleNavigationBlocker3D);
		GDREGISTER_CLASS(SimpleWorldObjectProfile);
		GDREGISTER_CLASS(SimpleWorldPlacementLibrary);
		GDREGISTER_CLASS(SimpleWorldPlacementData);
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		// Editor tooling is registered only for tools builds. Export templates can
		// still use the runtime terrain classes without carrying editor UI code.
		GDREGISTER_VIRTUAL_CLASS(SimpleTerrainEditorPlugin);
		EditorPlugins::add_by_type<SimpleTerrainEditorPlugin>();
	}
#endif
}

void uninitialize_simple_terrain_module(ModuleInitializationLevel p_level) {
}

#endif // _3D_DISABLED
