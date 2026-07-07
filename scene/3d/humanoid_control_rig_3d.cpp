/**************************************************************************/
/*  humanoid_control_rig_3d.cpp                                           */
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

#include "humanoid_control_rig_3d.h"

#include "core/object/class_db.h"
#include "core/templates/hash_set.h"

#include "scene/3d/look_at_modifier_3d.h"
#include "scene/3d/marker_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/two_bone_ik_3d.h"

static String _normalize_humanoid_bone_name(const String &p_name) {
	String normalized = p_name.to_lower();
	normalized = normalized.replace("_", "");
	normalized = normalized.replace("-", "");
	normalized = normalized.replace(".", "");
	normalized = normalized.replace(" ", "");
	return normalized;
}

Skeleton3D *HumanoidControlRig3D::_get_parent_skeleton() const {
	return Object::cast_to<Skeleton3D>(get_parent());
}

int HumanoidControlRig3D::_find_bone(Skeleton3D *p_skeleton, const Vector<StringName> &p_aliases) const {
	ERR_FAIL_NULL_V(p_skeleton, -1);

	HashSet<String> normalized_aliases;
	for (const StringName &alias : p_aliases) {
		normalized_aliases.insert(_normalize_humanoid_bone_name(String(alias)));
	}

	for (int i = 0; i < p_skeleton->get_bone_count(); i++) {
		if (normalized_aliases.has(_normalize_humanoid_bone_name(p_skeleton->get_bone_name(i)))) {
			return i;
		}
	}
	return -1;
}

void HumanoidControlRig3D::_push_missing(const String &p_name) {
	if (!missing_bones.has(p_name)) {
		missing_bones.push_back(p_name);
	}
}

void HumanoidControlRig3D::_add_generated_child(Node *p_parent, Node *p_child) {
	p_parent->add_child(p_child);
	p_child->set_name(String(GENERATED_PREFIX) + p_child->get_name());

	Node *owner = get_owner();
	if (owner) {
		p_child->set_owner(owner);
	}
}

Marker3D *HumanoidControlRig3D::_create_control(const StringName &p_control_name, const Vector3 &p_skeleton_position) {
	Marker3D *marker = memnew(Marker3D);
	marker->set_name(p_control_name);
	_add_generated_child(this, marker);
	marker->set_transform(get_transform().affine_inverse() * Transform3D(Basis(), p_skeleton_position));
	return marker;
}

void HumanoidControlRig3D::_setup_two_bone_ik(Skeleton3D *p_skeleton, const StringName &p_name, const Vector<BoneChain> &p_chains) {
	if (p_chains.is_empty()) {
		return;
	}

	TwoBoneIK3D *ik = memnew(TwoBoneIK3D);
	ik->set_name(p_name);
	_add_generated_child(p_skeleton, ik);
	ik->set_setting_count(p_chains.size());

	for (int i = 0; i < p_chains.size(); i++) {
		const BoneChain &chain = p_chains[i];
		ik->set_root_bone_name(i, chain.root);
		ik->set_middle_bone_name(i, chain.middle);
		ik->set_end_bone_name(i, chain.end);
		ik->set_target_node(i, ik->get_path_to(get_control_node(chain.target_control)));
		ik->set_pole_node(i, ik->get_path_to(get_control_node(chain.pole_control)));
		ik->set_pole_direction(i, SkeletonModifier3D::SECONDARY_DIRECTION_CUSTOM);
		ik->set_pole_direction_vector(i, chain.pole_offset.normalized());
	}
}

void HumanoidControlRig3D::_setup_head_look_at(Skeleton3D *p_skeleton, int p_head_bone) {
	LookAtModifier3D *look_at = memnew(LookAtModifier3D);
	look_at->set_name("HeadLookAt");
	_add_generated_child(p_skeleton, look_at);
	look_at->set_bone_name(p_skeleton->get_bone_name(p_head_bone));
	look_at->set_target_node(look_at->get_path_to(get_control_node("head_look")));
	look_at->set_forward_axis(SkeletonModifier3D::BONE_AXIS_PLUS_Z);
	look_at->set_primary_rotation_axis(Vector3::AXIS_Y);
}

void HumanoidControlRig3D::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY && auto_setup_on_ready) {
		setup_from_skeleton();
	}
}

