# Registration And Generated Data

Core type registration, binding support, constants, string names, generated metadata, version data, and builder scripts.

## Subtopics

- [Core Registration](core_registration.md): core type registration and unregister flow.
- [Core Binds And Constants](core_binds_and_constants.md): core binds, constants, global class docs data, and string names.
- [Generated Metadata](generated_metadata.md): generated author/license/version/script-key data and builder scripts.

## Global Entry Points

- `register_core_types()` registers core classes and singletons.
- `CoreBind` exposes core support APIs to scripts.
- Generated metadata is produced by build scripts and consumed during startup/docs generation.

## Code Links

- `core/register_core_types.*`
- `core/core_bind.*`
- `core/core_constants.*`
- `core/core_builders.py`