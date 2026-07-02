# String And Translation

Text primitives, interned names, node paths, translation resources, locale data, and print helpers.

## Subtopics

- [String Core](string_core.md): Unicode string storage, conversion, formatting, and search helpers.
- [StringName](string_name.md): interned immutable names used by ClassDB, properties, and signals.
- [NodePath](node_path.md): node/resource path representation used by scenes and serialized data.
- [Translation Resources](translation_resources.md): translation resources, optimized translations, and PO loading.
- [Translation Server](translation_server.md): domains, remaps, locale selection, and translation lookup.
- [Locale And Printing](locale_and_printing.md): locale tables, plural rules, fuzzy search, and print helpers.

## Global Entry Points

- `String` is the core Unicode text type.
- `StringName` provides fast identity-style comparisons for repeated names.
- `TranslationServer` coordinates locale and translation domains.

## Code Links

- `core/string/ustring.*`
- `core/string/string_name.*`
- `core/string/node_path.*`
- `core/string/translation*`
- `core/string/locales.h`
- `core/string/print_string.*`