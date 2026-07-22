/**************************************************************************/
/*  open_world_placement_3d.cpp                                     */
/**************************************************************************/

#include "open_world_placement_3d.h"

#include "open_world_rock_generator_3d.h"
#include "open_world_tree_generator_3d.h"
#include "open_world_vine_generator_3d.h"

#include "modules/simple_terrain/simple_terrain_3d.h"

#include "core/math/random_pcg.h"
#include "core/object/class_db.h"

namespace {
static constexpr const char *META_PLACEMENT_ID = "_open_world_placement_id";
static constexpr const char *META_PLACEMENT_OWNER = "_open_world_placement_owner";

static Vector3 _random_scale(RandomPCG &p_random, const Vector3 &p_min, const Vector3 &p_max) {
	return Vector3(
			p_random.random(MIN(p_min.x, p_max.x), MAX(p_min.x, p_max.x)),
			p_random.random(MIN(p_min.y, p_max.y), MAX(p_min.y, p_max.y)),
			p_random.random(MIN(p_min.z, p_max.z), MAX(p_min.z, p_max.z)));
}

static real_t _distance_xz(const Vector3 &p_a, const Vector3 &p_b) {
	return Vector2(p_a.x, p_a.z).distance_to(Vector2(p_b.x, p_b.z));
}

static void _apply_entry_materials_to_tree(OpenWorldTreeGenerator3D *p_tree, const Ref<OpenWorldPlacementEntry> &p_entry) {
	if (p_tree == nullptr || p_entry.is_null()) {
		return;
	}
	if (p_entry->get_trunk_material().is_valid()) {
		p_tree->set_trunk_material(p_entry->get_trunk_material());
	}
	if (p_entry->get_foliage_material().is_valid()) {
		p_tree->set_foliage_material(p_entry->get_foliage_material());
	}
}

static void _apply_entry_materials_to_rock(OpenWorldRockGenerator3D *p_rock, const Ref<OpenWorldPlacementEntry> &p_entry) {
	if (p_rock == nullptr || p_entry.is_null()) {
		return;
	}
	if (p_entry->get_preview_material().is_valid()) {
		p_rock->set_preview_material(p_entry->get_preview_material());
	}
}

static void _apply_entry_materials_to_vine(OpenWorldVineGenerator3D *p_vine, const Ref<OpenWorldPlacementEntry> &p_entry) {
	if (p_vine == nullptr || p_entry.is_null()) {
		return;
	}
	if (p_entry->get_stem_material().is_valid()) {
		p_vine->set_stem_material(p_entry->get_stem_material());
	}
	if (p_entry->get_foliage_material().is_valid()) {
		p_vine->set_foliage_material(p_entry->get_foliage_material());
	}
}
} // namespace

SimpleTerrain3D *OpenWorldPlacement3D::_resolve_terrain() const {
	if (terrain_path.is_empty()) {
		return nullptr;
	}
	return Object::cast_to<SimpleTerrain3D>(get_node_or_null(terrain_path));
}

Node3D *OpenWorldPlacement3D::_resolve_output_parent() const {
	if (output_parent_path.is_empty()) {
		return const_cast<OpenWorldPlacement3D *>(this);
	}
	return Object::cast_to<Node3D>(get_node_or_null(output_parent_path));
}

Node3D *OpenWorldPlacement3D::_get_generated_root() const {
	Node3D *output = _resolve_output_parent();
	if (output == nullptr) {
		return nullptr;
	}
	const String owner_key = String(get_path());
	for (int i = 0; i < output->get_child_count(); i++) {
		Node3D *child = Object::cast_to<Node3D>(output->get_child(i));
		if (child && child->get_meta(META_PLACEMENT_OWNER, String()) == owner_key) {
			return child;
		}
	}
	return nullptr;
}

Node3D *OpenWorldPlacement3D::_get_or_create_generated_root() {
	Node3D *root = _get_generated_root();
	if (root) {
		return root;
	}
	Node3D *output = _resolve_output_parent();
	ERR_FAIL_NULL_V(output, nullptr);
	root = memnew(Node3D);
	root->set_name("__OpenWorldPlacementGenerated");
	root->set_meta(META_PLACEMENT_OWNER, String(get_path()));
	output->add_child(root, true);
	_assign_scene_owner(root, output);
	return root;
}

