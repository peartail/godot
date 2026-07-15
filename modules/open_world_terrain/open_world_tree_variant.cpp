/**************************************************************************/
/*  open_world_tree_variant.cpp                                           */
/**************************************************************************/

#include "open_world_tree_variant.h"

#include "core/object/class_db.h"

void OpenWorldTreeVariant::set_variant_name(const String &p_name) {
	if (variant_name == p_name) {
		return;
	}
	variant_name = p_name;
	emit_changed();
}

void OpenWorldTreeVariant::set_source_seed(int p_seed) {
	if (source_seed == p_seed) {
		return;
	}
	source_seed = p_seed;
	emit_changed();
}

void OpenWorldTreeVariant::set_lod0_mesh(const Ref<Mesh> &p_mesh) {
	if (lod_meshes[0] == p_mesh) {
		return;
	}
	lod_meshes[0] = p_mesh;
	emit_changed();
}

void OpenWorldTreeVariant::set_lod1_mesh(const Ref<Mesh> &p_mesh) {
	if (lod_meshes[1] == p_mesh) {
		return;
	}
	lod_meshes[1] = p_mesh;
	emit_changed();
}

void OpenWorldTreeVariant::set_lod2_mesh(const Ref<Mesh> &p_mesh) {
	if (lod_meshes[2] == p_mesh) {
		return;
	}
	lod_meshes[2] = p_mesh;
	emit_changed();
}

Ref<Mesh> OpenWorldTreeVariant::get_lod_mesh(int p_lod) const {
	ERR_FAIL_INDEX_V(p_lod, 3, Ref<Mesh>());
	return lod_meshes[p_lod];
}

void OpenWorldTreeVariant::set_lod1_distance(real_t p_distance) {
	const real_t new_distance = MAX((real_t)0.0, p_distance);
	if (Math::is_equal_approx(lod1_distance, new_distance)) {
		return;
	}
	lod1_distance = new_distance;
	lod2_distance = MAX(lod2_distance, lod1_distance);
	max_distance = MAX(max_distance, lod2_distance);
	emit_changed();
}

void OpenWorldTreeVariant::set_lod2_distance(real_t p_distance) {
	const real_t new_distance = MAX(lod1_distance, p_distance);
	if (Math::is_equal_approx(lod2_distance, new_distance)) {
		return;
	}
	lod2_distance = new_distance;
	max_distance = MAX(max_distance, lod2_distance);
	emit_changed();
}

void OpenWorldTreeVariant::set_max_distance(real_t p_distance) {
	const real_t new_distance = MAX(lod2_distance, p_distance);
	if (Math::is_equal_approx(max_distance, new_distance)) {
		return;
	}
	max_distance = new_distance;
	emit_changed();
}

int OpenWorldTreeVariant::get_lod_index_for_distance(real_t p_distance) const {
	if (p_distance > max_distance) {
		return -1;
	}

	int preferred_lod = 0;
	if (p_distance >= lod2_distance) {
		preferred_lod = 2;
	} else if (p_distance >= lod1_distance) {
		preferred_lod = 1;
	}

	// Prefer a more detailed mesh when the requested LOD is missing. This
	// avoids making trees disappear while an asset is still being authored.
	for (int lod = preferred_lod; lod >= 0; lod--) {
		if (lod_meshes[lod].is_valid()) {
			return lod;
		}
	}
	for (int lod = preferred_lod + 1; lod < 3; lod++) {
		if (lod_meshes[lod].is_valid()) {
			return lod;
		}
	}
	return -1;
}

void OpenWorldTreeVariant::set_material_override(const Ref<Material> &p_material) {
	if (material_override == p_material) {
		return;
	}
	material_override = p_material;
	emit_changed();
}

void OpenWorldTreeVariant::set_collision_radius(real_t p_radius) {
	const real_t new_radius = MAX((real_t)0.0, p_radius);
	if (Math::is_equal_approx(collision_radius, new_radius)) {
		return;
	}
	collision_radius = new_radius;
	emit_changed();
}

void OpenWorldTreeVariant::set_collision_height(real_t p_height) {
	const real_t new_height = MAX((real_t)0.0, p_height);
	if (Math::is_equal_approx(collision_height, new_height)) {
		return;
	}
	collision_height = new_height;
	emit_changed();
}

