# Materials And Shaders

## Scope

Material resources, shader resources, shader includes, visual shaders, canvas materials, particle process materials, skies, and fog/sky materials.

## Entry Points

- `Material`
- `ShaderMaterial`
- `CanvasItemMaterial`
- `ParticleProcessMaterial`
- `Shader`
- `ShaderInclude`
- `VisualShader`
- `Sky`

## Flow Notes

- Materials configure rendering-server state for meshes, canvas items, particles, and sky/environment paths.
- Shader resources compile through rendering backends and may include external shader includes.
- Visual shader resources generate shader code from graph data.

## Code Links

- `scene/resources/material.*`
- `scene/resources/canvas_item_material.*`
- `scene/resources/particle_process_material.*`
- `scene/resources/shader.*`
- `scene/resources/shader_include.*`
- `scene/resources/shader_resource_format.*`
- `scene/resources/visual_shader*`
- `scene/resources/sky*`