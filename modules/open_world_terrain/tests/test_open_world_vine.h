/**************************************************************************/
/*  test_open_world_vine.h                                                */
/**************************************************************************/
#pragma once

#include "../open_world_tree_generation_profile.h"
#include "../open_world_tree_generator_3d.h"
#include "../open_world_vine_3d.h"
#include "../open_world_vine_generator_3d.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestOpenWorldVine {

static Ref<OpenWorldVineGenerationRequest> make_explicit_request(int p_seed = 1207) {
	Ref<OpenWorldVineGenerationProfile> profile; profile.instantiate();
	profile->set_side_branch_count(1); profile->set_leaf_density(1.0);
	Ref<OpenWorldVineGenerationRequest> request; request.instantiate(); request->set_profile(profile); request->set_seed(p_seed);
	PackedVector3Array points; points.push_back(Vector3(0, 0, 0)); points.push_back(Vector3(0.5, 0.1, 0.1)); points.push_back(Vector3(1.0, 0.2, 0)); points.push_back(Vector3(1.5, 0.35, -0.1)); points.push_back(Vector3(2.0, 0.5, 0)); points.push_back(Vector3(2.5, 0.7, 0.15)); request->set_explicit_anchors(points); return request;
}

TEST_CASE("[OpenWorldVine] Request is Agent-First and editor auto-instantiated") {
	OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D);
	CHECK(generator->get_generation_request().is_null());
	List<PropertyInfo> properties; generator->get_property_list(&properties); bool found = false;
	for (const PropertyInfo &property : properties) if (property.name == SNAME("generation_request")) { found = true; CHECK((property.usage & PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT) != 0); }
	CHECK(found); Dictionary report = generator->validate_request(); CHECK_FALSE((bool)report["success"]); memdelete(generator);
}

TEST_CASE("[OpenWorldVine] Explicit anchors generate deterministic two-surface LODs") {
	OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D); generator->set_auto_generate(false); generator->set_generation_request(make_explicit_request(8801)); generator->set_wind_enabled(true); generator->generate_vine();
	Ref<OpenWorldVinePathData> first_path = generator->get_generated_path(); REQUIRE(first_path.is_valid()); CHECK(first_path->get_path_count() == 2);
	Ref<ArrayMesh> lod0 = generator->get_generated_lod_mesh(0), lod1 = generator->get_generated_lod_mesh(1), lod2 = generator->get_generated_lod_mesh(2); REQUIRE(lod0.is_valid()); REQUIRE(lod1.is_valid()); REQUIRE(lod2.is_valid());
	CHECK(lod0->get_surface_count() == 2); CHECK(lod1->get_surface_count() == 2); CHECK(lod2->get_surface_count() == 2);
	Dictionary stats0 = generator->get_lod_statistics(0), stats1 = generator->get_lod_statistics(1), stats2 = generator->get_lod_statistics(2); CHECK((int)stats1["triangles"] < (int)stats0["triangles"]); CHECK((int)stats2["triangles"] < (int)stats1["triangles"]);
	PackedVector3Array first_vertices = lod0->surface_get_arrays(0)[Mesh::ARRAY_VERTEX]; generator->generate_vine(); CHECK(generator->get_generated_lod_mesh(0)->surface_get_arrays(0)[Mesh::ARRAY_VERTEX] == first_vertices);
	Array stem_arrays = lod0->surface_get_arrays(0); PackedVector2Array stem_uvs = stem_arrays[Mesh::ARRAY_TEX_UV]; PackedFloat32Array stem_tangents = stem_arrays[Mesh::ARRAY_TANGENT]; CHECK(stem_uvs.size() == first_vertices.size()); CHECK(stem_tangents.size() == first_vertices.size() * 4); for (const Vector3 &vertex : first_vertices) CHECK(vertex.is_finite());
	PackedColorArray wind = lod0->surface_get_arrays(1)[Mesh::ARRAY_COLOR]; REQUIRE_FALSE(wind.is_empty()); CHECK(wind[0].b > 0.0);
	Ref<OpenWorldVineVariant> baked = generator->create_baked_variant(); REQUIRE(baked.is_valid()); CHECK(baked->get_source_seed() == 8801); CHECK(baked->get_lod2_mesh().is_valid()); memdelete(generator);
}

