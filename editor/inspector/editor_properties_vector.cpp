/**************************************************************************/
/*  editor_properties_vector.cpp                                          */
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

#include "editor_properties_vector.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/texture_button.h"

Vector2 CircleDegreeRangeSlider::_get_center() const {
	return get_size() * 0.5;
}

double CircleDegreeRangeSlider::_get_radius() const {
	const Size2 size = get_size();
	return MAX(0.0, MIN(size.x, size.y) * 0.5 - 10.0 * EDSCALE);
}

double CircleDegreeRangeSlider::_get_sweep() const {
	return MAX(0.00001, max_value - min_value);
}

double CircleDegreeRangeSlider::_snap_value(double p_value) const {
	if (step > 0.0) {
		p_value = Math::snapped(p_value, step);
	}
	return _normalize_angle(p_value);
}

double CircleDegreeRangeSlider::_normalize_angle(double p_value) const {
	return min_value + Math::fposmod(p_value - min_value, _get_sweep());
}

Vector2 CircleDegreeRangeSlider::_angle_to_point(double p_angle) const {
	const double angle = Math::deg_to_rad(p_angle - 90.0);
	return _get_center() + Vector2(Math::cos(angle), Math::sin(angle)) * _get_radius();
}

double CircleDegreeRangeSlider::_point_to_angle(const Vector2 &p_point) const {
	const Vector2 dir = p_point - _get_center();
	return _snap_value(Math::rad_to_deg(Math::atan2(dir.y, dir.x)) + 90.0);
}

CircleDegreeRangeSlider::Handle CircleDegreeRangeSlider::_get_handle_at_position(const Vector2 &p_position) const {
	const double grab_radius = 8.0 * EDSCALE;
	const double dist_a = p_position.distance_to(_angle_to_point(value.x));
	const double dist_b = p_position.distance_to(_angle_to_point(value.y));

	if (dist_a <= grab_radius || dist_b <= grab_radius) {
		return dist_a <= dist_b ? HANDLE_A : HANDLE_B;
	}
	return HANDLE_NONE;
}

void CircleDegreeRangeSlider::_set_handle_value(Handle p_handle, double p_angle) {
	Vector2 new_value = value;
	if (p_handle == HANDLE_A) {
		new_value.x = p_angle;
	} else if (p_handle == HANDLE_B) {
		new_value.y = p_angle;
	}
	set_value(new_value);
}

void CircleDegreeRangeSlider::_update_theme() {
	const Color accent_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	track_color = get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor));
	range_color = accent_color.lerp(Color(1, 1, 1), 0.25);
	handle_a_color = get_theme_color(SNAME("property_color_x"), EditorStringName(Editor));
	handle_b_color = get_theme_color(SNAME("property_color_y"), EditorStringName(Editor));
	handle_outline_color = get_theme_color(SNAME("base_color"), EditorStringName(Editor));
}

void CircleDegreeRangeSlider::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			const Vector2 center = _get_center();
			const double radius = _get_radius();
			if (radius <= 0.0) {
				return;
			}

			const double a_angle = _normalize_angle(value.x);
			const double b_angle = _normalize_angle(value.y);
			const double start_rad = Math::deg_to_rad(a_angle - 90.0);
			const double end_rad = start_rad + Math::deg_to_rad(Math::fposmod(b_angle - a_angle, _get_sweep()));
			const real_t track_width = 4.0 * EDSCALE;
			const real_t range_width = 6.0 * EDSCALE;

			draw_arc(center, radius, -Math::PI * 0.5, Math::TAU - Math::PI * 0.5, 96, track_color, track_width, true);
			draw_arc(center, radius, start_rad, end_rad, 96, range_color, range_width, true);

			const Vector2 point_a = _angle_to_point(a_angle);
			const Vector2 point_b = _angle_to_point(b_angle);
			draw_line(center, point_a, handle_a_color.lerp(Color(1, 1, 1), hovered_handle == HANDLE_A || dragging_handle == HANDLE_A ? 0.25 : 0.0), 2.0 * EDSCALE, true);
			draw_line(center, point_b, handle_b_color.lerp(Color(1, 1, 1), hovered_handle == HANDLE_B || dragging_handle == HANDLE_B ? 0.25 : 0.0), 2.0 * EDSCALE, true);

			const real_t handle_radius = 5.0 * EDSCALE;
			draw_circle(point_a, handle_radius + 2.0 * EDSCALE, handle_outline_color);
			draw_circle(point_a, handle_radius, handle_a_color);
			draw_circle(point_b, handle_radius + 2.0 * EDSCALE, handle_outline_color);
			draw_circle(point_b, handle_radius, handle_b_color);
		} break;
	}
}

