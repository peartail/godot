# OS And Threading

Platform abstraction, main-loop base APIs, time, memory, synchronization, keyboard codes, and process/shared-object helpers.

## Subtopics

- [OS Abstraction](os_abstraction.md): platform services exposed through `OS`.
- [MainLoop And Time](main_loop_and_time.md): `MainLoop`, time APIs, and date/time enums.
- [Memory](memory.md): memory allocation helpers and memory tracking hooks.
- [Threading Primitives](threading_primitives.md): threads, mutexes, semaphores, locks, and thread-safety helpers.
- [Keyboard](keyboard.md): key codes, modifier masks, key locations, and key string helpers.
- [Shared Objects And Process IDs](shared_objects_and_process_ids.md): shared object handles and process identifiers.

## Global Entry Points

- `OS` is the process/platform abstraction used by engine code.
- `MainLoop` is the base loop interface run by `main`.
- Threading primitives are shared by core, servers, editor, and modules.

## Code Links

- `core/os/os.*`
- `core/os/main_loop.*`
- `core/os/time.*`
- `core/os/thread.*`
- `core/os/memory.*`