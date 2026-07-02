# RefCounted

## Scope

Reference-counted object lifetime and `Ref<T>` ownership semantics.

## Entry Points

- `RefCounted`
- `Ref<T>`
- `RefCounted::reference()`
- `RefCounted::unreference()`
- `RefCounted::init_ref()`

## Flow Notes

- `RefCounted` derives from `Object` but is normally owned through `Ref<T>`.
- `Ref<T>` increments and decrements the object reference count.
- Objects free themselves when the reference count reaches zero.
- Mixing raw pointers and `Ref<T>` requires care because raw pointers do not keep the instance alive.

## Code Links

- `core/object/ref_counted.h`
- `core/object/ref_counted.cpp`
- `core/object/object.h`