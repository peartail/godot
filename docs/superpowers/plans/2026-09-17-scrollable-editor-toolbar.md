# Scrollable Editor Toolbar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reusable `EditorScrollableToolbar` container so editor toolbars that overflow stay on one row and can be reached with overlay `<` `>` arrows or by dragging the toolbar sideways.

**Architecture:** A `Container` subclass holds a `ScrollContainer` (horizontal only) filling its whole rect, with two arrow `Button`s positioned on top of the left and right edges. `ScrollContainer` supplies clipping, zero minimum width, and wheel-to-horizontal scrolling. The class adds only arrow visibility/repeat and left-drag scrolling. Drag cancels the pending button click via `propagate_notification(NOTIFICATION_SCROLL_BEGIN)`, which `BaseButton` already handles.

**Tech Stack:** Godot Engine C++ (fork at `C:\GithubProjects\godot`), SCons via `scripts/build.ps1`, doctest unit tests under `tests/`.

**Spec:** `docs/superpowers/specs/2026-09-17-scrollable-editor-toolbar-design.md`

---

## Build and test commands

All tasks use the same two commands. Run them from the repository root in PowerShell.

Build (the `tests=yes` flag is required — `scripts/build.ps1` only passes `dev_build=yes`, and
`tests` defaults to off unless `dev_mode=yes`):

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

The first build with `tests=yes` compiles the whole test suite and is slow (`-Jobs 1` is the
default on Windows to avoid PDB conflicts). Later builds are incremental.

Run the new tests:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Launch the editor for manual checks:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.exe
```

## File structure

| File | Responsibility |
| --- | --- |
| `editor/gui/editor_scrollable_toolbar.h` (create) | Class declaration, tuning constants, public accessors |
| `editor/gui/editor_scrollable_toolbar.cpp` (create) | Layout, arrow state, hold-repeat, drag handling |
| `tests/editor/gui/test_editor_scrollable_toolbar.cpp` (create) | Headless coverage for layout, arrows, drag |
| `editor/run/game_view_plugin.cpp` (modify) | Game tab toolbar call site |
| `editor/docks/inspector_dock.cpp` (modify) | Two inspector toolbar call sites |
| `editor/scene/3d/node_3d_editor_plugin.cpp` (modify) | 3D viewport toolbar call site |
| `editor/scene/canvas_item_editor_plugin.cpp` (modify) | 2D viewport toolbar call site |

No SCsub changes are needed: `editor/gui/SCsub` globs `*.cpp`, and `tests/SCsub` globs
`*/**/*.cpp` recursively. The class is not registered in ClassDB (neither is `EditorToolbarGroup`),
so there is no `register_editor_types.cpp` change and no Mono glue regeneration.

## Known limitation

Right-to-left editor layouts are out of scope. `ScrollContainer` mirrors its content under RTL, so
the arrow-to-direction mapping would need its own handling and verification. The arrows are placed
at the physical left and right edges regardless of layout direction. Raise this as a follow-up if
RTL support is wanted.

---

### Task 1: Container skeleton, layout, and minimum size

**Files:**
- Create: `editor/gui/editor_scrollable_toolbar.h`
- Create: `editor/gui/editor_scrollable_toolbar.cpp`
- Test: `tests/editor/gui/test_editor_scrollable_toolbar.cpp`

- [ ] **Step 1: Write the failing test**

Create `tests/editor/gui/test_editor_scrollable_toolbar.cpp`. Start with the standard Godot
copyright header block — copy the 29-line comment from the top of `tests/scene/test_button.cpp`
and change the filename on its second line to `test_editor_scrollable_toolbar.cpp`. Then:

```cpp
#include "tests/test_macros.h"

TEST_FORCE_LINK(test_editor_scrollable_toolbar)

#ifdef TOOLS_ENABLED

#include "editor/gui/editor_scrollable_toolbar.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"

