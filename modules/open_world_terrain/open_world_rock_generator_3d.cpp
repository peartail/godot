/**************************************************************************/
/*  open_world_rock_generator_3d.cpp                                      */
/**************************************************************************/
#include "open_world_rock_generator_3d.h"

#include "core/math/convex_hull.h"
#include "core/math/random_pcg.h"
#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/material.h"
#include "servers/physics_3d/physics_server_3d.h"
#include "servers/rendering/rendering_server.h"

namespace {
struct RockTriangle { int a; int b; int c; };
static bool point_less(const Vector3 &p_a, const Vector3 &p_b) {
	if (p_a.x != p_b.x) return p_a.x < p_b.x;
	if (p_a.y != p_b.y) return p_a.y < p_b.y;
	return p_a.z < p_b.z;
}
static bool triangle_less(const RockTriangle &p_a, const RockTriangle &p_b) {
	if (p_a.a != p_b.a) return p_a.a < p_b.a;
	if (p_a.b != p_b.b) return p_a.b < p_b.b;
	return p_a.c < p_b.c;
}
static uint64_t hash_mix(uint64_t p_hash, int64_t p_value) {
	p_hash ^= (uint64_t)p_value;
	return p_hash * 1099511628211ULL;
}
static int quantize(real_t p_value) { return (int)Math::round(p_value * 100000.0); }
}

void OpenWorldRockGenerator3D::_request_changed() { if (auto_generate && is_inside_tree()) generate_rock(); }
void OpenWorldRockGenerator3D::set_generation_request(const Ref<OpenWorldRockGenerationRequest> &p_value) {
	if (generation_request == p_value) return;
	if (generation_request.is_valid()) generation_request->disconnect_changed(callable_mp(this, &OpenWorldRockGenerator3D::_request_changed));
	generation_request = p_value;
	if (generation_request.is_valid()) generation_request->connect_changed(callable_mp(this, &OpenWorldRockGenerator3D::_request_changed));
	if (auto_generate && is_inside_tree()) generate_rock();
}
void OpenWorldRockGenerator3D::set_auto_generate(bool p_value) { auto_generate = p_value; }
void OpenWorldRockGenerator3D::set_preview_lod(PreviewLOD p_value) { preview_lod = (PreviewLOD)CLAMP((int)p_value, 0, 2); _update_preview_mesh(); }
void OpenWorldRockGenerator3D::set_preview_material(const Ref<Material> &p_value) { preview_material = p_value; for (int lod = 0; lod < 3; lod++) if (generated_lod_meshes[lod].is_valid() && generated_lod_meshes[lod]->get_surface_count() > 0) generated_lod_meshes[lod]->surface_set_material(0, preview_material); }
void OpenWorldRockGenerator3D::set_lod1_distance(real_t p_value) { lod1_distance = MAX((real_t)0.0, p_value); }
void OpenWorldRockGenerator3D::set_lod2_distance(real_t p_value) { lod2_distance = MAX(lod1_distance, p_value); }
void OpenWorldRockGenerator3D::set_max_distance(real_t p_value) { max_distance = MAX(lod2_distance, p_value); }

Dictionary OpenWorldRockGenerator3D::validate_request() const {
	Dictionary result; PackedStringArray errors; PackedStringArray warnings;
	if (generation_request.is_null()) errors.push_back("REQUEST_MISSING");
	else {
		if (generation_request->get_profile().is_null()) errors.push_back("PROFILE_MISSING");
		Vector3 size = generation_request->get_size();
		if (size.x <= 0.0 || size.y <= 0.0 || size.z <= 0.0 || !size.is_finite()) errors.push_back("INVALID_SIZE");
		if (generation_request->get_primary_axis().is_zero_approx() || !generation_request->get_primary_axis().is_finite()) errors.push_back("INVALID_PRIMARY_AXIS");
		PackedVector3Array explicit_points = generation_request->get_explicit_points();
		if (!explicit_points.is_empty() && explicit_points.size() < 12) errors.push_back("EXPLICIT_POINTS_TOO_FEW");
		for (const Vector3 &point : explicit_points) if (!point.is_finite()) { errors.push_back("EXPLICIT_POINT_NON_FINITE"); break; }
		if (generation_request->get_profile().is_valid() && generation_request->get_profile()->get_point_count() > 128) warnings.push_back("HIGH_POINT_COUNT");
	}
	result["success"] = errors.is_empty(); result["error_codes"] = errors; result["warning_codes"] = warnings;
	result["message"] = errors.is_empty() ? "Rock request is valid." : "Rock request validation failed; inspect error_codes.";
	return result;
}