void OpenWorldPlacement3D::_assign_scene_owner(Node *p_node, Node3D *p_output_parent) const {
	ERR_FAIL_NULL(p_node);
	ERR_FAIL_NULL(p_output_parent);
	Node *scene_owner = p_output_parent->get_owner();
	if (scene_owner == nullptr) {
		scene_owner = p_output_parent;
	}
	p_node->set_owner(scene_owner);
}

bool OpenWorldPlacement3D::_contains_world_xz(const Vector3 &p_position, const Vector3 &p_center, const Ref<OpenWorldPlacementPreset> &p_preset) const {
	const real_t yaw = Math::deg_to_rad(p_preset->get_yaw_degrees());
	const real_t c = Math::cos(-yaw);
	const real_t s = Math::sin(-yaw);
	const Vector2 delta(p_position.x - p_center.x, p_position.z - p_center.z);
	const Vector2 local(delta.x * c - delta.y * s, delta.x * s + delta.y * c);
	const Vector2 half_size = p_preset->get_size() * 0.5;
	switch (p_preset->get_shape()) {
		case OpenWorldPlacementPreset::SHAPE_CIRCLE: {
			return local.length_squared() <= half_size.x * half_size.x;
		}
		case OpenWorldPlacementPreset::SHAPE_ELLIPSE: {
			const real_t x = local.x / half_size.x;
			const real_t y = local.y / half_size.y;
			return x * x + y * y <= 1.0;
		}
		case OpenWorldPlacementPreset::SHAPE_RECTANGLE:
		default:
			return Math::abs(local.x) <= half_size.x && Math::abs(local.y) <= half_size.y;
	}
}

Vector2 OpenWorldPlacement3D::_sample_footprint(RandomPCG &r_random, const Ref<OpenWorldPlacementPreset> &p_preset) const {
	const Vector2 half_size = p_preset->get_size() * 0.5;
	Vector2 local;
	if (p_preset->get_shape() == OpenWorldPlacementPreset::SHAPE_RECTANGLE) {
		local = Vector2(r_random.random(-half_size.x, half_size.x), r_random.random(-half_size.y, half_size.y));
	} else {
		const real_t radius = Math::sqrt(r_random.randf());
		const real_t angle = r_random.random((real_t)0.0, (real_t)Math::TAU);
		const Vector2 radial(Math::cos(angle) * radius, Math::sin(angle) * radius);
		local = p_preset->get_shape() == OpenWorldPlacementPreset::SHAPE_CIRCLE ? radial * half_size.x : Vector2(radial.x * half_size.x, radial.y * half_size.y);
	}
	return local.rotated(Math::deg_to_rad(p_preset->get_yaw_degrees()));
}

Ref<OpenWorldPlacementEntry> OpenWorldPlacement3D::_select_entry(RandomPCG &r_random, const Ref<OpenWorldPlacementPreset> &p_preset) const {
	real_t total = 0.0;
	for (int i = 0; i < p_preset->get_entry_count(); i++) {
		Ref<OpenWorldPlacementEntry> entry = p_preset->get_entry(i);
		if (entry.is_valid() && entry->is_enabled()) {
			total += entry->get_weight();
		}
	}
	if (total <= 0.0) {
		return Ref<OpenWorldPlacementEntry>();
	}
	real_t cursor = r_random.random((real_t)0.0, total);
	Ref<OpenWorldPlacementEntry> last_valid;
	for (int i = 0; i < p_preset->get_entry_count(); i++) {
		Ref<OpenWorldPlacementEntry> entry = p_preset->get_entry(i);
		if (entry.is_null() || !entry->is_enabled() || entry->get_weight() <= 0.0) {
			continue;
		}
		last_valid = entry;
		cursor -= entry->get_weight();
		if (cursor <= 0.0) {
			return entry;
		}
	}
	return last_valid;
}

