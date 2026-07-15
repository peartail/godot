/**************************************************************************/
/*  open_world_tree_species.h                                             */
/**************************************************************************/

#pragma once

#include "open_world_tree_variant.h"

#include "core/io/resource.h"

class OpenWorldTreeSpecies : public Resource {
	GDCLASS(OpenWorldTreeSpecies, Resource);
	RES_BASE_EXTENSION("owtreespecies");

	String species_id;
	String display_name = "Tree Species";
	Array variants;
	PackedStringArray tags;

	void _variant_changed();
	void _disconnect_variants();
	void _connect_variants();

protected:
	static void _bind_methods();

public:
	void set_species_id(const String &p_id);
	String get_species_id() const { return species_id; }

	void set_display_name(const String &p_name);
	String get_display_name() const { return display_name; }

	void set_variants(const Array &p_variants);
	Array get_variants() const { return variants; }
	int get_variant_count() const { return variants.size(); }
	Ref<OpenWorldTreeVariant> get_variant(int p_index) const;
	int get_variant_index_for_seed(int p_seed) const;

	void set_tags(const PackedStringArray &p_tags);
	PackedStringArray get_tags() const { return tags; }

	~OpenWorldTreeSpecies();
};
