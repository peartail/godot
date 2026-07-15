# Blender Stylized Tree Asset Development Guidelines

## 1. Purpose

This document defines the production rules for creating pre-generated, stylized tree variants in Blender and exporting them for use in Godot.

The intended workflow is:

1. Build reusable trunk, branch, and canopy components.
2. Assemble controlled variations with Geometry Nodes or equivalent non-destructive tools.
3. Generate multiple deterministic candidates from a seed.
4. Let an artist review and select acceptable variants.
5. Bake selected variants into ordinary meshes.
6. Produce LOD and collision assets.
7. Export the final assets as glTF/GLB for placement and instancing in Godot.

The system is an **art-directed variant authoring tool**, not a botanical growth simulator. Silhouette quality, readability, consistency, and runtime efficiency take priority over biological accuracy.

## 2. Target Visual Style

- Target a low- to mid-poly cartoon or stylized rendering style.
- Prefer clear, intentional silhouettes over small geometric detail.
- Represent foliage primarily as canopy masses or clusters, not thousands of individual leaves.
- Preserve visible planar changes on trunks and branches where they support the style.
- Avoid uniform spheres, perfectly straight cylinders, and uncontrolled procedural noise.
- Use asymmetry deliberately, while keeping every variant recognizable as the same species.
- Judge the asset at its expected gameplay camera distance, not only in close-up viewport inspection.

## 3. Core Production Principles

### 3.1 Controlled variation

Randomness must operate inside artist-defined ranges. Do not independently randomize every property.

Generate a tree hierarchically:

1. Select an overall silhouette archetype.
2. Determine correlated trunk height and thickness.
3. Place primary branches according to the species profile.
4. Distribute canopy clusters according to branch structure.
5. Apply small local changes to position, rotation, scale, and color.

Typical silhouette archetypes include:

- broad and low;
- balanced or average;
- tall and narrow;
- leaning;
- sparse or damaged;
- young tree.

Height, thickness, branch length, and canopy volume should be correlated. For example, increasing height without adjusting trunk thickness and branch proportions usually produces an implausible variant.

### 3.2 Deterministic generation

- Every generated tree must have a master integer `Seed`.
- The same inputs and seed must reproduce the same result.
- Derive separate sub-seeds for trunk, branches, canopy, and color when possible.
- Expose sub-seeds or lock controls so one part can be regenerated without changing the others.
- Do not rely on viewport state, object selection order, or unstable object names as sources of randomness.

Suggested seed domains:

```text
master_seed
|- trunk_seed
|- branch_seed
|- canopy_seed
`- color_seed
```

### 3.3 Non-destructive source, baked delivery

- Keep curves, source modules, Geometry Nodes modifiers, and generator controls in the authoring `.blend` file.
- Do not destructively apply the source generator until a candidate has been approved.
- Final delivery assets must be ordinary, stable meshes without runtime Blender dependencies.
- Preserve an editable source collection separately from generated candidates and export-ready meshes.

## 4. Scene Organization

Use collections with explicit responsibilities:

```text
TREE_SOURCE
|- Trunks
|- Branches
|- Canopies
|- Materials
`- GeometryNodeSources

TREE_GENERATED
|- Candidates
`- Approved

TREE_EXPORT
|- LOD0
|- LOD1
|- LOD2
`- Collision
```

Rules:

- Keep source modules outside export collections.
- Export only explicitly approved objects or collections.
- Do not leave hidden helper geometry inside export collections.
- Give every Geometry Nodes group and reusable component a descriptive name.
- Avoid names such as `Cube.001`, `Material.003`, or `GeometryNodes.002` in committed assets.

Recommended naming:

```text
SRC_Trunk_Oak_A
SRC_Branch_Oak_Medium_A
SRC_Canopy_Oak_Round_A
GEN_Tree_Oak_Seed0042
SM_Tree_Oak_01_LOD0
SM_Tree_Oak_01_LOD1
SM_Tree_Oak_01_LOD2
COL_Tree_Oak_01
MAT_Tree_Oak_Trunk
MAT_Tree_Oak_Canopy
GN_Tree_StylizedGenerator
```

## 5. Units, Origin, and Transforms

- Use Metric units.
- Treat 1 Blender unit as 1 meter.
- Place the object origin at the ground center of the trunk.
- Place the ground contact plane at Blender `Z = 0`.
- Model upright trees along positive Blender Z.
- Apply object rotation and scale before final export.
- Final export objects should normally have scale `(1, 1, 1)`.
- Do not leave negative scale on export objects.
- Keep the origin and forward convention consistent across all variants.
- Do not manually rotate the tree to compensate for Godot's coordinate system; glTF handles the Blender-to-Godot axis conversion.

## 6. Species Profile

Each species must define a limited visual grammar before variant generation begins.

At minimum, record:

- overall height range;
- trunk base radius range;
- height-to-thickness relationship;
- trunk taper and bend ranges;
- primary branch count range;
- allowed branch elevation and azimuth ranges;
- branch length and thickness ranges;
- canopy archetype;
- canopy cluster count and scale ranges;
- intended asymmetry range;
- trunk and canopy material palette;
- allowed damage, dead branch, or sparse variants;
- target triangle budgets and LOD distances.

Do not use one unrestricted generator profile for unrelated species. An oak, pine, birch, and dead tree should use different structural rules even if they share generator components.

