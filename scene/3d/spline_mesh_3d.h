/**************************************************************************/
/*  spline_mesh_3d.h                                                       */
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

#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/path_3d.h"

class SplineMesh3D : public MeshInstance3D {
	GDCLASS(SplineMesh3D, MeshInstance3D);

public:
	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
	};

	enum FrameMode {
		FRAME_PARALLEL_TRANSPORT,
		FRAME_CURVE_UP_VECTOR,
	};

private:
	struct Frame {
		real_t offset = 0.0;
		Basis basis;
		Vector3 origin;
	};

	Ref<Mesh> source_mesh;
	Ref<ArrayMesh> generated_mesh;
	NodePath path_node;
	ObjectID connected_path_id;

	Axis forward_axis = AXIS_Z;
	Axis up_axis = AXIS_Y;
	FrameMode frame_mode = FRAME_PARALLEL_TRANSPORT;
	bool stretch_to_fit = true;
	bool cubic_interpolation = true;
	bool tilt_enabled = true;
	real_t frame_interval = 0.25;

	void _source_mesh_changed();
	void _path_curve_changed();
	Path3D *_get_path() const;
	Ref<Curve3D> _get_curve() const;
	void _connect_path();
	void _disconnect_path();
	void _request_rebuild();
	void _rebuild();

	static Vector3 _axis_vector(Axis p_axis);
	static real_t _axis_value(const Vector3 &p_vector, Axis p_axis);
	static Basis _basis_from_forward_up(const Vector3 &p_forward, const Vector3 &p_up);
	Basis _source_to_deform_basis() const;
	void _build_frames(const Ref<Curve3D> &p_curve, LocalVector<Frame> &r_frames) const;
	Basis _sample_frame_basis(const LocalVector<Frame> &p_frames, real_t p_offset) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_source_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_source_mesh() const;

	void set_path_node(const NodePath &p_path);
	NodePath get_path_node() const;

	void set_forward_axis(Axis p_axis);
	Axis get_forward_axis() const;

	void set_up_axis(Axis p_axis);
	Axis get_up_axis() const;

	void set_frame_mode(FrameMode p_mode);
	FrameMode get_frame_mode() const;

	void set_stretch_to_fit(bool p_enabled);
	bool is_stretch_to_fit_enabled() const;

	void set_cubic_interpolation_enabled(bool p_enabled);
	bool is_cubic_interpolation_enabled() const;

	void set_tilt_enabled(bool p_enabled);
	bool is_tilt_enabled() const;

	void set_frame_interval(real_t p_interval);
	real_t get_frame_interval() const;

	void rebuild();

	virtual PackedStringArray get_configuration_warnings() const override;

	SplineMesh3D();
	~SplineMesh3D();
};

VARIANT_ENUM_CAST(SplineMesh3D::Axis);
VARIANT_ENUM_CAST(SplineMesh3D::FrameMode);
