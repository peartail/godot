/**************************************************************************/
/*  open_world_vine_generator_3d.cpp                                      */
/**************************************************************************/
#include "open_world_vine_generator_3d.h"

#include "open_world_terrain_3d.h"
#include "open_world_tree_generator_3d.h"
#include "open_world_tree_support_graph.h"

#include "core/math/random_pcg.h"
#include "core/math/triangle_mesh.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/shader.h"
#include "scene/resources/surface_tool.h"

namespace {
static real_t _vine_random(RandomPCG &r_random, real_t p_min, real_t p_max) { return Math::lerp(p_min, p_max, (real_t)r_random.randf()); }

static void _vine_vertex(const Ref<SurfaceTool> &p_surface, const Vector3 &p_position, const Vector3 &p_normal, const Vector2 &p_uv, const Color &p_color) {
	p_surface->set_normal(p_normal); p_surface->set_uv(p_uv); p_surface->set_color(p_color); p_surface->add_vertex(p_position);
}

static real_t _path_length(const PackedVector3Array &p_points) {
	real_t length = 0.0;
	for (int i = 1; i < p_points.size(); i++) length += p_points[i - 1].distance_to(p_points[i]);
	return length;
}

static Transform3D _authoring_transform(const Node3D *p_node) {
	if (p_node->is_inside_tree()) return p_node->get_global_transform();
	Transform3D result = p_node->get_transform();
	const Node *parent = p_node->get_parent();
	while (const Node3D *parent_3d = Object::cast_to<Node3D>(parent)) { result = parent_3d->get_transform() * result; parent = parent->get_parent(); }
	return result;
}

static void _append_tube(const Ref<SurfaceTool> &p_surface, const PackedVector3Array &p_points, real_t p_base_radius, real_t p_tip_scale, int p_sides, real_t p_phase, real_t p_branch_weight, const PackedByteArray &p_attached, bool p_ribbon) {
	if (p_points.size() < 2) return;
	const real_t total_length = MAX((real_t)0.001, _path_length(p_points));
	Vector<real_t> lengths; lengths.resize(p_points.size());
	lengths.write[0] = 0.0;
	for (int i = 1; i < p_points.size(); i++) lengths.write[i] = lengths[i - 1] + p_points[i - 1].distance_to(p_points[i]);
	Vector3 previous_side;
	Vector<Vector3> sides; Vector<Vector3> binormals; sides.resize(p_points.size()); binormals.resize(p_points.size());
	for (int i = 0; i < p_points.size(); i++) {
		Vector3 tangent = (p_points[MIN(i + 1, p_points.size() - 1)] - p_points[MAX(0, i - 1)]).normalized();
		if (tangent.is_zero_approx()) tangent = Vector3::UP;
		Vector3 side;
		if (i == 0) { Vector3 reference = Math::abs(tangent.dot(Vector3::UP)) > 0.95 ? Vector3::RIGHT : Vector3::UP; side = tangent.cross(reference).normalized(); }
		else { side = (previous_side - tangent * previous_side.dot(tangent)).normalized(); if (side.is_zero_approx()) side = tangent.cross(Vector3::RIGHT).normalized(); }
		previous_side = side; sides.write[i] = side; binormals.write[i] = tangent.cross(side).normalized();
	}
	if (p_ribbon) {
		for (int i = 0; i < p_points.size() - 1; i++) {
			real_t v0 = lengths[i] / total_length, v1 = lengths[i + 1] / total_length;
			real_t r0 = p_base_radius * Math::lerp((real_t)1.0, p_tip_scale, v0), r1 = p_base_radius * Math::lerp((real_t)1.0, p_tip_scale, v1);
			Vector3 a = p_points[i] - sides[i] * r0, b = p_points[i] + sides[i] * r0, c = p_points[i + 1] - sides[i + 1] * r1, d = p_points[i + 1] + sides[i + 1] * r1;
			Vector3 n = binormals[i]; real_t fixed0 = p_attached.size() == p_points.size() && p_attached[i] ? 0.12 : v0; real_t fixed1 = p_attached.size() == p_points.size() && p_attached[i + 1] ? 0.12 : v1;
			Color c0(fixed0, p_branch_weight * v0, 0.0, p_phase), c1(fixed1, p_branch_weight * v1, 0.0, p_phase);
			_vine_vertex(p_surface, a, n, Vector2(0, v0), c0); _vine_vertex(p_surface, c, n, Vector2(0, v1), c1); _vine_vertex(p_surface, d, n, Vector2(1, v1), c1);
			_vine_vertex(p_surface, a, n, Vector2(0, v0), c0); _vine_vertex(p_surface, d, n, Vector2(1, v1), c1); _vine_vertex(p_surface, b, n, Vector2(1, v0), c0);
			_vine_vertex(p_surface, d, -n, Vector2(1, v1), c1); _vine_vertex(p_surface, c, -n, Vector2(0, v1), c1); _vine_vertex(p_surface, a, -n, Vector2(0, v0), c0);
			_vine_vertex(p_surface, b, -n, Vector2(1, v0), c0); _vine_vertex(p_surface, d, -n, Vector2(1, v1), c1); _vine_vertex(p_surface, a, -n, Vector2(0, v0), c0);
		}
		return;
	}
	p_sides = MAX(3, p_sides);
	for (int i = 0; i < p_points.size() - 1; i++) {
		real_t v0 = lengths[i] / total_length, v1 = lengths[i + 1] / total_length;
		real_t r0 = p_base_radius * Math::lerp((real_t)1.0, p_tip_scale, v0), r1 = p_base_radius * Math::lerp((real_t)1.0, p_tip_scale, v1);
		real_t fixed0 = p_attached.size() == p_points.size() && p_attached[i] ? 0.12 : v0, fixed1 = p_attached.size() == p_points.size() && p_attached[i + 1] ? 0.12 : v1;
		Color c0(fixed0, p_branch_weight * v0, 0.0, p_phase), c1(fixed1, p_branch_weight * v1, 0.0, p_phase);
		for (int side_index = 0; side_index < p_sides; side_index++) {
			real_t a0 = Math::TAU * side_index / p_sides, a1 = Math::TAU * (side_index + 1) / p_sides;
			Vector3 n00 = (sides[i] * Math::cos(a0) + binormals[i] * Math::sin(a0)).normalized(); Vector3 n01 = (sides[i] * Math::cos(a1) + binormals[i] * Math::sin(a1)).normalized();
			Vector3 n10 = (sides[i + 1] * Math::cos(a0) + binormals[i + 1] * Math::sin(a0)).normalized(); Vector3 n11 = (sides[i + 1] * Math::cos(a1) + binormals[i + 1] * Math::sin(a1)).normalized();
			Vector3 p00 = p_points[i] + n00 * r0, p01 = p_points[i] + n01 * r0, p10 = p_points[i + 1] + n10 * r1, p11 = p_points[i + 1] + n11 * r1;
			real_t u0 = (real_t)side_index / p_sides, u1 = (real_t)(side_index + 1) / p_sides;
			_vine_vertex(p_surface, p00, n00, Vector2(u0, v0), c0); _vine_vertex(p_surface, p10, n10, Vector2(u0, v1), c1); _vine_vertex(p_surface, p11, n11, Vector2(u1, v1), c1);
			_vine_vertex(p_surface, p00, n00, Vector2(u0, v0), c0); _vine_vertex(p_surface, p11, n11, Vector2(u1, v1), c1); _vine_vertex(p_surface, p01, n01, Vector2(u1, v0), c0);
		}
	}
}

static void _append_branch_collar(const Ref<SurfaceTool> &p_surface, const Vector3 &p_origin, const Vector3 &p_direction, real_t p_parent_radius, real_t p_branch_radius, real_t p_scale, real_t p_length, int p_sides, real_t p_phase) {
	if (p_length <= 0.0 || p_direction.is_zero_approx()) return;
	Vector3 tangent = p_direction.normalized(); Vector3 reference = Math::abs(tangent.dot(Vector3::UP)) > 0.95 ? Vector3::RIGHT : Vector3::UP; Vector3 side = tangent.cross(reference).normalized(); Vector3 binormal = tangent.cross(side).normalized();
	Vector3 centers[3] = { p_origin + tangent * p_parent_radius * 0.3, p_origin + tangent * p_parent_radius * 0.8, p_origin + tangent * (p_parent_radius + p_length) };
	real_t radii[3] = { p_branch_radius * (real_t)0.78, p_branch_radius * p_scale, p_branch_radius };
	p_sides = MAX(3, p_sides);
	for (int ring = 0; ring < 2; ring++) {
		for (int side_index = 0; side_index < p_sides; side_index++) {
			real_t a0 = Math::TAU * side_index / p_sides, a1 = Math::TAU * (side_index + 1) / p_sides; Vector3 n0 = (side * Math::cos(a0) + binormal * Math::sin(a0)).normalized(); Vector3 n1 = (side * Math::cos(a1) + binormal * Math::sin(a1)).normalized();
			Vector3 p00 = centers[ring] + n0 * radii[ring], p01 = centers[ring] + n1 * radii[ring], p10 = centers[ring + 1] + n0 * radii[ring + 1], p11 = centers[ring + 1] + n1 * radii[ring + 1]; real_t u0 = (real_t)side_index / p_sides, u1 = (real_t)(side_index + 1) / p_sides, v0 = (real_t)ring * 0.5, v1 = (real_t)(ring + 1) * 0.5; Color color(0.12 + v1 * 0.18, v1, 0.0, p_phase);
			_vine_vertex(p_surface, p00, n0, Vector2(u0, v0), color); _vine_vertex(p_surface, p10, n0, Vector2(u0, v1), color); _vine_vertex(p_surface, p11, n1, Vector2(u1, v1), color); _vine_vertex(p_surface, p00, n0, Vector2(u0, v0), color); _vine_vertex(p_surface, p11, n1, Vector2(u1, v1), color); _vine_vertex(p_surface, p01, n1, Vector2(u1, v0), color);
		}
	}
}

static void _append_thorn(const Ref<SurfaceTool> &p_surface, const Vector3 &p_centerline, const Vector3 &p_tangent, const Vector3 &p_radial, real_t p_stem_radius, real_t p_size, real_t p_phase, real_t p_path_ratio, real_t p_branch_weight) {
	Vector3 tangent = p_tangent.normalized(), radial = p_radial.normalized(); if (tangent.is_zero_approx() || radial.is_zero_approx()) return; Vector3 side = tangent.cross(radial).normalized(); if (side.is_zero_approx()) return;
	Vector3 center = p_centerline + radial * p_stem_radius; real_t half_width = p_size * 0.22; Vector3 a = center + side * half_width + tangent * half_width, b = center - side * half_width + tangent * half_width, c = center - tangent * half_width, tip = center + radial * p_size; Color color(p_path_ratio, p_branch_weight, 0.0, p_phase);
	auto triangle = [&](const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c, const Vector2 &p_uv_a, const Vector2 &p_uv_b, const Vector2 &p_uv_c) { Vector3 normal = (p_b - p_a).cross(p_c - p_a).normalized(); if (normal.is_zero_approx()) normal = radial; _vine_vertex(p_surface, p_a, normal, p_uv_a, color); _vine_vertex(p_surface, p_b, normal, p_uv_b, color); _vine_vertex(p_surface, p_c, normal, p_uv_c, color); };
	triangle(a, b, tip, Vector2(0, 0), Vector2(1, 0), Vector2(0.5, 1)); triangle(b, c, tip, Vector2(0, 0), Vector2(1, 0), Vector2(0.5, 1)); triangle(c, a, tip, Vector2(0, 0), Vector2(1, 0), Vector2(0.5, 1)); triangle(c, b, a, Vector2(0.5, 1), Vector2(1, 0), Vector2(0, 0));
}

static void _append_leaf(const Ref<SurfaceTool> &p_surface, const Vector3 &p_center, const Vector3 &p_tangent, const Vector3 &p_normal_hint, real_t p_size, real_t p_width_scale, real_t p_phase, real_t p_path_ratio, int p_side_sign) {
	Vector3 normal = p_normal_hint.is_zero_approx() ? Vector3::UP : p_normal_hint.normalized();
	Vector3 side = p_tangent.cross(normal).normalized(); if (side.is_zero_approx()) side = Vector3::RIGHT; side *= p_side_sign;
	Vector3 tangent = p_tangent.normalized(), outward = normal; Vector3 root = p_center - tangent * p_size * 0.12; Vector3 a = p_center + tangent * p_size * 0.3 - side * p_size * p_width_scale * 0.5; Vector3 b = p_center + tangent * p_size * 0.3 + side * p_size * p_width_scale * 0.5; Vector3 tip = p_center + tangent * p_size + outward * p_size * 0.18;
	Color color(p_path_ratio, 0.8, 1.0, p_phase);
	_vine_vertex(p_surface, root, outward, Vector2(0.5, 0), color); _vine_vertex(p_surface, a, outward, Vector2(0, 0.35), color); _vine_vertex(p_surface, tip, outward, Vector2(0.5, 1), color); _vine_vertex(p_surface, root, outward, Vector2(0.5, 0), color); _vine_vertex(p_surface, tip, outward, Vector2(0.5, 1), color); _vine_vertex(p_surface, b, outward, Vector2(1, 0.35), color);
	_vine_vertex(p_surface, tip, -outward, Vector2(0.5, 1), color); _vine_vertex(p_surface, a, -outward, Vector2(0, 0.35), color); _vine_vertex(p_surface, root, -outward, Vector2(0.5, 0), color); _vine_vertex(p_surface, b, -outward, Vector2(1, 0.35), color); _vine_vertex(p_surface, tip, -outward, Vector2(0.5, 1), color); _vine_vertex(p_surface, root, -outward, Vector2(0.5, 0), color);
}

static Ref<ShaderMaterial> _make_vine_wind_material(const Color &p_color) {
	Ref<Shader> shader; shader.instantiate();
	shader->set_code(R"(shader_type spatial;
render_mode cull_disabled, depth_draw_opaque;
uniform vec4 base_color : source_color = vec4(1.0);
uniform float wind_strength = 0.18;
uniform float wind_speed = 1.0;
void vertex() {
	float wave = sin(TIME * wind_speed + VERTEX.y * 1.7 + COLOR.a * 6.28318);
	VERTEX.xz += vec2(1.0, 0.35) * wave * wind_strength * (COLOR.r + COLOR.g * 0.35 + COLOR.b * 0.18);
}
void fragment() { ALBEDO = base_color.rgb; ROUGHNESS = 0.9; }
)");
	Ref<ShaderMaterial> material; material.instantiate(); material->set_shader(shader); material->set_shader_parameter("base_color", p_color); return material;
}
}

