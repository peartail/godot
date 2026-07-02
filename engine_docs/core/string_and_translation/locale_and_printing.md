# Locale And Printing

## Scope

Locale tables, plural rules, fuzzy search helpers, capitalization data, and engine print helpers.

## Entry Points

- `TranslationServer::get_all_locales()`
- `PluralRules`
- `FuzzySearch`
- `print_line()`
- `print_error()`

## Flow Notes

- Locale tables are mostly static data used by translation and editor selection UIs.
- Plural rules influence translation lookup for count-dependent strings.
- Print helpers are used across core, tools, and command-line output.

## Code Links

- `core/string/locales.h`
- `core/string/plural_rules.*`
- `core/string/fuzzy_search.*`
- `core/string/print_string.*`
- `core/string/ucaps.h`
- `core/string/alt_codes.h`