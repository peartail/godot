# Property Helpers

## Scope

Scene property utilities, property list helper behavior, validation helpers, and inspector/serialization-facing metadata helpers.

## Entry Points

- `PropertyUtils`
- `PropertyListHelper`

## Flow Notes

- Property helpers support scene classes that expose dynamic or grouped property lists.
- These utilities are used by inspector-facing and serialization-facing code paths.
- Property metadata should remain consistent with ClassDB and resource serialization expectations.

## Code Links

- `scene/property_utils.h`
- `scene/property_utils.cpp`
- `scene/property_list_helper.h`
- `scene/property_list_helper.cpp`
- `core/object/property_info.*`