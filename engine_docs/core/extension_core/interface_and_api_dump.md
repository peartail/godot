# Interface And API Dump

## Scope

Public GDExtension C ABI, extension API JSON, interface dumps, and generated header output.

## Entry Points

- `GDExtensionInterface`
- `ExtensionAPIDump`
- `GDExtensionInterfaceHeaderGenerator`
- `--dump-extension-api`
- `--dump-gdextension-interface`

## Flow Notes

- The interface table is the stable ABI bridge used by external extension code.
- API dumps describe exposed engine classes and are used by bindings and tooling.
- Header/interface generation should be regenerated through tooling, not edited manually.

## Code Links

- `core/extension/gdextension_interface.cpp`
- `core/extension/gdextension_interface.gen.h`
- `core/extension/gdextension_interface.json`
- `core/extension/gdextension_interface.schema.json`
- `core/extension/extension_api_dump.*`
- `core/extension/gdextension_interface_header_generator.*`