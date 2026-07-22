/**************************************************************************/
/*  open_world_placement_preset.cpp                                 */
/**************************************************************************/

#include "open_world_placement_preset.h"

#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/templates/hash_set.h"

#define PLACEMENT_PRESET_SETTER(type, name, expression) \
	void OpenWorldPlacementPreset::set_##name(type p_value) { \
		type value = expression; \
		if (name == value) { \
			return; \
		} \
		name = value; \
		emit_changed(); \
	}

PLACEMENT_PRESET_SETTER(String, stable_id, p_value.strip_edges());
PLACEMENT_PRESET_SETTER(String, display_name, p_value);
PLACEMENT_PRESET_SETTER(PlacementShape, shape, (PlacementShape)CLAMP((int)p_value, 0, 2));
PLACEMENT_PRESET_SETTER(Vector2, size, Vector2(MAX((real_t)0.01, Math::abs(p_value.x)), MAX((real_t)0.01, Math::abs(p_value.y))));
PLACEMENT_PRESET_SETTER(real_t, yaw_degrees, Math::fposmod(p_value, (real_t)360.0));
PLACEMENT_PRESET_SETTER(real_t, density_per_100_square_meters, MAX((real_t)0.0, p_value));
PLACEMENT_PRESET_SETTER(real_t, minimum_spacing, MAX((real_t)0.0, p_value));
PLACEMENT_PRESET_SETTER(real_t, height_min, p_value);
PLACEMENT_PRESET_SETTER(real_t, height_max, p_value);
PLACEMENT_PRESET_SETTER(real_t, slope_min_degrees, CLAMP(p_value, (real_t)0.0, (real_t)180.0));
PLACEMENT_PRESET_SETTER(real_t, slope_max_degrees, CLAMP(p_value, (real_t)0.0, (real_t)180.0));
PLACEMENT_PRESET_SETTER(int, max_objects_per_operation, MAX(1, p_value));

#undef PLACEMENT_PRESET_SETTER

void OpenWorldPlacementPreset::set_entries(const Array &p_value) {
	entries = p_value;
	emit_changed();
}

void OpenWorldPlacementPreset::add_entry(const Ref<OpenWorldPlacementEntry> &p_entry) {
	ERR_FAIL_COND(p_entry.is_null());
	entries.push_back(p_entry);
	emit_changed();
}

void OpenWorldPlacementPreset::remove_entry_at(int p_index) {
	ERR_FAIL_INDEX(p_index, entries.size());
	entries.remove_at(p_index);
	emit_changed();
}

Ref<OpenWorldPlacementEntry> OpenWorldPlacementPreset::get_entry(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, entries.size(), Ref<OpenWorldPlacementEntry>());
	return entries[p_index];
}

real_t OpenWorldPlacementPreset::get_footprint_area() const {
	switch (shape) {
		case SHAPE_CIRCLE: {
			const real_t radius = size.x * 0.5;
			return Math::PI * radius * radius;
		}
		case SHAPE_ELLIPSE:
			return Math::PI * size.x * size.y * 0.25;
		case SHAPE_RECTANGLE:
		default:
			return size.x * size.y;
	}
}

int OpenWorldPlacementPreset::get_requested_object_count() const {
	return MAX(0, (int)Math::round(get_footprint_area() * density_per_100_square_meters / 100.0));
}

Dictionary OpenWorldPlacementPreset::validate_preset() const {
	Dictionary report;
	PackedStringArray errors;
	PackedStringArray error_codes;
	PackedStringArray warnings;
	PackedStringArray warning_codes;
	auto add_error = [&errors, &error_codes](const String &p_code, const String &p_message) {
		error_codes.push_back(p_code);
		errors.push_back(p_code + ": " + p_message);
	};
	if (stable_id.is_empty()) {
		add_error("STABLE_ID_MISSING", "preset stable_id is required.");
	}
	if (height_min > height_max) {
		add_error("HEIGHT_RANGE_INVALID", "height_min must not exceed height_max.");
	}
	if (slope_min_degrees > slope_max_degrees) {
		add_error("SLOPE_RANGE_INVALID", "slope_min_degrees must not exceed slope_max_degrees.");
	}
	real_t total_weight = 0.0;
	HashSet<String> ids;
	for (int i = 0; i < entries.size(); i++) {
		Ref<OpenWorldPlacementEntry> entry = entries[i];
		if (entry.is_null()) {
			add_error("ENTRY_MISSING", vformat("entry %d is null.", i));
			continue;
		}
		Dictionary entry_report = entry->validate_entry();
		if (!(bool)entry_report["success"]) {
			PackedStringArray codes = entry_report["error_codes"];
			for (const String &code : codes) {
				add_error("ENTRY_" + code, vformat("entry %d (%s) failed %s.", i, entry->get_stable_id(), code));
			}
		}
		if (ids.has(entry->get_stable_id())) {
			add_error("ENTRY_ID_DUPLICATE", vformat("entry stable_id '%s' is duplicated.", entry->get_stable_id()));
		}
		ids.insert(entry->get_stable_id());
		if (entry->is_enabled()) {
			total_weight += entry->get_weight();
		}
	}
	if (total_weight <= 0.0) {
		add_error("NO_WEIGHTED_ENTRIES", "at least one enabled entry with positive weight is required.");
	}
	const int requested_count = get_requested_object_count();
	if (requested_count > max_objects_per_operation) {
		add_error("MAX_OBJECTS_EXCEEDED", vformat("requested %d objects exceeds the operation limit %d; reduce footprint size or density, or explicitly raise the preset limit.", requested_count, max_objects_per_operation));
	}
	report["success"] = errors.is_empty();
	report["errors"] = errors;
	report["error_codes"] = error_codes;
	report["warnings"] = warnings;
	report["warning_codes"] = warning_codes;
	report["error_count"] = errors.size();
	report["footprint_area"] = get_footprint_area();
	report["density_per_100_square_meters"] = density_per_100_square_meters;
	report["requested_count"] = requested_count;
	report["max_objects_per_operation"] = max_objects_per_operation;
	report["total_weight"] = total_weight;
	return report;
}

