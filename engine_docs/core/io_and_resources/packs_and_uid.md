# Packs And UID

## Scope

Packed project data, PCK creation, ZIP bridge functions, and stable resource UID mapping.

## Entry Points

- `PackedData`
- `PCKPacker`
- `ResourceUID`
- `zipio_open()`

## Flow Notes

- Packed data lets exported projects and resource packs override or provide files.
- `ResourceUID` maps `uid://` text to paths and persists the cache.
- Pack replacement order matters when multiple packs provide the same path.

## Code Links

- `core/io/file_access_pack.*`
- `core/io/pck_packer.*`
- `core/io/resource_uid.*`
- `core/io/zip_io.*`
- `core/io/file_access_zip.*`