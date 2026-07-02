# Default Theme Generation

## Scope

Default theme setup, generated default font data, generated default icons, and builder script inputs.

## Entry Points

- `make_default_theme()`
- `default_theme_builders.py`
- `default_font.gen.h`
- `default_theme_icons.gen.h`

## Flow Notes

- Default theme data is generated and compiled into the engine.
- Generated headers should be updated through builder scripts, not edited manually.
- Default theme setup is used before project/editor themes override it.

## Code Links

- `scene/theme/default_theme.*`
- `scene/theme/default_theme_builders.py`
- `scene/theme/default_font.gen.h`
- `scene/theme/default_theme_icons.gen.h`