void OpenWorldVineGenerator3D::_request_changed() { if (auto_generate) generate_vine(); }

Dictionary OpenWorldVineGenerator3D::validate_request() const {
	Dictionary report; PackedStringArray errors; PackedStringArray error_codes; PackedStringArray warnings; PackedStringArray warning_codes;
	auto add_error = [&errors, &error_codes](const String &p_code, const String &p_message) { error_codes.push_back(p_code); errors.push_back(p_code + ": " + p_message); };
	auto add_warning = [&warnings, &warning_codes](const String &p_code, const String &p_message) { warning_codes.push_back(p_code); warnings.push_back(p_code + ": " + p_message); };
	report["success"] = false;
	if (generation_request.is_null()) add_error("REQUEST_MISSING", "assign generation_request.");
	else {
		if (generation_request->get_profile().is_null()) add_error("PROFILE_MISSING", "assign request.profile.");
		if (generation_request->get_desired_length() <= 0.0 && generation_request->get_explicit_anchors().size() < 2) add_error("LENGTH_INVALID", "desired_length must be positive.");
		if (!generation_request->get_explicit_anchors().is_empty() && generation_request->get_explicit_anchors().size() < 2) add_error("ANCHORS_INVALID", "explicit_anchors requires at least two points.");
		if (!generation_request->get_explicit_normals().is_empty() && generation_request->get_explicit_normals().size() != generation_request->get_explicit_anchors().size()) add_error("NORMALS_INVALID", "explicit_normals must match explicit_anchors.");
		const bool needs_support = generation_request->get_explicit_anchors().is_empty() && generation_request->get_mode() != OpenWorldVineGenerationRequest::MODE_HANGING && generation_request->get_mode() != OpenWorldVineGenerationRequest::MODE_BRAMBLE;
		if (needs_support && generation_request->get_support_path().is_empty()) add_error("SUPPORT_MISSING", "this mode requires support_path or explicit_anchors.");
		if (generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_BRAMBLE && !generation_request->get_support_path().is_empty()) add_warning("BRAMBLE_SUPPORT_IGNORED", "Bramble is self-supporting and ignores support_path.");
		if (needs_support && !generation_request->get_support_path().is_empty()) {
			Node *support = get_node_or_null(generation_request->get_support_path());
			if (!support) add_error("SUPPORT_NOT_FOUND", "support_path cannot be resolved from generator.");
			else if (needs_support && generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_TREE_WRAP && !Object::cast_to<OpenWorldTreeGenerator3D>(support)) add_error("SUPPORT_TYPE_UNSUPPORTED", "TreeWrap requires OpenWorldTreeGenerator3D.");
			else if (needs_support && generation_request->get_mode() != OpenWorldVineGenerationRequest::MODE_TREE_WRAP) {
				MeshInstance3D *mesh_support = Object::cast_to<MeshInstance3D>(support);
				if (!mesh_support && !Object::cast_to<OpenWorldTerrain3D>(support)) add_error("SUPPORT_TYPE_UNSUPPORTED", "surface modes require MeshInstance3D or OpenWorldTerrain3D.");
				else if (mesh_support && (mesh_support->get_mesh().is_null() || mesh_support->get_mesh()->get_surface_count() == 0)) add_error("MESH_EMPTY", "support MeshInstance3D has no mesh surfaces.");
			}
		}
	}
	report["errors"] = errors; report["error_codes"] = error_codes; report["warnings"] = warnings; report["warning_codes"] = warning_codes; report["error_count"] = errors.size(); report["success"] = errors.is_empty(); report["frame_error_count"] = 0; report["max_surface_gap"] = 0.0;
	if (generation_request.is_valid()) { report["mode"] = (int)generation_request->get_mode(); report["seed"] = generation_request->get_seed(); }
	return report;
}

