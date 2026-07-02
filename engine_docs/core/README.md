# Core

Low-level engine foundations shared by runtime, editor, servers, modules, and scripts.

## Feature Categories

- [Object Model](object_model/README.md): `Object`, `ClassDB`, method/property binding, signals, script instance hooks, and lifetime basics.
- [Variant And Containers](variant_and_containers/README.md): `Variant`, `Array`, `Dictionary`, typed containers, operators, calls, and serialization parser glue.
- [String And Translation](string_and_translation/README.md): `String`, `StringName`, `NodePath`, translation domains, locales, plural rules, and printing.
- [Math And Geometry](math_and_geometry/README.md): vectors, transforms, geometry helpers, AStar, BVH, random, expressions, and spatial algorithms.
- [IO And Resources](io_and_resources/README.md): `FileAccess`, `DirAccess`, resources, loaders/savers, packs, images, JSON/XML, compression, and networking streams.
- [OS And Threading](os_and_threading/README.md): `OS`, `MainLoop`, time, memory, threads, mutexes, locks, semaphores, keyboard, and shared objects.
- [Config And Engine](config_and_engine/README.md): `Engine`, `ProjectSettings`, global settings, feature tags, autoloads, and singleton registration.
- [Input Core](input_core/README.md): `Input`, `InputMap`, `InputEvent`, shortcuts, controller mappings, and event codecs.
- [Extension Core](extension_core/README.md): GDExtension interface, manager, library loading, API dumps, compatibility hashes, and generated wrappers.
- [Debugger And Profiling](debugger_and_profiling/README.md): engine debugger, local/remote debugger, script debugger, profiler, and profiling metadata.
- [Crypto](crypto/README.md): AES, hashing, crypto resources, certificates, and encryption helpers.
- [Error Handling](error_handling/README.md): error enum list, diagnostics macros, and reporting behavior.
- [Registration And Generated Data](registration_and_generated_data/README.md): core type registration, generated license/version/author data, builders, and bindings.
- [Templates](templates/README.md): engine container templates, copy-on-write storage, maps, sets, lists, and thread-safe queues.

## Global Entry Points

- `register_core_types()` / `unregister_core_types()` register core classes and singletons.
- `Engine` owns process-wide runtime metadata and singleton registry.
- `ProjectSettings` owns project configuration and global setting definitions.
- `ClassDB` exposes native classes, methods, properties, constants, and signals.
- `Variant` is the universal value type used by script bindings, properties, serialization, and calls.

## Code Links

- `core/register_core_types.*`
- `core/core_bind.*`
- `core/core_constants.*`
- `core/core_string_names.h`
- `core/doc_data.*`
- `core/typedefs.h`