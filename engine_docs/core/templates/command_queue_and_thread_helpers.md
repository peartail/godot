# Command Queue And Thread Helpers

## Scope

Command queues, thread-safe helper templates, safe references, and small synchronization-adjacent utilities.

## Entry Points

- `CommandQueueMT`
- `SafeRefCount`
- `SafeNumeric`
- `ThreadWorkPool`

## Flow Notes

- Command queues are used when work must be marshaled across threads or execution phases.
- Safe numeric/refcount helpers provide atomic-style state for low-level structures.
- Template helpers should stay independent from scene/editor-specific behavior.

## Code Links

- `core/templates/command_queue_mt.h`
- `core/templates/safe_refcount.h`
- `core/templates/safe_numeric.h`
- `core/templates/thread_work_pool.h`
- `core/object/message_queue.*`