/**************************************************************************/
/*  spline_mesh_3d.cpp                                                     */
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

#include "spline_mesh_3d.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/resources/mesh.h"

void SplineMesh3D::_source_mesh_changed() {
	_request_rebuild();
}

void SplineMesh3D::_path_curve_changed() {
	_request_rebuild();
}

Path3D *SplineMesh3D::_get_path() const {
	if (path_node.is_empty()) {
		return Object::cast_to<Path3D>(get_parent());
	}
	return Object::cast_to<Path3D>(get_node_or_null(path_node));
}

Ref<Curve3D> SplineMesh3D::_get_curve() const {
	Path3D *path = _get_path();
	if (!path) {
		return Ref<Curve3D>();
	}
	return path->get_curve();
}

void SplineMesh3D::_connect_path() {
	Path3D *path = _get_path();
	if (!path || connected_path_id == path->get_instance_id()) {
		return;
	}

	_disconnect_path();
	path->connect("curve_changed", callable_mp(this, &SplineMesh3D::_path_curve_changed));
	connected_path_id = path->get_instance_id();
}

void SplineMesh3D::_disconnect_path() {
	Object *connected_path = ObjectDB::get_instance(connected_path_id);
	if (connected_path) {
		Path3D *path = Object::cast_to<Path3D>(connected_path);
		if (path && path->is_connected("curve_changed", callable_mp(this, &SplineMesh3D::_path_curve_changed))) {
			path->disconnect("curve_changed", callable_mp(this, &SplineMesh3D::_path_curve_changed));
		}
	}
	connected_path_id = ObjectID();
}

Vector3 SplineMesh3D::_axis_vector(Axis p_axis) {
	switch (p_axis) {
		case AXIS_X:
			return Vector3(1, 0, 0);
		case AXIS_Y:
			return Vector3(0, 1, 0);
		case AXIS_Z:
			return Vector3(0, 0, 1);
	}
	return Vector3(0, 0, 1);
}

real_t SplineMesh3D::_axis_value(const Vector3 &p_vector, Axis p_axis) {
	switch (p_axis) {
		case AXIS_X:
			return p_vector.x;
		case AXIS_Y:
			return p_vector.y;
		case AXIS_Z:
			return p_vector.z;
	}
	return p_vector.z;
}

Basis SplineMesh3D::_basis_from_forward_up(const Vector3 &p_forward, const Vector3 &p_up) {
	Vector3 forward = p_forward.normalized();
	Vector3 up = p_up.normalized();

	if (forward.is_zero_approx()) {
		forward = Vector3(0, 0, 1);
	}
	if (up.is_zero_approx() || Math::is_equal_approx(Math::abs(forward.dot(up)), (real_t)1.0)) {
		up = Math::abs(forward.dot(Vector3(0, 1, 0))) < 0.999 ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
	}

	Vector3 right = up.cross(forward).normalized();
	up = forward.cross(right).normalized();
	return Basis(right, up, forward).orthonormalized();
}

Basis SplineMesh3D::_source_to_deform_basis() const {
	const Vector3 forward = _axis_vector(forward_axis);
	const Vector3 up = _axis_vector(up_axis);
	return _basis_from_forward_up(forward, up);
}

