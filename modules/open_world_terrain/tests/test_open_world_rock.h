/**************************************************************************/
/*  test_open_world_rock.h                                                */
/**************************************************************************/
#pragma once

#include "../open_world_rock_generator_3d.h"
#include "../open_world_rock_placement_data.h"
#include "../open_world_rock_variant_library.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestOpenWorldRock {

static Ref<OpenWorldRockGenerationRequest> make_request(OpenWorldRockGenerationRequest::RockMode p_mode = OpenWorldRockGenerationRequest::MODE_BOULDER, int p_seed = 1207) {
	Ref<OpenWorldRockGenerationProfile> profile; profile.instantiate(); profile->set_point_count(36); profile->set_bottom_flatten(0.22); profile->set_strata_strength(0.16);
	Ref<OpenWorldRockGenerationRequest> request; request.instantiate(); request->set_profile(profile); request->set_mode(p_mode); request->set_seed(p_seed); request->set_stable_id(vformat("rock-%d", p_seed)); PackedStringArray tags; tags.push_back("field"); tags.push_back("mineable"); request->set_tags(tags); return request;
}

TEST_CASE("[OpenWorldRock] Request validation is structured and Agent-First") {
	OpenWorldRockGenerator3D *generator = memnew(OpenWorldRockGenerator3D); Dictionary missing = generator->validate_request(); CHECK_FALSE((bool)missing["success"]); CHECK(((PackedStringArray)missing["error_codes"]).find("REQUEST_MISSING") >= 0);
	Ref<OpenWorldRockGenerationRequest> request; request.instantiate(); request->set_size(Vector3(0, 1, 1)); generator->set_generation_request(request); Dictionary invalid = generator->validate_request(); PackedStringArray errors = invalid["error_codes"]; CHECK(errors.find("PROFILE_MISSING") >= 0); CHECK(errors.find("INVALID_SIZE") >= 0);
	List<PropertyInfo> properties; generator->get_property_list(&properties); bool editor_instantiates = false; for (const PropertyInfo &property : properties) if (property.name == SNAME("generation_request")) editor_instantiates = (property.usage & PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT) != 0; CHECK(editor_instantiates); memdelete(generator);
}

TEST_CASE("[OpenWorldRock] Hull point and index arrays are deterministic") {
	OpenWorldRockGenerator3D *generator = memnew(OpenWorldRockGenerator3D); generator->set_auto_generate(false); generator->set_generation_request(make_request(OpenWorldRockGenerationRequest::MODE_BOULDER, 8801)); Ref<OpenWorldRockTopologyData> first = generator->generate_topology(); REQUIRE(first.is_valid()); PackedVector3Array points = first->get_source_points(); PackedVector3Array vertices = first->get_hull_vertices(); PackedInt32Array indices = first->get_hull_indices(); String hash = first->get_topology_hash();
	Ref<OpenWorldRockTopologyData> second = generator->generate_topology(); REQUIRE(second.is_valid()); CHECK(second->get_source_points() == points); CHECK(second->get_hull_vertices() == vertices); CHECK(second->get_hull_indices() == indices); CHECK(second->get_topology_hash() == hash); CHECK(second->get_base_contact_count() >= 3); memdelete(generator);
}

TEST_CASE("[OpenWorldRock] Boulder slab and shard generate monotonic flat LODs") {
	for (int mode = 0; mode < 3; mode++) {
		OpenWorldRockGenerator3D *generator = memnew(OpenWorldRockGenerator3D); generator->set_auto_generate(false); Ref<OpenWorldRockGenerationRequest> request = make_request((OpenWorldRockGenerationRequest::RockMode)mode, 7300 + mode); if (mode == OpenWorldRockGenerationRequest::MODE_SHARD) request->set_primary_axis(Vector3::RIGHT); generator->set_generation_request(request); generator->generate_rock();
		Dictionary report = generator->get_generation_report(); REQUIRE((bool)report["success"]); REQUIRE(generator->get_generated_collision_points().size() >= 4); int previous_triangles = INT32_MAX;
		for (int lod = 0; lod < 3; lod++) { Ref<ArrayMesh> mesh = generator->get_generated_lod_mesh(lod); REQUIRE(mesh.is_valid()); Dictionary stats = generator->get_lod_statistics(lod); CHECK((int)stats["triangles"] < previous_triangles); previous_triangles = stats["triangles"]; Array arrays = mesh->surface_get_arrays(0); PackedVector3Array mesh_vertices = arrays[Mesh::ARRAY_VERTEX]; PackedVector3Array normals = arrays[Mesh::ARRAY_NORMAL]; PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV]; PackedColorArray colors = arrays[Mesh::ARRAY_COLOR]; CHECK(mesh_vertices.size() == normals.size()); CHECK(mesh_vertices.size() == uvs.size()); CHECK(mesh_vertices.size() == colors.size()); for (int i = 0; i < mesh_vertices.size(); i++) { CHECK(mesh_vertices[i].is_finite()); CHECK(normals[i].is_finite()); CHECK(colors[i].r >= 0.0); CHECK(colors[i].r <= 1.0); CHECK(colors[i].g >= 0.0); CHECK(colors[i].g <= 1.0); CHECK(colors[i].b >= 0.0); CHECK(colors[i].b <= 1.0); } }
		AABB bounds = generator->get_generated_topology()->get_local_bounds(); if (mode == OpenWorldRockGenerationRequest::MODE_SLAB) CHECK(bounds.size.y < bounds.size.x); if (mode == OpenWorldRockGenerationRequest::MODE_SHARD) CHECK(bounds.size.x > bounds.size.y); memdelete(generator);
	}
}