void OpenWorldTreeVariant::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_variant_name", "name"), &OpenWorldTreeVariant::set_variant_name);
	ClassDB::bind_method(D_METHOD("get_variant_name"), &OpenWorldTreeVariant::get_variant_name);
	ClassDB::bind_method(D_METHOD("set_source_seed", "seed"), &OpenWorldTreeVariant::set_source_seed);
	ClassDB::bind_method(D_METHOD("get_source_seed"), &OpenWorldTreeVariant::get_source_seed);
	ClassDB::bind_method(D_METHOD("set_lod0_mesh", "mesh"), &OpenWorldTreeVariant::set_lod0_mesh);
	ClassDB::bind_method(D_METHOD("get_lod0_mesh"), &OpenWorldTreeVariant::get_lod0_mesh);
	ClassDB::bind_method(D_METHOD("set_lod1_mesh", "mesh"), &OpenWorldTreeVariant::set_lod1_mesh);
	ClassDB::bind_method(D_METHOD("get_lod1_mesh"), &OpenWorldTreeVariant::get_lod1_mesh);
	ClassDB::bind_method(D_METHOD("set_lod2_mesh", "mesh"), &OpenWorldTreeVariant::set_lod2_mesh);
	ClassDB::bind_method(D_METHOD("get_lod2_mesh"), &OpenWorldTreeVariant::get_lod2_mesh);
	ClassDB::bind_method(D_METHOD("get_lod_mesh", "lod"), &OpenWorldTreeVariant::get_lod_mesh);
	ClassDB::bind_method(D_METHOD("set_lod1_distance", "distance"), &OpenWorldTreeVariant::set_lod1_distance);
	ClassDB::bind_method(D_METHOD("get_lod1_distance"), &OpenWorldTreeVariant::get_lod1_distance);
	ClassDB::bind_method(D_METHOD("set_lod2_distance", "distance"), &OpenWorldTreeVariant::set_lod2_distance);
	ClassDB::bind_method(D_METHOD("get_lod2_distance"), &OpenWorldTreeVariant::get_lod2_distance);
	ClassDB::bind_method(D_METHOD("set_max_distance", "distance"), &OpenWorldTreeVariant::set_max_distance);
	ClassDB::bind_method(D_METHOD("get_max_distance"), &OpenWorldTreeVariant::get_max_distance);
	ClassDB::bind_method(D_METHOD("get_lod_index_for_distance", "distance"), &OpenWorldTreeVariant::get_lod_index_for_distance);
	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &OpenWorldTreeVariant::set_material_override);
	ClassDB::bind_method(D_METHOD("get_material_override"), &OpenWorldTreeVariant::get_material_override);
	ClassDB::bind_method(D_METHOD("set_collision_radius", "radius"), &OpenWorldTreeVariant::set_collision_radius);
	ClassDB::bind_method(D_METHOD("get_collision_radius"), &OpenWorldTreeVariant::get_collision_radius);
	ClassDB::bind_method(D_METHOD("set_collision_height", "height"), &OpenWorldTreeVariant::set_collision_height);
	ClassDB::bind_method(D_METHOD("get_collision_height"), &OpenWorldTreeVariant::get_collision_height);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "variant_name"), "set_variant_name", "get_variant_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "source_seed"), "set_source_seed", "get_source_seed");
	ADD_GROUP("LOD Meshes", "lod");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod0_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod0_mesh", "get_lod0_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod1_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod1_mesh", "get_lod1_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod2_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_lod2_mesh", "get_lod2_mesh");
	ADD_GROUP("LOD Distances", "lod");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "0,100000,0.1,or_greater,suffix:m"), "set_lod1_distance", "get_lod1_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "0,100000,0.1,or_greater,suffix:m"), "set_lod2_distance", "get_lod2_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,100000,0.1,or_greater,suffix:m"), "set_max_distance", "get_max_distance");
	ADD_GROUP("Rendering", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material_override", "get_material_override");
	ADD_GROUP("Collision Hint", "collision_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_radius", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_collision_radius", "get_collision_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_height", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater,suffix:m"), "set_collision_height", "get_collision_height");
}
