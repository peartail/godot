# Simple World Placement Editor Plan

## Goal

Build a data-driven world editor layer on top of `SimpleTerrain3D`.

The editor should not treat placed trees, rocks, grass, and roads as one-off scene edits first. Instead, it should store stable placement data, then regenerate scene instances from that data. This keeps editing, rebuilds, undo/redo, future chunking, and road/object exclusion easier to maintain.

## Core Data Model

### SimpleWorldObjectProfile

Defines one placeable object.

Stored fields:

- `id`: Stable string key used by placement instances.
- `display_name`: Human-readable editor label.
- `category`: Group such as `Trees`, `Rocks`, or `Grass`.
- `scene`: `PackedScene` to instantiate.
- `preview_icon`: Optional editor thumbnail.
- `placement_type`: Single, brush, scatter, or grass.
- `collision_radius`: Approximate spacing radius.
- `spacing`: Brush/scatter minimum spacing.
- `density`: Scatter or grass density.
- `min_scale` / `max_scale`: Random scale range.
- `random_yaw`: Whether Y rotation may be randomized.
- `align_to_terrain_normal`: Whether the instance should follow terrain slope.
- `slope_min_degrees` / `slope_max_degrees`: Placement slope filter.
- `height_min` / `height_max`: Placement height filter.
- `surface_offset`: Offset from sampled terrain surface.
- `tags`: Free-form labels for future filters.

### SimpleWorldPlacementLibrary

Stores all registered placeable profiles for a world or project.

Responsibilities:

- Add/remove profiles.
- Look up profiles by `id`.
- Keep object registration independent from actual placed instances.
- Provide the data source for the editor object palette.

### SimpleWorldPlacementData

Stores actual placement records.

Each placement stores:

- `profile_id`
- `position`
- `rotation`
- `scale`
- `terrain_normal`
- `seed`
- `chunk_coord`

The placed record references `profile_id` instead of directly storing the `PackedScene`. This lets artists replace a tree scene in the library and rebuild existing placements without rewriting every instance.

## Editor UI Plan

### Object Library Panel

Initial controls:

- Add profile
- Remove profile
- Duplicate profile
- Search
- Category filter
- Thumbnail/list display

This panel edits `SimpleWorldPlacementLibrary`.

### Profile Inspector

Shows the selected `SimpleWorldObjectProfile`.

Initial fields:

- Scene
- Display name
- Category
- Placement type
- Scale range
- Random yaw
- Align to terrain normal
- Radius/spacing/density
- Slope and height limits
- Surface offset

### Placement Data Panel

Shows the active `SimpleWorldPlacementData`.

Initial controls:

- Assign terrain path
- Assign library
- Rebuild generated instances
- Clear generated instances
- Clear placement data
- Validate missing profile ids

## First MVP

Phase 1 should only establish the data contract and a tiny editor flow:

1. Add the three runtime resources.
2. Register them with ClassDB.
3. Allow `.tres` resources to store a placement library and placement data.
4. Add methods for adding/removing profiles and placement records.
5. Later, build a small editor panel that edits these resources.

## Later Phases

### Object Placement

- Click terrain to place the selected profile.
- Store placement in `SimpleWorldPlacementData`.
- Generate a scene instance under an editor-owned generated root.
- Support undo/redo.

### Brush Placement

- Radius, density, spacing, jitter.
- Slope/height filters from the selected profile.
- Random seed per placement.

### Grass

- Store grass placement as data, but generate with `MultiMeshInstance3D`.
- Avoid one node per grass blade or clump.

### Roads

Roads should use a separate but similar profile/data system:

- `SimpleWorldRoadProfile`
- `SimpleWorldRoadData`
- point path records
- generated road mesh
- optional terrain flattening and object exclusion masks

Road work should start after object placement has stable terrain hit testing and rebuild behavior.
