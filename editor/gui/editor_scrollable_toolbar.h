/**************************************************************************/
/*  editor_scrollable_toolbar.h                                           */
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

	int hold_dir = 0;
	float hold_time = 0.0f;

	bool drag_pending = false;
	bool dragging = false;
	float drag_accum = 0.0f;
	float drag_last_x = 0.0f;

	void _scroll_by(float p_amount);
	void _update_arrows();
	void _scroll_value_changed(double p_value);
	void _arrow_down(int p_dir);
	void _arrow_up();

protected:
	void _notification(int p_what);

public:
	virtual void input(const Ref<InputEvent> &p_event) override;
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
