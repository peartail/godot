# Text Controls

## Scope

Text display/editing, code editing, rich text layout, labels, syntax highlighting, line edit behavior, and rich text effects.

## Entry Points

- `Label`
- `LineEdit`
- `TextEdit`
- `CodeEdit`
- `RichTextLabel`
- `SyntaxHighlighter`
- `RichTextEffect`

## Flow Notes

- Text controls depend on TextServer shaping and theme font data.
- `TextEdit` and `CodeEdit` are large editing surfaces with selection, caret, and syntax behavior.
- Rich text parses tags and can invoke custom effects.

## Code Links

- `scene/gui/label.*`
- `scene/gui/line_edit.*`
- `scene/gui/text_edit.*`
- `scene/gui/code_edit.*`
- `scene/gui/rich_text_label.*`
- `scene/gui/rich_text_effect.*`
- `scene/resources/syntax_highlighter.*`