namespace TestEditorScrollableToolbar {

// Builds a toolbar sized 100x40 at the origin holding p_count buttons 80px wide each,
// with no separation so positions are exact.
static EditorScrollableToolbar *make_toolbar(int p_count) {
	EditorScrollableToolbar *toolbar = memnew(EditorScrollableToolbar);
	SceneTree::get_singleton()->get_root()->add_child(toolbar);

	HBoxContainer *content = toolbar->get_content();
	content->add_theme_constant_override("separation", 0);
	for (int i = 0; i < p_count; i++) {
		Button *button = memnew(Button);
		button->set_custom_minimum_size(Size2(80, 20));
		content->add_child(button);
	}

	toolbar->set_position(Point2(0, 0));
	toolbar->set_size(Size2(100, 40));
	MessageQueue::get_singleton()->flush();
	return toolbar;
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] minimum width is zero") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);

	// The toolbar must never force its parent wider; it clips and scrolls instead.
	CHECK(toolbar->get_combined_minimum_size().width == 0);
	// Height still reflects the row, so the toolbar keeps its natural height.
	CHECK(toolbar->get_combined_minimum_size().height > 0);

	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] content fills the viewport when it fits") {
	EditorScrollableToolbar *toolbar = make_toolbar(1);

	// One 80px button fits in 100px, so the row stretches to the full width.
	CHECK(toolbar->get_content()->get_size().width == 100);

	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] content keeps its width when it overflows") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);

	// Three 80px buttons need 240px, more than the 100px viewport.
	CHECK(toolbar->get_content()->get_size().width == 240);

	memdelete(toolbar);
}

} // namespace TestEditorScrollableToolbar

#endif // TOOLS_ENABLED
```

- [ ] **Step 2: Run the build to verify it fails**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Expected: compile error, `editor/gui/editor_scrollable_toolbar.h: No such file or directory`.

- [ ] **Step 3: Write the header**

Create `editor/gui/editor_scrollable_toolbar.h`. Start with the standard Godot copyright header
block — copy the 29-line comment from the top of `editor/gui/editor_toolbar_group.h` and change
the filename on its second line to `editor_scrollable_toolbar.h`. Then:

```cpp
#pragma once

#include "scene/gui/container.h"

class Button;
class HBoxContainer;
class ScrollContainer;

class EditorScrollableToolbar : public Container {
	GDCLASS(EditorScrollableToolbar, Container);

public:
	// All unscaled; multiplied by EDSCALE where used.
	static constexpr float ARROW_WIDTH = 16.0f;
	static constexpr float CLICK_STEP = 40.0f;
	static constexpr float HOLD_SPEED = 250.0f;
	static constexpr float HOLD_DELAY = 0.4f;
	static constexpr float DRAG_THRESHOLD = 8.0f;

private:
	ScrollContainer *scroll = nullptr;
	HBoxContainer *content = nullptr;
	Button *left_arrow = nullptr;
	Button *right_arrow = nullptr;

	// ScrollContainer offsets are integers; this carries the sub-pixel part
	// so a slow continuous scroll does not lose speed to truncation.
	float scroll_remainder = 0.0f;

	void _scroll_by(float p_amount);
	void _update_arrows();
	void _scroll_value_changed(double p_value);

protected:
	void _notification(int p_what);

public:
	virtual Size2 get_minimum_size() const override;

	HBoxContainer *get_content() const { return content; }

	double get_scroll_offset() const;
	void set_scroll_offset(double p_offset);
	bool is_left_arrow_visible() const;
	bool is_right_arrow_visible() const;

	// Adds a toolbar to p_parent and returns the box that toolbar items go into,
	// matching the EditorToolbarGroup::create() idiom.
	static HBoxContainer *create(Control *p_parent);

	EditorScrollableToolbar();
};
```

- [ ] **Step 4: Write the implementation**

Create `editor/gui/editor_scrollable_toolbar.cpp`. Start with the standard Godot copyright header
block — copy the 29-line comment from the top of `editor/gui/editor_toolbar_group.cpp` and change
the filename on its second line to `editor_scrollable_toolbar.cpp`. Then:

```cpp
#include "editor_scrollable_toolbar.h"

#include "core/core_string_names.h"
#include "core/object/callable_mp.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/scroll_container.h"
#include "scene/scene_string_names.h"

