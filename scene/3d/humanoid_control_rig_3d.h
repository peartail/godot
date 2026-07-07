/**************************************************************************/
/*  humanoid_control_rig_3d.h                                             */
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

#include "scene/3d/node_3d.h"

class Marker3D;
class Skeleton3D;

class HumanoidControlRig3D : public Node3D {
	GDCLASS(HumanoidControlRig3D, Node3D);

	static constexpr const char *GENERATED_PREFIX = "ControlRig_";

	bool auto_setup_on_ready = false;
	bool create_hand_ik = true;
	bool create_foot_ik = true;
	bool create_head_look_at = true;
	float control_distance_scale = 1.0f;
	PackedStringArray missing_bones;

	struct BoneChain {
		StringName root;
		StringName middle;
		StringName end;
		StringName target_control;
		StringName pole_control;
		Vector3 pole_offset;
	};

	Skeleton3D *_get_parent_skeleton() const;
	int _find_bone(Skeleton3D *p_skeleton, const Vector<StringName> &p_aliases) const;
	void _push_missing(const String &p_name);
	Marker3D *_create_control(const StringName &p_control_name, const Vector3 &p_skeleton_position);
	void _add_generated_child(Node *p_parent, Node *p_child);
	void _setup_two_bone_ik(Skeleton3D *p_skeleton, const StringName &p_name, const Vector<BoneChain> &p_chains);
	void _setup_head_look_at(Skeleton3D *p_skeleton, int p_head_bone);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	void set_auto_setup_on_ready(bool p_enabled);
	bool is_auto_setup_on_ready() const;

	void set_create_hand_ik(bool p_enabled);
	bool is_hand_ik_created() const;

	void set_create_foot_ik(bool p_enabled);
	bool is_foot_ik_created() const;

	void set_create_head_look_at(bool p_enabled);
	bool is_head_look_at_created() const;

	void set_control_distance_scale(float p_scale);
	float get_control_distance_scale() const;

	void setup_from_skeleton();
	void clear_generated_rig();
	Node *get_control_node(const StringName &p_control_name) const;
	PackedStringArray get_missing_bones() const;
};
