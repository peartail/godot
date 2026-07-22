# Store placement results via area replacement

World Placement applies are destructive area-replacement operations, not persistent layered stamps and not continuous brush strokes. Each apply removes managed placements whose anchors lie inside its projected footprint and saves the new individual placement records plus their generated scene nodes; this avoids replay-order dependencies and lets future tile/MultiMesh backends consume the same authoritative records. Placement presets remain reusable inputs, terrain streaming stays separate, and Phase 1 supports only self-supporting Bramble vines because other vine modes require an independent surface/support density workflow.

Domain docs: `module_docs/world_placement/`.
