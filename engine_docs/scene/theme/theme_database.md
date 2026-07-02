# Theme Database

## Scope

Theme database singleton behavior, default/project theme lookup, type variation data, and theme owner support.

## Entry Points

- `ThemeDB`
- `ThemeOwner`
- `Theme`

## Flow Notes

- `ThemeDB` provides default/project theme state used by Control theme lookup.
- ThemeOwner helps nodes/resources expose theme override ownership behavior.
- Runtime UI theme resolution often crosses `Control`, `ThemeDB`, and `Theme` resources.

## Code Links

- `scene/theme/theme_db.*`
- `scene/theme/theme_owner.*`
- `scene/resources/theme.*`
- `scene/gui/control.*`