PackedVector3Array OpenWorldRockGenerator3D::_generate_source_points() const {
	PackedVector3Array explicit_points = generation_request->get_explicit_points();
	Ref<OpenWorldRockGenerationProfile> profile = generation_request->get_profile();
	PackedVector3Array points;
	if (!explicit_points.is_empty()) points = explicit_points;
	else {
		int point_count = profile->get_point_count();
		Vector3 half_size = generation_request->get_size() * profile->get_axis_scale() * 0.5;
		OpenWorldRockGenerationRequest::RockMode mode = generation_request->get_mode();
		if (mode == OpenWorldRockGenerationRequest::MODE_SLAB) half_size.y *= 0.45;
		if (mode == OpenWorldRockGenerationRequest::MODE_SHARD) {
			Vector3 axis = generation_request->get_primary_axis().abs(); int dominant = axis.max_axis_index();
			for (int component = 0; component < 3; component++) half_size[component] *= component == dominant ? 1.8 : 0.65;
		}
		int lobes = profile->get_lobe_count();
		for (int lobe = 0; lobe < lobes; lobe++) {
			RandomPCG lobe_rng((uint64_t)(uint32_t)generation_request->get_seed() ^ (0x9e3779b9ULL * (lobe + 1)));
			Vector3 lobe_offset;
			if (lobe > 0) lobe_offset = Vector3(lobe_rng.random(-1.0f, 1.0f), lobe_rng.random(-0.35f, 0.35f), lobe_rng.random(-1.0f, 1.0f)) * half_size * (1.0 - profile->get_lobe_overlap());
			for (int i = lobe; i < point_count; i += lobes) {
				real_t y = 1.0 - 2.0 * ((real_t)i + 0.5) / point_count;
				real_t radius = Math::sqrt(MAX((real_t)0.0, 1.0 - y * y));
				real_t angle = Math::TAU * Math::fmod((real_t)i * (real_t)0.6180339887498948, (real_t)1.0);
				Vector3 direction(Math::cos(angle) * radius, y, Math::sin(angle) * radius);
				RandomPCG point_rng((uint64_t)(uint32_t)generation_request->get_seed() ^ ((uint64_t)(i + 1) * 0x85ebca6bULL));
				real_t radial = 1.0 + point_rng.random(-profile->get_roughness(), profile->get_roughness());
				radial += profile->get_asymmetry() * (direction.x * 0.6 + direction.z * 0.4);
				Vector3 point = direction * half_size * radial + lobe_offset;
				if (mode == OpenWorldRockGenerationRequest::MODE_SLAB && profile->get_strata_strength() > 0.0) {
					real_t step = MAX((real_t)0.02, half_size.y * (0.18 + profile->get_strata_strength() * 0.25)); point.y = Math::snapped(point.y, step);
				}
				if (mode == OpenWorldRockGenerationRequest::MODE_SHARD) { real_t taper = 1.0 - 0.35 * MAX((real_t)0.0, direction.dot(generation_request->get_primary_axis().normalized())); point *= taper; }
				Vector3 strata = profile->get_strata_direction(); point += strata * Math::sin(point.dot(strata) * 4.0 + generation_request->get_seed()) * profile->get_strata_strength() * half_size.length() * 0.08;
				point += direction.sign() * half_size * profile->get_directional_facet_strength() * 0.04;
				points.push_back(point);
			}
		}
	}
	if (points.is_empty()) return points;
	real_t min_y = points[0].y, max_y = points[0].y;
	for (const Vector3 &point : points) { min_y = MIN(min_y, point.y); max_y = MAX(max_y, point.y); }
	real_t flatten_line = min_y + (max_y - min_y) * profile->get_bottom_flatten();
	real_t base_plane = -profile->get_ground_inset();
	for (int i = 0; i < points.size(); i++) { Vector3 point = points[i]; point.y -= min_y + profile->get_ground_inset(); if (point.y <= flatten_line - min_y) point.y = base_plane; points.set(i, point); }
	return points;
}

PackedVector3Array OpenWorldRockGenerator3D::_subset_points(const PackedVector3Array &p_points, int p_target) const {
	if (p_points.size() <= p_target) return p_points;
	PackedVector3Array subset; subset.resize(p_target);
	for (int i = 0; i < p_target; i++) subset.set(i, p_points[(int)((int64_t)i * p_points.size() / p_target)]);
	real_t base_y = p_points[0].y; for (const Vector3 &point : p_points) base_y = MIN(base_y, point.y);
	int replace = p_target - 1;
	for (int i = 0; i < p_points.size() && replace >= p_target - 3; i++) if (Math::is_equal_approx(p_points[i].y, base_y)) subset.set(replace--, p_points[i]);
	return subset;
}