TEST_CASE("[OpenWorldVine] Baked variant persists LOD and support metadata") {
	OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D); Ref<OpenWorldVineGenerationRequest> request = make_explicit_request(1207); request->set_support_stable_id(88011207); generator->set_auto_generate(false); generator->set_generation_request(request); generator->generate_vine(); Ref<OpenWorldVineVariant> baked = generator->create_baked_variant();
	const String save_path = TestUtils::get_temp_path("open_world_vine_variant.tres"); REQUIRE(ResourceSaver::save(baked, save_path) == OK); Ref<OpenWorldVineVariant> loaded = ResourceLoader::load(save_path, "OpenWorldVineVariant", ResourceFormatLoader::CACHE_MODE_IGNORE); REQUIRE(loaded.is_valid()); CHECK(loaded->get_source_seed() == 1207); CHECK(loaded->get_support_stable_id() == 88011207); CHECK(loaded->get_lod0_mesh().is_valid()); CHECK(loaded->get_lod2_mesh().is_valid()); memdelete(generator);
}

TEST_CASE("[OpenWorldVine] Hanging solver preserves endpoints and adds sag") {
	Ref<OpenWorldVineGenerationProfile> profile; profile.instantiate(); profile->set_side_branch_count(0); profile->set_hanging_sag(1.0);
	Ref<OpenWorldVineGenerationRequest> request; request.instantiate(); request->set_profile(profile); request->set_mode(OpenWorldVineGenerationRequest::MODE_HANGING); request->set_start_position(Vector3(0, 3, 0)); request->set_target_enabled(true); request->set_target_position(Vector3(4, 3, 0)); request->set_desired_length(5.0);
	OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D); generator->set_auto_generate(false); generator->set_generation_request(request); Ref<OpenWorldVinePathData> path = generator->generate_path(); REQUIRE(path.is_valid()); PackedVector3Array points = path->get_path_points(0); CHECK(points[0].is_equal_approx(Vector3(0, 3, 0))); CHECK(points[points.size() - 1].is_equal_approx(Vector3(4, 3, 0))); CHECK(points[points.size() / 2].y < 3.0); memdelete(generator);
}

TEST_CASE("[OpenWorldVine] Creeping projects onto a mesh support without physics") {
	Node3D *root = memnew(Node3D); MeshInstance3D *support = memnew(MeshInstance3D); support->set_name("Support"); Ref<PlaneMesh> plane; plane.instantiate(); plane->set_size(Vector2(10, 10)); support->set_mesh(plane); root->add_child(support);
	OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D); generator->set_name("Vine"); root->add_child(generator); generator->set_auto_generate(false);
	Ref<OpenWorldVineGenerationProfile> profile; profile.instantiate(); profile->set_side_branch_count(0); profile->set_surface_offset(0.05);
	Ref<OpenWorldVineGenerationRequest> request; request.instantiate(); request->set_profile(profile); request->set_support_path(NodePath("../Support")); request->set_start_position(Vector3(0, 0.4, 0)); request->set_start_direction(Vector3::RIGHT); request->set_desired_length(2.0); generator->set_generation_request(request);
	Ref<OpenWorldVinePathData> path = generator->generate_path(); REQUIRE(path.is_valid()); PackedVector3Array points = path->get_path_points(0); REQUIRE(points.size() >= 2); CHECK(Math::is_equal_approx(points[0].y, (real_t)0.05, (real_t)0.02)); memdelete(root);
}

TEST_CASE("[OpenWorldVine] Climbing follows a box surface upward") {
	Node3D *root = memnew(Node3D); MeshInstance3D *support = memnew(MeshInstance3D); support->set_name("Support"); Ref<BoxMesh> box; box.instantiate(); support->set_mesh(box); root->add_child(support); OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D); root->add_child(generator); generator->set_auto_generate(false);
	Ref<OpenWorldVineGenerationProfile> profile; profile.instantiate(); profile->set_side_branch_count(0); profile->set_turn_noise(0.0); profile->set_segment_length(0.2); Ref<OpenWorldVineGenerationRequest> request; request.instantiate(); request->set_profile(profile); request->set_mode(OpenWorldVineGenerationRequest::MODE_CLIMBING); request->set_support_path(NodePath("../Support")); request->set_start_position(Vector3(0.7, -0.3, 0)); request->set_start_direction(Vector3::UP); request->set_desired_length(0.6); generator->set_generation_request(request);
	Ref<OpenWorldVinePathData> path = generator->generate_path(); REQUIRE(path.is_valid()); PackedVector3Array points = path->get_path_points(0); REQUIRE(points.size() >= 3); CHECK(points[points.size() - 1].y > points[0].y); PackedByteArray attached = path->get_path_attached_flags(0); for (uint8_t value : attached) CHECK(value == 1); memdelete(root);
}

