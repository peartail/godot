# Runtime Memory Snapshot and Render Cost Report Plan

## Goal

Build two runtime analysis tools that complement the existing Godot debugger and the new Scene Size Map:

- **Runtime Memory Snapshot**: capture what is alive while the game is running, compare snapshots, and explain memory growth in project terms.
- **Render Cost Report**: summarize per-frame rendering load and identify scenes, viewports, resources, and rendering patterns that likely cause frame spikes.

These tools should work with local play and remote Android play through Godot's existing remote debugger/autoconnect path.

## Existing Godot Capabilities To Reuse

### Remote Debugger

Godot already sends profiler data over the remote debugger connection. Android one-click deploy can enable remote debug and uses `adb reverse` for USB debugging, so the same profiler path can receive data from a physical Android device.

Use this instead of building a separate socket protocol.

### Performance Monitors

The `Performance` singleton already exposes useful runtime counters:

- FPS and frame times.
- Static memory and max static memory.
- Object, resource, node, and orphan node counts.
- Rendered objects, primitives, and draw calls.
- Video, texture, and buffer memory.
- Pipeline compilation counters.

These are enough for a first Render Cost Report timeline and a first memory trend view.

### ObjectDB Profiler Module

This branch already has `modules/objectdb_profiler` enabled. It captures ObjectDB snapshots through the debugger, compresses snapshot chunks, stores context such as memory usage and Godot version, and provides editor-side snapshot/diff views.

Runtime Memory Snapshot should extend or wrap this module rather than duplicating it.

Important limitation already documented in the UI: ObjectDB snapshots capture memory owned by ObjectDB-visible objects, not all native engine allocations.

### RenderingDevice Memory Usage

`RenderingDevice` exposes memory categories such as textures, buffers, and total GPU memory usage. This should be sampled into Render Cost Report and memory snapshots when available.

### RenderDoc

Godot has support points useful for RenderDoc workflows, including SPIR-V debug info generation for Vulkan. RenderDoc can inspect a captured frame deeply, but it is not a replacement for an in-editor report.

Use RenderDoc as an optional external deep-dive capture for suspicious frames.

## Tool 1: Runtime Memory Snapshot

### Purpose

Answer questions like:

- What grew between snapshot A and snapshot B?
- Are nodes/resources being leaked?
- Which resource classes dominate memory at runtime?
- Are textures, meshes, buffers, or ObjectDB objects the main contributor?
- Did Android memory pressure come from Godot objects, GPU resources, or system/native allocations?

### MVP Scope

Reuse the ObjectDB Profiler snapshot transport and add a higher-level "Runtime Memory Snapshot" view.

Capture:

- Snapshot timestamp.
- Current scene path if available.
- Total static memory and max static memory.
- ObjectDB object count.
- Resource count.
- Node count and orphan node count.
- Class histogram.
- Node tree summary.
- RefCounted summary.
- Resource summary by type and path.
- GPU memory counters: texture memory, buffer memory, video memory.
- Android optional data if running on device:
  - `dumpsys meminfo <package>` summary.
  - PSS, native heap, Java heap, graphics, stack, code, system categories when available.

### Resource-Level Additions

For runtime resource inspection, collect resources visible through ObjectDB and resource cache paths.

Useful fields:

- Object ID.
- Class/type.
- Resource path.
- Resource name.
- Ref count where available.
- Estimated CPU memory if known.
- Estimated GPU memory if known.
- Owning node references where cheaply discoverable.

Initial estimates can be conservative:

- `Texture2D`: use existing rendering/texture memory counters globally first; per-resource GPU memory can come later.
- `Mesh`: surfaces, vertex/index counts, material references.
- `AudioStream`: stream type and source path, not decoded size at first.
- `PackedScene`, `Script`, `Material`, `Animation`: object count and file path first.

### Snapshot Diff

Diff should prioritize signal over exhaustive data:

- New objects by class.
- Removed objects by class.
- Retained objects with changed important properties.
- Resource paths added/removed.
- Top classes by count delta.
- Top resource types by count delta.
- Memory counter delta.
- Android `dumpsys meminfo` delta if available.

The first useful UI can be a set of sortable tables:

- Summary
- Classes
- Resources
- Nodes
- RefCounted
- Android Memory

### Spike-Triggered Snapshot

Add optional triggers:

- Capture snapshot when frame time exceeds threshold.
- Capture before/after scene change.
- Capture every N seconds for a limited window.
- Capture when static memory increases by more than threshold.
- Capture manually from Debugger tab.

For Android, this is especially useful because intermittent memory spikes are hard to reproduce.

### Storage

Use the existing ObjectDB snapshot storage pattern, but add metadata:

- Project name.
- Platform.
- Device name.
- Package name.
- Renderer.
- Graphics API.
- Current scene.
- Snapshot reason: manual, frame spike, memory growth, scene change.

## Tool 2: Render Cost Report

### Purpose

Answer questions like:

- Why did this frame spike?
- Is the frame expensive because of draw calls, primitives, shader/pipeline compilation, GPU memory, or many visible objects?
- Which scene or viewport was active when cost rose?
- Did a resource import or shader compilation cause stutter?
- On Android, did the issue correlate with thermal/memory/system pressure?

### MVP Scope

Start with frame-level sampling using existing monitors:

- FPS.
- Process time.
- Physics process time.
- Rendered objects.
- Rendered primitives.
- Draw calls.
- Video memory used.
- Texture memory used.
- Buffer memory used.
- Pipeline compilation counters.
- Object/resource/node counts.

Store a rolling frame history, then allow exporting a report for a selected frame range.

### Report Structure

