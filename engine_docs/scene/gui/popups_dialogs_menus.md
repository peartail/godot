# Popups Dialogs Menus

## Scope

Popup windows, popup menus, file dialogs, dialogs, menu bars, and menu button integration.

## Entry Points

- `Popup`
- `PopupMenu`
- `AcceptDialog`
- `ConfirmationDialog`
- `FileDialog`
- `MenuBar`
- `MenuButton`

## Flow Notes

- Popup behavior is tied to Window/Viewport focus and transient state.
- Popup menus own item state, accelerators, separators, and submenu behavior.
- File dialogs coordinate filesystem access, filters, and UI selection state.

## Code Links

- `scene/gui/popup.*`
- `scene/gui/popup_menu.*`
- `scene/gui/dialogs.*`
- `scene/gui/file_dialog.*`
- `scene/gui/menu_bar.*`
- `scene/gui/menu_button.*`