void CircleDegreeRangeSlider::gui_input(const Ref<InputEvent> &p_event) {
	if (read_only) {
		return;
	}

	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			dragging_handle = _get_handle_at_position(mb->get_position());
			if (dragging_handle == HANDLE_NONE) {
				const double angle = _point_to_angle(mb->get_position());
				const double dist_a = Math::abs(Math::fposmod(angle - value.x + 180.0, 360.0) - 180.0);
				const double dist_b = Math::abs(Math::fposmod(angle - value.y + 180.0, 360.0) - 180.0);
				dragging_handle = dist_a <= dist_b ? HANDLE_A : HANDLE_B;
			}
			grab_focus(true);
			_set_handle_value(dragging_handle, _point_to_angle(mb->get_position()));
		} else {
			dragging_handle = HANDLE_NONE;
			queue_redraw();
		}
		accept_event();
		return;
	}

	const Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (dragging_handle != HANDLE_NONE) {
			_set_handle_value(dragging_handle, _point_to_angle(mm->get_position()));
		} else {
			const Handle new_hovered_handle = _get_handle_at_position(mm->get_position());
			if (new_hovered_handle != hovered_handle) {
				hovered_handle = new_hovered_handle;
				queue_redraw();
			}
		}
		accept_event();
	}
}

Size2 CircleDegreeRangeSlider::get_minimum_size() const {
	return Size2(112, 112) * EDSCALE;
}

void CircleDegreeRangeSlider::set_value(const Vector2 &p_value) {
	const Vector2 new_value(_snap_value(p_value.x), _snap_value(p_value.y));
	if (value.is_equal_approx(new_value)) {
		return;
	}
	value = new_value;
	if (value_changed_callable.is_valid()) {
		value_changed_callable.call(value);
	}
	queue_redraw();
}

Vector2 CircleDegreeRangeSlider::get_value() const {
	return value;
}

void CircleDegreeRangeSlider::set_value_no_signal(const Vector2 &p_value) {
	const Vector2 new_value(_snap_value(p_value.x), _snap_value(p_value.y));
	if (value.is_equal_approx(new_value)) {
		return;
	}
	value = new_value;
	queue_redraw();
}

void CircleDegreeRangeSlider::set_value_changed_callable(const Callable &p_callable) {
	value_changed_callable = p_callable;
}

void CircleDegreeRangeSlider::setup(double p_min, double p_max, double p_step) {
	min_value = p_min;
	max_value = p_max;
	step = p_step;
	value = Vector2(_normalize_angle(value.x), _normalize_angle(value.y));
	queue_redraw();
}

void CircleDegreeRangeSlider::set_read_only(bool p_read_only) {
	read_only = p_read_only;
	set_mouse_filter(read_only ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
}

bool CircleDegreeRangeSlider::is_read_only() const {
	return read_only;
}

CircleDegreeRangeSlider::CircleDegreeRangeSlider() {
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_tooltip_text(TTR("Drag the colored handles to edit the angle range."));
}

void EditorPropertyCircleDegreeRange::_slider_value_changed(const Vector2 &p_value) {
	if (updating) {
		return;
	}
	updating = true;
	a_spin->set_value_no_signal(p_value.x);
	b_spin->set_value_no_signal(p_value.y);
	updating = false;
	emit_changed(get_edited_property(), p_value, "", true);
}

void EditorPropertyCircleDegreeRange::_spin_value_changed(double p_value, bool p_is_a) {
	if (updating) {
		return;
	}
	Vector2 value = slider->get_value();
	if (p_is_a) {
		value.x = p_value;
	} else {
		value.y = p_value;
	}
	updating = true;
	slider->set_value_no_signal(value);
	updating = false;
	emit_changed(get_edited_property(), slider->get_value(), p_is_a ? "x" : "y", true);
}

void EditorPropertyCircleDegreeRange::_set_read_only(bool p_read_only) {
	slider->set_read_only(p_read_only);
	a_spin->set_read_only(p_read_only);
	b_spin->set_read_only(p_read_only);
}

void EditorPropertyCircleDegreeRange::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		a_spin->add_theme_color_override("label_color", get_theme_color(SNAME("property_color_x"), EditorStringName(Editor)));
		b_spin->add_theme_color_override("label_color", get_theme_color(SNAME("property_color_y"), EditorStringName(Editor)));
	}
}

void EditorPropertyCircleDegreeRange::update_property() {
	const Vector2 value = get_edited_property_value();
	updating = true;
	slider->set_value_no_signal(value);
	a_spin->set_value_no_signal(slider->get_value().x);
	b_spin->set_value_no_signal(slider->get_value().y);
	updating = false;
}

