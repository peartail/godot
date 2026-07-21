/**************************************************************************/
/*  open_world_rock_variant_library.h                                     */
/**************************************************************************/
#pragma once

#include "open_world_rock_variant.h"

class OpenWorldRockVariantLibrary : public Resource {
	GDCLASS(OpenWorldRockVariantLibrary, Resource);
	RES_BASE_EXTENSION("owrocklibrary");

	PackedStringArray biome_names;
	Array variants;

protected:
	static void _bind_methods();

public:
	void set_biome_names(const PackedStringArray &p_value); PackedStringArray get_biome_names() const { return biome_names; }
	void set_variants(const Array &p_value); Array get_variants() const { return variants; }
	void add_variant(const String &p_biome, const Ref<OpenWorldRockVariant> &p_variant);
	int get_variant_count() const { return variants.size(); }
	Ref<OpenWorldRockVariant> get_variant(int p_index) const;
	Ref<OpenWorldRockVariant> select_variant(const String &p_biome, int p_seed) const;
	Dictionary validate_library() const;
};
