# Undo Redo

## Scope

Action history, do/undo method calls, property changes, reference tracking, and merge behavior.

## Entry Points

- `UndoRedo`
- `UndoRedo::create_action()`
- `UndoRedo::add_do_method()`
- `UndoRedo::add_undo_method()`
- `UndoRedo::add_do_property()`
- `UndoRedo::add_undo_property()`
- `UndoRedo::commit_action()`

## Flow Notes

- Actions collect do and undo operations before commit.
- Operations can target methods, properties, references, and object state.
- Editor systems usually wrap user-visible mutations in `UndoRedo`.
- Object lifetime matters because stored operations may reference objects later.

## Code Links

- `core/object/undo_redo.h`
- `core/object/undo_redo.cpp`
- `editor/editor_undo_redo_manager.*`