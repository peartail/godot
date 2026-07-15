# Blender Stylized Tree Modeling and Materials

## 7. Trunk and Branch Construction

### 7.1 Curve-based structure

- Author trunk and branch centerlines as curves.
- Use approximately 4 to 8 meaningful control points for a typical stylized trunk.
- Add curve resolution only where curvature changes the silhouette.
- Keep straight sections low resolution.
- Create the surface with `Curve to Mesh` or an equivalent controlled sweep.
- Begin with a 6- to 10-sided profile for a close-range stylized trunk, then adjust for the visual target.
- Slightly deform or rotate cross-sections to avoid a perfect extruded cylinder.

### 7.2 Taper

Use a predictable taper based on normalized distance along the curve. A suitable conceptual form is:

```text
radius = base_radius * pow(1 - curve_factor, taper_power)
```

The implementation may differ, but it must:

- preserve sufficient thickness at major branch junctions;
- avoid collapsing to degenerate triangles at tips;
- hide or terminate branch tips cleanly inside canopy volumes when appropriate;
- keep the root area visually stable across variants.

### 7.3 Junctions

- Branches may penetrate slightly into the trunk.
- A watertight Boolean union is not required for ordinary game assets.
- Avoid visible gaps, severe depth-fighting, and obviously floating branches.
- Do not add expensive topology solely to create biologically perfect junctions.
- Use a small number of modeled root flares or buttresses to improve the base silhouette where needed.

### 7.4 Branch hierarchy

- One level of primary branches is sufficient for the initial tool.
- Add a second branch level only if it materially improves the silhouette.
- Avoid unrestricted recursive branching.
- Use species-specific branch direction and length rules.
- Keep primary branches readable at gameplay distance.
- Omit branches that are completely hidden and do not affect shadowing or silhouette.

## 8. Canopy Module Library

Create a curated library of reusable canopy modules. Suggested module roles include:

- round large;
- round small;
- flattened;
- drooping;
- top cap;
- irregular or broken;
- conical for applicable species.

Each module must:

- have an intentional asymmetric silhouette;
- contain broad readable surfaces;
- avoid looking like an untouched UV sphere or ico sphere;
- have clean normals and no unintended holes;
- include UVs and any required vertex data;
- use the shared species canopy material;
- have a predictable local origin suitable for placement on a branch endpoint.

Canopy placement may vary:

- module choice;
- rotation;
- uniform and limited non-uniform scale;
- small position offset;
- palette or shade index.

Variation must not create detached floating clusters, severe self-intersection artifacts, or a noisy silhouette. Some overlap between foliage masses is expected and acceptable.

## 9. Geometry Nodes Generator Requirements

The main generator should expose at least:

- species/profile selection;
- master seed;
- silhouette archetype;
- total height;
- trunk base radius;
- trunk bend direction and strength;
- taper amount;
- primary branch count;
- branch length and elevation ranges;
- canopy module collection;
- canopy cluster count;
- canopy scale range;
- asymmetry amount;
- color or palette variation index;
- preview detail level;
- output realization or bake control.

Recommended node flow:

```text
Profile Inputs
  -> Trunk Curve and Radius
  -> Curve to Mesh
  -> Branch Curves and Radius
  -> Curve to Mesh
  -> Branch Endpoint / Canopy Placement Points
  -> Instance Canopy Modules
  -> Store Required Attributes
  -> Realize Instances for Approved Output
  -> Join Geometry
```

Implementation rules:

- Keep exposed inputs grouped and labeled.
- Use consistent units and sensible min/max values.
- Avoid duplicate nodes with unexplained magic constants.
- Add node frames for trunk, branch, canopy, materials/data, and output stages.
- Do not realize canopy instances during interactive preview unless required.
- Realize instances before final mesh export when the exporter or downstream data requires it.
- Validate that stored attributes survive realization and glTF export.

## 10. Topology and Shading

- Spend geometry on silhouette and deformation, not hidden interior detail.
- Remove degenerate faces, zero-length edges, and accidental duplicate geometry.
- Recalculate and visually inspect face orientation.
- Use consistent smooth/flat shading according to the art style.
- Mark sharp edges or use controlled normals only when they survive the intended export workflow.
- Avoid long, thin triangles that cause visible shading artifacts.
- Avoid unnecessary subdivision modifiers on export objects.
- Inspect the asset with backface culling enabled.
- Inspect both lit and unlit/material-preview views to separate geometry defects from material effects.

Boolean-unioning every intersecting trunk, branch, and canopy component is not required. Merge objects or surfaces only when it improves export, draw-call organization, deformation, or editing reliability.

## 11. UV and Material Rules

- Prefer one trunk material and one canopy material per tree.
- Add a third material only when it has a clear gameplay or visual purpose.
- Keep material slot ordering consistent across variants and LODs.
- Use shared materials across variants to support batching and predictable imports.
- Use texture atlases when multiple small source textures would otherwise create material proliferation.
- Keep UV islands within their assigned atlas regions.
- Avoid extremely different texel densities between variants of the same species.
- Ensure UV seams are not conspicuous at normal camera distance.
- Do not create new material instances solely for minor color changes.

Use a palette texture, vertex data, or an engine-side per-instance parameter for controlled color variation. Keep all colors inside the approved species palette.

## 12. Wind and Auxiliary Vertex Data

Plan the data contract before painting channels. A recommended vertex-color contract is:

```text
Color.R = primary wind bend weight
Color.G = branch-tip or secondary bend weight
Color.B = canopy flutter weight
Color.A = phase/random variation
```

Suggested behavior:

- trunk base: primary bend near 0;
- upper trunk: gradually increasing primary bend;
- branch roots: lower secondary bend;
- branch tips: higher secondary bend;
- canopy: nonzero flutter and stable per-cluster phase.

Rules:

- Clamp channel values to the expected `0..1` range.
- Avoid abrupt gradients that cause visible tearing-like deformation.
- Keep the lower trunk and ground contact visually anchored.
- Confirm that Blender color attributes are exported and imported correctly before producing the full library.
- If vertex color is also required for visible albedo tinting, define a separate attribute or UV-based encoding rather than silently overloading the same channels.

