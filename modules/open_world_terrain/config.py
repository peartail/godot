def can_build(env, platform):
    return not env["disable_3d"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "OpenWorldTerrain3D",
        "OpenWorldTerrainData",
        "OpenWorldTerrainLayer",
        "OpenWorldTreeVariant",
        "OpenWorldTreeSpecies",
        "OpenWorldTreePlacementData",
        "OpenWorldTree3D",
        "OpenWorldTreeGenerationProfile",
        "OpenWorldTreeGenerator3D",
    ]


def get_doc_path():
    return "doc_classes"