void EditorPropertyCircleDegreeRange::setup(const EditorPropertyRangeHint &p_range_hint) {
	slider->setup(p_range_hint.min, p_range_hint.max, p_range_hint.step);
	for (EditorSpinSlider *spin : { a_spin, b_spin }) {
		spin->set_min(p_range_hint.min);
		spin->set_max(p_range_hint.max);
		spin->set_step(p_range_hint.step);
		spin->set_allow_lesser(false);
		spin->set_allow_greater(false);
		spin->set_suffix(p_range_hint.suffix.is_empty() ? String(U"\u00B0") : p_range_hint.suffix);
	}
}

EditorPropertyCircleDegreeRange::EditorPropertyCircleDegreeRange() {
	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_theme_constant_override(SNAME("separation"), 2 * EDSCALE);
	add_child(vb);

	slider = memnew(CircleDegreeRangeSlider);
	slider->set_h_size_flags(SIZE_SHRINK_CENTER);
	vb->add_child(slider);
	slider->set_value_changed_callable(callable_mp(this, &EditorPropertyCircleDegreeRange::_slider_value_changed));

	HBoxContainer *hb = memnew(HBoxContainer);
	vb->add_child(hb);

	a_spin = memnew(EditorSpinSlider);
	a_spin->set_flat(true);
	a_spin->set_label("a");
	a_spin->set_h_size_flags(SIZE_EXPAND_FILL);
	hb->add_child(a_spin);
	a_spin->connect(SceneStringName(value_changed), callable_mp(this, &EditorPropertyCircleDegreeRange::_spin_value_changed).bind(true));
	add_focusable(a_spin);

	b_spin = memnew(EditorSpinSlider);
	b_spin->set_flat(true);
	b_spin->set_label("b");
	b_spin->set_h_size_flags(SIZE_EXPAND_FILL);
	hb->add_child(b_spin);
	b_spin->connect(SceneStringName(value_changed), callable_mp(this, &EditorPropertyCircleDegreeRange::_spin_value_changed).bind(false));
	add_focusable(b_spin);

	set_bottom_editor(vb);
}
const String EditorPropertyVectorN::COMPONENT_LABELS[4] = { "x", "y", "z", "w" };

void EditorPropertyVectorN::_set_read_only(bool p_read_only) {
	for (EditorSpinSlider *spin : spin_sliders) {
		spin->set_read_only(p_read_only);
	}
}

void EditorPropertyVectorN::_value_changed(double val, const String &p_name) {
	if (linked->is_pressed()) {
		int changed_component = -1;
		for (int i = 0; i < component_count; i++) {
			if (p_name == COMPONENT_LABELS[i]) {
				changed_component = i;
				break;
			}
		}
		DEV_ASSERT(changed_component >= 0);

		for (int i = 0; i < component_count - 1; i++) {
			int slider_idx = (changed_component + 1 + i) % component_count;
			int ratio_idx = changed_component * (component_count - 1) + i;

			if (ratio[ratio_idx] == 0) {
				continue;
			}

			spin_sliders[slider_idx]->set_value_no_signal(spin_sliders[changed_component]->get_value() * ratio[ratio_idx]);
		}
	}

	Variant v;
	Callable::CallError cerror;
	Variant::construct(vector_type, v, nullptr, 0, cerror);

	for (int i = 0; i < component_count; i++) {
		if (radians_as_degrees) {
			v.set(i, Math::deg_to_rad(spin_sliders[i]->get_value()));
		} else {
			v.set(i, spin_sliders[i]->get_value());
		}
	}
	emit_changed(get_edited_property(), v, linked->is_pressed() ? "" : p_name);
}

void EditorPropertyVectorN::update_property() {
	Variant val = get_edited_property_value();
	for (int i = 0; i < component_count; i++) {
		if (radians_as_degrees) {
			spin_sliders[i]->set_value_no_signal(Math::rad_to_deg((real_t)val.get(i)));
		} else {
			spin_sliders[i]->set_value_no_signal(val.get(i));
		}
	}
	_update_ratio();
}

void EditorPropertyVectorN::_update_ratio() {
	linked->set_modulate(Color(1, 1, 1, linked->is_pressed() ? 1.0 : 0.5));

	double *ratio_write = ratio.ptrw();
	for (int i = 0; i < ratio.size(); i++) {
		int base_slider_idx = i / (component_count - 1);
		int secondary_slider_idx = ((base_slider_idx + 1) + i % (component_count - 1)) % component_count;

		if (spin_sliders[base_slider_idx]->get_value() != 0) {
			ratio_write[i] = spin_sliders[secondary_slider_idx]->get_value() / spin_sliders[base_slider_idx]->get_value();
		}
	}
}

void EditorPropertyVectorN::_store_link(bool p_linked) {
	if (!get_edited_object()) {
		return;
	}
	const String key = vformat("%s:%s", get_edited_object()->get_class(), get_edited_property());
	EditorSettings::get_singleton()->set_project_metadata("linked_properties", key, p_linked);
}