void EditorScrollableToolbar::_scroll_by(float p_amount) {
	// set_h_scroll() takes an int, so carry the fraction across calls. Without this a
	// 250 px/s hold scroll loses its sub-pixel part every frame and runs slow.
	scroll_remainder += p_amount;
	const int whole = (int)scroll_remainder;
	if (whole == 0) {
		return;
	}
	scroll_remainder -= (float)whole;
	scroll->set_h_scroll(scroll->get_h_scroll() + whole);
}

void EditorScrollableToolbar::_update_arrows() {
	// Filled in by Task 2.
}

void EditorScrollableToolbar::_scroll_value_changed(double p_value) {
	_update_arrows();
}

Size2 EditorScrollableToolbar::get_minimum_size() const {
	// Width comes back as 0 because the scroll container scrolls horizontally;
	// height is the natural height of the toolbar row.
	return scroll->get_combined_minimum_size();
}

double EditorScrollableToolbar::get_scroll_offset() const {
	return scroll->get_h_scroll();
}

void EditorScrollableToolbar::set_scroll_offset(double p_offset) {
	scroll->set_h_scroll(p_offset);
}

bool EditorScrollableToolbar::is_left_arrow_visible() const {
	return left_arrow->is_visible();
}

bool EditorScrollableToolbar::is_right_arrow_visible() const {
	return right_arrow->is_visible();
}

void EditorScrollableToolbar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_SORT_CHILDREN: {
			const Size2 size = get_size();
			// Never narrower than the buttons themselves, or the right arrow would be
			// laid out wider than its slot and spill past the toolbar's right edge.
			const float button_width = MAX(left_arrow->get_combined_minimum_size().width,
					right_arrow->get_combined_minimum_size().width);
			const float arrow_width = MAX(Math::round(ARROW_WIDTH * EDSCALE), button_width);

			fit_child_in_rect(scroll, Rect2(Point2(), size));
			fit_child_in_rect(left_arrow, Rect2(0, 0, arrow_width, size.height));
			fit_child_in_rect(right_arrow, Rect2(size.width - arrow_width, 0, arrow_width, size.height));

			_update_arrows();
		} break;
	}
}

HBoxContainer *EditorScrollableToolbar::create(Control *p_parent) {
	EditorScrollableToolbar *toolbar = memnew(EditorScrollableToolbar);
	p_parent->add_child(toolbar);
	return toolbar->get_content();
}

