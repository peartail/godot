# Filesystem

## Scope

File and directory access abstractions, specialized file wrappers, packed/ZIP file access, and filesystem-facing compatibility.

## Entry Points

- `FileAccess`
- `DirAccess`
- `FileAccessMemory`
- `FileAccessCompressed`
- `FileAccessEncrypted`
- `FileAccessPack`
- `FileAccessZip`

## Flow Notes

- Platform-specific file access is registered behind the common `FileAccess` API.
- Wrapper file access classes layer compression, encryption, memory, patching, or pack lookup over base access.
- Path localization/globalization often involves `ProjectSettings`, not only `FileAccess`.

## Code Links

- `core/io/file_access.*`
- `core/io/dir_access.*`
- `core/io/file_access_memory.*`
- `core/io/file_access_compressed.*`
- `core/io/file_access_encrypted.*`
- `core/io/file_access_pack.*`
- `core/io/file_access_zip.*`
- `core/io/file_access_patched.*`