void SplineMesh3D::_build_frames(const Ref<Curve3D> &p_curve, LocalVector<Frame> &r_frames) const {
	r_frames.clear();

	const real_t curve_length = p_curve->get_baked_length();
	if (curve_length <= CMP_EPSILON) {
		return;
	}

	const int frame_count = MAX(2, int(Math::ceil(curve_length / frame_interval)) + 1);
	r_frames.resize(frame_count);

	if (frame_mode == FRAME_CURVE_UP_VECTOR) {
		for (int i = 0; i < frame_count; i++) {
			const real_t offset = curve_length * i / (frame_count - 1);
			Transform3D xform = p_curve->sample_baked_with_rotation(offset, cubic_interpolation, tilt_enabled);
			Frame &frame = r_frames[i];
			frame.offset = offset;
			frame.origin = xform.origin;
			frame.basis = Basis(xform.basis.get_column(0), xform.basis.get_column(1), -xform.basis.get_column(2)).orthonormalized();
		}
		return;
	}

	Vector3 prev_forward;
	Vector3 prev_up;
	Vector3 prev_right;

	for (int i = 0; i < frame_count; i++) {
		const real_t offset = curve_length * i / (frame_count - 1);
		const Vector3 pos = p_curve->sample_baked(offset, cubic_interpolation);
		const real_t tangent_delta = MIN(MAX(frame_interval * 0.5, 0.001), curve_length * 0.5);
		const Vector3 before = p_curve->sample_baked(MAX(0.0, offset - tangent_delta), cubic_interpolation);
		const Vector3 after = p_curve->sample_baked(MIN(curve_length, offset + tangent_delta), cubic_interpolation);
		Vector3 forward = (after - before).normalized();

		if (forward.is_zero_approx()) {
			forward = i == 0 ? Vector3(0, 0, 1) : prev_forward;
		}

		Basis basis;
		if (i == 0) {
			basis = _basis_from_forward_up(forward, Vector3(0, 1, 0));
		} else {
			Vector3 axis = prev_forward.cross(forward);
			real_t axis_length = axis.length();
			if (axis_length > CMP_EPSILON) {
				axis /= axis_length;
				const real_t angle = Math::acos(CLAMP(prev_forward.dot(forward), -1.0, 1.0));
				const Basis rotation(axis, angle);
				prev_right = rotation.xform(prev_right).normalized();
				prev_up = rotation.xform(prev_up).normalized();
			}
			prev_right = (prev_up.cross(forward)).normalized();
			prev_up = (forward.cross(prev_right)).normalized();
			basis = Basis(prev_right, prev_up, forward).orthonormalized();
		}

		if (tilt_enabled) {
			const real_t tilt = p_curve->sample_baked_tilt(offset);
			basis = Basis(forward, tilt) * basis;
		}

		Frame &frame = r_frames[i];
		frame.offset = offset;
		frame.origin = pos;
		frame.basis = basis;

		prev_forward = frame.basis.get_column(2);
		prev_up = frame.basis.get_column(1);
		prev_right = frame.basis.get_column(0);
	}
}