EditorScrollableToolbar::EditorScrollableToolbar() {
	set_h_size_flags(SIZE_EXPAND_FILL);
	set_clip_contents(true);

	scroll = memnew(ScrollContainer);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_NEVER);
	scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	// All three are internal children, added in draw order: the arrows are added
	// after the scroll container so they paint on top of the toolbar contents.
	add_child(scroll, false, INTERNAL_MODE_BACK);

	content = memnew(HBoxContainer);
	// Lets the row stretch to the viewport when items fit, and fall back to its
	// own minimum width (and scroll) when they do not.
	content->set_h_size_flags(SIZE_EXPAND_FILL);
	scroll->add_child(content);

	left_arrow = memnew(Button);
	left_arrow->set_focus_mode(FOCUS_NONE);
	left_arrow->set_visible(false);
	add_child(left_arrow, false, INTERNAL_MODE_BACK);

	right_arrow = memnew(Button);
	right_arrow->set_focus_mode(FOCUS_NONE);
	right_arrow->set_visible(false);
	add_child(right_arrow, false, INTERNAL_MODE_BACK);

	scroll->get_h_scroll_bar()->connect(SceneStringName(value_changed), callable_mp(this, &EditorScrollableToolbar::_scroll_value_changed));
	scroll->get_h_scroll_bar()->connect(CoreStringName(changed), callable_mp(this, &EditorScrollableToolbar::_update_arrows));
}
```

Use the `StringName` constants, not raw strings: `ScrollContainer` connects to this same
`HScrollBar` signal with `SceneStringName(value_changed)` at `scroll_container.cpp:1085`, and
`CoreStringName(changed)` is the established idiom for the `Range` "changed" signal.

- [ ] **Step 5: Run the tests to verify they pass**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: `test cases: 3 | 3 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add editor/gui/editor_scrollable_toolbar.h editor/gui/editor_scrollable_toolbar.cpp tests/editor/gui/test_editor_scrollable_toolbar.cpp
git commit -m "Add EditorScrollableToolbar container skeleton"
```

---

### Task 2: Overlay arrows and visibility rules

**Files:**
- Modify: `editor/gui/editor_scrollable_toolbar.cpp` (`_update_arrows`, `_notification`)
- Test: `tests/editor/gui/test_editor_scrollable_toolbar.cpp`

- [ ] **Step 1: Write the failing tests**

Append these test cases inside the `TestEditorScrollableToolbar` namespace, before its closing
brace:

```cpp
TEST_CASE("[SceneTree][EditorScrollableToolbar] no arrows without overflow") {
	EditorScrollableToolbar *toolbar = make_toolbar(1);

	CHECK_FALSE(toolbar->is_left_arrow_visible());
	CHECK_FALSE(toolbar->is_right_arrow_visible());

	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] arrows follow the scroll position") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);

	// Content is 240px in a 100px viewport, so the maximum offset is 140.
	SUBCASE("at the left end only the right arrow shows") {
		CHECK(toolbar->get_scroll_offset() == 0);
		CHECK_FALSE(toolbar->is_left_arrow_visible());
		CHECK(toolbar->is_right_arrow_visible());
	}

	SUBCASE("in the middle both arrows show") {
		toolbar->set_scroll_offset(70);
		MessageQueue::get_singleton()->flush();

		CHECK(toolbar->is_left_arrow_visible());
		CHECK(toolbar->is_right_arrow_visible());
	}

	SUBCASE("at the right end only the left arrow shows") {
		toolbar->set_scroll_offset(1000); // Clamped to the maximum.
		MessageQueue::get_singleton()->flush();

		CHECK(toolbar->get_scroll_offset() == 140);
		CHECK(toolbar->is_left_arrow_visible());
		CHECK_FALSE(toolbar->is_right_arrow_visible());
	}

	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] arrows do not shrink the scroll viewport") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);

	// Arrows overlay the content, so showing one must not narrow the scroll area.
	// If it did, the overflow test would flip and the arrows would flicker.
	toolbar->set_scroll_offset(1000);
	MessageQueue::get_singleton()->flush();

	CHECK(toolbar->get_scroll_offset() == 140); // 240 content - 100 viewport.

	memdelete(toolbar);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: the two arrow cases fail. `_update_arrows()` is still empty, so both arrows stay hidden.

- [ ] **Step 3: Implement arrow visibility**

Replace the empty `_update_arrows()` body in `editor/gui/editor_scrollable_toolbar.cpp` with:

```cpp
void EditorScrollableToolbar::_update_arrows() {
	HScrollBar *bar = scroll->get_h_scroll_bar();
	const double max_offset = MAX(0.0, bar->get_max() - bar->get_page());
	const double offset = scroll->get_h_scroll();

	// Half a pixel of slack so a rounding remainder does not keep an arrow alive.
	left_arrow->set_visible(offset > 0.5);
	right_arrow->set_visible(offset < max_offset - 0.5);
}
```

- [ ] **Step 4: Add the arrow icons**

Add a `NOTIFICATION_THEME_CHANGED` case to `_notification()`, before the
`NOTIFICATION_SORT_CHILDREN` case:

```cpp
		case NOTIFICATION_THEME_CHANGED: {
			// Guarded so headless tests without an editor theme stay quiet.
			if (has_theme_icon(SNAME("GuiScrollArrowLeft"), SNAME("EditorIcons"))) {
				left_arrow->set_button_icon(get_editor_theme_icon(SNAME("GuiScrollArrowLeft")));
				right_arrow->set_button_icon(get_editor_theme_icon(SNAME("GuiScrollArrowRight")));
			}
			const float arrow_width = Math::round(ARROW_WIDTH * EDSCALE);
			left_arrow->set_custom_minimum_size(Size2(arrow_width, 0));
			right_arrow->set_custom_minimum_size(Size2(arrow_width, 0));
		} break;
```