bool OpenWorldVineGenerator3D::_project_to_support(Node *p_support, const Vector3 &p_origin, const Vector3 &p_direction, real_t p_distance, Vector3 &r_position, Vector3 &r_normal) const {
	Node3D *support_3d = Object::cast_to<Node3D>(p_support); if (!support_3d || p_direction.is_zero_approx()) return false;
	Transform3D generator_transform = _authoring_transform(this); Vector3 global_origin = generator_transform.xform(p_origin); Vector3 global_direction = generator_transform.basis.xform(p_direction).normalized();
	if (OpenWorldTerrain3D *terrain = Object::cast_to<OpenWorldTerrain3D>(p_support)) {
		Dictionary hit = terrain->get_brush_hit(global_origin, global_direction); if (hit.is_empty() || (real_t)hit.get("distance", Math::INF) > p_distance) return false;
		r_position = generator_transform.affine_inverse().xform((Vector3)hit["position"]); Vector3 world_normal = hit.get("normal", Vector3::UP); r_normal = generator_transform.basis.inverse().xform(world_normal).normalized(); return true;
	}
	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(p_support); if (!mesh_instance) return false;
	Ref<TriangleMesh> triangles = mesh_instance->generate_triangle_mesh(); if (triangles.is_null() || !triangles->is_valid()) return false;
	Transform3D support_transform = _authoring_transform(mesh_instance); Transform3D support_inverse = support_transform.affine_inverse(); Vector3 local_begin = support_inverse.xform(global_origin); Vector3 local_end = support_inverse.xform(global_origin + global_direction * p_distance);
	Vector3 hit_position, hit_normal; if (!triangles->intersect_segment(local_begin, local_end, hit_position, hit_normal)) return false;
	Vector3 world_position = support_transform.xform(hit_position); Vector3 world_normal = support_transform.basis.xform(hit_normal).normalized();
	r_position = generator_transform.affine_inverse().xform(world_position); r_normal = generator_transform.basis.inverse().xform(world_normal).normalized(); return true;
}

