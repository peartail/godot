# Placement external preview mesh cache

Date: 2026-07-24

## Goal

Keep World Placement generated Tree/Rock/Vine **nodes** in the scene, but store the preview `ArrayMesh` (LOD0 on `MeshInstance3D.mesh`) as external `.res` files so `.tscn` does not embed large mesh blobs.

## Layout

```
{scene_dir}/_generated/world_placement/{placement_stable_id}_lod0.res
```

- Scene path comes from the nearest ancestor/`owner` with a non-empty `scene_file_path`.
- Unsaved scenes skip externalize (in-memory mesh only) and warn once.

## Rules

| Event | Behavior |
| --- | --- |
| Apply (new/replaced id) | Generate, then write/overwrite that id’s `.res` and `set_path(takeover)` on the node mesh |
| Rebuild | Prefer load existing `.res` if present; otherwise generate then write |
| Scene save | No bake; ExtResource references only |
| Delete by placement id | Delete matching `.res` |
| `clear_generated` | Remove nodes only; keep mesh files for later rebuild |
| `clear_placements` | Clear records + nodes; sweep orphan `.res` files |
| After apply | Sweep files whose ids are not in `placement_data.stable_ids` |

## Non-goals

- Data-only scenes
- Externalizing all LOD layers / Variants
- Auto-migrating already-embedded meshes in old scenes (clear + rebuild once)
