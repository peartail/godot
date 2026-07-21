/**************************************************************************/
/*  open_world_rock_variant_library.cpp                                   */
/**************************************************************************/
#include "open_world_rock_variant_library.h"

#include "core/object/class_db.h"

void OpenWorldRockVariantLibrary::set_biome_names(const PackedStringArray &p_value) { biome_names = p_value; emit_changed(); }
void OpenWorldRockVariantLibrary::set_variants(const Array &p_value) { variants = p_value; emit_changed(); }
void OpenWorldRockVariantLibrary::add_variant(const String &p_biome, const Ref<OpenWorldRockVariant> &p_variant) { ERR_FAIL_COND(p_variant.is_null()); biome_names.push_back(p_biome); variants.push_back(p_variant); emit_changed(); }
Ref<OpenWorldRockVariant> OpenWorldRockVariantLibrary::get_variant(int p_index) const { ERR_FAIL_INDEX_V(p_index, variants.size(), Ref<OpenWorldRockVariant>()); return variants[p_index]; }
Ref<OpenWorldRockVariant> OpenWorldRockVariantLibrary::select_variant(const String &p_biome, int p_seed) const {
	Vector<int> candidates; for (int i = 0; i < variants.size() && i < biome_names.size(); i++) if (biome_names[i] == p_biome && ((Ref<OpenWorldRockVariant>)variants[i]).is_valid()) candidates.push_back(i);
	if (candidates.is_empty()) return Ref<OpenWorldRockVariant>(); uint32_t mixed = (uint32_t)p_seed * 747796405u + 2891336453u; return variants[candidates[mixed % candidates.size()]];
}
Dictionary OpenWorldRockVariantLibrary::validate_library() const {
	Dictionary result; PackedStringArray errors; if (biome_names.size() != variants.size()) errors.push_back("ENTRY_COUNT_MISMATCH");
	for (int i = 0; i < variants.size(); i++) { Ref<OpenWorldRockVariant> variant = variants[i]; if (variant.is_null()) errors.push_back(vformat("VARIANT_MISSING:%d", i)); if (i < biome_names.size() && biome_names[i].is_empty()) errors.push_back(vformat("BIOME_MISSING:%d", i)); }
	result["success"] = errors.is_empty(); result["error_codes"] = errors; result["entry_count"] = variants.size(); return result;
}
void OpenWorldRockVariantLibrary::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_biome_names", "value"), &OpenWorldRockVariantLibrary::set_biome_names); ClassDB::bind_method(D_METHOD("get_biome_names"), &OpenWorldRockVariantLibrary::get_biome_names); ClassDB::bind_method(D_METHOD("set_variants", "value"), &OpenWorldRockVariantLibrary::set_variants); ClassDB::bind_method(D_METHOD("get_variants"), &OpenWorldRockVariantLibrary::get_variants); ClassDB::bind_method(D_METHOD("add_variant", "biome", "variant"), &OpenWorldRockVariantLibrary::add_variant); ClassDB::bind_method(D_METHOD("get_variant_count"), &OpenWorldRockVariantLibrary::get_variant_count); ClassDB::bind_method(D_METHOD("get_variant", "index"), &OpenWorldRockVariantLibrary::get_variant); ClassDB::bind_method(D_METHOD("select_variant", "biome", "seed"), &OpenWorldRockVariantLibrary::select_variant); ClassDB::bind_method(D_METHOD("validate_library"), &OpenWorldRockVariantLibrary::validate_library);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "biome_names"), "set_biome_names", "get_biome_names"); ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "variants", PROPERTY_HINT_ARRAY_TYPE, "OpenWorldRockVariant"), "set_variants", "get_variants");
}