Basis SplineMesh3D::_sample_frame_basis(const LocalVector<Frame> &p_frames, real_t p_offset) const {
	if (p_frames.size() == 1) {
		return p_frames[0].basis;
	}

	int index = 0;
	while (index < int(p_frames.size()) - 2 && p_offset > p_frames[index + 1].offset) {
		index++;
	}

	const Frame &begin = p_frames[index];
	const Frame &end = p_frames[index + 1];
	const real_t length = end.offset - begin.offset;
	const real_t weight = length <= CMP_EPSILON ? 0.0 : CLAMP((p_offset - begin.offset) / length, 0.0, 1.0);
	return begin.basis.slerp(end.basis, weight).orthonormalized();
}
void SplineMesh3D::_request_rebuild() {
	if (!is_accessible_from_caller_thread()) {
		call_deferred(SNAME("rebuild"));
		return;
	}

	_rebuild();
}
void SplineMesh3D::_rebuild() {
	if (generated_mesh.is_null()) {
		generated_mesh.instantiate();
	}
	generated_mesh->clear_surfaces();

	Ref<Curve3D> curve = _get_curve();
	if (source_mesh.is_null() || curve.is_null() || curve->get_point_count() < 2 || curve->get_baked_length() <= CMP_EPSILON) {
		MeshInstance3D::set_mesh(Ref<Mesh>());
		update_configuration_warnings();
		return;
	}

	if (forward_axis == up_axis) {
		MeshInstance3D::set_mesh(Ref<Mesh>());
		update_configuration_warnings();
		return;
	}

	const AABB source_aabb = source_mesh->get_aabb();
	const real_t source_min = _axis_value(source_aabb.position, forward_axis);
	const real_t source_size = _axis_value(source_aabb.size, forward_axis);
	if (source_size <= CMP_EPSILON) {
		MeshInstance3D::set_mesh(Ref<Mesh>());
		update_configuration_warnings();
		return;
	}

	LocalVector<Frame> frames;
	_build_frames(curve, frames);
	if (frames.is_empty()) {
		MeshInstance3D::set_mesh(Ref<Mesh>());
		update_configuration_warnings();
		return;
	}

	const real_t curve_length = curve->get_baked_length();
	const Basis source_to_deform = _source_to_deform_basis().transposed();

	for (int surface_index = 0; surface_index < source_mesh->get_surface_count(); surface_index++) {
		Array arrays = source_mesh->surface_get_arrays(surface_index);
		ERR_CONTINUE(arrays.size() != Mesh::ARRAY_MAX);

		Vector<Vector3> vertices = arrays[Mesh::ARRAY_VERTEX];
		Vector<Vector3> normals = arrays[Mesh::ARRAY_NORMAL];
		Vector<float> tangents = arrays[Mesh::ARRAY_TANGENT];

		const bool has_normals = normals.size() == vertices.size();
		const bool has_tangents = tangents.size() == vertices.size() * 4;

		Vector3 *vertices_ptrw = vertices.ptrw();
		Vector3 *normals_ptrw = normals.ptrw();
		float *tangents_ptrw = tangents.ptrw();

		for (int i = 0; i < vertices.size(); i++) {
			const Vector3 deform_vertex = source_to_deform.xform(vertices_ptrw[i]);
			const real_t local_forward = _axis_value(vertices_ptrw[i], forward_axis) - source_min;
			const real_t offset = stretch_to_fit ? (local_forward / source_size) * curve_length : CLAMP(local_forward, 0.0, curve_length);
			const Vector3 origin = curve->sample_baked(offset, cubic_interpolation);
			const Basis frame = _sample_frame_basis(frames, offset);

			Vector3 local_offset = deform_vertex;
			local_offset.z = 0.0;
			vertices_ptrw[i] = origin + frame.xform(local_offset);

			if (has_normals) {
				Vector3 deform_normal = source_to_deform.xform(normals_ptrw[i]);
				normals_ptrw[i] = frame.xform(deform_normal).normalized();
			}

			if (has_tangents) {
				const int tangent_index = i * 4;
				Vector3 tangent(tangents_ptrw[tangent_index + 0], tangents_ptrw[tangent_index + 1], tangents_ptrw[tangent_index + 2]);
				tangent = frame.xform(source_to_deform.xform(tangent)).normalized();
				tangents_ptrw[tangent_index + 0] = tangent.x;
				tangents_ptrw[tangent_index + 1] = tangent.y;
				tangents_ptrw[tangent_index + 2] = tangent.z;
			}
		}

		arrays[Mesh::ARRAY_VERTEX] = vertices;
		if (has_normals) {
			arrays[Mesh::ARRAY_NORMAL] = normals;
		}
		if (has_tangents) {
			arrays[Mesh::ARRAY_TANGENT] = tangents;
		}

		const uint32_t surface_format = source_mesh->surface_get_format(surface_index);
		generated_mesh->add_surface_from_arrays(source_mesh->surface_get_primitive_type(surface_index), arrays, Array(), Dictionary(), surface_format);
		generated_mesh->surface_set_material(surface_index, source_mesh->surface_get_material(surface_index));
	}

	MeshInstance3D::set_mesh(generated_mesh);
	update_configuration_warnings();
}

void SplineMesh3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_connect_path();
			_request_rebuild();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_disconnect_path();
		} break;
	}
}

