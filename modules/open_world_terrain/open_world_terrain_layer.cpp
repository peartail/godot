/**************************************************************************/
/*  open_world_terrain_layer.cpp                                          */
/**************************************************************************/

#include "open_world_terrain_layer.h"

#include "core/object/class_db.h"

void OpenWorldTerrainLayer::set_layer_name(const String &p_name) {
	layer_name = p_name;
	emit_changed();
}

void OpenWorldTerrainLayer::set_generation_role(GenerationRole p_role) {
	generation_role = p_role;
	emit_changed();
}

void OpenWorldTerrainLayer::set_terrain_feature(TerrainFeature p_feature) {
	terrain_feature = p_feature;
	emit_changed();
}

void OpenWorldTerrainLayer::set_material_enabled(bool p_enabled) {
	material_enabled = p_enabled;
	emit_changed();
}

void OpenWorldTerrainLayer::set_albedo_texture(const Ref<Texture2D> &p_texture) {
	albedo_texture = p_texture;
	emit_changed();
}

void OpenWorldTerrainLayer::set_normal_texture(const Ref<Texture2D> &p_texture) {
	normal_texture = p_texture;
	emit_changed();
}

void OpenWorldTerrainLayer::set_roughness_texture(const Ref<Texture2D> &p_texture) {
	roughness_texture = p_texture;
	emit_changed();
}

void OpenWorldTerrainLayer::set_ao_texture(const Ref<Texture2D> &p_texture) {
	ao_texture = p_texture;
	emit_changed();
}

void OpenWorldTerrainLayer::set_parallax_texture(const Ref<Texture2D> &p_texture) {
	parallax_texture = p_texture;
	emit_changed();
}

void OpenWorldTerrainLayer::set_tint_color(const Color &p_color) {
	tint_color = p_color;
	emit_changed();
}

void OpenWorldTerrainLayer::set_texture_scale(real_t p_scale) {
	texture_scale = MAX((real_t)0.0001, p_scale);
	emit_changed();
}

void OpenWorldTerrainLayer::set_roughness(real_t p_roughness) {
	roughness = CLAMP(p_roughness, (real_t)0.0, (real_t)1.0);
	emit_changed();
}

void OpenWorldTerrainLayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_layer_name", "name"), &OpenWorldTerrainLayer::set_layer_name);
	ClassDB::bind_method(D_METHOD("get_layer_name"), &OpenWorldTerrainLayer::get_layer_name);
	ClassDB::bind_method(D_METHOD("set_generation_role", "role"), &OpenWorldTerrainLayer::set_generation_role);
	ClassDB::bind_method(D_METHOD("get_generation_role"), &OpenWorldTerrainLayer::get_generation_role);
	ClassDB::bind_method(D_METHOD("set_terrain_feature", "feature"), &OpenWorldTerrainLayer::set_terrain_feature);
	ClassDB::bind_method(D_METHOD("get_terrain_feature"), &OpenWorldTerrainLayer::get_terrain_feature);
	ClassDB::bind_method(D_METHOD("set_material_enabled", "enabled"), &OpenWorldTerrainLayer::set_material_enabled);
	ClassDB::bind_method(D_METHOD("is_material_enabled"), &OpenWorldTerrainLayer::is_material_enabled);
	ClassDB::bind_method(D_METHOD("set_albedo_texture", "texture"), &OpenWorldTerrainLayer::set_albedo_texture);
	ClassDB::bind_method(D_METHOD("get_albedo_texture"), &OpenWorldTerrainLayer::get_albedo_texture);
	ClassDB::bind_method(D_METHOD("set_normal_texture", "texture"), &OpenWorldTerrainLayer::set_normal_texture);
	ClassDB::bind_method(D_METHOD("get_normal_texture"), &OpenWorldTerrainLayer::get_normal_texture);
	ClassDB::bind_method(D_METHOD("set_roughness_texture", "texture"), &OpenWorldTerrainLayer::set_roughness_texture);
	ClassDB::bind_method(D_METHOD("get_roughness_texture"), &OpenWorldTerrainLayer::get_roughness_texture);
	ClassDB::bind_method(D_METHOD("set_ao_texture", "texture"), &OpenWorldTerrainLayer::set_ao_texture);
	ClassDB::bind_method(D_METHOD("get_ao_texture"), &OpenWorldTerrainLayer::get_ao_texture);
	ClassDB::bind_method(D_METHOD("set_parallax_texture", "texture"), &OpenWorldTerrainLayer::set_parallax_texture);
	ClassDB::bind_method(D_METHOD("get_parallax_texture"), &OpenWorldTerrainLayer::get_parallax_texture);
	ClassDB::bind_method(D_METHOD("set_tint_color", "color"), &OpenWorldTerrainLayer::set_tint_color);
	ClassDB::bind_method(D_METHOD("get_tint_color"), &OpenWorldTerrainLayer::get_tint_color);
	ClassDB::bind_method(D_METHOD("set_texture_scale", "scale"), &OpenWorldTerrainLayer::set_texture_scale);
	ClassDB::bind_method(D_METHOD("get_texture_scale"), &OpenWorldTerrainLayer::get_texture_scale);
	ClassDB::bind_method(D_METHOD("set_roughness", "roughness"), &OpenWorldTerrainLayer::set_roughness);
	ClassDB::bind_method(D_METHOD("get_roughness"), &OpenWorldTerrainLayer::get_roughness);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "layer_name"), "set_layer_name", "get_layer_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "generation_role", PROPERTY_HINT_ENUM, "Height Generated,Surface Only,Excluded"), "set_generation_role", "get_generation_role");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "terrain_feature", PROPERTY_HINT_ENUM, "Generic,Mountain,Plain,Cliff,Road,Dirt Path,Grass,Rock"), "set_terrain_feature", "get_terrain_feature");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "material_enabled"), "set_material_enabled", "is_material_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "albedo_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_albedo_texture", "get_albedo_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "normal_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_normal_texture", "get_normal_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "roughness_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_roughness_texture", "get_roughness_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "ao_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_ao_texture", "get_ao_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parallax_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_parallax_texture", "get_parallax_texture");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "tint_color"), "set_tint_color", "get_tint_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_texture_scale", "get_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "roughness", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_roughness", "get_roughness");

	BIND_ENUM_CONSTANT(GENERATION_ROLE_HEIGHT);
	BIND_ENUM_CONSTANT(GENERATION_ROLE_SURFACE);
	BIND_ENUM_CONSTANT(GENERATION_ROLE_EXCLUDED);
	BIND_ENUM_CONSTANT(FEATURE_GENERIC);
	BIND_ENUM_CONSTANT(FEATURE_MOUNTAIN);
	BIND_ENUM_CONSTANT(FEATURE_PLAIN);
	BIND_ENUM_CONSTANT(FEATURE_CLIFF);
	BIND_ENUM_CONSTANT(FEATURE_ROAD);
	BIND_ENUM_CONSTANT(FEATURE_DIRT_PATH);
	BIND_ENUM_CONSTANT(FEATURE_GRASS);
	BIND_ENUM_CONSTANT(FEATURE_ROCK);
}
