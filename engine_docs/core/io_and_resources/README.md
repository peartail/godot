# IO And Resources

Filesystem access, resources, load/save pipeline, packs, image data, structured formats, compression, and network streams.

## Subtopics

- [Filesystem](filesystem.md): file and directory access, memory/compressed/encrypted/patched file wrappers, and path-facing behavior.
- [Resources](resources.md): `Resource`, missing resources, import metadata, and resource identity basics.
- [Resource Loading And Saving](resource_loading_and_saving.md): format loaders/savers, threaded loading, callbacks, and custom loaders.
- [Packs And UID](packs_and_uid.md): PCK/ZIP access, packed data containers, pack building, and `ResourceUID`.
- [Image](image.md): image storage, formats, image resource formats, and image loading.
- [Serialization Formats](serialization_formats.md): binary resource format, marshalls, JSON, XML, plist, and Variant serialization links.
- [Config And Compression](config_and_compression.md): `ConfigFile`, compression helpers, delta encoding, and logger output.
- [Network Streams](network_streams.md): stream peers, packet peers, sockets, TCP/UDP/UDS, TLS, DTLS, and HTTP client.

## Global Entry Points

- `FileAccess` and `DirAccess` abstract filesystem operations.
- `ResourceLoader` and `ResourceSaver` route resources through registered formats.
- `ResourceUID` maps stable `uid://` identifiers to project paths.

## Code Links

- `core/io/file_access.*`
- `core/io/dir_access.*`
- `core/io/resource.*`
- `core/io/resource_loader.*`
- `core/io/resource_saver.*`
- `core/io/resource_uid.*`