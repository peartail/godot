/**************************************************************************/
/*  simple_world_placement_legacy.h                                       */
/**************************************************************************/

#pragma once

// Legacy PackedScene world-object placement (SimpleWorld*).
// Superseded by OpenWorldPlacement / World Placement (trees, rocks, bramble apply).
// Keep runtime + scene STORAGE so existing scenes load until cleanup.
// Set to 1 only for temporary migration debugging.
#ifndef SIMPLE_WORLD_PLACEMENT_EDITOR_ENABLED
#define SIMPLE_WORLD_PLACEMENT_EDITOR_ENABLED 0
#endif
