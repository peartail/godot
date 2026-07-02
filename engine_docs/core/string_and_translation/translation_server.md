# Translation Server

## Scope

Locale selection, translation lookup, domain routing, fallback behavior, and translation remaps.

## Entry Points

- `TranslationServer`
- `TranslationDomain`
- `TranslationServer::translate()`
- `TranslationServer::set_locale()`

## Flow Notes

- `TranslationServer` owns the active locale and default translation lookup path.
- Translation domains allow different systems or resources to isolate translation tables.
- Remaps let resources be substituted based on locale.

## Code Links

- `core/string/translation_server.h`
- `core/string/translation_server.cpp`
- `core/string/translation_domain.*`
- `core/string/translation_server.compat.inc`