Ref<OpenWorldRockTopologyData> OpenWorldRockGenerator3D::_build_topology(const PackedVector3Array &p_points, int p_seed) const {
	Vector<Vector3> input = p_points; Geometry3D::MeshData hull;
	if (ConvexHullComputer::convex_hull(input, hull) != OK) return Ref<OpenWorldRockTopologyData>();
	if (hull.vertices.size() < 4 || hull.faces.size() < 4) return Ref<OpenWorldRockTopologyData>();
	Vector<Vector3> vertices; for (const Vector3 &vertex : hull.vertices) vertices.push_back(vertex);
	for (int i = 1; i < vertices.size(); i++) { Vector3 value = vertices[i]; int j = i - 1; while (j >= 0 && point_less(value, vertices[j])) { vertices.write[j + 1] = vertices[j]; j--; } vertices.write[j + 1] = value; }
	Vector3 center; for (const Vector3 &vertex : vertices) center += vertex; center /= vertices.size();
	Vector<RockTriangle> triangles;
	for (const Geometry3D::MeshData::Face &face : hull.faces) {
		for (uint32_t k = 2; k < face.indices.size(); k++) {
			int ids[3]; int old_ids[3] = { face.indices[0], face.indices[k - 1], face.indices[k] };
			for (int q = 0; q < 3; q++) { ids[q] = 0; while (ids[q] < vertices.size() && vertices[ids[q]] != hull.vertices[old_ids[q]]) ids[q]++; }
			Vector3 normal = (vertices[ids[1]] - vertices[ids[0]]).cross(vertices[ids[2]] - vertices[ids[0]]);
			// Godot front faces are clockwise. Keep winding so the geometric cross points
			// toward the hull center (inward); that reads as CW when viewed from outside.
			if (normal.dot((vertices[ids[0]] + vertices[ids[1]] + vertices[ids[2]]) / 3.0 - center) > 0.0) {
				SWAP(ids[1], ids[2]);
			}
			int smallest = ids[0] <= ids[1] && ids[0] <= ids[2] ? 0 : (ids[1] <= ids[2] ? 1 : 2);
			triangles.push_back({ ids[smallest], ids[(smallest + 1) % 3], ids[(smallest + 2) % 3] });
		}
	}
	for (int i = 1; i < triangles.size(); i++) { RockTriangle value = triangles[i]; int j = i - 1; while (j >= 0 && triangle_less(value, triangles[j])) { triangles.write[j + 1] = triangles[j]; j--; } triangles.write[j + 1] = value; }
	PackedVector3Array packed_vertices; for (const Vector3 &vertex : vertices) packed_vertices.push_back(vertex);
	PackedInt32Array indices; PackedInt32Array groups;
	for (const RockTriangle &triangle : triangles) {
		indices.push_back(triangle.a);
		indices.push_back(triangle.b);
		indices.push_back(triangle.c);
		// Outward normal for face grouping (opposite of CW geometric cross).
		Vector3 n = -(vertices[triangle.b] - vertices[triangle.a]).cross(vertices[triangle.c] - vertices[triangle.a]).normalized();
		groups.push_back(n.y > 0.55 ? 0 : (n.y < -0.55 ? 1 : 2));
	}
	AABB bounds(vertices[0], Vector3()); for (int i = 1; i < vertices.size(); i++) bounds.expand_to(vertices[i]);
	real_t base_y = vertices[0].y; for (const Vector3 &vertex : vertices) base_y = MIN(base_y, vertex.y);
	uint64_t hash = 1469598103934665603ULL; for (const Vector3 &vertex : vertices) { hash = hash_mix(hash, quantize(vertex.x)); hash = hash_mix(hash, quantize(vertex.y)); hash = hash_mix(hash, quantize(vertex.z)); } for (int index : indices) hash = hash_mix(hash, index);
	Ref<OpenWorldRockTopologyData> topology; topology.instantiate(); topology->set_source_points(p_points); topology->set_hull_vertices(packed_vertices); topology->set_hull_indices(indices); topology->set_face_groups(groups); topology->set_base_plane(base_y); topology->set_local_bounds(bounds); topology->set_source_seed(p_seed); topology->set_topology_hash(String::num_uint64(hash, 16)); return topology;
}

