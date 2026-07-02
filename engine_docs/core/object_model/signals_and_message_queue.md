# Signals And Message Queue

## Scope

Signal connections, signal emission, deferred method calls, deferred notifications, and queued object work.

## Entry Points

- `Object::connect()`
- `Object::disconnect()`
- `Object::emit_signal()`
- `Object::call_deferred()`
- `MessageQueue`
- `CallQueue`

## Flow Notes

- Signal metadata is registered through `ClassDB::add_signal()`.
- Signal connections are stored on `Object` and can target callables.
- Deferred calls and notifications are queued through `MessageQueue`.
- Queued calls are flushed from the main loop, so code must account for object lifetime between enqueue and execution.

## Code Links

- `core/object/object.h`
- `core/object/object.cpp`
- `core/object/message_queue.h`
- `core/object/message_queue.cpp`
- `main/main.cpp`