void HumanoidControlRig3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_auto_setup_on_ready", "enabled"), &HumanoidControlRig3D::set_auto_setup_on_ready);
	ClassDB::bind_method(D_METHOD("is_auto_setup_on_ready"), &HumanoidControlRig3D::is_auto_setup_on_ready);
	ClassDB::bind_method(D_METHOD("set_create_hand_ik", "enabled"), &HumanoidControlRig3D::set_create_hand_ik);
	ClassDB::bind_method(D_METHOD("is_hand_ik_created"), &HumanoidControlRig3D::is_hand_ik_created);
	ClassDB::bind_method(D_METHOD("set_create_foot_ik", "enabled"), &HumanoidControlRig3D::set_create_foot_ik);
	ClassDB::bind_method(D_METHOD("is_foot_ik_created"), &HumanoidControlRig3D::is_foot_ik_created);
	ClassDB::bind_method(D_METHOD("set_create_head_look_at", "enabled"), &HumanoidControlRig3D::set_create_head_look_at);
	ClassDB::bind_method(D_METHOD("is_head_look_at_created"), &HumanoidControlRig3D::is_head_look_at_created);
	ClassDB::bind_method(D_METHOD("set_control_distance_scale", "scale"), &HumanoidControlRig3D::set_control_distance_scale);
	ClassDB::bind_method(D_METHOD("get_control_distance_scale"), &HumanoidControlRig3D::get_control_distance_scale);
	ClassDB::bind_method(D_METHOD("setup_from_skeleton"), &HumanoidControlRig3D::setup_from_skeleton);
	ClassDB::bind_method(D_METHOD("clear_generated_rig"), &HumanoidControlRig3D::clear_generated_rig);
	ClassDB::bind_method(D_METHOD("get_control_node", "control_name"), &HumanoidControlRig3D::get_control_node);
	ClassDB::bind_method(D_METHOD("get_missing_bones"), &HumanoidControlRig3D::get_missing_bones);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_setup_on_ready"), "set_auto_setup_on_ready", "is_auto_setup_on_ready");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_hand_ik"), "set_create_hand_ik", "is_hand_ik_created");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_foot_ik"), "set_create_foot_ik", "is_foot_ik_created");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_head_look_at"), "set_create_head_look_at", "is_head_look_at_created");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "control_distance_scale", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater"), "set_control_distance_scale", "get_control_distance_scale");
}

PackedStringArray HumanoidControlRig3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!_get_parent_skeleton()) {
		warnings.push_back(RTR("HumanoidControlRig3D must be a child of a Skeleton3D."));
	}
	if (!missing_bones.is_empty()) {
		warnings.push_back(vformat(RTR("Some requested humanoid rig bones were not found: %s."), String(", ").join(missing_bones)));
	}
	return warnings;
}

void HumanoidControlRig3D::set_auto_setup_on_ready(bool p_enabled) {
	auto_setup_on_ready = p_enabled;
}

bool HumanoidControlRig3D::is_auto_setup_on_ready() const {
	return auto_setup_on_ready;
}

void HumanoidControlRig3D::set_create_hand_ik(bool p_enabled) {
	create_hand_ik = p_enabled;
}

bool HumanoidControlRig3D::is_hand_ik_created() const {
	return create_hand_ik;
}

void HumanoidControlRig3D::set_create_foot_ik(bool p_enabled) {
	create_foot_ik = p_enabled;
}

bool HumanoidControlRig3D::is_foot_ik_created() const {
	return create_foot_ik;
}

void HumanoidControlRig3D::set_create_head_look_at(bool p_enabled) {
	create_head_look_at = p_enabled;
}

bool HumanoidControlRig3D::is_head_look_at_created() const {
	return create_head_look_at;
}

void HumanoidControlRig3D::set_control_distance_scale(float p_scale) {
	control_distance_scale = MAX(0.001f, p_scale);
}

float HumanoidControlRig3D::get_control_distance_scale() const {
	return control_distance_scale;
}

