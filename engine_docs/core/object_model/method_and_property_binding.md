# Method And Property Binding

## Scope

Reflection call glue, method signatures, property metadata, argument defaults, return information, and bind compatibility.

## Entry Points

- `MethodBind`
- `MethodInfo`
- `PropertyInfo`
- `CallableCustomMethodPointerBase`
- `MethodBindT` templates
- `ClassDB::bind_method()`

## Flow Notes

- `MethodBind` stores how to call a native method through the reflected API.
- `method_bind_common.h` provides typed template binders for common member function shapes.
- `MethodInfo` describes method names, arguments, defaults, flags, and return type.
- `PropertyInfo` describes inspector/serialization-facing property type, hint, usage, and class metadata.

## Code Links

- `core/object/method_bind.h`
- `core/object/method_bind.cpp`
- `core/object/method_bind_common.h`
- `core/object/method_info.h`
- `core/object/method_info.cpp`
- `core/object/property_info.h`
- `core/object/property_info.cpp`
- `core/object/callable_mp.*`