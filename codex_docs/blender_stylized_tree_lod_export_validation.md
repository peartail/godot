# Blender Stylized Tree LOD, Export, and Validation

## 13. LOD Requirements

Create LODs intentionally; do not depend only on an automatic Decimate pass.

### LOD0

- Preserve the authored trunk and major branch silhouette.
- Preserve the full approved canopy arrangement.
- Retain wind data and required shading attributes.

### LOD1

- Reduce radial and longitudinal trunk segments where the silhouette permits.
- Remove small branches that do not affect the mid-distance silhouette.
- Simplify canopy modules while preserving total canopy volume.
- Preserve material slots and required vertex data.

### LOD2

- Retain a simple trunk and only the largest structural branches if visible.
- Merge or replace small canopy clusters with larger simplified masses.
- Prioritize outer silhouette and stable color over local shape detail.

### Impostor

- Treat billboard or impostor generation as a separate optional stage.
- Match the baked tree's scale, pivot, color response, and ground contact.
- Verify transitions under the game's expected lighting and camera angles.

Initial triangle-budget guidance for a stylized tree is:

```text
LOD0: approximately 2,000 to 8,000 triangles
LOD1: approximately 40% to 60% of LOD0
LOD2: approximately 10% to 20% of LOD0
```

These are starting ranges, not hard limits. Final budgets must be based on target hardware, visible tree count, shadow settings, overdraw, and gameplay camera distance.

## 14. Collision

- Collision must be separate from render geometry.
- Use a capsule or low-sided cylinder for the trunk in the default case.
- Add simplified branch collision only when gameplay requires climbing or physical contact.
- Do not generate collision for canopy masses by default.
- Use a separate interaction volume for selection, harvesting, or chopping when appropriate.
- Keep collision centered and aligned with the baked tree variant.
- Name collision objects explicitly, for example `COL_Tree_Oak_01`.

## 15. Candidate Generation and Curation

For each species:

1. Generate more candidates than will ship.
2. Review candidates from multiple fixed camera angles.
3. Reject candidates with broken proportions, tangled silhouettes, floating clusters, or obvious intersections.
4. Compare accepted trees side by side to ensure meaningful variation.
5. Remove near-duplicates.
6. Select approximately 8 to 16 strong variants for the initial library.
7. Record the source seed and profile parameters for every accepted variant.

Do not ship every procedurally valid result. Procedural validity is not equivalent to artistic approval.

## 16. Export to Godot

- Prefer glTF 2.0 or binary GLB.
- Export only approved export collections or selected objects.
- Apply required modifiers to the exported result while preserving editable source objects in the `.blend` file.
- Apply final scale and rotation.
- Include normals, tangents when required, UVs, materials, and required color attributes.
- Exclude cameras, lights, helpers, generators, and source modules unless explicitly requested.
- Keep the root pivot at the trunk's ground center.
- Keep material names stable across re-exports.
- Verify object and material names in the imported Godot scene.
- Verify that the imported tree stands upright and has the expected real-world size.
- Verify LOD, collision, vertex color, and wind behavior inside Godot, not only in Blender.

Runtime placement should select among baked tree meshes and apply only inexpensive per-instance variation such as:

- Y-axis rotation;
- limited uniform or non-uniform scale;
- controlled color tint;
- wind phase offset.

Do not generate unique runtime meshes for every placed tree if Godot `MultiMesh` or another instancing path is expected.

## 17. Validation Checklist

Before accepting a tree variant, verify all of the following.

### Structure and silhouette

- The tree is recognizable as the intended species.
- The silhouette is readable at gameplay distance.
- The variant is meaningfully different from other accepted variants.
- No branch or canopy cluster appears detached.
- The ground contact looks stable.
- Randomness has not broken the intended proportions.

### Mesh

- No unintended non-manifold gaps are visible.
- No degenerate or zero-area faces remain.
- No accidental duplicate geometry remains.
- Face orientation is correct.
- Shading and normals are stable.
- Triangle count is within the target range.

### Data

- UVs are present and correctly mapped.
- Material slots are minimal and consistently ordered.
- Wind and auxiliary attributes exist and use the documented channel contract.
- Attribute gradients are smooth where deformation requires them.

### Transform and naming

- Origin is at the ground center.
- Ground contact is at `Z = 0` in Blender.
- Final rotation and scale are applied.
- Object, material, collection, and node-group names follow the naming convention.
- The accepted seed and profile values are recorded.

### LOD and collision

- LOD silhouettes remain consistent with LOD0.
- LOD transitions do not cause major volume or color popping.
- Required attributes survive on every LOD.
- Collision is simple, separate, and correctly aligned.

### Engine import

- The GLB/glTF imports successfully in Godot.
- Scale, orientation, origin, materials, and vertex data are correct.
- The tree works with the intended instancing path.
- Wind deformation remains anchored at the base.
- Representative trees are tested under target lighting, shadows, and camera distance.

## 18. Initial Implementation Scope

The first production version should include:

- one species profile;
- 3 to 5 trunk curve archetypes;
- 6 to 10 reusable branch forms or controlled procedural branch patterns;
- 5 to 8 canopy modules;
- deterministic seed-based generation;
- one level of primary branches;
- controlled canopy placement and color variation;
- candidate generation and artist selection;
- baked LOD0, LOD1, and LOD2 meshes;
- simple trunk collision;
- documented wind vertex data;
- GLB/glTF export validated in Godot.

The first version does not require:

- a biologically accurate growth simulation;
- an unrestricted L-system;
- runtime mesh generation in Godot;
- individual modeled leaves across the whole canopy;
- fully watertight Boolean branch junctions;
- automatic acceptance of every generated seed;
- seasonal growth or destruction simulation.

Build the smallest generator that reliably creates art-directable, reusable tree variants. Expand recursion, species complexity, and environmental response only after the basic bake-and-export pipeline is visually and technically validated.