Each report should contain:

- Capture metadata:
  - project, platform, renderer, device, resolution, current scene.
- Timeline summary:
  - average, min, max, p95 frame time.
  - worst frames.
  - draw call and primitive trends.
  - memory trends.
- Spike table:
  - frame index/time.
  - frame duration.
  - draw calls.
  - primitives.
  - visible objects.
  - texture/buffer/video memory.
  - pipeline compilation deltas.
- Recommendations:
  - high draw calls.
  - high primitive count.
  - pipeline compilation during gameplay.
  - texture/buffer memory growth.
  - object/node growth.

### Viewport and Scene Awareness

MVP can be global. Later phases should add:

- Current scene path.
- Active camera.
- Viewport size.
- Viewport render target info.
- Viewport-level counters if exposed or added.
- Optional project-specific tags such as terrain chunk count, visible sector count, or loaded streaming cells.

### Rendering Resource Breakdown

Later phases should expose renderer-side resource data:

- Textures by RID/resource path.
- Buffers by category.
- Mesh surfaces by vertex/index count.
- Materials/shaders/pipelines used in recent frames.
- Render target memory.
- Shadow/gi/reflection resources.

This likely requires adding renderer instrumentation, because current public `Performance` counters are mostly global.

## RenderDoc Integration

RenderDoc can be useful, but it should be treated as a complementary deep inspection tool.

### What RenderDoc Is Good For

- Inspecting a captured GPU frame.
- Draw call list and render pass sequence.
- Pipeline state.
- Bound textures/buffers.
- Shader debugging when debug info is available.
- Overdraw/texture inspection depending on platform/API support.

### What RenderDoc Is Not Good For

- Long-running memory history.
- Godot ObjectDB/resource ownership.
- Scene/resource semantic grouping.
- Android system memory, Java heap, or native heap summaries.
- Automated project-level recommendations without extra mapping data.

### Proposed Integration

Add optional "Capture Suspicious Frame With RenderDoc" workflow:

1. Render Cost Report detects a spike.
2. User selects the frame or enables "capture next spike".
3. Godot triggers or instructs a RenderDoc capture if RenderDoc is available.
4. The report stores a link/path to the `.rdc` capture.
5. Godot report includes frame metadata so the user knows why the capture was taken.

### Practical Constraints

- RenderDoc capture is renderer/API/platform dependent.
- Android capture may require RenderDoc tooling, debuggable build, compatible GPU/API, and device setup.
- Godot should not depend on RenderDoc for core reports.
- The integration should fail gracefully and explain setup requirements.

## Android-Specific Extensions

Add an Android Device Monitor panel or subtab:

- Device selection.
- Package detection for current run.
- `adb logcat` stream with filters.
- `dumpsys meminfo <package>` snapshots.
- `dumpsys gfxinfo <package>` frame stats where useful.
- Battery/thermal status if available.
- ADB reverse/remote debug status.

Correlate Android data with Godot profiler data by timestamp.

Useful Android report sections:

- Godot memory counters.
- Android PSS/native/graphics memory.
- Frame stats from `gfxinfo`.
- Logcat warnings/errors around spike windows.
- Device thermal/battery state.

## Proposed Implementation Phases

### Phase 1: Documentation and UI Entry Points

- Document current profiler, ObjectDB snapshot, Android remote debug, and RenderDoc boundaries.
- Add clear menu/dock names:
  - Runtime Memory Snapshot
  - Render Cost Report
  - Android Device Monitor

### Phase 2: Runtime Memory Snapshot MVP

- Reuse `modules/objectdb_profiler` snapshot transport.
- Add richer snapshot context.
- Add Resource summary view.
- Add Performance monitor values into snapshot metadata.
- Add snapshot diff summary focused on growth.

### Phase 3: Render Cost Report MVP

- Add rolling monitor sampler.
- Record frame history from `Performance` monitors.
- Add spike detection.
- Add frame range report view/export.
- Add simple recommendations.

### Phase 4: Android Device Data

- Add ADB helper abstraction.
- Stream logcat into editor panel.
- Capture `dumpsys meminfo` on demand and on spike.
- Capture `dumpsys gfxinfo` on demand and on spike.
- Attach Android data to memory/render reports.

### Phase 5: Renderer Resource Attribution

- Add renderer-side debug queries for textures, buffers, render targets, mesh surfaces, and pipelines.
- Map renderer resources back to Godot resource paths where possible.
- Add per-resource GPU memory estimates.
- Add top resource tables and diffs.

### Phase 6: RenderDoc Assist

- Detect RenderDoc availability.
- Add manual capture instructions or trigger path.
- Store RenderDoc capture references in Render Cost Report.
- Add optional "capture next spike" workflow.

## Recommended First Implementation

Start with **Phase 2 + Phase 3 minimal versions**:

- Extend ObjectDB snapshot metadata with current `Performance` monitor values.
- Add Resource summary/diff to ObjectDB Profiler.
- Add a Render Cost Report dock that samples existing Performance monitors.
- Add spike detection and frame range export.

This gives useful runtime reports without waiting for deep renderer instrumentation or Android-specific ADB integration.

## Open Questions

- Should Runtime Memory Snapshot live inside `modules/objectdb_profiler`, or should it be a higher-level editor plugin that reuses it?
- Should Render Cost Report be part of the Debugger panel, or a separate dock?
- Which report export format is preferred: JSON, CSV, Markdown, or all three?
- Should Android Device Monitor be built into the Android export platform plugin or as a general debugger plugin?
- Do we need project-specific hooks for OpenWorldTerrain, such as visible chunk count, terrain material count, or streaming cache size?
