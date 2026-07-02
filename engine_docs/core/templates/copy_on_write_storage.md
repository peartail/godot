# Copy On Write Storage

## Scope

Shared storage patterns and copy-on-write backing data for engine value/container types.

## Entry Points

- `CowData`
- `Vector`
- `String`
- `Array`

## Flow Notes

- Copy-on-write storage lets values share data until mutation.
- Mutating paths must detach when data is shared or read-only.
- Refcount and allocation behavior here has wide performance impact.

## Code Links

- `core/templates/cowdata.h`
- `core/templates/vector.h`
- `core/string/ustring.*`
- `core/variant/array.*`