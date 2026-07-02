# Serialization Formats

## Scope

Binary resource format, Variant marshaling, JSON, XML, plist, and structured format helpers.

## Entry Points

- `ResourceFormatLoaderBinary`
- `ResourceFormatSaverBinary`
- `Marshalls`
- `JSON`
- `XMLParser`

## Flow Notes

- Binary resource format is central to `.res` and `.scn` loading/saving.
- Marshaling helpers encode Variant and primitive values for storage or transport.
- JSON/XML parsers are script-facing utilities and internal data helpers.

## Code Links

- `core/io/resource_format_binary.*`
- `core/io/marshalls.*`
- `core/io/json.*`
- `core/io/xml_parser.*`
- `core/io/plist.*`
- `core/variant/variant_parser.*`