void SplineMesh3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_source_mesh", "mesh"), &SplineMesh3D::set_source_mesh);
	ClassDB::bind_method(D_METHOD("get_source_mesh"), &SplineMesh3D::get_source_mesh);
	ClassDB::bind_method(D_METHOD("set_path_node", "path"), &SplineMesh3D::set_path_node);
	ClassDB::bind_method(D_METHOD("get_path_node"), &SplineMesh3D::get_path_node);
	ClassDB::bind_method(D_METHOD("set_forward_axis", "axis"), &SplineMesh3D::set_forward_axis);
	ClassDB::bind_method(D_METHOD("get_forward_axis"), &SplineMesh3D::get_forward_axis);
	ClassDB::bind_method(D_METHOD("set_up_axis", "axis"), &SplineMesh3D::set_up_axis);
	ClassDB::bind_method(D_METHOD("get_up_axis"), &SplineMesh3D::get_up_axis);
	ClassDB::bind_method(D_METHOD("set_frame_mode", "mode"), &SplineMesh3D::set_frame_mode);
	ClassDB::bind_method(D_METHOD("get_frame_mode"), &SplineMesh3D::get_frame_mode);
	ClassDB::bind_method(D_METHOD("set_stretch_to_fit", "enabled"), &SplineMesh3D::set_stretch_to_fit);
	ClassDB::bind_method(D_METHOD("is_stretch_to_fit_enabled"), &SplineMesh3D::is_stretch_to_fit_enabled);
	ClassDB::bind_method(D_METHOD("set_cubic_interpolation", "enabled"), &SplineMesh3D::set_cubic_interpolation_enabled);
	ClassDB::bind_method(D_METHOD("get_cubic_interpolation"), &SplineMesh3D::is_cubic_interpolation_enabled);
	ClassDB::bind_method(D_METHOD("set_tilt_enabled", "enabled"), &SplineMesh3D::set_tilt_enabled);
	ClassDB::bind_method(D_METHOD("is_tilt_enabled"), &SplineMesh3D::is_tilt_enabled);
	ClassDB::bind_method(D_METHOD("set_frame_interval", "interval"), &SplineMesh3D::set_frame_interval);
	ClassDB::bind_method(D_METHOD("get_frame_interval"), &SplineMesh3D::get_frame_interval);
	ClassDB::bind_method(D_METHOD("rebuild"), &SplineMesh3D::rebuild);

	ADD_GROUP("Spline", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_mesh", PROPERTY_HINT_RESOURCE_TYPE, Mesh::get_class_static()), "set_source_mesh", "get_source_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "path_node", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Path3D"), "set_path_node", "get_path_node");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "forward_axis", PROPERTY_HINT_ENUM, "X,Y,Z"), "set_forward_axis", "get_forward_axis");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "up_axis", PROPERTY_HINT_ENUM, "X,Y,Z"), "set_up_axis", "get_up_axis");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frame_mode", PROPERTY_HINT_ENUM, "Parallel Transport,Curve Up Vector"), "set_frame_mode", "get_frame_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stretch_to_fit"), "set_stretch_to_fit", "is_stretch_to_fit_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cubic_interp"), "set_cubic_interpolation", "get_cubic_interpolation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tilt_enabled"), "set_tilt_enabled", "is_tilt_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frame_interval", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater,suffix:m"), "set_frame_interval", "get_frame_interval");

	BIND_ENUM_CONSTANT(AXIS_X);
	BIND_ENUM_CONSTANT(AXIS_Y);
	BIND_ENUM_CONSTANT(AXIS_Z);
	BIND_ENUM_CONSTANT(FRAME_PARALLEL_TRANSPORT);
	BIND_ENUM_CONSTANT(FRAME_CURVE_UP_VECTOR);
}

void SplineMesh3D::set_source_mesh(const Ref<Mesh> &p_mesh) {
	if (source_mesh == p_mesh) {
		return;
	}
	if (source_mesh.is_valid()) {
		source_mesh->disconnect_changed(callable_mp(this, &SplineMesh3D::_source_mesh_changed));
	}
	source_mesh = p_mesh;
	if (source_mesh.is_valid()) {
		source_mesh->connect_changed(callable_mp(this, &SplineMesh3D::_source_mesh_changed));
	}
	_request_rebuild();
}

Ref<Mesh> SplineMesh3D::get_source_mesh() const {
	return source_mesh;
}

void SplineMesh3D::set_path_node(const NodePath &p_path) {
	if (path_node == p_path) {
		return;
	}
	_disconnect_path();
	path_node = p_path;
	if (is_inside_tree()) {
		_connect_path();
	}
	_request_rebuild();
}

NodePath SplineMesh3D::get_path_node() const {
	return path_node;
}