The arrows deliberately keep the default (non-flat) `Button` style: they sit on top of the
content, so they need an opaque background.

- [ ] **Step 5: Run the tests to verify they pass**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: `test cases: 6 | 6 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add editor/gui/editor_scrollable_toolbar.cpp tests/editor/gui/test_editor_scrollable_toolbar.cpp
git commit -m "Show overlay scroll arrows on toolbar overflow"
```

---

### Task 3: Arrow click step and hold-to-scroll

**Files:**
- Modify: `editor/gui/editor_scrollable_toolbar.h`
- Modify: `editor/gui/editor_scrollable_toolbar.cpp`
- Test: `tests/editor/gui/test_editor_scrollable_toolbar.cpp`

- [ ] **Step 1: Write the failing test**

Append this test case inside the `TestEditorScrollableToolbar` namespace, before its closing brace:

```cpp
TEST_CASE("[SceneTree][EditorScrollableToolbar] pressing an arrow steps and starts repeating") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);

	// The right arrow is anchored to the right edge; 3px in from it is inside the
	// arrow whatever width the theme gives the button.
	const Point2i arrow_pos = Point2i(97, 20);

	SEND_GUI_MOUSE_BUTTON_EVENT(arrow_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

	// One press scrolls a fixed step immediately and arms the repeat.
	CHECK(toolbar->get_scroll_offset() == 40);
	CHECK(toolbar->is_processing_internal());

	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(arrow_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	CHECK_FALSE(toolbar->is_processing_internal());
	CHECK(toolbar->get_scroll_offset() == 40);

	// The offset is non-zero now, so the left arrow is showing. Pressing it walks
	// the offset back, which also exercises the left-edge layout rect.
	const Point2i left_arrow_pos = Point2i(2, 20);

	SEND_GUI_MOUSE_BUTTON_EVENT(left_arrow_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

	CHECK(toolbar->get_scroll_offset() == 0);

	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(left_arrow_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	memdelete(toolbar);
}
```

Hitting both arrows by screen position is what covers the rects computed in
`NOTIFICATION_SORT_CHILDREN`. Nothing in Tasks 1-2 exercises that geometry, so a sign error in
`size.width - arrow_width` would otherwise go unnoticed until someone ran the editor.

- [ ] **Step 2: Run the test to verify it fails**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: fails at `toolbar->get_scroll_offset() == 40` with the offset still 0, because the
arrows are not wired to anything.

- [ ] **Step 3: Declare the hold state**

In `editor/gui/editor_scrollable_toolbar.h`, add to the private section immediately after
`float scroll_remainder = 0.0f;`:

```cpp
	int hold_dir = 0;
	float hold_time = 0.0f;
```

and immediately after `void _scroll_value_changed(double p_value);`:

```cpp
	void _arrow_down(int p_dir);
	void _arrow_up();
```

- [ ] **Step 4: Implement the handlers**

In `editor/gui/editor_scrollable_toolbar.cpp`, add these two functions after
`_scroll_value_changed()`:

```cpp
void EditorScrollableToolbar::_arrow_down(int p_dir) {
	hold_dir = p_dir;
	hold_time = 0.0f;
	scroll_remainder = 0.0f;
	// A quick click nudges one step; holding starts a slow continuous scroll
	// once HOLD_DELAY has elapsed.
	_scroll_by(p_dir * CLICK_STEP * EDSCALE);
	set_process_internal(true);
}

void EditorScrollableToolbar::_arrow_up() {
	hold_dir = 0;
	set_process_internal(false);
}
```

Add a `NOTIFICATION_INTERNAL_PROCESS` case to `_notification()`, after the
`NOTIFICATION_SORT_CHILDREN` case:

```cpp
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (hold_dir == 0) {
				break;
			}
			const float delta = get_process_delta_time();
			hold_time += delta;
			if (hold_time < HOLD_DELAY) {
				break;
			}
			_scroll_by(hold_dir * HOLD_SPEED * EDSCALE * delta);
		} break;
```

