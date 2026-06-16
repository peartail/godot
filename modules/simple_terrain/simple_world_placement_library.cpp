/**************************************************************************/
/*  simple_world_placement_library.cpp                                    */
/**************************************************************************/

#include "simple_world_placement_library.h"

#include "simple_world_object_profile.h"

#include "core/object/class_db.h"

void SimpleWorldPlacementLibrary::set_profiles(const Array &p_profiles) {
	profiles.clear();
	for (int i = 0; i < p_profiles.size(); i++) {
		Ref<SimpleWorldObjectProfile> profile = p_profiles[i];
		if (profile.is_valid()) {
			profiles.push_back(profile);
		}
	}
	emit_changed();
}

Ref<SimpleWorldObjectProfile> SimpleWorldPlacementLibrary::get_profile(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, profiles.size(), Ref<SimpleWorldObjectProfile>());
	return profiles[p_index];
}

Ref<SimpleWorldObjectProfile> SimpleWorldPlacementLibrary::get_profile_by_id(const String &p_id) const {
	const int index = find_profile_index(p_id);
	if (index < 0) {
		return Ref<SimpleWorldObjectProfile>();
	}
	return profiles[index];
}

int SimpleWorldPlacementLibrary::find_profile_index(const String &p_id) const {
	for (int i = 0; i < profiles.size(); i++) {
		Ref<SimpleWorldObjectProfile> profile = profiles[i];
		if (profile.is_valid() && profile->get_id() == p_id) {
			return i;
		}
	}
	return -1;
}

bool SimpleWorldPlacementLibrary::has_profile_id(const String &p_id) const {
	return find_profile_index(p_id) >= 0;
}

void SimpleWorldPlacementLibrary::add_profile(const Ref<SimpleWorldObjectProfile> &p_profile) {
	ERR_FAIL_COND(p_profile.is_null());
	profiles.push_back(p_profile);
	emit_changed();
}

void SimpleWorldPlacementLibrary::remove_profile_at(int p_index) {
	ERR_FAIL_INDEX(p_index, profiles.size());
	profiles.remove_at(p_index);
	emit_changed();
}

void SimpleWorldPlacementLibrary::remove_profile_by_id(const String &p_id) {
	const int index = find_profile_index(p_id);
	if (index < 0) {
		return;
	}
	remove_profile_at(index);
}

void SimpleWorldPlacementLibrary::clear_profiles() {
	if (profiles.is_empty()) {
		return;
	}
	profiles.clear();
	emit_changed();
}

void SimpleWorldPlacementLibrary::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_profiles", "profiles"), &SimpleWorldPlacementLibrary::set_profiles);
	ClassDB::bind_method(D_METHOD("get_profiles"), &SimpleWorldPlacementLibrary::get_profiles);
	ClassDB::bind_method(D_METHOD("get_profile_count"), &SimpleWorldPlacementLibrary::get_profile_count);
	ClassDB::bind_method(D_METHOD("get_profile", "index"), &SimpleWorldPlacementLibrary::get_profile);
	ClassDB::bind_method(D_METHOD("get_profile_by_id", "id"), &SimpleWorldPlacementLibrary::get_profile_by_id);
	ClassDB::bind_method(D_METHOD("find_profile_index", "id"), &SimpleWorldPlacementLibrary::find_profile_index);
	ClassDB::bind_method(D_METHOD("has_profile_id", "id"), &SimpleWorldPlacementLibrary::has_profile_id);
	ClassDB::bind_method(D_METHOD("add_profile", "profile"), &SimpleWorldPlacementLibrary::add_profile);
	ClassDB::bind_method(D_METHOD("remove_profile_at", "index"), &SimpleWorldPlacementLibrary::remove_profile_at);
	ClassDB::bind_method(D_METHOD("remove_profile_by_id", "id"), &SimpleWorldPlacementLibrary::remove_profile_by_id);
	ClassDB::bind_method(D_METHOD("clear_profiles"), &SimpleWorldPlacementLibrary::clear_profiles);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "profiles", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("SimpleWorldObjectProfile")), "set_profiles", "get_profiles");
}
