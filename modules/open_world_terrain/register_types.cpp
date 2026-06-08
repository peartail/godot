/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#ifndef _3D_DISABLED

#include "register_types.h"

#include "modules/open_world_terrain/open_world_terrain_3d.h"
#include "modules/open_world_terrain/open_world_terrain_data.h"

#ifdef TOOLS_ENABLED
#include "modules/open_world_terrain/editor/open_world_terrain_editor_plugin.h"
#include "editor/editor_node.h"
#endif

#include "core/object/class_db.h"

void initialize_open_world_terrain_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// OpenWorldTerrain is intentionally separate from SimpleTerrain. These
		// classes form the experimental height-texture/GPU-displacement path and
		// can evolve without changing saved SimpleTerrain scenes or resources.
		GDREGISTER_CLASS(OpenWorldTerrainData);
		GDREGISTER_CLASS(OpenWorldTerrain3D);
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_VIRTUAL_CLASS(OpenWorldTerrainEditorPlugin);
		EditorPlugins::add_by_type<OpenWorldTerrainEditorPlugin>();
	}
#endif
}

void uninitialize_open_world_terrain_module(ModuleInitializationLevel p_level) {
}

#endif // _3D_DISABLED