TEST_CASE("[OpenWorldRock] Explicit degenerate points fail without crashing") {
	OpenWorldRockGenerator3D *generator = memnew(OpenWorldRockGenerator3D); generator->set_auto_generate(false); Ref<OpenWorldRockGenerationRequest> request = make_request(); PackedVector3Array points; for (int i = 0; i < 12; i++) points.push_back(Vector3(i, 0, 0)); request->set_explicit_points(points); generator->set_generation_request(request); CHECK(generator->generate_topology().is_null()); Dictionary report = generator->get_generation_report(); CHECK_FALSE((bool)report["success"]); CHECK(((PackedStringArray)report["error_codes"]).find("HULL_DEGENERATE") >= 0); memdelete(generator);
}

TEST_CASE("[OpenWorldRock] Baked variant persists meshes collision and metadata") {
	OpenWorldRockGenerator3D *generator = memnew(OpenWorldRockGenerator3D); generator->set_auto_generate(false); generator->set_generation_request(make_request(OpenWorldRockGenerationRequest::MODE_SLAB, 9917)); generator->generate_rock(); Ref<OpenWorldRockVariant> baked = generator->create_baked_variant(); REQUIRE(baked.is_valid()); const String path = TestUtils::get_temp_path("open_world_rock_variant.tres"); REQUIRE(ResourceSaver::save(baked, path) == OK); Ref<OpenWorldRockVariant> loaded = ResourceLoader::load(path, "OpenWorldRockVariant", ResourceFormatLoader::CACHE_MODE_IGNORE); REQUIRE(loaded.is_valid()); CHECK(loaded->get_source_seed() == 9917); CHECK(loaded->get_source_mode() == OpenWorldRockGenerationRequest::MODE_SLAB); CHECK(loaded->get_lod0_mesh().is_valid()); CHECK(loaded->get_lod2_mesh().is_valid()); CHECK(loaded->get_collision_points().size() >= 4); CHECK(loaded->get_stable_id() == "rock-9917"); memdelete(generator);
}

TEST_CASE("[OpenWorldRock] Library placement cells and state variants are deterministic") {
	Ref<OpenWorldRockVariant> intact; intact.instantiate(); intact->set_stable_id("ore-a"); Ref<OpenWorldRockVariant> damaged; damaged.instantiate(); damaged->set_gameplay_state(OpenWorldRockVariant::STATE_DAMAGED); intact->set_damaged_variant(damaged); CHECK(intact->get_variant_for_state(OpenWorldRockVariant::STATE_DAMAGED) == damaged);
	Ref<OpenWorldRockVariantLibrary> library; library.instantiate(); library->add_variant("alpine", intact); library->add_variant("alpine", damaged); CHECK((bool)library->validate_library()["success"]); CHECK(library->select_variant("alpine", 42) == library->select_variant("alpine", 42));
	Ref<OpenWorldRockPlacementData> placements; placements.instantiate(); placements->add_placement(Vector3(2, 0, 3), Vector3(), Vector3(1, 1, 1), 0, "cell-0-rock-0"); placements->add_placement(Vector3(18, 0, 3), Vector3(), Vector3(1, 1, 1), 1, "cell-1-rock-0"); CHECK((bool)placements->validate_placements()["success"]); CHECK(placements->get_placements_in_cell(Vector2i(0, 0), 16.0).size() == 1); CHECK(placements->get_placements_in_cell(Vector2i(1, 0), 16.0).size() == 1);
}

} // namespace TestOpenWorldRock