void EditorPropertyVectorN::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (linked->is_visible()) {
				if (get_edited_object()) {
					const String key = vformat("%s:%s", get_edited_object()->get_class(), get_edited_property());
					linked->set_pressed_no_signal(EditorSettings::get_singleton()->get_project_metadata("linked_properties", key, true));
					_update_ratio();
				}
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			int icon_size = get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));

			linked->set_texture_normal(get_editor_theme_icon(SNAME("Unlinked")));
			linked->set_texture_pressed(get_editor_theme_icon(SNAME("Instance")));
			linked->set_custom_minimum_size(Size2(icon_size + 8 * EDSCALE, 0));

			const Color *colors = _get_property_colors();
			for (int i = 0; i < component_count; i++) {
				spin_sliders[i]->add_theme_color_override("label_color", colors[i]);
			}
		} break;
	}
}

void EditorPropertyVectorN::setup(const EditorPropertyRangeHint &p_range_hint, bool p_link, bool p_is_int) {
	radians_as_degrees = p_range_hint.radians_as_degrees;

	for (EditorSpinSlider *spin : spin_sliders) {
		spin->set_min(p_range_hint.min);
		spin->set_max(p_range_hint.max);
		spin->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin->set_allow_greater(true);
		spin->set_allow_lesser(true);
		spin->set_suffix(p_range_hint.suffix);
		spin->set_editing_integer(p_is_int);
	}

	if (!p_link) {
		linked->hide();
	}
}

EditorPropertyVectorN::EditorPropertyVectorN(Variant::Type p_type, bool p_force_wide, bool p_horizontal) {
	vector_type = p_type;
	switch (vector_type) {
		case Variant::VECTOR2:
		case Variant::VECTOR2I:
			component_count = 2;
			break;

		case Variant::VECTOR3:
		case Variant::VECTOR3I:
			component_count = 3;
			break;

		case Variant::VECTOR4:
		case Variant::VECTOR4I:
			component_count = 4;
			break;

		default: // Needed to silence a warning.
			ERR_PRINT("Not a Vector type.");
			break;
	}
	bool horizontal = p_force_wide || p_horizontal;

	HBoxContainer *hb = memnew(HBoxContainer);
	hb->set_h_size_flags(SIZE_EXPAND_FILL);

	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		hb->add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		hb->add_child(bc);
		set_bottom_editor(hb);
	} else {
		bc = memnew(VBoxContainer);
		hb->add_child(bc);
	}
	bc->set_h_size_flags(SIZE_EXPAND_FILL);

	spin_sliders.resize(component_count);
	EditorSpinSlider **spin = spin_sliders.ptrw();

	for (int i = 0; i < component_count; i++) {
		spin[i] = memnew(EditorSpinSlider);
		bc->add_child(spin[i]);
		spin[i]->set_flat(true);
		spin[i]->set_label(String(COMPONENT_LABELS[i]));
		spin[i]->set_accessibility_name(String(COMPONENT_LABELS[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
		spin[i]->connect(SceneStringName(value_changed), callable_mp(this, &EditorPropertyVectorN::_value_changed).bind(String(COMPONENT_LABELS[i])));
		add_focusable(spin[i]);
	}

	ratio.resize(component_count * (component_count - 1));
	ratio.fill(1.0);

	linked = memnew(TextureButton);
	linked->set_toggle_mode(true);
	linked->set_stretch_mode(TextureButton::STRETCH_KEEP_CENTERED);
	linked->set_tooltip_text(TTR("Lock/Unlock Component Ratio"));
	linked->connect(SceneStringName(pressed), callable_mp(this, &EditorPropertyVectorN::_update_ratio));
	linked->connect(SceneStringName(toggled), callable_mp(this, &EditorPropertyVectorN::_store_link));
	hb->add_child(linked);

	add_child(hb);
	if (!horizontal) {
		set_label_reference(spin_sliders[0]); // Show text and buttons around this.
	}
}

void EditorPropertyVectorN::set_deferred_drag_mode_enabled(bool p_enabled) {
	EditorProperty::set_deferred_drag_mode_enabled(p_enabled);

	for (int i = 0; i < component_count; i++) {
		spin_sliders[i]->set_deferred_drag_mode_enabled(p_enabled);
	}
}

EditorPropertyVector2::EditorPropertyVector2(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR2, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector2_editing")) {}

EditorPropertyVector2i::EditorPropertyVector2i(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR2I, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector2_editing")) {}

EditorPropertyVector3::EditorPropertyVector3(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR3, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}

EditorPropertyVector3i::EditorPropertyVector3i(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR3I, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}

EditorPropertyVector4::EditorPropertyVector4(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR4, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}

EditorPropertyVector4i::EditorPropertyVector4i(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR4I, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}
