# Textures And Images

## Scope

Texture resources, image-backed textures, compressed/portable textures, atlas/mesh/gradient textures, camera/external textures, and placeholders.

## Entry Points

- `Texture2D`
- `ImageTexture`
- `CompressedTexture2D`
- `PortableCompressedTexture2D`
- `AtlasTexture`
- `GradientTexture`
- `CameraTexture`
- `ExternalTexture`

## Flow Notes

- Texture resources bridge CPU image data and rendering server texture RIDs.
- Compressed and portable textures have resource format loaders/savers.
- Placeholder textures preserve references when data is unavailable or stripped.

## Code Links

- `scene/resources/texture.*`
- `scene/resources/image_texture.*`
- `scene/resources/compressed_texture.*`
- `scene/resources/portable_compressed_texture.*`
- `scene/resources/atlas_texture.*`
- `scene/resources/gradient_texture.*`
- `scene/resources/camera_texture.*`
- `scene/resources/external_texture.*`
- `scene/resources/placeholder_textures.*`