Ref<OpenWorldVinePathData> OpenWorldVineGenerator3D::generate_path() {
	generation_report = validate_request(); if (!(bool)generation_report["success"]) { generated_path.unref(); return generated_path; }
	Ref<OpenWorldVineGenerationProfile> profile = generation_request->get_profile(); Ref<OpenWorldVinePathData> result; result.instantiate();
	PackedVector3Array points; PackedVector3Array normals; PackedByteArray attached; int tree_support_segment = -1; RandomPCG random((uint64_t)(uint32_t)generation_request->get_seed());
	const PackedVector3Array explicit_points = generation_request->get_explicit_anchors();
	if (!explicit_points.is_empty()) {
		points = explicit_points; normals = generation_request->get_explicit_normals(); bool supplied_normals = normals.size() == points.size(); normals.resize(points.size()); attached.resize(points.size());
		for (int i = 0; i < points.size(); i++) { if (!supplied_normals || normals[i].is_zero_approx()) normals.set(i, Vector3::UP); attached.set(i, 0); }
	} else if (generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_HANGING) {
		int segments = MAX(2, (int)Math::ceil(generation_request->get_desired_length() / profile->get_segment_length())); points.resize(segments + 1); normals.resize(segments + 1); attached.resize(segments + 1);
		Vector3 start = generation_request->get_start_position(); Vector3 end = generation_request->is_target_enabled() ? generation_request->get_target_position() : start + generation_request->get_start_direction() * generation_request->get_desired_length() * 0.55 + Vector3::DOWN * generation_request->get_desired_length() * 0.55;
		for (int i = 0; i <= segments; i++) { real_t t = (real_t)i / segments; points.set(i, start.lerp(end, t) + Vector3::DOWN * (4.0 * profile->get_hanging_sag() * t * (1.0 - t))); normals.set(i, Vector3::UP); attached.set(i, i == 0 ? 1 : 0); }
	} else if (generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_BRAMBLE) {
		const Vector3 bramble_base = generation_request->get_start_position(); const Vector3 center = bramble_base + Vector3::UP * profile->get_bramble_height() * 0.5; const real_t radius = profile->get_bramble_radius(); const real_t half_height = profile->get_bramble_height() * 0.5;
		const int stem_count = MIN(profile->get_bramble_stem_count(), MAX(1, generation_request->get_branch_budget() + 1)); const int segments = CLAMP((int)Math::ceil(generation_request->get_desired_length() / profile->get_segment_length()), 3, 4096); int total_anchors = 0; AABB path_bounds; bool bounds_initialized = false;
		for (int stem_index = 0; stem_index < stem_count; stem_index++) {
			PackedVector3Array stem_points; PackedVector3Array stem_normals; PackedByteArray stem_attached; const bool grounded = random.randf() < profile->get_bramble_ground_anchor_ratio(); const real_t start_angle = Math::TAU * stem_index / stem_count + _vine_random(random, -0.28, 0.28); const real_t start_radius = radius * (grounded ? _vine_random(random, 0.05, 0.28) : _vine_random(random, 0.3, 0.78)); const real_t spin_sign = random.randf() < 0.5 ? -1.0 : 1.0;
			Vector3 current = bramble_base + Vector3(Math::cos(start_angle) * start_radius, grounded ? profile->get_bramble_height() * 0.04 : _vine_random(random, profile->get_bramble_height() * 0.12, profile->get_bramble_height() * 0.62), Math::sin(start_angle) * start_radius); Vector3 direction(-Math::sin(start_angle) * spin_sign, _vine_random(random, 0.15, 0.65), Math::cos(start_angle) * spin_sign); direction.normalize();
			for (int step = 0; step <= segments; step++) {
				Vector3 relative = current - center; Vector3 ellipsoid_normal(relative.x / (radius * radius), relative.y / (half_height * half_height), relative.z / (radius * radius)); if (ellipsoid_normal.is_zero_approx()) ellipsoid_normal = Vector3::UP; else ellipsoid_normal.normalize(); stem_points.push_back(current); stem_normals.push_back(ellipsoid_normal); stem_attached.push_back(step == 0 && grounded ? 1 : 0); if (!bounds_initialized) { path_bounds = AABB(current, Vector3()); bounds_initialized = true; } else path_bounds.expand_to(current); if (step == segments) break;
				Vector3 horizontal(relative.x, 0, relative.z); if (horizontal.is_zero_approx()) horizontal = Vector3(Math::cos(start_angle), 0, Math::sin(start_angle)); Vector3 outward = horizontal.normalized(); Vector3 swirl(-outward.z * spin_sign, 0, outward.x * spin_sign); real_t radial_ratio = horizontal.length() / radius; real_t target_ratio = Math::lerp((real_t)0.32, (real_t)0.88, profile->get_bramble_surface_bias()) + Math::sin((real_t)step * 0.71 + start_angle) * 0.12; Vector3 radial_correction = outward * (target_ratio - radial_ratio); real_t vertical_target = center.y + Math::sin((real_t)step * profile->get_segment_length() / MAX(radius, (real_t)0.1) * 1.7 + start_angle * 1.9) * half_height * 0.78; Vector3 vertical = Vector3::UP * ((vertical_target - current.y) / MAX(half_height, (real_t)0.05)); Vector3 noise(_vine_random(random, -1.0, 1.0), _vine_random(random, -0.7, 0.7), _vine_random(random, -1.0, 1.0)); Vector3 field = swirl + radial_correction * (0.8 + profile->get_bramble_tangle_strength()) + vertical * (0.45 + profile->get_bramble_tangle_strength() * 0.4) + noise * profile->get_bramble_tangle_strength() * 0.38; if (!field.is_zero_approx()) direction = direction.lerp(field.normalized(), 0.24 + profile->get_bramble_tangle_strength() * 0.38).normalized(); current += direction * profile->get_segment_length(); current.y = CLAMP(current.y, bramble_base.y, bramble_base.y + profile->get_bramble_height()); Vector3 bounded = current - center; real_t ellipsoid_distance = Math::sqrt(bounded.x * bounded.x / (radius * radius) + bounded.y * bounded.y / (half_height * half_height) + bounded.z * bounded.z / (radius * radius)); if (ellipsoid_distance > 0.98) current = center + bounded * (0.98 / ellipsoid_distance);
			}
			result->add_path(stem_points, stem_normals, stem_attached, -1, -1); total_anchors += stem_points.size();
		}
		generated_path = result; generation_report["success"] = true; generation_report["anchor_count"] = total_anchors; generation_report["branch_count"] = 0; generation_report["bramble_stem_count"] = stem_count; generation_report["bramble_bounds"] = path_bounds; generation_report["total_length"] = result->get_total_length(); return result;
	} else if (generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_TREE_WRAP) {
		OpenWorldTreeGenerator3D *tree = Object::cast_to<OpenWorldTreeGenerator3D>(get_node_or_null(generation_request->get_support_path())); Ref<OpenWorldTreeSupportGraph> graph = tree ? tree->get_generated_support_graph() : Ref<OpenWorldTreeSupportGraph>();
		if (graph.is_null() || graph->get_path_count() == 0) { PackedStringArray errors; errors.push_back("TREE_SUPPORT_MISSING: generate the support tree first."); PackedStringArray error_codes; error_codes.push_back("TREE_SUPPORT_MISSING"); generation_report["success"] = false; generation_report["errors"] = errors; generation_report["error_codes"] = error_codes; generated_path.unref(); return generated_path; }
		tree_support_segment = 0; if (graph->get_path_count() > 1 && random.randf() < profile->get_tree_wrap_branch_chance()) tree_support_segment = 1 + (random.rand() % (graph->get_path_count() - 1)); PackedVector3Array trunk = graph->get_path_points(tree_support_segment); PackedFloat32Array radii = graph->get_path_radii(tree_support_segment); int segments = MAX(2, (int)Math::ceil(generation_request->get_desired_length() / profile->get_segment_length())); points.resize(segments + 1); normals.resize(segments + 1); attached.resize(segments + 1);
		Transform3D to_generator = _authoring_transform(this).affine_inverse() * _authoring_transform(tree); real_t start_angle = _vine_random(random, 0.0, Math::TAU);
		for (int i = 0; i <= segments; i++) { real_t t = (real_t)i / segments; real_t sample = t * (trunk.size() - 1); int index = MIN((int)Math::floor(sample), trunk.size() - 2); real_t f = sample - index; Vector3 center = trunk[index].lerp(trunk[index + 1], f); real_t radius = Math::lerp((real_t)radii[index], (real_t)radii[index + 1], f) + profile->get_surface_offset(); Vector3 tangent = (trunk[index + 1] - trunk[index]).normalized(); Vector3 side = tangent.cross(Math::abs(tangent.dot(Vector3::UP)) > 0.95 ? Vector3::RIGHT : Vector3::UP).normalized(); Vector3 binormal = tangent.cross(side).normalized(); real_t angle = start_angle + Math::TAU * profile->get_tree_wrap_turns() * t; Vector3 normal = side * Math::cos(angle) + binormal * Math::sin(angle); points.set(i, to_generator.xform(center + normal * radius)); normals.set(i, to_generator.basis.xform(normal).normalized()); attached.set(i, 1); }
	} else {
		Node *support = get_node_or_null(generation_request->get_support_path()); int max_steps = CLAMP((int)Math::ceil(generation_request->get_desired_length() / profile->get_segment_length()), 1, 4096);
		Vector3 current = generation_request->get_start_position(), normal = Vector3::UP, direction = generation_request->get_start_direction().normalized(), projected, projected_normal;
		Vector3 initial_ray = generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_CREEPING ? Vector3::DOWN : -direction;
		bool initial_hit = _project_to_support(support, current - initial_ray * profile->get_support_probe_distance() * 0.5, initial_ray, profile->get_support_probe_distance() * 1.5, projected, projected_normal);
		if (!initial_hit && generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_CLIMBING) { const Vector3 axes[6] = { Vector3::LEFT, Vector3::RIGHT, Vector3::UP, Vector3::DOWN, Vector3::FORWARD, Vector3::BACK }; for (Vector3 axis : axes) if (_project_to_support(support, current - axis * profile->get_support_probe_distance() * 0.5, axis, profile->get_support_probe_distance(), projected, projected_normal)) { initial_hit = true; break; } }
		if (initial_hit && generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_CREEPING && Math::rad_to_deg(projected_normal.angle_to(Vector3::UP)) > profile->get_max_slope_degrees()) initial_hit = false;
		if (!initial_hit) { PackedStringArray errors; errors.push_back("SURFACE_START_MISS: no acceptable support surface near start_position."); PackedStringArray error_codes; error_codes.push_back("SURFACE_START_MISS"); generation_report["success"] = false; generation_report["errors"] = errors; generation_report["error_codes"] = error_codes; generation_report["max_surface_gap"] = profile->get_support_probe_distance(); generated_path.unref(); return generated_path; }
		current = projected + projected_normal * profile->get_surface_offset(); normal = projected_normal; points.push_back(current); normals.push_back(normal); attached.push_back(1);
		for (int step = 0; step < max_steps; step++) {
			Vector3 tangent = direction - normal * direction.dot(normal); if (generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_CLIMBING) { Vector3 up_tangent = Vector3::UP - normal * Vector3::UP.dot(normal); tangent = tangent.lerp(up_tangent.normalized(), profile->get_climbing_up_bias()); }
			Vector3 side = normal.cross(tangent).normalized(); tangent = (tangent.normalized() + side * _vine_random(random, -profile->get_turn_noise(), profile->get_turn_noise())).normalized(); Vector3 candidate = current + tangent * profile->get_segment_length();
			Vector3 ray_direction = generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_CREEPING ? Vector3::DOWN : -normal; Vector3 ray_origin = candidate - ray_direction * profile->get_support_probe_distance() * 0.5;
			bool support_hit = _project_to_support(support, ray_origin, ray_direction, profile->get_support_probe_distance() * 1.5, projected, projected_normal); if (support_hit && generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_CREEPING && Math::rad_to_deg(projected_normal.angle_to(Vector3::UP)) > profile->get_max_slope_degrees()) support_hit = false;
			if (!support_hit) { generation_report["surface_gap_anchor"] = points.size(); generation_report["max_surface_gap"] = profile->get_support_probe_distance(); if (profile->get_surface_gap_policy() == OpenWorldVineGenerationProfile::SURFACE_GAP_SWITCH_TO_HANGING) { current = candidate + Vector3::DOWN * profile->get_segment_length() * 0.35; points.push_back(current); normals.push_back(normal); attached.push_back(0); direction = tangent + Vector3::DOWN * 0.2; continue; } break; }
			current = projected + projected_normal * profile->get_surface_offset(); direction = current - points[points.size() - 1]; normal = projected_normal; points.push_back(current); normals.push_back(normal); attached.push_back(1);
		}
	}
	result->add_path(points, normals, attached, -1, tree_support_segment);
	int branch_count = MIN(generation_request->get_branch_budget(), profile->get_side_branch_count());
	for (int branch = 0; branch < branch_count && points.size() >= 4; branch++) { int origin_index = (branch + 1) * (points.size() - 1) / (branch_count + 1); Vector3 tangent = (points[MIN(origin_index + 1, points.size() - 1)] - points[MAX(0, origin_index - 1)]).normalized(); Vector3 branch_normal = normals[origin_index]; Vector3 side = tangent.cross(branch_normal).normalized() * (branch % 2 == 0 ? 1.0 : -1.0); int count = MAX(1, (int)Math::ceil(profile->get_side_branch_length() / profile->get_segment_length())); PackedVector3Array branch_points; PackedVector3Array branch_normals; PackedByteArray branch_attached; for (int i = 0; i <= count; i++) { real_t t = (real_t)i / count; branch_points.push_back(points[origin_index] + side * profile->get_side_branch_length() * t + Vector3::DOWN * profile->get_hanging_sag() * 0.25 * t * t); branch_normals.push_back(branch_normal); branch_attached.push_back(i == 0 ? 1 : 0); } result->add_path(branch_points, branch_normals, branch_attached, 0, -1); }
	generated_path = result; generation_report["success"] = true; generation_report["anchor_count"] = points.size(); generation_report["branch_count"] = result->get_path_count() - 1; generation_report["total_length"] = result->get_total_length(); return result;
}

