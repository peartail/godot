# Templates

Reusable engine containers, copy-on-write storage, maps, sets, lists, queues, sort helpers, and compile-time traits.

## Subtopics

- [Containers](containers.md): vectors, lists, fixed vectors, paged arrays, and local vectors.
- [Copy On Write Storage](copy_on_write_storage.md): `CowData` and shared storage patterns.
- [Maps And Sets](maps_and_sets.md): hash maps, hash sets, RB maps, ordered maps, and supporting hash utilities.
- [Command Queue And Thread Helpers](command_queue_and_thread_helpers.md): multi-thread command queues and thread-facing helper templates.

## Global Entry Points

- Templates are low-level and should avoid dependencies on higher engine layers.
- Many Variant, String, Resource, and server data structures depend on these containers.

## Code Links

- `core/templates/`
- `core/templates/cowdata.h`
- `core/templates/vector.h`
- `core/templates/hash_map.h`
- `core/templates/command_queue_mt.h`