TEST_CASE("[OpenWorldVine] Invalid supports return stable error codes") {
	Node3D *root = memnew(Node3D); MeshInstance3D *empty = memnew(MeshInstance3D); empty->set_name("Empty"); root->add_child(empty); OpenWorldVineGenerator3D *generator = memnew(OpenWorldVineGenerator3D); root->add_child(generator); generator->set_auto_generate(false); Ref<OpenWorldVineGenerationRequest> request; request.instantiate(); Ref<OpenWorldVineGenerationProfile> profile; profile.instantiate(); request->set_profile(profile); request->set_support_path(NodePath("../Empty")); generator->set_generation_request(request);
	Dictionary report = generator->validate_request(); CHECK_FALSE((bool)report["success"]); PackedStringArray codes = report["error_codes"]; CHECK(codes.find("MESH_EMPTY") >= 0); request->set_support_path(NodePath("../Missing")); report = generator->validate_request(); codes = report["error_codes"]; CHECK(codes.find("SUPPORT_NOT_FOUND") >= 0); memdelete(root);
}

TEST_CASE("[OpenWorldVine] Generated trees expose deterministic TreeWrap support") {
	Node3D *root = memnew(Node3D); OpenWorldTreeGenerator3D *tree = memnew(OpenWorldTreeGenerator3D); tree->set_name("Tree"); tree->set_auto_generate(false); Ref<OpenWorldTreeGenerationProfile> tree_profile; tree_profile.instantiate(); tree_profile->set_tree_height(5.0); tree_profile->set_trunk_segments(6); tree->set_generation_profile(tree_profile); tree->set_seed(1207); root->add_child(tree); tree->generate_tree(); REQUIRE(tree->get_generated_support_graph().is_valid()); CHECK(tree->get_generated_support_graph()->get_path_count() > 0);
	OpenWorldVineGenerator3D *vine = memnew(OpenWorldVineGenerator3D); vine->set_name("Vine"); vine->set_auto_generate(false); root->add_child(vine); Ref<OpenWorldVineGenerationProfile> profile; profile.instantiate(); profile->set_side_branch_count(0); Ref<OpenWorldVineGenerationRequest> request; request.instantiate(); request->set_profile(profile); request->set_mode(OpenWorldVineGenerationRequest::MODE_TREE_WRAP); request->set_support_path(NodePath("../Tree")); request->set_desired_length(4.0); vine->set_generation_request(request); Ref<OpenWorldVinePathData> path = vine->generate_path(); REQUIRE(path.is_valid()); CHECK(path->get_path_points(0).size() > 2); Ref<OpenWorldTreeVariant> baked_tree = tree->create_baked_variant(); REQUIRE(baked_tree->get_support_graph().is_valid()); memdelete(root);
}

TEST_CASE("[OpenWorldVine] Variant applies runtime LOD and support-loss policy") {
	Ref<OpenWorldVineVariant> variant; variant.instantiate(); Ref<BoxMesh> lod0; lod0.instantiate(); Ref<BoxMesh> lod2; lod2.instantiate(); variant->set_lod0_mesh(lod0); variant->set_lod2_mesh(lod2); variant->set_lod1_distance(10); variant->set_lod2_distance(20); variant->set_max_distance(30); CHECK(variant->get_lod_index_for_distance(5) == 0); CHECK(variant->get_lod_index_for_distance(25) == 2); CHECK(variant->get_lod_index_for_distance(31) == -1);
	OpenWorldVine3D *vine = memnew(OpenWorldVine3D); vine->set_variant(variant); CHECK(vine->get_mesh().is_valid()); variant->set_support_lost_policy(OpenWorldVineVariant::SUPPORT_HIDE); vine->notify_support_lost(); CHECK_FALSE(vine->is_visible()); memdelete(vine);
}

} // namespace TestOpenWorldVine
