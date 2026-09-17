# Scrollable editor toolbar

Date: 2026-09-17

## Goal

Editor toolbars that are too narrow for their contents currently clip icons with no way to
reach them. Add a reusable container that keeps the toolbar on a single row and lets the user
reach hidden items by clicking overlay arrows or dragging the toolbar sideways.

## Problem

| Toolbar | Container today | Behavior when too narrow |
| --- | --- | --- |
| Game tab (`game_view_plugin.cpp:1499`) | `HBoxContainer main_menu_fc` | Clips; pushes parent wider |
| Inspector dock (`inspector_dock.cpp:778`, `:855`) | `HBoxContainer` | Clips |
| 3D viewport (`node_3d_editor_plugin.cpp:3513`) | `HFlowContainer main_flow` | Wraps; eats viewport height |
| 2D viewport (`canvas_item_editor_plugin.cpp:5816`) | `HFlowContainer main_flow` | Wraps; eats viewport height |

`scene/gui/` has no reusable scrolling-with-arrows container. `TabBar` is the only control with
`<` `>` arrows and it draws them in `_draw()`, so none of it is reusable.

## Component

`editor/gui/editor_scrollable_toolbar.{h,cpp}`, following the `EditorToolbarGroup::create()`
idiom already used by the Game and 2D toolbars.

```cpp
class EditorScrollableToolbar : public Container {
    GDCLASS(EditorScrollableToolbar, Container);

    ScrollContainer *scroll = nullptr;
    HBoxContainer *content = nullptr;   // toolbar items go here
    Button *left_arrow = nullptr;       // '<'
    Button *right_arrow = nullptr;      // '>'

public:
    HBoxContainer *get_content() const { return content; }
    static HBoxContainer *create(Control *p_parent);
    Size2 get_minimum_size() const override;
};
```

`scroll` is configured `horizontal_scroll_mode = SCROLL_MODE_SHOW_NEVER` (scrolls, no scrollbar
drawn) and `vertical_scroll_mode = SCROLL_MODE_DISABLED`. `content` gets `SIZE_EXPAND_FILL` so it
fills the viewport when items fit and takes its minimum width when they do not.

What `ScrollContainer` provides without extra code:

- **Minimum width 0.** `_get_minimum_size()` only sets `min_size.x` for `SCROLL_MODE_DISABLED`
  and `SCROLL_MODE_MAXIMIZE_FIRST` (`scroll_container.cpp:176`). The toolbar shrinks and clips
  instead of forcing the parent wider.
- **Natural height.** Vertical `SCROLL_MODE_DISABLED` sets `min_size.y` from content.
- **Clipping.**
- **Wheel to horizontal scroll.** With the vertical bar hidden, `WHEEL_UP`/`WHEEL_DOWN` route to
  `h_scroll` (`scroll_container.cpp:205`).

`get_minimum_size()` returns the scroll container's minimum size: width 0, height from content.
Arrows never contribute, because they overlay.

## Layout

Children are placed in `NOTIFICATION_SORT_CHILDREN`:

| Child | Rect |
| --- | --- |
| `scroll` | full rect |
| `left_arrow` | `Rect2(0, 0, arrow_w, h)` |
| `right_arrow` | `Rect2(w - arrow_w, 0, arrow_w, h)` |

Children are added `scroll` first, then the arrows, so the arrows draw on top.

**Arrows overlay rather than take layout space.** Reserving space creates a feedback loop —
showing an arrow narrows the scroll viewport, which can remove the overflow, which hides the
arrow, which widens the viewport again. Near-threshold content widths make the arrows flicker on
window resize. With an overlay the scroll viewport is always the full toolbar width, so
"is there overflow" is a pure function of content width vs. toolbar width and nothing feeds back.

## Arrow behavior

- `left_arrow` visible iff `h_scroll > 0`; `right_arrow` visible iff `h_scroll < max`.
- An arrow therefore only covers content while there is more content to scroll to in that
  direction. Scrolling to either end hides that arrow and reveals the item under it, so no item
  is permanently unreachable and no padding is needed.
- `button_down` scrolls one step (`40 * EDSCALE`) immediately, then after ~0.4s begins continuous
  scrolling at `250 * EDSCALE` px/s driven by `delta` in `_process`. `button_up` stops it.