Ref<OpenWorldRockTopologyData> OpenWorldRockGenerator3D::generate_topology() {
	generation_report = validate_request(); if (!(bool)generation_report["success"]) { generated_topology.unref(); return generated_topology; }
	PackedVector3Array points = _generate_source_points(); generated_topology = _build_topology(points, generation_request->get_seed());
	if (generated_topology.is_null()) { generation_report["success"] = false; PackedStringArray errors = generation_report["error_codes"]; errors.push_back("HULL_DEGENERATE"); generation_report["error_codes"] = errors; generation_report["message"] = "Convex hull generation failed; provide non-coplanar points."; }
	return generated_topology;
}

Ref<ArrayMesh> OpenWorldRockGenerator3D::_build_mesh(const Ref<OpenWorldRockTopologyData> &p_topology) const {
	if (p_topology.is_null()) return Ref<ArrayMesh>(); PackedVector3Array hull_vertices = p_topology->get_hull_vertices(); PackedInt32Array hull_indices = p_topology->get_hull_indices();
	PackedVector3Array vertices; PackedVector3Array normals; PackedVector2Array uvs; PackedColorArray colors;
	for (int i = 0; i < hull_indices.size(); i += 3) {
		Vector3 a = hull_vertices[hull_indices[i]], b = hull_vertices[hull_indices[i + 1]], c = hull_vertices[hull_indices[i + 2]];
		// Indices are Godot-CW (front from outside); geometric cross points inward — flip for lighting.
		Vector3 normal = -(b - a).cross(c - a).normalized();
		Vector3 face_vertices[3] = { a, b, c };
		for (const Vector3 &vertex : face_vertices) { vertices.push_back(vertex); normals.push_back(normal); uvs.push_back(Vector2(vertex.x, vertex.z)); real_t upward = CLAMP(normal.y * 0.5 + 0.5, (real_t)0.0, (real_t)1.0); real_t downward = CLAMP(-normal.y, (real_t)0.0, (real_t)1.0); real_t strata = CLAMP(Math::abs(normal.dot(generation_request->get_profile()->get_strata_direction())), (real_t)0.0, (real_t)1.0); real_t phase = (real_t)((uint32_t)generation_request->get_seed() & 255) / 255.0; colors.push_back(Color(upward, downward, strata, phase)); }
	}
	Array arrays; arrays.resize(Mesh::ARRAY_MAX); arrays[Mesh::ARRAY_VERTEX] = vertices; arrays[Mesh::ARRAY_NORMAL] = normals; arrays[Mesh::ARRAY_TEX_UV] = uvs; arrays[Mesh::ARRAY_COLOR] = colors;
	Ref<ArrayMesh> result_mesh; result_mesh.instantiate(); result_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays); if (preview_material.is_valid()) result_mesh->surface_set_material(0, preview_material); return result_mesh;
}

void OpenWorldRockGenerator3D::generate_rock() {
	if (generate_topology().is_null()) { clear_generated_rock(); return; }
	Ref<OpenWorldRockGenerationProfile> profile = generation_request->get_profile(); PackedVector3Array source = generated_topology->get_source_points(); int lod1_target = MIN(source.size() - 1, MAX(10, (int)Math::floor(source.size() * profile->get_lod1_quality()))); int lod_targets[3] = { (int)source.size(), lod1_target, MIN(profile->get_lod2_point_count(), lod1_target - 2) };
	for (int lod = 0; lod < 3; lod++) { generated_lod_topologies[lod] = lod == 0 ? generated_topology : _build_topology(_subset_points(source, MIN(source.size(), lod_targets[lod])), generation_request->get_seed()); generated_lod_meshes[lod] = _build_mesh(generated_lod_topologies[lod]); }
	if (generated_lod_topologies[2].is_valid()) { generated_collision_points = _subset_points(generated_lod_topologies[2]->get_hull_vertices(), profile->get_collision_point_limit()); if (PhysicsServer3D::get_singleton()) { Ref<ConvexPolygonShape3D> shape; shape.instantiate(); shape->set_points(generated_collision_points); generated_collision_shape = shape; } }
	Array lod_report; for (int lod = 0; lod < 3; lod++) lod_report.push_back(get_lod_statistics(lod));
	int duplicate_count = 0; for (int i = 0; i < source.size(); i++) for (int j = 0; j < i; j++) if (source[i] == source[j]) { duplicate_count++; break; }
	Ref<OpenWorldRockTopologyData> collision_topology = _build_topology(generated_collision_points, generation_request->get_seed());
	generation_report["mode"] = (int)generation_request->get_mode(); generation_report["seed"] = generation_request->get_seed(); generation_report["source_point_count"] = source.size(); generation_report["hull_vertex_count"] = generated_topology->get_hull_vertices().size(); generation_report["hull_face_count"] = generated_topology->get_triangle_count(); generation_report["discarded_point_count"] = MAX(0, source.size() - generated_topology->get_hull_vertices().size() - duplicate_count); generation_report["duplicate_point_count"] = duplicate_count; generation_report["base_contact_count"] = generated_topology->get_base_contact_count(); generation_report["base_plane"] = generated_topology->get_base_plane(); generation_report["topology_hash"] = generated_topology->get_topology_hash(); generation_report["lods"] = lod_report; generation_report["collision_point_count"] = generated_collision_points.size(); generation_report["collision_face_count"] = collision_topology.is_valid() ? collision_topology->get_triangle_count() : 0;
	_update_preview_mesh(); emit_signal(SNAME("rock_generated"), generated_lod_meshes[0]);
}