void HumanoidControlRig3D::setup_from_skeleton() {
	Skeleton3D *skeleton = _get_parent_skeleton();
	ERR_FAIL_NULL_MSG(skeleton, "HumanoidControlRig3D must be a child of a Skeleton3D.");

	clear_generated_rig();
	missing_bones.clear();

	const int upper_arm_l = _find_bone(skeleton, { "upper_arm_l", "left_upper_arm", "upperarm_l", "LeftArm", "mixamorig:LeftArm" });
	const int lower_arm_l = _find_bone(skeleton, { "lower_arm_l", "left_lower_arm", "lowerarm_l", "LeftForeArm", "mixamorig:LeftForeArm" });
	const int hand_l = _find_bone(skeleton, { "hand_l", "left_hand", "LeftHand", "mixamorig:LeftHand" });
	const int upper_arm_r = _find_bone(skeleton, { "upper_arm_r", "right_upper_arm", "upperarm_r", "RightArm", "mixamorig:RightArm" });
	const int lower_arm_r = _find_bone(skeleton, { "lower_arm_r", "right_lower_arm", "lowerarm_r", "RightForeArm", "mixamorig:RightForeArm" });
	const int hand_r = _find_bone(skeleton, { "hand_r", "right_hand", "RightHand", "mixamorig:RightHand" });
	const int thigh_l = _find_bone(skeleton, { "thigh_l", "left_upper_leg", "upper_leg_l", "LeftUpLeg", "mixamorig:LeftUpLeg" });
	const int calf_l = _find_bone(skeleton, { "calf_l", "left_lower_leg", "lower_leg_l", "LeftLeg", "mixamorig:LeftLeg" });
	const int foot_l = _find_bone(skeleton, { "foot_l", "left_foot", "LeftFoot", "mixamorig:LeftFoot" });
	const int thigh_r = _find_bone(skeleton, { "thigh_r", "right_upper_leg", "upper_leg_r", "RightUpLeg", "mixamorig:RightUpLeg" });
	const int calf_r = _find_bone(skeleton, { "calf_r", "right_lower_leg", "lower_leg_r", "RightLeg", "mixamorig:RightLeg" });
	const int foot_r = _find_bone(skeleton, { "foot_r", "right_foot", "RightFoot", "mixamorig:RightFoot" });
	const int head = _find_bone(skeleton, { "head", "Head", "mixamorig:Head" });

	Vector<BoneChain> arm_chains;
	if (create_hand_ik) {
		if (upper_arm_l >= 0 && lower_arm_l >= 0 && hand_l >= 0) {
			_create_control("hand_l", skeleton->get_bone_global_pose(hand_l).origin);
			_create_control("elbow_l", skeleton->get_bone_global_pose(lower_arm_l).origin + Vector3(-0.35f, 0.0f, -0.35f) * control_distance_scale);
			arm_chains.push_back({ skeleton->get_bone_name(upper_arm_l), skeleton->get_bone_name(lower_arm_l), skeleton->get_bone_name(hand_l), "hand_l", "elbow_l", Vector3(-1, 0, -1) });
		} else {
			_push_missing("left arm IK chain");
		}
		if (upper_arm_r >= 0 && lower_arm_r >= 0 && hand_r >= 0) {
			_create_control("hand_r", skeleton->get_bone_global_pose(hand_r).origin);
			_create_control("elbow_r", skeleton->get_bone_global_pose(lower_arm_r).origin + Vector3(0.35f, 0.0f, -0.35f) * control_distance_scale);
			arm_chains.push_back({ skeleton->get_bone_name(upper_arm_r), skeleton->get_bone_name(lower_arm_r), skeleton->get_bone_name(hand_r), "hand_r", "elbow_r", Vector3(1, 0, -1) });
		} else {
			_push_missing("right arm IK chain");
		}
		_setup_two_bone_ik(skeleton, "ArmIK", arm_chains);
	}

	Vector<BoneChain> leg_chains;
	if (create_foot_ik) {
		if (thigh_l >= 0 && calf_l >= 0 && foot_l >= 0) {
			_create_control("foot_l", skeleton->get_bone_global_pose(foot_l).origin);
			_create_control("knee_l", skeleton->get_bone_global_pose(calf_l).origin + Vector3(-0.2f, 0.0f, 0.55f) * control_distance_scale);
			leg_chains.push_back({ skeleton->get_bone_name(thigh_l), skeleton->get_bone_name(calf_l), skeleton->get_bone_name(foot_l), "foot_l", "knee_l", Vector3(-1, 0, 1) });
		} else {
			_push_missing("left leg IK chain");
		}
		if (thigh_r >= 0 && calf_r >= 0 && foot_r >= 0) {
			_create_control("foot_r", skeleton->get_bone_global_pose(foot_r).origin);
			_create_control("knee_r", skeleton->get_bone_global_pose(calf_r).origin + Vector3(0.2f, 0.0f, 0.55f) * control_distance_scale);
			leg_chains.push_back({ skeleton->get_bone_name(thigh_r), skeleton->get_bone_name(calf_r), skeleton->get_bone_name(foot_r), "foot_r", "knee_r", Vector3(1, 0, 1) });
		} else {
			_push_missing("right leg IK chain");
		}
		_setup_two_bone_ik(skeleton, "LegIK", leg_chains);
	}

	if (create_head_look_at) {
		if (head >= 0) {
			_create_control("head_look", skeleton->get_bone_global_pose(head).origin + Vector3(0.0f, 0.0f, 1.0f) * control_distance_scale);
			_setup_head_look_at(skeleton, head);
		} else {
			_push_missing("head");
		}
	}

	update_configuration_warnings();
}

void HumanoidControlRig3D::clear_generated_rig() {
	Skeleton3D *skeleton = _get_parent_skeleton();
	if (skeleton) {
		for (int i = skeleton->get_child_count() - 1; i >= 0; i--) {
			Node *child = skeleton->get_child(i);
			if (child != this && String(child->get_name()).begins_with(GENERATED_PREFIX)) {
				skeleton->remove_child(child);
				memdelete(child);
			}
		}
	}

	for (int i = get_child_count() - 1; i >= 0; i--) {
		Node *child = get_child(i);
		if (String(child->get_name()).begins_with(GENERATED_PREFIX)) {
			remove_child(child);
			memdelete(child);
		}
	}
}

Node *HumanoidControlRig3D::get_control_node(const StringName &p_control_name) const {
	const String generated_name = String(GENERATED_PREFIX) + String(p_control_name);
	for (int i = 0; i < get_child_count(); i++) {
		Node *child = get_child(i);
		if (child->get_name() == generated_name) {
			return child;
		}
	}
	return nullptr;
}

PackedStringArray HumanoidControlRig3D::get_missing_bones() const {
	return missing_bones;
}