In the constructor, add after `left_arrow->set_visible(false);`:

```cpp
	left_arrow->connect("button_down", callable_mp(this, &EditorScrollableToolbar::_arrow_down).bind(-1));
	left_arrow->connect("button_up", callable_mp(this, &EditorScrollableToolbar::_arrow_up));
```

and after `right_arrow->set_visible(false);`:

```cpp
	right_arrow->connect("button_down", callable_mp(this, &EditorScrollableToolbar::_arrow_down).bind(1));
	right_arrow->connect("button_up", callable_mp(this, &EditorScrollableToolbar::_arrow_up));
```

- [ ] **Step 5: Run the tests to verify they pass**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: `test cases: 7 | 7 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add editor/gui/editor_scrollable_toolbar.h editor/gui/editor_scrollable_toolbar.cpp tests/editor/gui/test_editor_scrollable_toolbar.cpp
git commit -m "Scroll toolbar by step on arrow click and repeat while held"
```

---

### Task 4: Left-drag scrolling that cancels the click

**Files:**
- Modify: `editor/gui/editor_scrollable_toolbar.h`
- Modify: `editor/gui/editor_scrollable_toolbar.cpp`
- Test: `tests/editor/gui/test_editor_scrollable_toolbar.cpp`

- [ ] **Step 1: Write the failing tests**

Append these test cases inside the `TestEditorScrollableToolbar` namespace, before its closing
brace:

```cpp
TEST_CASE("[SceneTree][EditorScrollableToolbar] dragging past the threshold scrolls and cancels the click") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);
	Button *first = Object::cast_to<Button>(toolbar->get_content()->get_child(0));
	SIGNAL_WATCH(first, "pressed");

	// Press on the first button, then drag 20px left: past the 8px threshold.
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(50, 20), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(30, 20), MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(30, 20), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	// Dragging left moves the content left, which raises the scroll offset.
	CHECK(toolbar->get_scroll_offset() == 20);
	// NOTIFICATION_SCROLL_BEGIN cleared the button's pending press.
	SIGNAL_CHECK_FALSE("pressed");

	SIGNAL_UNWATCH(first, "pressed");
	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] a small move still clicks the button") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);
	Button *first = Object::cast_to<Button>(toolbar->get_content()->get_child(0));
	SIGNAL_WATCH(first, "pressed");

	// Move only 3px, below the 8px threshold.
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(50, 20), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(47, 20), MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(47, 20), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	CHECK(toolbar->get_scroll_offset() == 0);
	Array pressed_once = { {} };
	SIGNAL_CHECK("pressed", pressed_once);

	SIGNAL_UNWATCH(first, "pressed");
	memdelete(toolbar);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: the first new case fails — the offset stays 0 and `pressed` does fire, because nothing
handles drags yet.

- [ ] **Step 3: Declare the drag state**

In `editor/gui/editor_scrollable_toolbar.h`, add to the private section immediately after
`float hold_time = 0.0f;`:

```cpp
	bool drag_pending = false;
	bool dragging = false;
	float drag_accum = 0.0f;
	float drag_last_x = 0.0f;
```

and to the public section, immediately above `virtual Size2 get_minimum_size() const override;`:

```cpp
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
```

- [ ] **Step 4: Implement drag handling**

In `editor/gui/editor_scrollable_toolbar.cpp`, add this function after `_arrow_up()`:

```cpp
void EditorScrollableToolbar::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	// BaseButton::gui_input() never calls accept_event(), so presses and motion over
	// the toolbar's own buttons bubble up to here.
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			drag_pending = true;
			dragging = false;
			drag_accum = 0.0f;
			drag_last_x = mb->get_position().x;
			scroll_remainder = 0.0f;
		} else {
			if (dragging) {
				propagate_notification(NOTIFICATION_SCROLL_END);
			}
			drag_pending = false;
			dragging = false;
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_null() || !drag_pending) {
		return;
	}
	if (!mm->get_button_mask().has_flag(MouseButtonMask::LEFT)) {
		drag_pending = false;
		return;
	}

	// Position deltas, not get_relative(): the relative field is not populated by
	// synthesized events, and the toolbar itself never moves while dragging.
	const float x = mm->get_position().x;
	const float dx = x - drag_last_x;
	drag_last_x = x;

	if (!dragging) {
		drag_accum += Math::abs(dx);
		if (drag_accum < DRAG_THRESHOLD * EDSCALE) {
			return;
		}
		dragging = true;
		// Every descendant BaseButton drops its pending press, so releasing fires no click.
		propagate_notification(NOTIFICATION_SCROLL_BEGIN);
	}

	_scroll_by(-dx);
	accept_event();
}
```

- [ ] **Step 5: Run the tests to verify they pass**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test --test-case="*EditorScrollableToolbar*"
```

