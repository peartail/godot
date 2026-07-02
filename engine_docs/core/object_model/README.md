# Object Model

Native object identity, reflection metadata, binding, signals, references, script hooks, and object-adjacent services.

## Subtopics

- [Object And ObjectDB](object_and_objectdb.md): object identity, metadata, notifications, paths, properties, signals, and instance lookup.
- [ClassDB Registration](classdb_registration.md): native class registration, inheritance metadata, methods, properties, constants, signals, and enums.
- [Method And Property Binding](method_and_property_binding.md): `MethodBind`, `MethodInfo`, `PropertyInfo`, argument metadata, and call glue.
- [Signals And Message Queue](signals_and_message_queue.md): signal connection behavior, deferred calls, queued notifications, and `MessageQueue`.
- [RefCounted](ref_counted.md): reference-counted object lifetime and `Ref<T>` ownership.
- [Script Language Hooks](script_language_hooks.md): script instances, script languages, extension languages, and script backtraces.
- [Worker Thread Pool](worker_thread_pool.md): shared task groups and worker-thread scheduling.
- [Undo Redo](undo_redo.md): command/action history used by editor and engine tools.
- [Generated Virtuals](generated_virtuals.md): generated `GDVIRTUAL` glue and virtual call helpers.

## Global Entry Points

- `Object` is the base class for most reflected engine objects.
- `ObjectDB` maps `ObjectID` values back to live object instances.
- `ClassDB` stores native class metadata used by scripting, serialization, editor inspection, and docs.
- `MethodBind` bridges reflected method calls to C++ member functions.
- `ScriptLanguage` lets language backends attach script instances to `Object`.

## Code Links

- `core/object/object.*`
- `core/object/object_id.h`
- `core/object/class_db.*`
- `core/object/method_bind.*`
- `core/object/method_info.*`
- `core/object/property_info.*`
- `core/object/message_queue.*`
- `core/object/ref_counted.*`
- `core/object/script_language.*`
- `core/object/worker_thread_pool.*`
- `core/object/undo_redo.*`