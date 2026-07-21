# Open World Procedural Placement

This context defines the authoring language for deterministic placement of procedural world objects onto terrain.

## Language

**Brush Preset**:
A reusable recipe containing a projected footprint, total density, spacing rules, and weighted content entries.
_Avoid_: Mask, stamp template

**Brush Entry**:
One weighted procedural generation recipe in a brush preset, categorized as Tree, Vine, or Rock. A preset may contain multiple entries of the same category.
_Avoid_: Type slot, object slot

**Brush Operation**:
One destructive area-replacement action that removes managed placements whose anchors are inside the projected footprint and creates replacement placements from the active preset.
_Avoid_: Persistent stamp, stroke history

**Placement Record**:
The authoritative saved description of one placed result, including its source entry, anchor, transform, seed, and spacing radius.
_Avoid_: Generated node, stamp

**Generated Node**:
The scene-owned Tree, Vine, or Rock generator node materialized from a placement record. It is saved for the initial workflow but remains rebuildable derived output.
_Avoid_: Placement record

**Managed Placement**:
A placement record and generated node owned by the same world placement system. Area replacement never deletes unrelated or manually authored scene objects.
_Avoid_: Any child of the output parent

**Anchor**:
The terrain-projected origin used to decide whether a managed placement belongs to a brush footprint.
_Avoid_: Mesh bounds, collision bounds

**Total Density**:
The expected number of new placements per 100 square meters before terrain and spacing rejection. Entry weights divide this total instead of adding independent densities.
_Avoid_: Per-entry density

