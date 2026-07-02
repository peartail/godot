# Generated Wrappers And Compatibility

## Scope

Generated extension wrappers, compatibility hashes, special compatibility mappings, and generator scripts.

## Entry Points

- `ext_wrappers.gen.h`
- `ext_wrappers.gen.inc`
- `GDExtensionCompatHashes`
- `GDExtensionSpecialCompatHashes`

## Flow Notes

- Generated wrappers adapt engine types and calls for extension-facing APIs.
- Compatibility hashes help detect or bridge API compatibility across versions.
- Generated files should be changed through generator scripts where possible.

## Code Links

- `core/extension/ext_wrappers.gen.h`
- `core/extension/ext_wrappers.gen.inc`
- `core/extension/gdextension_special_compat_hashes.*`
- `core/extension/make_wrappers.py`
- `core/extension/make_interface_dumper.py`
- `core/extension/make_interface_header.py`