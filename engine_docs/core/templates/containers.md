# Containers

## Scope

General-purpose engine containers for contiguous, linked, paged, fixed-size, and local storage.

## Entry Points

- `Vector`
- `LocalVector`
- `List`
- `FixedVector`
- `PagedArray`
- `BinSortedArray`

## Flow Notes

- Core containers are used broadly and should remain lightweight and dependency-minimal.
- Allocation behavior affects performance in servers, scene code, and editor tools.
- Some containers use engine allocators and copy-on-write storage.

## Code Links

- `core/templates/vector.h`
- `core/templates/local_vector.h`
- `core/templates/list.h`
- `core/templates/fixed_vector.h`
- `core/templates/paged_array.h`
- `core/templates/bin_sorted_array.h`