Ref<ArrayMesh> OpenWorldVineGenerator3D::_generate_lod_mesh(int p_lod) const {
	if (generated_path.is_null() || generation_request.is_null() || generation_request->get_profile().is_null()) return Ref<ArrayMesh>();
	Ref<OpenWorldVineGenerationProfile> profile = generation_request->get_profile(); p_lod = CLAMP(p_lod, 0, 2); real_t quality = p_lod == 0 ? 1.0 : (p_lod == 1 ? profile->get_lod1_quality() : profile->get_lod2_quality()); int sides = p_lod == 2 ? 2 : MAX(3, (int)Math::round(profile->get_radial_sides() * Math::lerp((real_t)0.55, (real_t)1.0, quality))); real_t phase = Math::fposmod((real_t)(uint32_t)generation_request->get_seed() * (real_t)0.0000001192092896, (real_t)1.0); int path_stride = p_lod == 2 ? 2 : 1; const bool bramble = generation_request->get_mode() == OpenWorldVineGenerationRequest::MODE_BRAMBLE;
	Ref<SurfaceTool> stem; stem.instantiate(); stem->begin(Mesh::PRIMITIVE_TRIANGLES); PackedInt32Array parent_paths = generated_path->get_parent_paths(); int junction_count = 0;
	for (int path_index = 0; path_index < generated_path->get_path_count(); path_index++) {
		if (path_index > 0 && path_index % path_stride != 0) continue; PackedVector3Array path = generated_path->get_path_points(path_index); real_t path_radius = profile->get_stem_radius() * (path_index == 0 || bramble ? 1.0 : 0.62); _append_tube(stem, path, path_radius, profile->get_tip_radius_scale(), sides, phase, bramble ? 0.65 : (path_index == 0 ? 0.3 : 1.0), generated_path->get_path_attached_flags(path_index), p_lod == 2);
		if (p_lod < 2 && path_index < parent_paths.size() && parent_paths[path_index] >= 0 && path.size() >= 2) {
			int parent_index = parent_paths[path_index]; PackedVector3Array parent = generated_path->get_path_points(parent_index); if (parent.size() >= 2) { int nearest = 0; real_t nearest_distance = Math::INF; for (int i = 0; i < parent.size(); i++) { real_t distance = parent[i].distance_squared_to(path[0]); if (distance < nearest_distance) { nearest = i; nearest_distance = distance; } } real_t parent_ratio = (real_t)nearest / MAX(1, parent.size() - 1); real_t parent_base = profile->get_stem_radius() * (parent_index == 0 ? 1.0 : 0.62); real_t parent_radius = parent_base * Math::lerp((real_t)1.0, profile->get_tip_radius_scale(), parent_ratio); _append_branch_collar(stem, path[0], path[1] - path[0], parent_radius, path_radius, profile->get_branch_junction_scale(), profile->get_branch_junction_length(), sides, phase); junction_count++; }
		}
	}
	RandomPCG thorn_random((uint64_t)(uint32_t)(generation_request->get_seed() ^ 0x2c9277b5)); int thorn_count = 0;
	if (profile->get_thorn_density() > 0.0 && p_lod < 2) {
		real_t lod_density = profile->get_thorn_density() * (p_lod == 0 ? 1.0 : 0.45); real_t spacing = profile->get_thorn_spacing() * (p_lod == 0 ? 1.0 : 1.7);
		for (int path_index = 0; path_index < generated_path->get_path_count(); path_index++) { if (path_index > 0 && path_index % path_stride != 0) continue; PackedVector3Array path = generated_path->get_path_points(path_index); PackedVector3Array path_normals = generated_path->get_path_normals(path_index); real_t accumulated = 0.0, next_thorn = spacing; for (int i = 1; i < path.size() - 1; i++) { accumulated += path[i - 1].distance_to(path[i]); if (accumulated < next_thorn) continue; next_thorn += spacing; if (thorn_random.randf() > lod_density) continue; Vector3 tangent = (path[i + 1] - path[i - 1]).normalized(); Vector3 normal = path_normals.size() == path.size() && !path_normals[i].is_zero_approx() ? path_normals[i].normalized() : Vector3::UP; Vector3 radial = normal.rotated(tangent, _vine_random(thorn_random, 0.0, Math::TAU)).normalized(); real_t ratio = (real_t)i / (path.size() - 1); real_t base_radius = profile->get_stem_radius() * (path_index == 0 || bramble ? 1.0 : 0.62) * Math::lerp((real_t)1.0, profile->get_tip_radius_scale(), ratio); _append_thorn(stem, path[i], tangent, radial, base_radius, profile->get_thorn_size() * _vine_random(thorn_random, 0.82, 1.18), phase, ratio, bramble ? 0.65 : (path_index == 0 ? 0.3 : 1.0)); thorn_count++; } }
	}
	stem->index(); stem->generate_tangents(); Ref<ArrayMesh> result_mesh = stem->commit();
	Ref<SurfaceTool> foliage; foliage.instantiate(); foliage->begin(Mesh::PRIMITIVE_TRIANGLES); int leaf_stride = p_lod == 0 ? 1 : (p_lod == 1 ? 2 : 4); int cluster_cards = p_lod == 0 ? profile->get_leaf_cluster_card_count() : (p_lod == 1 ? MAX(1, (profile->get_leaf_cluster_card_count() + 1) / 2) : 1); RandomPCG leaf_random((uint64_t)(uint32_t)(generation_request->get_seed() ^ 0x5f3759df)); int leaf_cluster_count = 0, leaf_card_count = 0;
	for (int path_index = 0; path_index < generated_path->get_path_count(); path_index++) {
		PackedVector3Array path = generated_path->get_path_points(path_index); PackedVector3Array path_normals = generated_path->get_path_normals(path_index); real_t accumulated = 0.0, next_leaf = profile->get_leaf_spacing() * leaf_stride;
		for (int i = 1; i < path.size() - 1; i++) { accumulated += path[i - 1].distance_to(path[i]); if (accumulated < next_leaf) continue; next_leaf += profile->get_leaf_spacing() * leaf_stride; if (leaf_random.randf() > profile->get_leaf_density()) continue; Vector3 tangent = (path[i + 1] - path[i - 1]).normalized(); Vector3 normal = path_normals.size() == path.size() && !path_normals[i].is_zero_approx() ? path_normals[i].normalized() : Vector3::UP; real_t ratio = (real_t)i / (path.size() - 1); for (int card = 0; card < cluster_cards; card++) { real_t angle = Math::TAU * card / cluster_cards + _vine_random(leaf_random, -0.22, 0.22); Vector3 card_normal = normal.rotated(tangent, angle).normalized(); real_t size_scale = _vine_random(leaf_random, 1.0 - profile->get_leaf_size_variation(), 1.0 + profile->get_leaf_size_variation()); real_t spread = profile->get_leaf_cluster_spread() * _vine_random(leaf_random, 0.35, 1.0); Vector3 center = path[i] + card_normal * profile->get_leaf_size() * (0.12 + spread) + tangent * profile->get_leaf_size() * _vine_random(leaf_random, -spread * 0.2, spread * 0.2); _append_leaf(foliage, center, tangent, card_normal, profile->get_leaf_size() * size_scale * (p_lod == 2 ? 1.35 : 1.0), profile->get_leaf_width_scale(), phase, ratio, (leaf_card_count + card) % 2 == 0 ? 1 : -1); leaf_card_count++; } leaf_cluster_count++; }
	}
	if (leaf_card_count > 0) { foliage->index(); foliage->generate_tangents(); foliage->commit(result_mesh); }
	result_mesh->set_meta(SNAME("leaf_cluster_count"), leaf_cluster_count); result_mesh->set_meta(SNAME("leaf_card_count"), leaf_card_count); result_mesh->set_meta(SNAME("thorn_count"), thorn_count); result_mesh->set_meta(SNAME("junction_count"), junction_count); result_mesh->set_name(vformat("StylizedVine_%d_%d_LOD%d", (int)generation_request->get_mode(), generation_request->get_seed(), p_lod)); return result_mesh;
}

