# Shared Objects And Process IDs

## Scope

Shared library handles, process identifier type, and small platform-facing OS support types.

## Entry Points

- `SharedObject`
- `ProcessID`
- `MIDIDriver`

## Flow Notes

- Shared object handling is used by extension loading and platform integration code.
- `ProcessID` keeps process identifiers abstract across platforms.
- MIDI driver abstraction is platform-backed but declared in core OS support.

## Code Links

- `core/os/shared_object.h`
- `core/os/process_id.h`
- `core/os/midi_driver.*`
- `core/extension/gdextension_library_loader.*`