void OpenWorldRockGenerator3D::clear_generated_rock() { generated_topology.unref(); for (int i = 0; i < 3; i++) { generated_lod_topologies[i].unref(); generated_lod_meshes[i].unref(); } generated_collision_shape.unref(); generated_collision_points.clear(); set_mesh(Ref<Mesh>()); }
Ref<ArrayMesh> OpenWorldRockGenerator3D::get_generated_lod_mesh(int p_lod) const { ERR_FAIL_INDEX_V(p_lod, 3, Ref<ArrayMesh>()); return generated_lod_meshes[p_lod]; }
Dictionary OpenWorldRockGenerator3D::get_lod_statistics(int p_lod) const { Dictionary result; result["lod"] = p_lod; Ref<OpenWorldRockTopologyData> topology = generated_lod_topologies[p_lod]; result["available"] = topology.is_valid(); result["vertices"] = topology.is_valid() ? topology->get_hull_vertices().size() : 0; result["triangles"] = topology.is_valid() ? topology->get_triangle_count() : 0; result["aabb"] = topology.is_valid() ? topology->get_local_bounds() : AABB(); result["base_plane"] = topology.is_valid() ? topology->get_base_plane() : 0.0; return result; }
Ref<OpenWorldRockVariant> OpenWorldRockGenerator3D::create_baked_variant() const {
	ERR_FAIL_COND_V_MSG(generated_lod_meshes[0].is_null(), Ref<OpenWorldRockVariant>(), "Generate a rock before baking."); Ref<OpenWorldRockVariant> variant; variant.instantiate(); variant->set_variant_name(vformat("Rock %d", generation_request->get_seed())); variant->set_source_mode(generation_request->get_mode()); variant->set_source_seed(generation_request->get_seed()); variant->set_lod0_mesh(generated_lod_meshes[0]->duplicate(true)); variant->set_lod1_mesh(generated_lod_meshes[1]->duplicate(true)); variant->set_lod2_mesh(generated_lod_meshes[2]->duplicate(true)); variant->set_collision_points(generated_collision_points); if (generated_collision_shape.is_valid()) variant->set_collision_shape(generated_collision_shape->duplicate(true)); variant->set_lod1_distance(lod1_distance); variant->set_lod2_distance(lod2_distance); variant->set_max_distance(max_distance); variant->set_local_bounds(generated_topology->get_local_bounds()); variant->set_stable_id(generation_request->get_stable_id()); variant->set_tags(generation_request->get_tags()); return variant;
}
void OpenWorldRockGenerator3D::_update_preview_mesh() { Ref<ArrayMesh> preview_mesh = generated_lod_meshes[(int)preview_lod]; if (preview_mesh.is_null()) preview_mesh = generated_lod_meshes[0]; set_mesh(preview_mesh); update_gizmos(); }
void OpenWorldRockGenerator3D::_notification(int p_what) { if (p_what == NOTIFICATION_ENTER_TREE && auto_generate && generation_request.is_valid()) generate_rock(); }

OpenWorldRockGenerator3D::OpenWorldRockGenerator3D() { if (RenderingServer::get_singleton()) { Ref<StandardMaterial3D> material; material.instantiate(); material->set_albedo(Color(0.38, 0.34, 0.3)); material->set_roughness(0.9); material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX); preview_material = material; } }
OpenWorldRockGenerator3D::~OpenWorldRockGenerator3D() { if (generation_request.is_valid() && generation_request->is_connected("changed", callable_mp(this, &OpenWorldRockGenerator3D::_request_changed))) generation_request->disconnect_changed(callable_mp(this, &OpenWorldRockGenerator3D::_request_changed)); }

