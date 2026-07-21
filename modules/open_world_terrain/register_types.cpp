/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#ifndef _3D_DISABLED

#include "register_types.h"

#include "modules/open_world_terrain/open_world_terrain_3d.h"
#include "modules/open_world_terrain/open_world_terrain_data.h"
#include "modules/open_world_terrain/open_world_terrain_layer.h"
#include "modules/open_world_terrain/open_world_placement_brush_3d.h"
#include "modules/open_world_terrain/open_world_placement_brush_entry.h"
#include "modules/open_world_terrain/open_world_placement_brush_preset.h"
#include "modules/open_world_terrain/open_world_placement_data.h"
#include "modules/open_world_terrain/open_world_rock_generation_profile.h"
#include "modules/open_world_terrain/open_world_rock_generation_request.h"
#include "modules/open_world_terrain/open_world_rock_generator_3d.h"
#include "modules/open_world_terrain/open_world_rock_placement_data.h"
#include "modules/open_world_terrain/open_world_rock_topology_data.h"
#include "modules/open_world_terrain/open_world_rock_variant.h"
#include "modules/open_world_terrain/open_world_rock_variant_library.h"
#include "modules/open_world_terrain/open_world_tree_3d.h"
#include "modules/open_world_terrain/open_world_tree_generation_profile.h"
#include "modules/open_world_terrain/open_world_tree_generator_3d.h"
#include "modules/open_world_terrain/open_world_tree_placement_data.h"
#include "modules/open_world_terrain/open_world_tree_species.h"
#include "modules/open_world_terrain/open_world_tree_variant.h"
#include "modules/open_world_terrain/open_world_tree_support_graph.h"
#include "modules/open_world_terrain/open_world_vine_3d.h"
#include "modules/open_world_terrain/open_world_vine_generation_profile.h"
#include "modules/open_world_terrain/open_world_vine_generation_request.h"
#include "modules/open_world_terrain/open_world_vine_generator_3d.h"
#include "modules/open_world_terrain/open_world_vine_path_data.h"
#include "modules/open_world_terrain/open_world_vine_variant.h"

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
		GDREGISTER_CLASS(OpenWorldTerrainLayer);
		GDREGISTER_CLASS(OpenWorldTerrainData);
		GDREGISTER_CLASS(OpenWorldTerrain3D);
		GDREGISTER_CLASS(OpenWorldPlacementBrushEntry);
		GDREGISTER_CLASS(OpenWorldPlacementBrushPreset);
		GDREGISTER_CLASS(OpenWorldPlacementData);
		GDREGISTER_CLASS(OpenWorldPlacementBrush3D);
		GDREGISTER_CLASS(OpenWorldTreeSupportGraph);
		GDREGISTER_CLASS(OpenWorldTreeVariant);
		GDREGISTER_CLASS(OpenWorldTreeSpecies);
		GDREGISTER_CLASS(OpenWorldTreePlacementData);
		GDREGISTER_CLASS(OpenWorldTree3D);
		GDREGISTER_CLASS(OpenWorldTreeGenerationProfile);
		GDREGISTER_CLASS(OpenWorldTreeGenerator3D);
		GDREGISTER_CLASS(OpenWorldVineGenerationProfile);
		GDREGISTER_CLASS(OpenWorldVineGenerationRequest);
		GDREGISTER_CLASS(OpenWorldVinePathData);
		GDREGISTER_CLASS(OpenWorldVineVariant);
		GDREGISTER_CLASS(OpenWorldVineGenerator3D);
		GDREGISTER_CLASS(OpenWorldVine3D);
		GDREGISTER_CLASS(OpenWorldRockGenerationProfile);
		GDREGISTER_CLASS(OpenWorldRockGenerationRequest);
		GDREGISTER_CLASS(OpenWorldRockTopologyData);
		GDREGISTER_CLASS(OpenWorldRockVariant);
		GDREGISTER_CLASS(OpenWorldRockVariantLibrary);
		GDREGISTER_CLASS(OpenWorldRockPlacementData);
		GDREGISTER_CLASS(OpenWorldRockGenerator3D);
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
