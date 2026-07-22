/**************************************************************************/
/*  open_world_placement_brush_entry.cpp                                  */
/**************************************************************************/

#include "open_world_placement_brush_entry.h"

#include "core/object/class_db.h"

#define PLACEMENT_ENTRY_SETTER(type, name, expression) \
	void OpenWorldPlacementBrushEntry::set_##name(type p_value) { \
		type value = expression; \
		if (name == value) { \
			return; \
		} \
		name = value; \
		emit_changed(); \
	}

PLACEMENT_ENTRY_SETTER(String, stable_id, p_value.strip_edges());
PLACEMENT_ENTRY_SETTER(ContentKind, content_kind, (ContentKind)CLAMP((int)p_value, 0, 2));
PLACEMENT_ENTRY_SETTER(bool, enabled, p_value);
PLACEMENT_ENTRY_SETTER(real_t, weight, MAX((real_t)0.0, p_value));
PLACEMENT_ENTRY_SETTER(Vector3, min_scale, p_value);
PLACEMENT_ENTRY_SETTER(Vector3, max_scale, p_value);
PLACEMENT_ENTRY_SETTER(bool, random_yaw, p_value);
PLACEMENT_ENTRY_SETTER(bool, align_to_surface_normal, p_value);
PLACEMENT_ENTRY_SETTER(real_t, surface_offset, p_value);
PLACEMENT_ENTRY_SETTER(real_t, minimum_spacing_override, MAX((real_t)0.0, p_value));

#undef PLACEMENT_ENTRY_SETTER

void OpenWorldPlacementBrushEntry::set_tree_profile(const Ref<OpenWorldTreeGenerationProfile> &p_value) {
	if (tree_profile == p_value) {
		return;
	}
	tree_profile = p_value;
	emit_changed();
}

void OpenWorldPlacementBrushEntry::set_vine_request_template(const Ref<OpenWorldVineGenerationRequest> &p_value) {
	if (vine_request_template == p_value) {
		return;
	}
	vine_request_template = p_value;
	emit_changed();
}

void OpenWorldPlacementBrushEntry::set_rock_request_template(const Ref<OpenWorldRockGenerationRequest> &p_value) {
	if (rock_request_template == p_value) {
		return;
	}
	rock_request_template = p_value;
	emit_changed();
}

Dictionary OpenWorldPlacementBrushEntry::validate_entry() const {
	Dictionary report;
	PackedStringArray errors;
	PackedStringArray error_codes;
	auto add_error = [&errors, &error_codes](const String &p_code, const String &p_message) {
		error_codes.push_back(p_code);
		errors.push_back(p_code + ": " + p_message);
	};
	if (stable_id.is_empty()) {
		add_error("STABLE_ID_MISSING", "entry stable_id is required.");
	}
	if (enabled && weight <= 0.0) {
		add_error("WEIGHT_INVALID", "enabled entries require a positive weight.");
	}
	if (min_scale.x <= 0.0 || min_scale.y <= 0.0 || min_scale.z <= 0.0 || max_scale.x <= 0.0 || max_scale.y <= 0.0 || max_scale.z <= 0.0) {
		add_error("SCALE_INVALID", "scale components must be positive.");
	}
	switch (content_kind) {
		case CONTENT_TREE:
			if (tree_profile.is_null()) {
				add_error("TREE_PROFILE_MISSING", "tree entries require tree_profile.");
			}
			break;
		case CONTENT_VINE:
			if (vine_request_template.is_null()) {
				add_error("VINE_REQUEST_MISSING", "vine entries require vine_request_template.");
			} else if (vine_request_template->get_mode() != OpenWorldVineGenerationRequest::MODE_BRAMBLE) {
				add_error("VINE_MODE_PHASE2", "Phase 1 placement supports MODE_BRAMBLE only.");
			} else if (vine_request_template->get_profile().is_null()) {
				add_error("VINE_PROFILE_MISSING", "vine request template requires a profile.");
			}
			break;
		case CONTENT_ROCK:
			if (rock_request_template.is_null()) {
				add_error("ROCK_REQUEST_MISSING", "rock entries require rock_request_template.");
			} else if (rock_request_template->get_profile().is_null()) {
				add_error("ROCK_PROFILE_MISSING", "rock request template requires a profile.");
			}
			break;
	}
	report["success"] = errors.is_empty();
	report["errors"] = errors;
	report["error_codes"] = error_codes;
	report["error_count"] = errors.size();
	return report;
}