- Arrows use an opaque stylebox, not `FlatButton`, so items underneath do not show through.

## Drag behavior

Handled in `Node::input()`, which runs before GUI dispatch (`Viewport::push_input()`:
`order is _input -> gui input -> _unhandled input`).

It cannot be handled in `Control::gui_input()`. `Viewport::_gui_call_input()` walks the ancestor
chain but breaks at any control whose mouse filter is `MOUSE_FILTER_STOP`, independent of whether
that control called `accept_event()`. `Button` sets that filter in its constructor
(`button.cpp:881`), so every toolbar button — the arrows included — swallows the press, and
`gui_input()` would only ever see drags starting on the container's bare background.

1. Left press inside the toolbar's global rect: record the start position and arm the drag. The
   event is left alone, so the button under the cursor still shows as pressed.
2. Left-held motion: accumulate total path length, not net displacement from the press point, so a
   back-and-forth drag registers as a drag rather than cancelling itself out. Past `8 * EDSCALE`
   px, enter drag mode.
3. On entering drag mode, `propagate_notification(NOTIFICATION_SCROLL_BEGIN)`. Every descendant
   `BaseButton` clears `press_attempt` (`base_button.cpp:168`), so releasing fires no click. This
   is the same mechanism `ScrollContainer` uses for touch drag (`scroll_container.cpp:298`).
4. While dragging, apply the motion delta to `scroll->set_h_scroll()` 1:1 and immediately. No
   inertia, deceleration, or interpolation.
5. On release, `propagate_notification(NOTIFICATION_SCROLL_END)`.

The toolbar handles `NOTIFICATION_SCROLL_BEGIN` itself as well. `propagate_notification()` delivers
to the node before its children, and `BaseButton` consumes that notification without emitting
`button_up`, so an arrow held when a drag starts would otherwise keep repeating for the rest of the
gesture.

Events are never marked handled. The click is already cancelled by the notification, and swallowing
the release would leave the viewport's `gui.mouse_focus` on a button that never saw its mouse-up.

Releasing before the threshold leaves the button click intact.

## Application sites

| File | Target | Change |
| --- | --- | --- |
| `editor/run/game_view_plugin.cpp:1499` | `main_menu_fc` | `memnew(HBoxContainer)` -> `create()` |
| `editor/docks/inspector_dock.cpp:778` | `general_options_hb` | same |
| `editor/docks/inspector_dock.cpp:855` | `property_tools_hb` | same |
| `editor/scene/3d/node_3d_editor_plugin.cpp:3513` | `main_flow` | `HFlowContainer` -> `create()`; wrapping removed |
| `editor/scene/canvas_item_editor_plugin.cpp:5816` | `main_flow` | same |

Subsequent `add_child()` calls at each site are unchanged.

`general_options_hb` holds `object_selector` with `SIZE_EXPAND_FILL`, and that keeps working
unchanged. `content` is given `SIZE_EXPAND_FILL` inside the scroll container, and
`ScrollContainer::_reposition_children()` then sizes an expanding child to
`MAX(viewport_width, child_min_width)` (`scroll_container.cpp:183`). With room to spare the row
fills the viewport and `object_selector` expands as it does today; on overflow the row takes its
minimum width and scrolls. No call site needs its size flags changed.

## Non-goals

- Replacing `TabBar`'s own arrow drawing.
- An editor setting to switch between wrapping and scrolling; viewport toolbars always scroll.
- Smooth/inertial drag scrolling.
- Touch input support beyond what `ScrollContainer` already does.
- The 3D/2D contextual toolbar (`context_toolbar_hbox`), which sits inside `main_flow` and is
  covered transitively.

## Verification

Build the editor and check each toolbar with the window narrowed:

1. Arrows appear only on overflow, `<` on the left and `>` on the right.
2. Holding an arrow scrolls continuously; a short click nudges one step.
3. Left-dragging starting on top of a button scrolls the toolbar and fires no click.
4. A left press that moves less than the threshold still activates the button.
5. Mouse wheel over the toolbar scrolls it horizontally.
6. Scrolling to either end hides that arrow and fully reveals the end item.
7. 3D and 2D viewport toolbars stay on one row and no longer consume viewport height.
8. Resizing the window across the overflow threshold does not make arrows flicker.
