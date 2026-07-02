# Resources

## Scope

Resource base behavior, paths, scene/resource identity, missing resources, import metadata, and resource data containers.

## Entry Points

- `Resource`
- `MissingResource`
- `ResourceImporter`
- `PackedDataContainer`

## Flow Notes

- `Resource` derives from `RefCounted` and is normally held through `Ref<Resource>`.
- Resource paths and UIDs affect caching, duplication, serialization, and editor workflows.
- `MissingResource` preserves references when a class or resource cannot be loaded.

## Code Links

- `core/io/resource.h`
- `core/io/resource.cpp`
- `core/io/missing_resource.*`
- `core/io/resource_importer.*`
- `core/io/packed_data_container.*`