/**************************************************************************/
/*  test_open_world_tree.h                                                */
/**************************************************************************/

#pragma once

#include "../open_world_tree_generator_3d.h"
#include "../open_world_tree_placement_data.h"
#include "../open_world_tree_species.h"
#include "../open_world_tree_variant.h"

#ifdef TOOLS_ENABLED
#include "../editor/open_world_terrain_editor_plugin.h"
#include "core/math/triangle_mesh.h"
#endif

#include "scene/resources/3d/primitive_meshes.h"
#include "tests/test_macros.h"

namespace TestOpenWorldTree {

TEST_CASE("[OpenWorldTree] Variant selects and falls back between LOD meshes") {
	Ref<OpenWorldTreeVariant> variant;
	variant.instantiate();
	Ref<BoxMesh> lod0;
	lod0.instantiate();
	Ref<BoxMesh> lod2;
	lod2.instantiate();
	variant->set_lod0_mesh(lod0);
	variant->set_lod2_mesh(lod2);
	variant->set_lod1_distance(20.0);
	variant->set_lod2_distance(50.0);
	variant->set_max_distance(100.0);

	CHECK(variant->get_lod_index_for_distance(0.0) == 0);
	CHECK(variant->get_lod_index_for_distance(30.0) == 0);
	CHECK(variant->get_lod_index_for_distance(60.0) == 2);
	CHECK(variant->get_lod_index_for_distance(101.0) == -1);
}

TEST_CASE("[OpenWorldTree] Species variant selection is deterministic") {
	Ref<OpenWorldTreeVariant> first;
	first.instantiate();
	Ref<OpenWorldTreeVariant> second;
	second.instantiate();
	Array variants;
	variants.push_back(first);
	variants.push_back(second);

	Ref<OpenWorldTreeSpecies> species;
	species.instantiate();
	species->set_variants(variants);
	CHECK(species->get_variant_count() == 2);
	CHECK(species->get_variant_index_for_seed(1207) == species->get_variant_index_for_seed(1207));
	CHECK(species->get_variant_index_for_seed(1207) >= 0);
	CHECK(species->get_variant_index_for_seed(1207) < 2);
}

TEST_CASE("[OpenWorldTree] Placement data keeps stable IDs and enabled state") {
	Ref<OpenWorldTreePlacementData> data;
	data.instantiate();
	const int first_id = data->add_tree(Vector3(1.0, 2.0, 3.0), Vector3(), Vector3(1.0, 1.0, 1.0), 1207);
	const int second_id = data->add_tree(Vector3(4.0, 5.0, 6.0));

	CHECK(first_id > 0);
	CHECK(second_id > first_id);
	CHECK(data->get_instance_count() == 2);
	CHECK(data->find_index_by_id(first_id) == 0);
	CHECK(data->is_instance_enabled(0));

	data->set_instance_enabled(0, false);
	CHECK_FALSE(data->is_instance_enabled(0));
	CHECK(data->find_index_by_id(second_id) == 1);

	data->remove_tree_at(0);
	CHECK(data->get_instance_count() == 1);
	CHECK(data->find_index_by_id(second_id) == 0);
	const int remaining_id = data->get_instance(0)["instance_id"];
	CHECK(remaining_id == second_id);
}

TEST_CASE("[OpenWorldTree] Generator is deterministic and bakes two surfaces") {
    OpenWorldTreeGenerator3D *generator = memnew(OpenWorldTreeGenerator3D);
    generator->set_auto_generate(false);

    Ref<OpenWorldTreeGenerationProfile> profile;
    profile.instantiate();
    profile->set_tree_height(4.0);
    profile->set_trunk_segments(4);
    profile->set_branch_start_ratio(0.4);
    profile->set_branch_end_ratio(0.75);
    profile->set_branch_interval(0.8);
    profile->set_canopy_blob_count(4);
    generator->set_generation_profile(profile);
    generator->set_seed(1207);
    generator->generate_tree();

    Ref<ArrayMesh> first_mesh = generator->get_generated_mesh();
    REQUIRE(first_mesh.is_valid());
    REQUIRE(first_mesh->get_surface_count() == 2);
    const PackedVector3Array first_trunk_vertices = first_mesh->surface_get_arrays(0)[Mesh::ARRAY_VERTEX];
    const PackedVector3Array first_foliage_vertices = first_mesh->surface_get_arrays(1)[Mesh::ARRAY_VERTEX];
    CHECK_FALSE(first_trunk_vertices.is_empty());
    CHECK_FALSE(first_foliage_vertices.is_empty());

    generator->generate_tree();
    Ref<ArrayMesh> second_mesh = generator->get_generated_mesh();
    REQUIRE(second_mesh.is_valid());
    CHECK(second_mesh->surface_get_arrays(0)[Mesh::ARRAY_VERTEX] == first_trunk_vertices);
    CHECK(second_mesh->surface_get_arrays(1)[Mesh::ARRAY_VERTEX] == first_foliage_vertices);

    Ref<OpenWorldTreeVariant> baked = generator->create_baked_variant();
    REQUIRE(baked.is_valid());
    CHECK(baked->get_source_seed() == 1207);
    REQUIRE(baked->get_lod0_mesh().is_valid());
    CHECK(baked->get_lod0_mesh()->get_surface_count() == 2);

    memdelete(generator);
}

TEST_CASE("[OpenWorldTree] Phase 2 archetypes create distinct static silhouettes") {
	OpenWorldTreeGenerator3D *generator = memnew(OpenWorldTreeGenerator3D);
	generator->set_auto_generate(false);

	Ref<OpenWorldTreeGenerationProfile> profile;
	profile.instantiate();
	profile->set_tree_height(5.0);
	profile->set_trunk_segments(5);
	profile->set_branch_interval(0.9);
	profile->set_branch_segments(3);
	profile->set_canopy_blob_count(5);
	profile->set_root_count(5);
	generator->set_generation_profile(profile);
	generator->set_seed(2402);

	profile->set_archetype(OpenWorldTreeGenerationProfile::ARCHETYPE_TEMPERATE_BROADLEAF);
	generator->generate_tree();
	Ref<ArrayMesh> temperate_mesh = generator->get_generated_mesh();
	REQUIRE(temperate_mesh.is_valid());
	REQUIRE(temperate_mesh->get_surface_count() == 2);
	const AABB temperate_aabb = temperate_mesh->get_aabb();
	const int temperate_foliage_vertices = ((PackedVector3Array)temperate_mesh->surface_get_arrays(1)[Mesh::ARRAY_VERTEX]).size();

	profile->set_archetype(OpenWorldTreeGenerationProfile::ARCHETYPE_TROPICAL_BROADLEAF);
	generator->generate_tree();
	Ref<ArrayMesh> tropical_mesh = generator->get_generated_mesh();
	REQUIRE(tropical_mesh.is_valid());
	REQUIRE(tropical_mesh->get_surface_count() == 2);
	const AABB tropical_aabb = tropical_mesh->get_aabb();
	const bool tropical_roots_expand_crown = tropical_aabb.size.x > temperate_aabb.size.x || tropical_aabb.size.z > temperate_aabb.size.z;
	CHECK(tropical_roots_expand_crown);

	profile->set_archetype(OpenWorldTreeGenerationProfile::ARCHETYPE_UMBRELLA);
	generator->generate_tree();
	const AABB umbrella_aabb = generator->get_generated_mesh()->get_aabb();

	profile->set_archetype(OpenWorldTreeGenerationProfile::ARCHETYPE_CONIFER);
	generator->generate_tree();
	const AABB conifer_aabb = generator->get_generated_mesh()->get_aabb();
	CHECK_FALSE(umbrella_aabb.is_equal_approx(conifer_aabb));

	profile->set_archetype(OpenWorldTreeGenerationProfile::ARCHETYPE_PALM);
	profile->set_palm_frond_count(9);
	generator->generate_tree();
	Ref<ArrayMesh> palm_mesh = generator->get_generated_mesh();
	REQUIRE(palm_mesh.is_valid());
	REQUIRE(palm_mesh->get_surface_count() == 2);
	const int palm_foliage_vertices = ((PackedVector3Array)palm_mesh->surface_get_arrays(1)[Mesh::ARRAY_VERTEX]).size();
	CHECK(palm_foliage_vertices > 0);
	CHECK(palm_foliage_vertices != temperate_foliage_vertices);
	CHECK(palm_mesh->get_name().contains("4_2402"));

	profile->set_archetype(OpenWorldTreeGenerationProfile::ARCHETYPE_MANGROVE);
	generator->generate_tree();
	Ref<ArrayMesh> mangrove_mesh = generator->get_generated_mesh();
	REQUIRE(mangrove_mesh.is_valid());
	CHECK(mangrove_mesh->get_aabb().position.y <= 0.02);

	memdelete(generator);
}
#ifdef TOOLS_ENABLED
TEST_CASE("[OpenWorldTree] Editor selection mesh follows enabled placements") {
	Ref<OpenWorldTreeVariant> variant;
	variant.instantiate();
	Ref<BoxMesh> mesh;
	mesh.instantiate();
	variant->set_lod0_mesh(mesh);

	Array variants;
	variants.push_back(variant);
	Ref<OpenWorldTreeSpecies> species;
	species.instantiate();
	species->set_variants(variants);

	Ref<OpenWorldTreePlacementData> placements;
	placements.instantiate();
	placements->add_tree(Vector3(4.0, 0.0, 0.0));

	OpenWorldTree3D *tree = memnew(OpenWorldTree3D);
	tree->set_species(species);
	tree->set_placement_data(placements);

	Ref<TriangleMesh> selection_mesh = OpenWorldTree3DGizmoPlugin::build_selection_mesh(tree);
	REQUIRE(selection_mesh.is_valid());
	Vector3 hit_position;
	Vector3 hit_normal;
	CHECK(selection_mesh->intersect_ray(Vector3(4.0, 0.0, 5.0), Vector3(0.0, 0.0, -1.0), hit_position, hit_normal));

	placements->set_instance_enabled(0, false);
	CHECK(OpenWorldTree3DGizmoPlugin::build_selection_mesh(tree).is_null());

	memdelete(tree);
}
#endif

} // namespace TestOpenWorldTree
