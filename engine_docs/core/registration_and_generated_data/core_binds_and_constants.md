# Core Binds And Constants

## Scope

Script-facing core bind classes, core constants, core string names, documentation data, and type definitions.

## Entry Points

- `CoreBind`
- `CoreConstants`
- `CoreStringNames`
- `DocData`

## Flow Notes

- Core binds expose foundational helpers to scripts without belonging to scene/server layers.
- Constants and string names are shared across bindings, docs, and native code.
- Documentation data is consumed by editor help, extension API dumps, and agent docs.

## Code Links

- `core/core_bind.h`
- `core/core_bind.cpp`
- `core/core_constants.*`
- `core/core_string_names.h`
- `core/doc_data.*`
- `core/typedefs.h`