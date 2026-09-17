/**************************************************************************/
/*  test_editor_scrollable_toolbar.cpp                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

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

TEST_CASE("[SceneTree][EditorScrollableToolbar] a hold stops when its arrow reaches the end") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);
	toolbar->set_scroll_offset(120); // 20px short of the 140 maximum.
	MessageQueue::get_singleton()->flush();

	const Point2i arrow_pos = Point2i(97, 20);
	SEND_GUI_MOUSE_BUTTON_EVENT(arrow_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

	// The step runs into the end, so the arrow hides. BaseButton clears the pending
	// press on hide without emitting button_up, so the repeat has to stop here or it
	// would run forever with a stale direction.
	CHECK(toolbar->get_scroll_offset() == 140);
	CHECK_FALSE(toolbar->is_right_arrow_visible());
	CHECK_FALSE(toolbar->is_processing_internal());

	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(arrow_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] dragging past the threshold scrolls and cancels the click") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);
	Button *first = Object::cast_to<Button>(toolbar->get_content()->get_child(0));
	SIGNAL_WATCH(first, "pressed");

	// Aim at the button's real centre. The content row is only as tall as the buttons
	// (20px), not as tall as the toolbar (40px), so a hardcoded y can miss the button
	// and make the "no click" assertion below pass for the wrong reason.
	const Point2i press_pos = Point2i(first->get_global_rect().get_center());
	const Point2i drag_pos = press_pos - Point2i(20, 0);

	// Press on the first button, then drag 20px left: past the 8px threshold.
	SEND_GUI_MOUSE_BUTTON_EVENT(press_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	// Guard: without this the test would still pass if the press missed entirely.
	CHECK(first->is_pressing());
	SEND_GUI_MOUSE_MOTION_EVENT(drag_pos, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(drag_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

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

	const Point2i press_pos = Point2i(first->get_global_rect().get_center());
	const Point2i nudge_pos = press_pos - Point2i(3, 0);

	// Move only 3px, below the 8px threshold.
	SEND_GUI_MOUSE_BUTTON_EVENT(press_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	CHECK(first->is_pressing());
	SEND_GUI_MOUSE_MOTION_EVENT(nudge_pos, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(nudge_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	CHECK(toolbar->get_scroll_offset() == 0);
	Array pressed_once = { {} };
	SIGNAL_CHECK("pressed", pressed_once);

	SIGNAL_UNWATCH(first, "pressed");
	memdelete(toolbar);
}

TEST_CASE("[SceneTree][EditorScrollableToolbar] starting a drag stops an arrow hold") {
	EditorScrollableToolbar *toolbar = make_toolbar(3);

	const Point2i arrow_pos = Point2i(97, 20);
	SEND_GUI_MOUSE_BUTTON_EVENT(arrow_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

	CHECK(toolbar->get_scroll_offset() == 40);
	CHECK(toolbar->is_processing_internal());

	// Drag away from the arrow, past the threshold. BaseButton cancels its pending
	// press on NOTIFICATION_SCROLL_BEGIN without emitting button_up, so only the
	// toolbar's own handler for that notification can stop the repeat.
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(77, 20), MouseButtonMask::LEFT, Key::NONE);

	CHECK_FALSE(toolbar->is_processing_internal());

	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(77, 20), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

	memdelete(toolbar);
}

} // namespace TestEditorScrollableToolbar

#endif // TOOLS_ENABLED