void OpenWorldPlacementPreset::_bind_methods() {
#define BIND_ACCESSOR(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldPlacementPreset::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldPlacementPreset::get_##name)
	BIND_ACCESSOR(stable_id);
	BIND_ACCESSOR(display_name);
	BIND_ACCESSOR(shape);
	BIND_ACCESSOR(size);
	BIND_ACCESSOR(yaw_degrees);
	BIND_ACCESSOR(density_per_100_square_meters);
	BIND_ACCESSOR(minimum_spacing);
	BIND_ACCESSOR(height_min);
	BIND_ACCESSOR(height_max);
	BIND_ACCESSOR(slope_min_degrees);
	BIND_ACCESSOR(slope_max_degrees);
	BIND_ACCESSOR(max_objects_per_operation);
	BIND_ACCESSOR(entries);
#undef BIND_ACCESSOR
	ClassDB::bind_method(D_METHOD("add_entry", "entry"), &OpenWorldPlacementPreset::add_entry);
	ClassDB::bind_method(D_METHOD("remove_entry_at", "index"), &OpenWorldPlacementPreset::remove_entry_at);
	ClassDB::bind_method(D_METHOD("get_entry_count"), &OpenWorldPlacementPreset::get_entry_count);
	ClassDB::bind_method(D_METHOD("get_entry", "index"), &OpenWorldPlacementPreset::get_entry);
	ClassDB::bind_method(D_METHOD("get_footprint_area"), &OpenWorldPlacementPreset::get_footprint_area);
	ClassDB::bind_method(D_METHOD("get_requested_object_count"), &OpenWorldPlacementPreset::get_requested_object_count);
	ClassDB::bind_method(D_METHOD("validate_preset"), &OpenWorldPlacementPreset::validate_preset);

	BIND_ENUM_CONSTANT(SHAPE_CIRCLE);
	BIND_ENUM_CONSTANT(SHAPE_RECTANGLE);
	BIND_ENUM_CONSTANT(SHAPE_ELLIPSE);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "stable_id"), "set_stable_id", "get_stable_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "shape", PROPERTY_HINT_ENUM, "Circle,Rectangle,Ellipse"), "set_shape", "get_shape");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size", PROPERTY_HINT_NONE, "suffix:m"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "yaw_degrees", PROPERTY_HINT_RANGE, "-360,360,0.1,radians_as_degrees"), "set_yaw_degrees", "get_yaw_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density_per_100_square_meters", PROPERTY_HINT_RANGE, "0,10000,0.1,or_greater"), "set_density_per_100_square_meters", "get_density_per_100_square_meters");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "minimum_spacing", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_minimum_spacing", "get_minimum_spacing");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_min", PROPERTY_HINT_RANGE, "-1000000,1000000,0.1,suffix:m"), "set_height_min", "get_height_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_max", PROPERTY_HINT_RANGE, "-1000000,1000000,0.1,suffix:m"), "set_height_max", "get_height_max");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_min_degrees", PROPERTY_HINT_RANGE, "0,180,0.1,degrees"), "set_slope_min_degrees", "get_slope_min_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_max_degrees", PROPERTY_HINT_RANGE, "0,180,0.1,degrees"), "set_slope_max_degrees", "get_slope_max_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_objects_per_operation", PROPERTY_HINT_RANGE, "1,100000,1,or_greater"), "set_max_objects_per_operation", "get_max_objects_per_operation");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_ARRAY_TYPE, "OpenWorldPlacementEntry"), "set_entries", "get_entries");
}