void OpenWorldPlacementBrushEntry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_stable_id", "value"), &OpenWorldPlacementBrushEntry::set_stable_id);
	ClassDB::bind_method(D_METHOD("get_stable_id"), &OpenWorldPlacementBrushEntry::get_stable_id);
	ClassDB::bind_method(D_METHOD("set_content_kind", "value"), &OpenWorldPlacementBrushEntry::set_content_kind);
	ClassDB::bind_method(D_METHOD("get_content_kind"), &OpenWorldPlacementBrushEntry::get_content_kind);
	ClassDB::bind_method(D_METHOD("set_enabled", "value"), &OpenWorldPlacementBrushEntry::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &OpenWorldPlacementBrushEntry::is_enabled);
	ClassDB::bind_method(D_METHOD("set_weight", "value"), &OpenWorldPlacementBrushEntry::set_weight);
	ClassDB::bind_method(D_METHOD("get_weight"), &OpenWorldPlacementBrushEntry::get_weight);
	ClassDB::bind_method(D_METHOD("set_min_scale", "value"), &OpenWorldPlacementBrushEntry::set_min_scale);
	ClassDB::bind_method(D_METHOD("get_min_scale"), &OpenWorldPlacementBrushEntry::get_min_scale);
	ClassDB::bind_method(D_METHOD("set_max_scale", "value"), &OpenWorldPlacementBrushEntry::set_max_scale);
	ClassDB::bind_method(D_METHOD("get_max_scale"), &OpenWorldPlacementBrushEntry::get_max_scale);
	ClassDB::bind_method(D_METHOD("set_random_yaw", "value"), &OpenWorldPlacementBrushEntry::set_random_yaw);
	ClassDB::bind_method(D_METHOD("is_random_yaw_enabled"), &OpenWorldPlacementBrushEntry::is_random_yaw_enabled);
	ClassDB::bind_method(D_METHOD("set_align_to_surface_normal", "value"), &OpenWorldPlacementBrushEntry::set_align_to_surface_normal);
	ClassDB::bind_method(D_METHOD("is_aligning_to_surface_normal"), &OpenWorldPlacementBrushEntry::is_aligning_to_surface_normal);
	ClassDB::bind_method(D_METHOD("set_surface_offset", "value"), &OpenWorldPlacementBrushEntry::set_surface_offset);
	ClassDB::bind_method(D_METHOD("get_surface_offset"), &OpenWorldPlacementBrushEntry::get_surface_offset);
	ClassDB::bind_method(D_METHOD("set_minimum_spacing_override", "value"), &OpenWorldPlacementBrushEntry::set_minimum_spacing_override);
	ClassDB::bind_method(D_METHOD("get_minimum_spacing_override"), &OpenWorldPlacementBrushEntry::get_minimum_spacing_override);
	ClassDB::bind_method(D_METHOD("set_tree_profile", "value"), &OpenWorldPlacementBrushEntry::set_tree_profile);
	ClassDB::bind_method(D_METHOD("get_tree_profile"), &OpenWorldPlacementBrushEntry::get_tree_profile);
	ClassDB::bind_method(D_METHOD("set_vine_request_template", "value"), &OpenWorldPlacementBrushEntry::set_vine_request_template);
	ClassDB::bind_method(D_METHOD("get_vine_request_template"), &OpenWorldPlacementBrushEntry::get_vine_request_template);
	ClassDB::bind_method(D_METHOD("set_rock_request_template", "value"), &OpenWorldPlacementBrushEntry::set_rock_request_template);
	ClassDB::bind_method(D_METHOD("get_rock_request_template"), &OpenWorldPlacementBrushEntry::get_rock_request_template);
	ClassDB::bind_method(D_METHOD("validate_entry"), &OpenWorldPlacementBrushEntry::validate_entry);

	BIND_ENUM_CONSTANT(CONTENT_TREE);
	BIND_ENUM_CONSTANT(CONTENT_VINE);
	BIND_ENUM_CONSTANT(CONTENT_ROCK);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "stable_id"), "set_stable_id", "get_stable_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "content_kind", PROPERTY_HINT_ENUM, "Tree,Vine,Rocks"), "set_content_kind", "get_content_kind");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"), "set_weight", "get_weight");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "min_scale"), "set_min_scale", "get_min_scale");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "max_scale"), "set_max_scale", "get_max_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_yaw"), "set_random_yaw", "is_random_yaw_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "align_to_surface_normal"), "set_align_to_surface_normal", "is_aligning_to_surface_normal");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_offset", PROPERTY_HINT_RANGE, "-100,100,0.01,suffix:m"), "set_surface_offset", "get_surface_offset");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "minimum_spacing_override", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_minimum_spacing_override", "get_minimum_spacing_override");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tree_profile", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldTreeGenerationProfile"), "set_tree_profile", "get_tree_profile");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "vine_request_template", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldVineGenerationRequest"), "set_vine_request_template", "get_vine_request_template");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "rock_request_template", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldRockGenerationRequest"), "set_rock_request_template", "get_rock_request_template");
}