void OpenWorldVineGenerator3D::generate_vine() { if (generate_path().is_null()) { clear_generated_vine(); return; } for (int lod = 0; lod < 3; lod++) generated_lod_meshes[lod] = _generate_lod_mesh(lod); _update_materials(); _update_preview_mesh(); for (int lod = 0; lod < 3; lod++) generation_report[vformat("lod%d", lod)] = get_lod_statistics(lod); emit_signal(SNAME("vine_generated"), generated_lod_meshes[0]); }
void OpenWorldVineGenerator3D::clear_generated_vine() { generated_path.unref(); for (int i = 0; i < 3; i++) generated_lod_meshes[i].unref(); set_mesh(Ref<Mesh>()); }
Ref<ArrayMesh> OpenWorldVineGenerator3D::get_generated_lod_mesh(int p_lod) const { ERR_FAIL_INDEX_V(p_lod, 3, Ref<ArrayMesh>()); return generated_lod_meshes[p_lod]; }
Dictionary OpenWorldVineGenerator3D::get_lod_statistics(int p_lod) const { Dictionary result; result["lod"] = p_lod; Ref<ArrayMesh> lod_mesh = get_generated_lod_mesh(p_lod); result["available"] = lod_mesh.is_valid(); int vertices = 0, triangles = 0; if (lod_mesh.is_valid()) { for (int surface = 0; surface < lod_mesh->get_surface_count(); surface++) { Array arrays = lod_mesh->surface_get_arrays(surface); PackedVector3Array surface_vertices = arrays[Mesh::ARRAY_VERTEX]; PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX]; vertices += surface_vertices.size(); triangles += indices.is_empty() ? surface_vertices.size() / 3 : indices.size() / 3; } result["surfaces"] = lod_mesh->get_surface_count(); result["aabb"] = lod_mesh->get_aabb(); result["leaf_clusters"] = lod_mesh->get_meta(SNAME("leaf_cluster_count"), 0); result["leaf_cards"] = lod_mesh->get_meta(SNAME("leaf_card_count"), 0); result["thorns"] = lod_mesh->get_meta(SNAME("thorn_count"), 0); result["junctions"] = lod_mesh->get_meta(SNAME("junction_count"), 0); } result["vertices"] = vertices; result["triangles"] = triangles; return result; }