Dictionary OpenWorldPlacement3D::_solve_candidates(const Vector3 &p_world_position, const Ref<OpenWorldPlacementPreset> &p_preset, int p_seed, Vector<Candidate> &r_candidates, Vector<int> &r_replaced_indices) const {
	Dictionary report = p_preset->validate_preset();
	if (!(bool)report["success"]) {
		return report;
	}
	SimpleTerrain3D *terrain = _resolve_terrain();
	const bool use_terrain_projection = terrain != nullptr && terrain->is_inside_tree();

	const int requested_count = p_preset->get_requested_object_count();
	const int operation_seed = p_seed == 0 ? default_seed : p_seed;
	const PackedVector3Array existing_positions = placement_data.is_valid() ? placement_data->get_positions() : PackedVector3Array();
	const PackedFloat32Array existing_radii = placement_data.is_valid() ? placement_data->get_spacing_radii() : PackedFloat32Array();
	for (int i = 0; i < existing_positions.size(); i++) {
		if (_contains_world_xz(existing_positions[i], p_world_position, p_preset)) {
			r_replaced_indices.push_back(i);
		}
	}

	int rejected_missing_surface = 0;
	int rejected_height = 0;
	int rejected_slope = 0;
	int rejected_spacing = 0;
	for (int candidate_index = 0; candidate_index < requested_count; candidate_index++) {
		RandomPCG position_random((uint64_t)(uint32_t)operation_seed ^ ((uint64_t)(candidate_index + 1) * 0x9E3779B97F4A7C15ULL));
		RandomPCG entry_random((uint64_t)(uint32_t)operation_seed ^ ((uint64_t)(candidate_index + 1) * 0xD1B54A32D192ED03ULL));
		RandomPCG transform_random((uint64_t)(uint32_t)operation_seed ^ ((uint64_t)(candidate_index + 1) * 0x94D049BB133111EBULL));
		const Vector2 offset = _sample_footprint(position_random, p_preset);
		Vector3 query_position(p_world_position.x + offset.x, p_world_position.y, p_world_position.z + offset.y);
		Vector3 position = query_position;
		Vector3 normal = Vector3::UP;
		if (use_terrain_projection) {
			Dictionary hit = terrain->sample_surface_at_world_xz(query_position);
			if (!(bool)hit.get("success", false)) {
				rejected_missing_surface++;
				continue;
			}
			position = hit["position"];
			normal = ((Vector3)hit.get("normal", Vector3::UP)).normalized();
		}
		if (position.y < p_preset->get_height_min() || position.y > p_preset->get_height_max()) {
			rejected_height++;
			continue;
		}
		const real_t slope = Math::rad_to_deg(normal.angle_to(Vector3::UP));
		if (slope < p_preset->get_slope_min_degrees() || slope > p_preset->get_slope_max_degrees()) {
			rejected_slope++;
			continue;
		}
		Ref<OpenWorldPlacementEntry> entry = _select_entry(entry_random, p_preset);
		if (entry.is_null()) {
			continue;
		}
		const real_t spacing = MAX(p_preset->get_minimum_spacing(), entry->get_minimum_spacing_override());
		bool blocked = false;
		for (int i = 0; i < existing_positions.size() && !blocked; i++) {
			if (_contains_world_xz(existing_positions[i], p_world_position, p_preset)) {
				continue;
			}
			const real_t other_radius = i < existing_radii.size() ? existing_radii[i] : 0.0;
			blocked = _distance_xz(position, existing_positions[i]) < MAX(spacing, other_radius);
		}
		for (int i = 0; i < r_candidates.size() && !blocked; i++) {
			blocked = _distance_xz(position, r_candidates[i].position) < MAX(spacing, r_candidates[i].spacing_radius);
		}
		if (blocked) {
			rejected_spacing++;
			continue;
		}
		Candidate candidate;
		candidate.entry = entry;
		candidate.position = position + normal * entry->get_surface_offset();
		candidate.normal = normal;
		candidate.seed = (int)transform_random.rand();
		candidate.rotation = Vector3(0.0, entry->is_random_yaw_enabled() ? transform_random.random((real_t)0.0, (real_t)Math::TAU) : 0.0, 0.0);
		candidate.scale = _random_scale(transform_random, entry->get_min_scale(), entry->get_max_scale());
		candidate.spacing_radius = spacing;
		candidate.stable_id = vformat("%s_%d_%d_%d_%d", p_preset->get_stable_id(), operation_seed, (int)Math::round(p_world_position.x * 100.0), (int)Math::round(p_world_position.z * 100.0), candidate_index);
		r_candidates.push_back(candidate);
	}
	report["success"] = true;
	report["operation_seed"] = operation_seed;
	report["accepted_count"] = r_candidates.size();
	report["replacement_count"] = r_replaced_indices.size();
	report["terrain_projection"] = use_terrain_projection;
	report["rejected_missing_surface"] = rejected_missing_surface;
	report["rejected_height"] = rejected_height;
	report["rejected_slope"] = rejected_slope;
	report["rejected_spacing"] = rejected_spacing;
	return report;
}

