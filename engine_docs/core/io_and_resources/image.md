# Image

## Scope

CPU image data, format conversion, pixel access, mipmaps, image loading, and resource format glue.

## Entry Points

- `Image`
- `ImageLoader`
- `ImageFormatLoader`
- `ImageResourceFormatLoader`
- `ImageResourceFormatSaver`

## Flow Notes

- `Image` stores CPU-side pixel data and format metadata.
- Image format loaders decode external files into `Image` instances.
- Rendering textures are separate server resources, though they often originate from `Image` data.

## Code Links

- `core/io/image.h`
- `core/io/image.cpp`
- `core/io/image_loader.*`
- `core/io/image_resource_format.*`
- `core/io/image.compat.inc`