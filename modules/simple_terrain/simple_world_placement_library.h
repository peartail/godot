/**************************************************************************/
/*  simple_world_placement_library.h                                      */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"

class SimpleWorldObjectProfile;

class SimpleWorldPlacementLibrary : public Resource {
	GDCLASS(SimpleWorldPlacementLibrary, Resource);
	RES_BASE_EXTENSION("swplacementlibrary");

	Array profiles;

protected:
	static void _bind_methods();

public:
	void set_profiles(const Array &p_profiles);
	Array get_profiles() const { return profiles; }

	int get_profile_count() const { return profiles.size(); }
	Ref<SimpleWorldObjectProfile> get_profile(int p_index) const;
	Ref<SimpleWorldObjectProfile> get_profile_by_id(const String &p_id) const;
	int find_profile_index(const String &p_id) const;
	bool has_profile_id(const String &p_id) const;
	void add_profile(const Ref<SimpleWorldObjectProfile> &p_profile);
	void remove_profile_at(int p_index);
	void remove_profile_by_id(const String &p_id);
	void clear_profiles();
};