void SplineMesh3D::set_forward_axis(Axis p_axis) {
	if (forward_axis == p_axis) {
		return;
	}
	forward_axis = p_axis;
	_request_rebuild();
}

SplineMesh3D::Axis SplineMesh3D::get_forward_axis() const {
	return forward_axis;
}

void SplineMesh3D::set_up_axis(Axis p_axis) {
	if (up_axis == p_axis) {
		return;
	}
	up_axis = p_axis;
	_request_rebuild();
}

SplineMesh3D::Axis SplineMesh3D::get_up_axis() const {
	return up_axis;
}

void SplineMesh3D::set_frame_mode(FrameMode p_mode) {
	if (frame_mode == p_mode) {
		return;
	}
	frame_mode = p_mode;
	_request_rebuild();
}

SplineMesh3D::FrameMode SplineMesh3D::get_frame_mode() const {
	return frame_mode;
}

void SplineMesh3D::set_stretch_to_fit(bool p_enabled) {
	if (stretch_to_fit == p_enabled) {
		return;
	}
	stretch_to_fit = p_enabled;
	_request_rebuild();
}

bool SplineMesh3D::is_stretch_to_fit_enabled() const {
	return stretch_to_fit;
}

void SplineMesh3D::set_cubic_interpolation_enabled(bool p_enabled) {
	if (cubic_interpolation == p_enabled) {
		return;
	}
	cubic_interpolation = p_enabled;
	_request_rebuild();
}

bool SplineMesh3D::is_cubic_interpolation_enabled() const {
	return cubic_interpolation;
}

void SplineMesh3D::set_tilt_enabled(bool p_enabled) {
	if (tilt_enabled == p_enabled) {
		return;
	}
	tilt_enabled = p_enabled;
	_request_rebuild();
}

bool SplineMesh3D::is_tilt_enabled() const {
	return tilt_enabled;
}

void SplineMesh3D::set_frame_interval(real_t p_interval) {
	ERR_FAIL_COND(p_interval <= 0.0);
	if (Math::is_equal_approx(frame_interval, p_interval)) {
		return;
	}
	frame_interval = p_interval;
	_request_rebuild();
}

real_t SplineMesh3D::get_frame_interval() const {
	return frame_interval;
}

void SplineMesh3D::rebuild() {
	if (!is_accessible_from_caller_thread()) {
		_request_rebuild();
		return;
	}
	_rebuild();
}

PackedStringArray SplineMesh3D::get_configuration_warnings() const {
	PackedStringArray warnings = MeshInstance3D::get_configuration_warnings();

	if (source_mesh.is_null()) {
		warnings.push_back(RTR("SplineMesh3D requires a source Mesh to deform."));
	}
	if (!_get_path()) {
		warnings.push_back(RTR("SplineMesh3D requires a Path3D. Set path_node or make it a child of a Path3D."));
	}
	Ref<Curve3D> curve = _get_curve();
	if (curve.is_null()) {
		warnings.push_back(RTR("SplineMesh3D requires its Path3D to have a Curve3D."));
	} else if (curve->get_point_count() < 2 || curve->get_baked_length() <= CMP_EPSILON) {
		warnings.push_back(RTR("SplineMesh3D requires a Curve3D with at least two points and a non-zero baked length."));
	}
	if (forward_axis == up_axis) {
		warnings.push_back(RTR("SplineMesh3D's forward_axis and up_axis must be different."));
	}
	if (frame_mode == FRAME_CURVE_UP_VECTOR && curve.is_valid() && !curve->is_up_vector_enabled()) {
		warnings.push_back(RTR("SplineMesh3D's Curve Up Vector frame mode requires the Curve3D's up_vector_enabled property."));
	}

	return warnings;
}

SplineMesh3D::SplineMesh3D() {
	generated_mesh.instantiate();
}

SplineMesh3D::~SplineMesh3D() {
	_disconnect_path();
	if (source_mesh.is_valid()) {
		source_mesh->disconnect_changed(callable_mp(this, &SplineMesh3D::_source_mesh_changed));
	}
}
