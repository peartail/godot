# Area Replacement Semantics

Canonical decision: [`docs/adr/0001-world-placement-area-replacement.md`](../../docs/adr/0001-world-placement-area-replacement.md).

## Summary

A World Placement **apply** is a destructive **area replacement** over a projected footprint:

1. Resolve candidates for the new region.
2. On success only, remove managed placements whose **anchors** lie inside the footprint.
3. Append the new authoritative placement records and attach generated nodes.

This is not:

- a persistent layered stamp that stacks with previous applies in the same footprint
- deletion of unrelated children under the output parent

(Continuous drag strokes are a rejected interaction model, not a deferred feature.)

## Why

- Avoids replay-order dependencies between overlapping applies.
- Keeps `OpenWorldPlacementData` as the rebuildable source of truth.
- Lets future tile/MultiMesh backends consume the same records without re-authoring the world.

## UX wording

Prefer **apply** / **replace region** / **click-to-apply**.  
If “stamp” appears in UI copy, qualify it as a one-shot region apply that **replaces** the footprint, not a stackable overlay.
