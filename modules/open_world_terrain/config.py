def can_build(env, platform):
    return not env["disable_3d"]


def configure(env):
    env.module_add_dependencies("open_world_terrain", ["simple_terrain"])


def get_doc_classes():
    return [
        "OpenWorldTerrain3D",
        "OpenWorldTerrainData",
        "OpenWorldTerrainLayer",
        "OpenWorldPlacementEntry",
        "OpenWorldPlacementPreset",
        "OpenWorldPlacementData",
        "OpenWorldPlacement3D",
        "OpenWorldTreeVariant",
        "OpenWorldTreeSpecies",
        "OpenWorldTreePlacementData",
        "OpenWorldTree3D",
        "OpenWorldTreeGenerationProfile",
        "OpenWorldTreeGenerator3D",
        "OpenWorldTreeSupportGraph",
        "OpenWorldVineGenerationProfile",
        "OpenWorldVineGenerationRequest",
        "OpenWorldVinePathData",
        "OpenWorldVineVariant",
        "OpenWorldVineGenerator3D",
        "OpenWorldVine3D",
        "OpenWorldRockGenerationProfile",
        "OpenWorldRockGenerationRequest",
        "OpenWorldRockTopologyData",
        "OpenWorldRockVariant",
        "OpenWorldRockVariantLibrary",
        "OpenWorldRockPlacementData",
        "OpenWorldRockGenerator3D",
    ]


def get_doc_path():
    return "doc_classes"