Ref<OpenWorldVineVariant> OpenWorldVineGenerator3D::create_baked_variant() const { ERR_FAIL_COND_V_MSG(generated_lod_meshes[0].is_null(), Ref<OpenWorldVineVariant>(), "Generate a vine before baking."); Ref<OpenWorldVineVariant> variant; variant.instantiate(); variant->set_variant_name(vformat("Stylized Vine %d", generation_request->get_seed())); variant->set_source_seed(generation_request->get_seed()); variant->set_source_mode(generation_request->get_mode()); variant->set_lod0_mesh(generated_lod_meshes[0]->duplicate(true)); variant->set_lod1_mesh(generated_lod_meshes[1]->duplicate(true)); variant->set_lod2_mesh(generated_lod_meshes[2]->duplicate(true)); variant->set_lod1_distance(lod1_distance); variant->set_lod2_distance(lod2_distance); variant->set_max_distance(max_distance); variant->set_support_stable_id(generation_request->get_support_stable_id()); return variant; }

void OpenWorldVineGenerator3D::_update_preview_mesh() { int lod = CLAMP((int)preview_lod, 0, 2); Ref<ArrayMesh> preview = generated_lod_meshes[lod]; if (preview.is_null()) preview = generated_lod_meshes[0]; set_mesh(preview); update_gizmos(); }
void OpenWorldVineGenerator3D::_update_materials() { if (generation_request.is_valid() && generation_request->get_profile().is_valid()) { real_t strength = generation_request->get_profile()->get_wind_strength(), speed = generation_request->get_profile()->get_wind_speed(); wind_stem_material->set_shader_parameter("wind_strength", strength); wind_stem_material->set_shader_parameter("wind_speed", speed); wind_foliage_material->set_shader_parameter("wind_strength", strength); wind_foliage_material->set_shader_parameter("wind_speed", speed); } Ref<Material> selected_stem = wind_enabled ? Ref<Material>(wind_stem_material) : stem_material; Ref<Material> selected_foliage = wind_enabled ? Ref<Material>(wind_foliage_material) : foliage_material; for (int lod = 0; lod < 3; lod++) { Ref<ArrayMesh> vine_mesh = generated_lod_meshes[lod]; if (vine_mesh.is_null()) continue; if (vine_mesh->get_surface_count() > 0) vine_mesh->surface_set_material(0, selected_stem); if (vine_mesh->get_surface_count() > 1) vine_mesh->surface_set_material(1, selected_foliage); } }

