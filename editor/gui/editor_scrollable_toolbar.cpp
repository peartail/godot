/**************************************************************************/
/*  editor_scrollable_toolbar.cpp                                         */
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

#include "editor_scrollable_toolbar.h"

#include "core/core_string_names.h"
#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
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
	HScrollBar *bar = scroll->get_h_scroll_bar();
	const double max_offset = MAX(0.0, bar->get_max() - bar->get_page());
	const double offset = scroll->get_h_scroll();

	// Half a pixel of slack so a rounding remainder does not keep an arrow alive.
	left_arrow->set_visible(offset > 0.5);
	right_arrow->set_visible(offset < max_offset - 0.5);
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
		case NOTIFICATION_THEME_CHANGED: {
			// Guarded so headless tests without an editor theme stay quiet.
			if (has_theme_icon(SNAME("GuiScrollArrowLeft"), EditorStringName(EditorIcons))) {
				left_arrow->set_button_icon(get_editor_theme_icon(SNAME("GuiScrollArrowLeft")));
				right_arrow->set_button_icon(get_editor_theme_icon(SNAME("GuiScrollArrowRight")));
			}
		} break;

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
	left_arrow->set_tooltip_text(TTRC("Scroll the toolbar left."));
	left_arrow->set_accessibility_name(TTRC("Scroll Left"));
	left_arrow->set_visible(false);
	add_child(left_arrow, false, INTERNAL_MODE_BACK);

	right_arrow = memnew(Button);
	right_arrow->set_focus_mode(FOCUS_NONE);
	right_arrow->set_tooltip_text(TTRC("Scroll the toolbar right."));
	right_arrow->set_accessibility_name(TTRC("Scroll Right"));
	right_arrow->set_visible(false);
	add_child(right_arrow, false, INTERNAL_MODE_BACK);

	scroll->get_h_scroll_bar()->connect(SceneStringName(value_changed), callable_mp(this, &EditorScrollableToolbar::_scroll_value_changed));
	scroll->get_h_scroll_bar()->connect(CoreStringName(changed), callable_mp(this, &EditorScrollableToolbar::_update_arrows));
}