Expected: `test cases: 9 | 9 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add editor/gui/editor_scrollable_toolbar.h editor/gui/editor_scrollable_toolbar.cpp tests/editor/gui/test_editor_scrollable_toolbar.cpp
git commit -m "Scroll toolbar by left-dragging without firing button clicks"
```

---

### Task 5: Apply to the Game tab toolbar

**Files:**
- Modify: `editor/run/game_view_plugin.cpp:1499`

- [ ] **Step 1: Replace the container**

In `editor/run/game_view_plugin.cpp`, replace these two lines:

```cpp
	HBoxContainer *main_menu_fc = memnew(HBoxContainer);
	toolbar_margin->add_child(main_menu_fc);
```

with:

```cpp
	HBoxContainer *main_menu_fc = EditorScrollableToolbar::create(toolbar_margin);
```

Every later `main_menu_fc->add_child(...)` call stays exactly as it is.

- [ ] **Step 2: Add the include**

In the include block of `editor/run/game_view_plugin.cpp`, add this line in alphabetical order
among the other `editor/gui/...` includes:

```cpp
#include "editor/gui/editor_scrollable_toolbar.h"
```

- [ ] **Step 3: Build**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Expected: build succeeds.

- [ ] **Step 4: Verify in the editor**

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.exe
```

Open a project, run it, and switch to the Game tab. Narrow the editor window until the toolbar
overflows. Confirm the `>` arrow appears at the right edge, holding it scrolls slowly, and
dragging the toolbar sideways starting on top of a button scrolls without triggering that button.

- [ ] **Step 5: Commit**

```bash
git add editor/run/game_view_plugin.cpp
git commit -m "Make the Game tab toolbar scrollable"
```

---

### Task 6: Apply to the inspector dock toolbars

**Files:**
- Modify: `editor/docks/inspector_dock.cpp:778` and `editor/docks/inspector_dock.cpp:855`

- [ ] **Step 1: Replace the first container**

In `editor/docks/inspector_dock.cpp`, replace:

```cpp
	HBoxContainer *general_options_hb = memnew(HBoxContainer);
	main_vb->add_child(general_options_hb);
```

with:

```cpp
	HBoxContainer *general_options_hb = EditorScrollableToolbar::create(main_vb);
```

- [ ] **Step 2: Replace the second container**

Replace:

```cpp
	HBoxContainer *property_tools_hb = memnew(HBoxContainer);
	main_vb->add_child(property_tools_hb);
```

with:

```cpp
	HBoxContainer *property_tools_hb = EditorScrollableToolbar::create(main_vb);
```

`object_selector` keeps its `SIZE_EXPAND_FILL` flag unchanged:
`ScrollContainer::_reposition_children()` sizes an expanding child to
`MAX(viewport_width, child_min_width)`, so the row still fills the dock when there is room.

- [ ] **Step 3: Add the include**

In the include block of `editor/docks/inspector_dock.cpp`, add this line in alphabetical order
among the other `editor/gui/...` includes:

```cpp
#include "editor/gui/editor_scrollable_toolbar.h"
```

- [ ] **Step 4: Build**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Expected: build succeeds.

- [ ] **Step 5: Verify in the editor**

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.exe
```

Select a node so the inspector fills, then drag the inspector dock's inner edge to narrow it.
Confirm both toolbar rows gain arrows instead of clipping, that the object selector still
stretches when the dock is wide, and that dragging either row scrolls it.