void OpenWorldVineGenerator3D::set_generation_request(const Ref<OpenWorldVineGenerationRequest> &p_request) { if (generation_request == p_request) return; if (generation_request.is_valid()) generation_request->disconnect_changed(callable_mp(this, &OpenWorldVineGenerator3D::_request_changed)); generation_request = p_request; if (generation_request.is_valid()) generation_request->connect_changed(callable_mp(this, &OpenWorldVineGenerator3D::_request_changed)); if (auto_generate) generate_vine(); }
void OpenWorldVineGenerator3D::set_auto_generate(bool p_enabled) { auto_generate = p_enabled; }
void OpenWorldVineGenerator3D::set_stem_material(const Ref<Material> &p_material) { stem_material = p_material; _update_materials(); }
void OpenWorldVineGenerator3D::set_foliage_material(const Ref<Material> &p_material) { foliage_material = p_material; _update_materials(); }
void OpenWorldVineGenerator3D::set_wind_enabled(bool p_enabled) { wind_enabled = p_enabled; _update_materials(); }
void OpenWorldVineGenerator3D::set_preview_lod(PreviewLOD p_lod) { preview_lod = (PreviewLOD)CLAMP((int)p_lod, 0, 2); _update_preview_mesh(); }
void OpenWorldVineGenerator3D::set_lod1_distance(real_t p_value) { lod1_distance = MAX((real_t)0.0, p_value); lod2_distance = MAX(lod2_distance, lod1_distance); max_distance = MAX(max_distance, lod2_distance); }
void OpenWorldVineGenerator3D::set_lod2_distance(real_t p_value) { lod2_distance = MAX(lod1_distance, p_value); max_distance = MAX(max_distance, lod2_distance); }
void OpenWorldVineGenerator3D::set_max_distance(real_t p_value) { max_distance = MAX(lod2_distance, p_value); }
void OpenWorldVineGenerator3D::randomize_seed() { if (generation_request.is_null()) return; RandomPCG random; random.randomize(); generation_request->set_seed((int)random.rand()); if (!auto_generate) generate_vine(); }
void OpenWorldVineGenerator3D::_notification(int p_what) { if (p_what == NOTIFICATION_ENTER_TREE && auto_generate && generated_lod_meshes[0].is_null()) generate_vine(); }
OpenWorldVineGenerator3D::OpenWorldVineGenerator3D() { wind_stem_material = _make_vine_wind_material(Color(0.18, 0.32, 0.08)); wind_foliage_material = _make_vine_wind_material(Color(0.24, 0.72, 0.18)); }
OpenWorldVineGenerator3D::~OpenWorldVineGenerator3D() { if (generation_request.is_valid()) generation_request->disconnect_changed(callable_mp(this, &OpenWorldVineGenerator3D::_request_changed)); }

void OpenWorldVineGenerator3D::_bind_methods() {
	BIND_ENUM_CONSTANT(PREVIEW_LOD0); BIND_ENUM_CONSTANT(PREVIEW_LOD1); BIND_ENUM_CONSTANT(PREVIEW_LOD2);
	#define GEN_BIND(name) ClassDB::bind_method(D_METHOD("set_" #name, "value"), &OpenWorldVineGenerator3D::set_##name); ClassDB::bind_method(D_METHOD("get_" #name), &OpenWorldVineGenerator3D::get_##name)
	GEN_BIND(generation_request); GEN_BIND(stem_material); GEN_BIND(foliage_material); GEN_BIND(preview_lod); GEN_BIND(lod1_distance); GEN_BIND(lod2_distance); GEN_BIND(max_distance);
	#undef GEN_BIND
	ClassDB::bind_method(D_METHOD("set_auto_generate", "enabled"), &OpenWorldVineGenerator3D::set_auto_generate); ClassDB::bind_method(D_METHOD("is_auto_generate"), &OpenWorldVineGenerator3D::is_auto_generate);
	ClassDB::bind_method(D_METHOD("set_wind_enabled", "enabled"), &OpenWorldVineGenerator3D::set_wind_enabled); ClassDB::bind_method(D_METHOD("is_wind_enabled"), &OpenWorldVineGenerator3D::is_wind_enabled);
	ClassDB::bind_method(D_METHOD("validate_request"), &OpenWorldVineGenerator3D::validate_request); ClassDB::bind_method(D_METHOD("generate_path"), &OpenWorldVineGenerator3D::generate_path); ClassDB::bind_method(D_METHOD("generate_vine"), &OpenWorldVineGenerator3D::generate_vine); ClassDB::bind_method(D_METHOD("randomize_seed"), &OpenWorldVineGenerator3D::randomize_seed); ClassDB::bind_method(D_METHOD("clear_generated_vine"), &OpenWorldVineGenerator3D::clear_generated_vine);
	ClassDB::bind_method(D_METHOD("get_generated_path"), &OpenWorldVineGenerator3D::get_generated_path); ClassDB::bind_method(D_METHOD("get_generated_lod_mesh", "lod"), &OpenWorldVineGenerator3D::get_generated_lod_mesh); ClassDB::bind_method(D_METHOD("get_lod_statistics", "lod"), &OpenWorldVineGenerator3D::get_lod_statistics); ClassDB::bind_method(D_METHOD("get_generation_report"), &OpenWorldVineGenerator3D::get_generation_report); ClassDB::bind_method(D_METHOD("create_baked_variant"), &OpenWorldVineGenerator3D::create_baked_variant);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "generation_request", PROPERTY_HINT_RESOURCE_TYPE, OpenWorldVineGenerationRequest::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_generation_request", "get_generation_request"); ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate"), "set_auto_generate", "is_auto_generate");
	ADD_GROUP("LOD", ""); ADD_PROPERTY(PropertyInfo(Variant::INT, "preview_lod", PROPERTY_HINT_ENUM, "LOD0,LOD1,LOD2"), "set_preview_lod", "get_preview_lod"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod1_distance", "get_lod1_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod2_distance", "get_lod2_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_max_distance", "get_max_distance");
	ADD_GROUP("Rendering", ""); ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wind_enabled"), "set_wind_enabled", "is_wind_enabled"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stem_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_stem_material", "get_stem_material"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "foliage_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_foliage_material", "get_foliage_material");
	ADD_SIGNAL(MethodInfo("vine_generated", PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "ArrayMesh")));
}
