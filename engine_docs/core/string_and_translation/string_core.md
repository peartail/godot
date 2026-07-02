# String Core

## Scope

Unicode text storage, construction, conversion, formatting, splitting, searching, and mutation helpers.

## Entry Points

- `String`
- `CharString`
- `StringBuffer`
- `StringBuilder`
- `char_utils.h`

## Flow Notes

- `String` is the engine-wide Unicode text representation used by APIs and serialization.
- Conversion helpers bridge UTF-8, UTF-16, UTF-32, ASCII, and platform encodings.
- Builder/buffer helpers reduce allocation pressure in repeated concatenation paths.

## Code Links

- `core/string/ustring.h`
- `core/string/ustring.cpp`
- `core/string/string_buffer.h`
- `core/string/string_builder.*`
- `core/string/char_utils.h`