Node3D *OpenWorldPlacement3D::_instantiate_candidate(const Candidate &p_candidate) const {
	Node3D *node = nullptr;
	switch (p_candidate.entry->get_content_kind()) {
		case OpenWorldPlacementEntry::CONTENT_TREE: {
			OpenWorldTreeGenerator3D *tree = memnew(OpenWorldTreeGenerator3D);
			tree->set_auto_generate(false);
			tree->set_generation_profile(p_candidate.entry->get_tree_profile());
			tree->set_seed(p_candidate.seed);
			tree->generate_tree();
			if (tree->get_generated_mesh().is_null()) {
				memdelete(tree);
				return nullptr;
			}
			_apply_entry_materials_to_tree(tree, p_candidate.entry);
			node = tree;
		} break;
		case OpenWorldPlacementEntry::CONTENT_ROCK: {
			Ref<OpenWorldRockGenerationRequest> source = p_candidate.entry->get_rock_request_template();
			Ref<OpenWorldRockGenerationRequest> request;
			request.instantiate();
			request->set_mode(source->get_mode());
			request->set_profile(source->get_profile());
			request->set_seed(p_candidate.seed);
			request->set_size(source->get_size());
			request->set_primary_axis(source->get_primary_axis());
			request->set_explicit_points(source->get_explicit_points());
			request->set_stable_id(p_candidate.stable_id);
			request->set_tags(source->get_tags());
			OpenWorldRockGenerator3D *rock = memnew(OpenWorldRockGenerator3D);
			rock->set_auto_generate(false);
			rock->set_generation_request(request);
			rock->generate_rock();
			if (rock->get_generated_lod_mesh(0).is_null()) {
				memdelete(rock);
				return nullptr;
			}
			_apply_entry_materials_to_rock(rock, p_candidate.entry);
			node = rock;
		} break;
		case OpenWorldPlacementEntry::CONTENT_VINE: {
			Ref<OpenWorldVineGenerationRequest> source = p_candidate.entry->get_vine_request_template();
			if (source->get_mode() != OpenWorldVineGenerationRequest::MODE_BRAMBLE) {
				return nullptr;
			}
			Ref<OpenWorldVineGenerationRequest> request;
			request.instantiate();
			request->set_mode(OpenWorldVineGenerationRequest::MODE_BRAMBLE);
			request->set_profile(source->get_profile());
			request->set_seed(p_candidate.seed);
			request->set_start_position(Vector3());
			request->set_start_direction(source->get_start_direction());
			request->set_desired_length(source->get_desired_length());
			request->set_branch_budget(source->get_branch_budget());
			OpenWorldVineGenerator3D *vine = memnew(OpenWorldVineGenerator3D);
			vine->set_auto_generate(false);
			vine->set_generation_request(request);
			vine->generate_vine();
			if (vine->get_generated_lod_mesh(0).is_null()) {
				memdelete(vine);
				return nullptr;
			}
			_apply_entry_materials_to_vine(vine, p_candidate.entry);
			node = vine;
		} break;
	}
	ERR_FAIL_NULL_V(node, nullptr);
	Node3D *output = _resolve_output_parent();
	ERR_FAIL_NULL_V(output, nullptr);
	Transform3D transform;
	Basis basis = Basis::from_euler(p_candidate.rotation);
	if (p_candidate.entry->is_aligning_to_surface_normal()) {
		basis = Basis(Quaternion(Vector3::UP, p_candidate.normal)) * basis;
	}
	basis.scale(p_candidate.scale);
	const Transform3D desired_global_transform(basis, p_candidate.position);
	transform = output->get_global_transform().affine_inverse() * desired_global_transform;
	node->set_transform(transform);
	node->set_name(p_candidate.entry->get_stable_id().is_empty() ? String("WorldPlacement") : p_candidate.entry->get_stable_id());
	node->set_meta(META_PLACEMENT_ID, p_candidate.stable_id);
	return node;
}

