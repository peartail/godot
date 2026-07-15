/**************************************************************************/
/*  open_world_tree_species.cpp                                           */
/**************************************************************************/

#include "open_world_tree_species.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

void OpenWorldTreeSpecies::_variant_changed() {
	emit_changed();
}

void OpenWorldTreeSpecies::_disconnect_variants() {
	for (int i = 0; i < variants.size(); i++) {
		Ref<OpenWorldTreeVariant> variant = variants[i];
		if (variant.is_valid()) {
			variant->disconnect_changed(callable_mp(this, &OpenWorldTreeSpecies::_variant_changed));
		}
	}
}

void OpenWorldTreeSpecies::_connect_variants() {
	for (int i = 0; i < variants.size(); i++) {
		Ref<OpenWorldTreeVariant> variant = variants[i];
		if (variant.is_valid()) {
			variant->connect_changed(callable_mp(this, &OpenWorldTreeSpecies::_variant_changed), CONNECT_REFERENCE_COUNTED);
		}
	}
}

void OpenWorldTreeSpecies::set_species_id(const String &p_id) {
	if (species_id == p_id) {
		return;
	}
	species_id = p_id;
	emit_changed();
}

void OpenWorldTreeSpecies::set_display_name(const String &p_name) {
	if (display_name == p_name) {
		return;
	}
	display_name = p_name;
	emit_changed();
}

void OpenWorldTreeSpecies::set_variants(const Array &p_variants) {
	_disconnect_variants();
	variants.clear();
	for (int i = 0; i < p_variants.size(); i++) {
		Ref<OpenWorldTreeVariant> variant = p_variants[i];
		if (variant.is_valid()) {
			variants.push_back(variant);
		}
	}
	_connect_variants();
	emit_changed();
}

Ref<OpenWorldTreeVariant> OpenWorldTreeSpecies::get_variant(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, variants.size(), Ref<OpenWorldTreeVariant>());
	return variants[p_index];
}

int OpenWorldTreeSpecies::get_variant_index_for_seed(int p_seed) const {
	if (variants.is_empty()) {
		return -1;
	}
	uint32_t value = (uint32_t)p_seed;
	value ^= value >> 16;
	value *= 0x7feb352dU;
	value ^= value >> 15;
	value *= 0x846ca68bU;
	value ^= value >> 16;
	return value % variants.size();
}

void OpenWorldTreeSpecies::set_tags(const PackedStringArray &p_tags) {
	if (tags == p_tags) {
		return;
	}
	tags = p_tags;
	emit_changed();
}

OpenWorldTreeSpecies::~OpenWorldTreeSpecies() {
	_disconnect_variants();
}

void OpenWorldTreeSpecies::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_species_id", "id"), &OpenWorldTreeSpecies::set_species_id);
	ClassDB::bind_method(D_METHOD("get_species_id"), &OpenWorldTreeSpecies::get_species_id);
	ClassDB::bind_method(D_METHOD("set_display_name", "name"), &OpenWorldTreeSpecies::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &OpenWorldTreeSpecies::get_display_name);
	ClassDB::bind_method(D_METHOD("set_variants", "variants"), &OpenWorldTreeSpecies::set_variants);
	ClassDB::bind_method(D_METHOD("get_variants"), &OpenWorldTreeSpecies::get_variants);
	ClassDB::bind_method(D_METHOD("get_variant_count"), &OpenWorldTreeSpecies::get_variant_count);
	ClassDB::bind_method(D_METHOD("get_variant", "index"), &OpenWorldTreeSpecies::get_variant);
	ClassDB::bind_method(D_METHOD("get_variant_index_for_seed", "seed"), &OpenWorldTreeSpecies::get_variant_index_for_seed);
	ClassDB::bind_method(D_METHOD("set_tags", "tags"), &OpenWorldTreeSpecies::set_tags);
	ClassDB::bind_method(D_METHOD("get_tags"), &OpenWorldTreeSpecies::get_tags);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "species_id"), "set_species_id", "get_species_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "variants", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("OpenWorldTreeVariant")), "set_variants", "get_variants");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tags"), "set_tags", "get_tags");
}