void OpenWorldRockGenerator3D::_bind_methods() {
	BIND_ENUM_CONSTANT(PREVIEW_LOD0); BIND_ENUM_CONSTANT(PREVIEW_LOD1); BIND_ENUM_CONSTANT(PREVIEW_LOD2);
	ClassDB::bind_method(D_METHOD("set_generation_request", "request"), &OpenWorldRockGenerator3D::set_generation_request); ClassDB::bind_method(D_METHOD("get_generation_request"), &OpenWorldRockGenerator3D::get_generation_request); ClassDB::bind_method(D_METHOD("set_auto_generate", "enabled"), &OpenWorldRockGenerator3D::set_auto_generate); ClassDB::bind_method(D_METHOD("is_auto_generate"), &OpenWorldRockGenerator3D::is_auto_generate); ClassDB::bind_method(D_METHOD("set_preview_lod", "lod"), &OpenWorldRockGenerator3D::set_preview_lod); ClassDB::bind_method(D_METHOD("get_preview_lod"), &OpenWorldRockGenerator3D::get_preview_lod); ClassDB::bind_method(D_METHOD("set_preview_material", "material"), &OpenWorldRockGenerator3D::set_preview_material); ClassDB::bind_method(D_METHOD("get_preview_material"), &OpenWorldRockGenerator3D::get_preview_material); ClassDB::bind_method(D_METHOD("set_lod1_distance", "value"), &OpenWorldRockGenerator3D::set_lod1_distance); ClassDB::bind_method(D_METHOD("get_lod1_distance"), &OpenWorldRockGenerator3D::get_lod1_distance); ClassDB::bind_method(D_METHOD("set_lod2_distance", "value"), &OpenWorldRockGenerator3D::set_lod2_distance); ClassDB::bind_method(D_METHOD("get_lod2_distance"), &OpenWorldRockGenerator3D::get_lod2_distance); ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &OpenWorldRockGenerator3D::set_max_distance); ClassDB::bind_method(D_METHOD("get_max_distance"), &OpenWorldRockGenerator3D::get_max_distance);
	ClassDB::bind_method(D_METHOD("validate_request"), &OpenWorldRockGenerator3D::validate_request); ClassDB::bind_method(D_METHOD("generate_topology"), &OpenWorldRockGenerator3D::generate_topology); ClassDB::bind_method(D_METHOD("generate_rock"), &OpenWorldRockGenerator3D::generate_rock); ClassDB::bind_method(D_METHOD("clear_generated_rock"), &OpenWorldRockGenerator3D::clear_generated_rock); ClassDB::bind_method(D_METHOD("get_generated_topology"), &OpenWorldRockGenerator3D::get_generated_topology); ClassDB::bind_method(D_METHOD("get_generated_lod_mesh", "lod"), &OpenWorldRockGenerator3D::get_generated_lod_mesh); ClassDB::bind_method(D_METHOD("get_generated_collision_shape"), &OpenWorldRockGenerator3D::get_generated_collision_shape); ClassDB::bind_method(D_METHOD("get_generated_collision_points"), &OpenWorldRockGenerator3D::get_generated_collision_points); ClassDB::bind_method(D_METHOD("get_lod_statistics", "lod"), &OpenWorldRockGenerator3D::get_lod_statistics); ClassDB::bind_method(D_METHOD("get_generation_report"), &OpenWorldRockGenerator3D::get_generation_report); ClassDB::bind_method(D_METHOD("create_baked_variant"), &OpenWorldRockGenerator3D::create_baked_variant);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "generation_request", PROPERTY_HINT_RESOURCE_TYPE, "OpenWorldRockGenerationRequest", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_generation_request", "get_generation_request"); ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate"), "set_auto_generate", "is_auto_generate"); ADD_PROPERTY(PropertyInfo(Variant::INT, "preview_lod", PROPERTY_HINT_ENUM, "LOD0,LOD1,LOD2"), "set_preview_lod", "get_preview_lod"); ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "preview_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_preview_material", "get_preview_material"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod1_distance", "get_lod1_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_lod2_distance", "get_lod2_distance"); ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,10000,0.5,suffix:m"), "set_max_distance", "get_max_distance"); ADD_SIGNAL(MethodInfo("rock_generated", PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "ArrayMesh")));
}