Node3D *OpenWorldPlacement3D::_instantiate_record(int p_index) const {
	Dictionary record = placement_data->get_placement(p_index);
	Candidate candidate;
	candidate.entry = record["source_entry"];
	candidate.stable_id = record["stable_id"];
	candidate.position = record["position"];
	candidate.rotation = record["rotation"];
	candidate.scale = record["scale"];
	candidate.normal = record["terrain_normal"];
	candidate.seed = record["seed"];
	candidate.spacing_radius = record["spacing_radius"];
	return _instantiate_candidate(candidate);
}

void OpenWorldPlacement3D::_delete_generated_by_id(const String &p_stable_id) {
	Node3D *root = _get_generated_root();
	if (root == nullptr) {
		return;
	}
	for (int i = root->get_child_count() - 1; i >= 0; i--) {
		Node *child = root->get_child(i);
		if ((String)child->get_meta(META_PLACEMENT_ID, String()) == p_stable_id) {
			root->remove_child(child);
			memdelete(child);
		}
	}
}

Dictionary OpenWorldPlacement3D::preview_placement(const Vector3 &p_world_position, const Ref<OpenWorldPlacementPreset> &p_preset, int p_seed) const {
	Ref<OpenWorldPlacementPreset> preset = p_preset.is_valid() ? p_preset : active_preset;
	if (preset.is_null()) {
		Dictionary report;
		PackedStringArray errors;
		errors.push_back("PRESET_MISSING: assign a placement preset.");
		PackedStringArray codes;
		codes.push_back("PRESET_MISSING");
		report["success"] = false;
		report["errors"] = errors;
		report["error_codes"] = codes;
		return report;
	}
	Vector<Candidate> candidates;
	Vector<int> replaced;
	return _solve_candidates(p_world_position, preset, p_seed, candidates, replaced);
}

Dictionary OpenWorldPlacement3D::apply_placement(const Vector3 &p_world_position, const Ref<OpenWorldPlacementPreset> &p_preset, int p_seed) {
	Ref<OpenWorldPlacementPreset> preset = p_preset.is_valid() ? p_preset : active_preset;
	if (preset.is_null()) {
		generation_report = preview_placement(p_world_position, preset, p_seed);
		return generation_report;
	}
	if (placement_data.is_null()) {
		placement_data.instantiate();
	}
	generation_report = placement_data->validate_data();
	if (!(bool)generation_report["success"]) {
		return generation_report;
	}
	Vector<Candidate> candidates;
	Vector<int> replaced;
	generation_report = _solve_candidates(p_world_position, preset, p_seed, candidates, replaced);
	if (!(bool)generation_report["success"]) {
		return generation_report;
	}
	Node3D *output = _resolve_output_parent();
	if (output == nullptr) {
		generation_report["success"] = false;
		PackedStringArray codes;
		codes.push_back("OUTPUT_PARENT_MISSING");
		generation_report["error_codes"] = codes;
		return generation_report;
	}
	Vector<Node3D *> generated_nodes;
	for (const Candidate &candidate : candidates) {
		Node3D *node = _instantiate_candidate(candidate);
		if (node == nullptr) {
			for (Node3D *generated : generated_nodes) {
				memdelete(generated);
			}
			generation_report["success"] = false;
			PackedStringArray codes;
			codes.push_back("GENERATOR_FAILED");
			generation_report["error_codes"] = codes;
			generation_report["generator_failure_count"] = 1;
			return generation_report;
		}
		generated_nodes.push_back(node);
	}

	for (int i = replaced.size() - 1; i >= 0; i--) {
		const int index = replaced[i];
		const String stable_id = placement_data->get_stable_ids()[index];
		_delete_generated_by_id(stable_id);
		placement_data->remove_placement_at(index);
	}
	Node3D *root = _get_or_create_generated_root();
	for (int i = 0; i < candidates.size(); i++) {
		const Candidate &candidate = candidates[i];
		Node3D *node = generated_nodes[i];
		root->add_child(node, true);
		_assign_scene_owner(node, output);
		placement_data->add_placement(candidate.stable_id, candidate.entry, candidate.position, candidate.rotation, candidate.scale, candidate.normal, candidate.seed, candidate.spacing_radius);
	}
	generation_report["placement_count"] = placement_data->get_placement_count();
	generation_report["generator_failure_count"] = 0;
	return generation_report;
}