- [ ] **Step 6: Commit**

```bash
git add editor/docks/inspector_dock.cpp
git commit -m "Make the inspector dock toolbars scrollable"
```

---

### Task 7: Apply to the 3D and 2D viewport toolbars

**Files:**
- Modify: `editor/scene/3d/node_3d_editor_plugin.cpp:3513`
- Modify: `editor/scene/canvas_item_editor_plugin.cpp:5816`

- [ ] **Step 1: Replace the 3D container**

In `editor/scene/3d/node_3d_editor_plugin.cpp`, replace:

```cpp
	// A fluid container for all toolbars.
	HFlowContainer *main_flow = memnew(HFlowContainer);
	toolbar_margin->add_child(main_flow);
```

with:

```cpp
	// A single scrolling row for all toolbars; overflow is reached with the arrows or by dragging.
	HBoxContainer *main_flow = EditorScrollableToolbar::create(toolbar_margin);
```

The comment a few lines below now describes behavior that no longer exists. Replace:

```cpp
	// Split into separate `HBoxContainer` so they can wrap onto multiple lines as the window width decreases (the parent is a `FlowContainer`).
```

with:

```cpp
	// Split into separate `HBoxContainer` groups so separators stay with the buttons they delimit.
```

- [ ] **Step 2: Replace the 2D container**

In `editor/scene/canvas_item_editor_plugin.cpp`, replace:

```cpp
	HFlowContainer *main_flow = memnew(HFlowContainer);
	toolbar_margin->add_child(main_flow);
```

with:

```cpp
	HBoxContainer *main_flow = EditorScrollableToolbar::create(toolbar_margin);
```

- [ ] **Step 3: Add the includes and drop the unused one**

Add to the include block of both files, in alphabetical order among the other `editor/...`
includes:

```cpp
#include "editor/gui/editor_scrollable_toolbar.h"
```

Then check whether `HFlowContainer` is still referenced in either file:

```bash
grep -n "HFlowContainer" editor/scene/3d/node_3d_editor_plugin.cpp editor/scene/canvas_item_editor_plugin.cpp
```

If a file has no remaining references, remove its now-unused `#include "scene/gui/flow_container.h"`.

- [ ] **Step 4: Build**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Expected: build succeeds.

- [ ] **Step 5: Verify in the editor**

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.exe
```

Open a 3D scene and narrow the window. Confirm the toolbar stays on one row instead of wrapping,
that the viewport keeps its full height, and that arrows and dragging both work. Select a node
that adds a contextual toolbar (for example a `Path3D`) and confirm the contextual group scrolls
along with the rest. Repeat in a 2D scene.

- [ ] **Step 6: Commit**

```bash
git add editor/scene/3d/node_3d_editor_plugin.cpp editor/scene/canvas_item_editor_plugin.cpp
git commit -m "Make the 3D and 2D viewport toolbars scroll instead of wrap"
```

---

### Task 8: Full verification pass

**Files:** none, unless a check fails.

- [ ] **Step 1: Run the whole test suite**

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test
```

Expected: no failures. Existing tests must not regress.

- [ ] **Step 2: Walk the spec's verification list**

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.exe
```

Confirm each item in every one of the four toolbars (Game tab, both inspector rows, 3D viewport,
2D viewport):

1. Arrows appear only on overflow, `<` on the left and `>` on the right.
2. Holding an arrow scrolls continuously; a short click nudges one step.
3. Left-dragging starting on top of a button scrolls the toolbar and fires no click.
4. A left press that moves less than the threshold still activates the button.
5. Mouse wheel over the toolbar scrolls it horizontally.
6. Scrolling to either end hides that arrow and fully reveals the end item.
7. The 3D and 2D viewport toolbars stay on one row and no longer consume viewport height.
8. Resizing the window back and forth across the overflow threshold does not make arrows flicker.

- [ ] **Step 3: Commit any fixes**

If a check fails, fix it, re-run both the unit tests and the failing manual check, then commit
with a message naming the specific behavior that was wrong.
