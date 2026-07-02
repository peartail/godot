# Maps And Sets

## Scope

Hash maps, hash sets, ordered maps, tree maps, pair helpers, hash functions, and lookup containers.

## Entry Points

- `HashMap`
- `HashSet`
- `AHashMap`
- `RBMap`
- `OAHashMap`

## Flow Notes

- Hash and equality behavior must stay stable for stored key types.
- Ordered containers are used where deterministic iteration or sorted lookup matters.
- Map/set templates are used in reflection, resources, servers, editor, and import code.

## Code Links

- `core/templates/hash_map.h`
- `core/templates/hash_set.h`
- `core/templates/a_hash_map.*`
- `core/templates/rb_map.h`
- `core/templates/oa_hash_map.h`
- `core/templates/hashfuncs.h`