Dictionary OpenWorldPlacement3D::rebuild_generated() {
	Dictionary report;
	report["success"] = false;
	if (placement_data.is_null()) {
		PackedStringArray codes;
		codes.push_back("PLACEMENT_DATA_MISSING");
		report["error_codes"] = codes;
		return report;
	}
	report = placement_data->validate_data();
	if (!(bool)report["success"]) {
		generation_report = report;
		return report;
	}
	Vector<Node3D *> nodes;
	for (int i = 0; i < placement_data->get_placement_count(); i++) {
		Node3D *node = _instantiate_record(i);
		if (node == nullptr) {
			for (Node3D *created : nodes) {
				memdelete(created);
			}
			PackedStringArray codes;
			codes.push_back("GENERATOR_FAILED");
			report["error_codes"] = codes;
			report["failed_index"] = i;
			return report;
		}
		nodes.push_back(node);
	}
	clear_generated();
	Node3D *root = _get_or_create_generated_root();
	Node3D *output = _resolve_output_parent();
	for (Node3D *node : nodes) {
		root->add_child(node, true);
		_assign_scene_owner(node, output);
	}
	report["success"] = true;
	report["generated_count"] = nodes.size();
	generation_report = report;
	return report;
}

Dictionary OpenWorldPlacement3D::clear_generated() {
	Dictionary report;
	Node3D *root = _get_generated_root();
	int removed = 0;
	if (root) {
		while (root->get_child_count() > 0) {
			Node *child = root->get_child(root->get_child_count() - 1);
			root->remove_child(child);
			memdelete(child);
			removed++;
		}
	}
	report["success"] = true;
	report["removed_count"] = removed;
	return report;
}

Dictionary OpenWorldPlacement3D::clear_placements() {
	Dictionary report = clear_generated();
	if (placement_data.is_valid()) {
		placement_data->clear_placements();
	}
	return report;
}

#define BRUSH_SETTER(type, name) \
	void OpenWorldPlacement3D::set_##name(const type &p_value) { \
		if (name == p_value) return; \
		name = p_value; \
	}
BRUSH_SETTER(NodePath, terrain_path);
BRUSH_SETTER(NodePath, output_parent_path);
#undef BRUSH_SETTER

void OpenWorldPlacement3D::set_default_seed(int p_value) {
	default_seed = p_value;
}

void OpenWorldPlacement3D::set_active_preset(const Ref<OpenWorldPlacementPreset> &p_value) {
	active_preset = p_value;
}

void OpenWorldPlacement3D::set_placement_data(const Ref<OpenWorldPlacementData> &p_value) {
	placement_data = p_value;
}

void OpenWorldPlacement3D::_bind_methods() {
#define BIND_ACCESSOR(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldPlacement3D::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldPlacement3D::get_##name)
	BIND_ACCESSOR(terrain_path);
	BIND_ACCESSOR(output_parent_path);
	BIND_ACCESSOR(active_preset);
	BIND_ACCESSOR(placement_data);
	BIND_ACCESSOR(default_seed);
#undef BIND_ACCESSOR
	ClassDB::bind_method(D_METHOD("preview_placement", "world_position", "preset", "seed"), &OpenWorldPlacement3D::preview_placement, DEFVAL(Ref<OpenWorldPlacementPreset>()), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("apply_placement", "world_position", "preset", "seed"), &OpenWorldPlacement3D::apply_placement, DEFVAL(Ref<OpenWorldPlacementPreset>()), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("rebuild_generated"), &OpenWorldPlacement3D::rebuild_generated);
	ClassDB::bind_method(D_METHOD("clear_generated"), &OpenWorldPlacement3D::clear_generated);
	ClassDB::bind_method(D_METHOD("clear_placements"), &OpenWorldPlacement3D::clear_placements);
	ClassDB::bind_method(D_METHOD("get_generation_report"), &OpenWorldPlacement3D::get_generation_report);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "terrain_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "SimpleTerrain3D"), "set_terrain_path", "get_terrain_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "output_parent_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_output_parent_path", "get_output_parent_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_preset", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldPlacementPreset", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_active_preset", "get_active_preset");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "placement_data", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldPlacementData", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_placement_data", "get_placement_data");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_seed"), "set_default_seed", "get_default_seed");
}
