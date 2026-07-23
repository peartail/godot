/**************************************************************************/
/*  test_open_world_placement.h                                     */
/**************************************************************************/

#pragma once

#include "../open_world_placement_3d.h"
#include "../open_world_placement_entry.h"
#include "../open_world_placement_preset.h"
#include "../open_world_placement_data.h"
#include "../open_world_tree_generation_profile.h"
#include "../open_world_vine_generation_profile.h"
#include "../open_world_vine_generation_request.h"
#include "../open_world_vine_generator_3d.h"

#include "modules/simple_terrain/simple_terrain_3d.h"
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestOpenWorldPlacement {

static Ref<OpenWorldPlacementEntry> make_tree_entry(const String &p_id = "tree") {
	Ref<OpenWorldTreeGenerationProfile> profile;
	profile.instantiate();
	profile->set_tree_height(4.0);
	profile->set_trunk_segments(5);
	Ref<OpenWorldPlacementEntry> entry;
	entry.instantiate();
	entry->set_stable_id(p_id);
	entry->set_content_kind(OpenWorldPlacementEntry::CONTENT_TREE);
	entry->set_tree_profile(profile);
	return entry;
}

static Ref<OpenWorldPlacementEntry> make_creeping_entry(const String &p_id = "creeping") {
	Ref<OpenWorldVineGenerationProfile> profile;
	profile.instantiate();
	profile->set_side_branch_count(0);
	profile->set_turn_noise(0.0);
	profile->set_segment_length(0.25);
	Ref<OpenWorldVineGenerationRequest> request;
	request.instantiate();
	request->set_profile(profile);
	request->set_mode(OpenWorldVineGenerationRequest::MODE_CREEPING);
	request->set_start_direction(Vector3::RIGHT);
	request->set_desired_length(1.5);
	Ref<OpenWorldPlacementEntry> entry;
	entry.instantiate();
	entry->set_stable_id(p_id);
	entry->set_content_kind(OpenWorldPlacementEntry::CONTENT_VINE);
	entry->set_vine_request_template(request);
	entry->set_minimum_spacing_override(0.0);
	return entry;
}

static Ref<OpenWorldPlacementPreset> make_tree_preset() {
	Ref<OpenWorldPlacementPreset> preset;
	preset.instantiate();
	preset->set_stable_id("forest-test");
	preset->set_shape(OpenWorldPlacementPreset::SHAPE_RECTANGLE);
	preset->set_size(Vector2(4.0, 4.0));
	preset->set_density_per_100_square_meters(6.25); // Exactly one requested placement.
	preset->set_minimum_spacing(0.0);
	preset->add_entry(make_tree_entry());
	return preset;
}

static Ref<OpenWorldPlacementPreset> make_creeping_preset() {
	Ref<OpenWorldPlacementPreset> preset;
	preset.instantiate();
	preset->set_stable_id("creeping-test");
	preset->set_shape(OpenWorldPlacementPreset::SHAPE_RECTANGLE);
	preset->set_size(Vector2(4.0, 4.0));
	preset->set_density_per_100_square_meters(0.0);
	preset->set_vine_density_per_100_square_meters(6.25); // Exactly one creeping placement.
	preset->set_minimum_spacing(0.0);
	preset->set_slope_max_degrees(90.0);
	preset->add_entry(make_creeping_entry());
	return preset;
}

TEST_CASE("[OpenWorldPlacement] Presets support multiple weighted entries and structured limits") {
	Ref<OpenWorldPlacementPreset> preset = make_tree_preset();
	Ref<OpenWorldPlacementEntry> second = make_tree_entry("tree-young");
	second->set_weight(3.0);
	preset->add_entry(second);
	Dictionary valid = preset->validate_preset();
	CHECK((bool)valid["success"]);
	CHECK(preset->get_entry_count() == 2);
	CHECK(preset->get_requested_object_count() == 1);
	CHECK(preset->get_requested_primary_object_count() == 1);
	CHECK(preset->get_requested_vine_object_count() == 0);

	preset->set_size(Vector2(100.0, 100.0));
	preset->set_density_per_100_square_meters(10.0);
	preset->set_max_objects_per_operation(500);
	Dictionary limited = preset->validate_preset();
	CHECK_FALSE((bool)limited["success"]);
	CHECK(((PackedStringArray)limited["error_codes"]).find("MAX_OBJECTS_EXCEEDED") >= 0);
	CHECK((int)limited["requested_count"] == 1000);
}

TEST_CASE("[OpenWorldPlacement] Vine density is independent and sums into the operation limit") {
	Ref<OpenWorldPlacementPreset> preset = make_tree_preset();
	preset->add_entry(make_creeping_entry());
	preset->set_vine_density_per_100_square_meters(6.25);
	Dictionary valid = preset->validate_preset();
	CHECK((bool)valid["success"]);
	CHECK(preset->get_requested_primary_object_count() == 1);
	CHECK(preset->get_requested_vine_object_count() == 1);
	CHECK(preset->get_requested_object_count() == 2);

	preset->set_size(Vector2(100.0, 100.0));
	preset->set_density_per_100_square_meters(3.0);
	preset->set_vine_density_per_100_square_meters(3.0);
	preset->set_max_objects_per_operation(500);
	Dictionary limited = preset->validate_preset();
	CHECK_FALSE((bool)limited["success"]);
	CHECK(((PackedStringArray)limited["error_codes"]).find("MAX_OBJECTS_EXCEEDED") >= 0);
	CHECK((int)limited["requested_count"] == 600);
}

TEST_CASE("[OpenWorldPlacement] Accepts Bramble and Creeping; rejects Climbing") {
	Ref<OpenWorldVineGenerationProfile> profile;
	profile.instantiate();
	Ref<OpenWorldVineGenerationRequest> request;
	request.instantiate();
	request->set_profile(profile);
	request->set_mode(OpenWorldVineGenerationRequest::MODE_BRAMBLE);
	Ref<OpenWorldPlacementEntry> entry;
	entry.instantiate();
	entry->set_stable_id("bramble");
	entry->set_content_kind(OpenWorldPlacementEntry::CONTENT_VINE);
	entry->set_vine_request_template(request);
	CHECK((bool)entry->validate_entry()["success"]);

	request->set_mode(OpenWorldVineGenerationRequest::MODE_CREEPING);
	CHECK((bool)entry->validate_entry()["success"]);

	request->set_mode(OpenWorldVineGenerationRequest::MODE_CLIMBING);
	Dictionary invalid = entry->validate_entry();
	CHECK_FALSE((bool)invalid["success"]);
	CHECK(((PackedStringArray)invalid["error_codes"]).find("VINE_MODE_PHASE2") >= 0);
}

TEST_CASE("[SceneTree][OpenWorldPlacement] Placement records remain authoritative across area replacement") {
	SceneTree *tree = SceneTree::get_singleton();
	REQUIRE(tree != nullptr);
	Node3D *test_root = memnew(Node3D);
	test_root->set_name("PlacementTestRoot");
	tree->get_root()->add_child(test_root);

	SimpleTerrain3D *terrain = memnew(SimpleTerrain3D);
	terrain->set_name("Terrain");
	test_root->add_child(terrain);
	terrain->set_tile_size(16);
	terrain->set_cell_size(1.0);
	terrain->create_tile(Vector2i(0, 0));
	terrain->reset_flat_terrain();

	OpenWorldPlacement3D *placement = memnew(OpenWorldPlacement3D);
	placement->set_name("Placement");
	test_root->add_child(placement);
	placement->set_active_preset(make_tree_preset());

	Dictionary first = placement->apply_placement(Vector3(6.0, 0.0, 6.0), Ref<OpenWorldPlacementPreset>(), 101);
	REQUIRE((bool)first["success"]);
	CHECK((int)first["accepted_count"] == 1);
	REQUIRE(placement->get_placement_data().is_valid());
	CHECK(placement->get_placement_data()->get_placement_count() == 1);
	const String first_id = placement->get_placement_data()->get_stable_ids()[0];

	Dictionary second_apply = placement->apply_placement(Vector3(6.0, 0.0, 6.0), Ref<OpenWorldPlacementPreset>(), 202);
	REQUIRE((bool)second_apply["success"]);
	CHECK((int)second_apply["replacement_count"] == 1);
	CHECK(placement->get_placement_data()->get_placement_count() == 1);
	CHECK(placement->get_placement_data()->get_stable_ids()[0] != first_id);
	CHECK((bool)placement->get_placement_data()->validate_data()["success"]);

	tree->get_root()->remove_child(test_root);
	memdelete(test_root);
}

TEST_CASE("[SceneTree][OpenWorldPlacement] Creeping apply auto-wires SimpleTerrain3D support") {
	SceneTree *tree = SceneTree::get_singleton();
	REQUIRE(tree != nullptr);
	Node3D *test_root = memnew(Node3D);
	test_root->set_name("CreepingPlacementRoot");
	tree->get_root()->add_child(test_root);

	SimpleTerrain3D *terrain = memnew(SimpleTerrain3D);
	terrain->set_name("Terrain");
	test_root->add_child(terrain);
	terrain->set_tile_size(16);
	terrain->set_cell_size(1.0);
	terrain->create_tile(Vector2i(0, 0));
	terrain->reset_flat_terrain();

	OpenWorldPlacement3D *placement = memnew(OpenWorldPlacement3D);
	placement->set_name("Placement");
	test_root->add_child(placement);
	placement->set_active_preset(make_creeping_preset());

	Dictionary report = placement->apply_placement(Vector3(6.0, 0.0, 6.0), Ref<OpenWorldPlacementPreset>(), 303);
	REQUIRE((bool)report["success"]);
	CHECK((int)report["accepted_vine"] == 1);
	CHECK((int)report["accepted_primary"] == 0);
	REQUIRE(placement->get_placement_data().is_valid());
	CHECK(placement->get_placement_data()->get_placement_count() == 1);

	Node3D *generated_root = nullptr;
	for (int i = 0; i < placement->get_child_count(); i++) {
		Node3D *child = Object::cast_to<Node3D>(placement->get_child(i));
		if (child && String(child->get_name()).begins_with("__OpenWorldPlacementGenerated")) {
			generated_root = child;
			break;
		}
	}
	REQUIRE(generated_root != nullptr);
	REQUIRE(generated_root->get_child_count() >= 1);
	OpenWorldVineGenerator3D *vine = Object::cast_to<OpenWorldVineGenerator3D>(generated_root->get_child(0));
	REQUIRE(vine != nullptr);
	Ref<OpenWorldVineGenerationRequest> request = vine->get_generation_request();
	REQUIRE(request.is_valid());
	CHECK(request->get_mode() == OpenWorldVineGenerationRequest::MODE_CREEPING);
	CHECK_FALSE(request->get_support_path().is_empty());
	CHECK(vine->get_node_or_null(request->get_support_path()) == terrain);
	CHECK(vine->get_generated_lod_mesh(0).is_valid());

	tree->get_root()->remove_child(test_root);
	memdelete(test_root);
}

} // namespace TestOpenWorldPlacement
