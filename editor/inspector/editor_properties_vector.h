/**************************************************************************/
/*  editor_properties_vector.h                                            */
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

#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_properties.h"

class EditorSpinSlider;
class TextureButton;

class CircleDegreeRangeSlider : public Control {
	GDCLASS(CircleDegreeRangeSlider, Control);

public:
	enum Handle {
		HANDLE_NONE,
		HANDLE_A,
		HANDLE_B,
	};

private:
	Vector2 value;
	double min_value = 0.0;
	double max_value = 360.0;
	double step = 1.0;
	bool read_only = false;
	Handle hovered_handle = HANDLE_NONE;
	Handle dragging_handle = HANDLE_NONE;

	Color track_color;
	Color range_color;
	Color handle_a_color;
	Color handle_b_color;
	Color handle_outline_color;

	Callable value_changed_callable;

	Vector2 _get_center() const;
	double _get_radius() const;
	double _get_sweep() const;
	double _snap_value(double p_value) const;
	double _normalize_angle(double p_value) const;
	Vector2 _angle_to_point(double p_angle) const;
	double _point_to_angle(const Vector2 &p_point) const;
	Handle _get_handle_at_position(const Vector2 &p_position) const;
	void _set_handle_value(Handle p_handle, double p_angle);
	void _update_theme();

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	virtual Size2 get_minimum_size() const override;

	void set_value(const Vector2 &p_value);
	Vector2 get_value() const;
	void set_value_no_signal(const Vector2 &p_value);
	void set_value_changed_callable(const Callable &p_callable);

	void setup(double p_min, double p_max, double p_step);
	void set_read_only(bool p_read_only);
	bool is_read_only() const;

	CircleDegreeRangeSlider();
};

class EditorPropertyCircleDegreeRange : public EditorProperty {
	GDCLASS(EditorPropertyCircleDegreeRange, EditorProperty);

	CircleDegreeRangeSlider *slider = nullptr;
	EditorSpinSlider *a_spin = nullptr;
	EditorSpinSlider *b_spin = nullptr;
	bool updating = false;

	void _slider_value_changed(const Vector2 &p_value);
	void _spin_value_changed(double p_value, bool p_is_a);

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	virtual void update_property() override;
	void setup(const EditorPropertyRangeHint &p_range_hint);
	EditorPropertyCircleDegreeRange();
};

class EditorPropertyVectorN : public EditorProperty {
	GDCLASS(EditorPropertyVectorN, EditorProperty);

	static const String COMPONENT_LABELS[4];

	int component_count = 0;
	Variant::Type vector_type;

	Vector<EditorSpinSlider *> spin_sliders;
	TextureButton *linked = nullptr;
	Vector<double> ratio;

	bool radians_as_degrees = false;

	void _update_ratio();
	void _store_link(bool p_linked);
	void _value_changed(double p_val, const String &p_name);

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	virtual void set_deferred_drag_mode_enabled(bool p_enabled = true) override;
	virtual void update_property() override;
	void setup(const EditorPropertyRangeHint &p_range_hint, bool p_link = false, bool p_is_int = false);
	EditorPropertyVectorN(Variant::Type p_type, bool p_force_wide, bool p_horizontal);
};

class EditorPropertyVector2 : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector2, EditorPropertyVectorN);

public:
	EditorPropertyVector2(bool p_force_wide = false);
};

class EditorPropertyVector2i : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector2i, EditorPropertyVectorN);

public:
	EditorPropertyVector2i(bool p_force_wide = false);
};

class EditorPropertyVector3 : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector3, EditorPropertyVectorN);

public:
	EditorPropertyVector3(bool p_force_wide = false);
};

class EditorPropertyVector3i : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector3i, EditorPropertyVectorN);

public:
	EditorPropertyVector3i(bool p_force_wide = false);
};

class EditorPropertyVector4 : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector4, EditorPropertyVectorN);

public:
	EditorPropertyVector4(bool p_force_wide = false);
};

class EditorPropertyVector4i : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector4i, EditorPropertyVectorN);

public:
	EditorPropertyVector4i(bool p_force_wide = false);
};
