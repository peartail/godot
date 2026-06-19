/**************************************************************************/
/*  simple_terrain_3d.cpp                                                        */
/**************************************************************************/

#include "simple_terrain_3d.h"

#include "simple_world_object_profile.h"

#include "core/math/geometry_3d.h"
#include "core/math/random_pcg.h"
#include "core/math/triangle_mesh.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/resources/3d/world_3d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/mesh.h"
#include "scene/resources/shader.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"

namespace {
struct SimpleTerrainNavigationObstruction {
	Vector<Vector3> vertices;
	real_t elevation = 0.0;
	real_t height = 0.0;
};

void _append_circle_obstruction(Vector<SimpleTerrainNavigationObstruction> &r_obstructions, const Transform3D &p_transform, real_t p_radius, real_t p_half_height) {
	if (p_radius <= 0.0) {
		return;
	}

	static const int circle_points = 12;
	const real_t circle_step = Math::TAU / circle_points;

	SimpleTerrainNavigationObstruction obstruction;
	obstruction.vertices.resize(circle_points);
	Vector3 *vertices_w = obstruction.vertices.ptrw();
	for (int i = 0; i < circle_points; i++) {
		const real_t angle = i * circle_step;
		vertices_w[i] = p_transform.xform(Vector3(Math::cos(angle) * p_radius, 0.0, Math::sin(angle) * p_radius));
	}

	const Vector3 top = p_transform.xform(Vector3(0.0, p_half_height, 0.0));
	const Vector3 bottom = p_transform.xform(Vector3(0.0, -p_half_height, 0.0));
	obstruction.elevation = MIN(top.y, bottom.y);
	obstruction.height = MAX((real_t)0.0, Math::abs(top.y - bottom.y));
	r_obstructions.push_back(obstruction);
}

void _append_box_obstruction(Vector<SimpleTerrainNavigationObstruction> &r_obstructions, const Transform3D &p_transform, const Vector3 &p_size) {
	if (p_size.x <= 0.0 || p_size.z <= 0.0) {
		return;
	}

	const Vector3 half_size = p_size * 0.5;
	const Vector3 local_corners[4] = {
		Vector3(-half_size.x, 0.0, -half_size.z),
		Vector3(half_size.x, 0.0, -half_size.z),
		Vector3(half_size.x, 0.0, half_size.z),
		Vector3(-half_size.x, 0.0, half_size.z),
	};

	SimpleTerrainNavigationObstruction obstruction;
	obstruction.vertices.resize(4);
	Vector3 *vertices_w = obstruction.vertices.ptrw();
	for (int i = 0; i < 4; i++) {
		vertices_w[i] = p_transform.xform(local_corners[i]);
	}

	real_t min_y = Math::INF;
	real_t max_y = -Math::INF;
	for (int y_index = 0; y_index < 2; y_index++) {
		const real_t y = y_index == 0 ? -half_size.y : half_size.y;
		for (int z_index = 0; z_index < 2; z_index++) {
			const real_t z = z_index == 0 ? -half_size.z : half_size.z;
			for (int x_index = 0; x_index < 2; x_index++) {
				const real_t x = x_index == 0 ? -half_size.x : half_size.x;
				const real_t world_y = p_transform.xform(Vector3(x, y, z)).y;
				min_y = MIN(min_y, world_y);
				max_y = MAX(max_y, world_y);
			}
		}
	}
	obstruction.elevation = min_y;
	obstruction.height = MAX((real_t)0.0, max_y - min_y);
	r_obstructions.push_back(obstruction);
}

void _append_aabb_obstruction(Vector<SimpleTerrainNavigationObstruction> &r_obstructions, const Transform3D &p_transform, const AABB &p_aabb) {
	if (p_aabb.size.x <= 0.0 || p_aabb.size.z <= 0.0) {
		return;
	}

	const Vector3 min = p_aabb.position;
	const Vector3 max = p_aabb.position + p_aabb.size;
	const real_t center_y = min.y + p_aabb.size.y * 0.5;
	const Vector3 local_corners[4] = {
		Vector3(min.x, center_y, min.z),
		Vector3(max.x, center_y, min.z),
		Vector3(max.x, center_y, max.z),
		Vector3(min.x, center_y, max.z),
	};

	SimpleTerrainNavigationObstruction obstruction;
	obstruction.vertices.resize(4);
	Vector3 *vertices_w = obstruction.vertices.ptrw();
	for (int i = 0; i < 4; i++) {
		vertices_w[i] = p_transform.xform(local_corners[i]);
	}

	real_t min_y = Math::INF;
	real_t max_y = -Math::INF;
	for (int i = 0; i < 8; i++) {
		const real_t world_y = p_transform.xform(p_aabb.get_endpoint(i)).y;
		min_y = MIN(min_y, world_y);
		max_y = MAX(max_y, world_y);
	}
	obstruction.elevation = min_y;
	obstruction.height = MAX((real_t)0.0, max_y - min_y);
	r_obstructions.push_back(obstruction);
}

void _collect_collision_obstructions(Node *p_node, const Transform3D &p_root_to_target, Vector<SimpleTerrainNavigationObstruction> &r_obstructions) {
	CollisionShape3D *collision_shape = Object::cast_to<CollisionShape3D>(p_node);
	if (collision_shape != nullptr && !collision_shape->is_disabled()) {
		Ref<Shape3D> shape = collision_shape->get_shape();
		if (shape.is_valid()) {
			const Transform3D shape_transform = p_root_to_target * collision_shape->get_global_transform();
			Ref<BoxShape3D> box = shape;
			Ref<SphereShape3D> sphere = shape;
			Ref<CapsuleShape3D> capsule = shape;
			Ref<CylinderShape3D> cylinder = shape;

			if (box.is_valid()) {
				_append_box_obstruction(r_obstructions, shape_transform, box->get_size());
			} else if (sphere.is_valid()) {
				_append_circle_obstruction(r_obstructions, shape_transform, sphere->get_radius(), sphere->get_radius());
			} else if (capsule.is_valid()) {
				_append_circle_obstruction(r_obstructions, shape_transform, capsule->get_radius(), capsule->get_height() * 0.5);
			} else if (cylinder.is_valid()) {
				_append_circle_obstruction(r_obstructions, shape_transform, cylinder->get_radius(), cylinder->get_height() * 0.5);
			}
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_collect_collision_obstructions(p_node->get_child(i), p_root_to_target, r_obstructions);
	}
}

void _collect_mesh_aabb_obstructions(Node *p_node, const Transform3D &p_root_to_target, Vector<SimpleTerrainNavigationObstruction> &r_obstructions) {
	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(p_node);
	if (mesh_instance != nullptr && mesh_instance->get_mesh().is_valid()) {
		_append_aabb_obstruction(r_obstructions, p_root_to_target * mesh_instance->get_global_transform(), mesh_instance->get_mesh()->get_aabb());
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_collect_mesh_aabb_obstructions(p_node->get_child(i), p_root_to_target, r_obstructions);
	}
}

Transform3D _build_profile_placement_transform(const Vector3 &p_local_position, const Vector3 &p_rotation, const Vector3 &p_scale, const Vector3 &p_local_normal, bool p_align_to_terrain_normal) {
	Transform3D placement_transform;
	placement_transform.origin = p_local_position;
	if (p_align_to_terrain_normal) {
		Vector3 local_y = p_local_normal.is_zero_approx() ? Vector3(0.0, 1.0, 0.0) : p_local_normal;
		Vector3 local_z = Vector3(Math::sin(p_rotation.y), 0.0, Math::cos(p_rotation.y));
		local_z = (local_z - local_y * local_y.dot(local_z));
		if (local_z.is_zero_approx()) {
			local_z = local_y.cross(Vector3(1.0, 0.0, 0.0));
		}
		if (local_z.is_zero_approx()) {
			local_z = local_y.cross(Vector3(0.0, 0.0, 1.0));
		}
		local_z.normalize();
		Vector3 local_x = local_y.cross(local_z).normalized();
		local_z = local_x.cross(local_y).normalized();
		placement_transform.basis = Basis(local_x, local_y, local_z);
	} else {
		placement_transform.basis = Basis::from_euler(p_rotation);
	}
	placement_transform.basis.scale(p_scale);
	return placement_transform;
}

void _append_profile_placement_obstructions(const Ref<SimpleWorldObjectProfile> &p_profile, const Transform3D &p_placement_transform, Vector<SimpleTerrainNavigationObstruction> &r_obstructions) {
	if (p_profile.is_null()) {
		return;
	}

	if ((p_profile->get_navigation_obstacle_shape_source() == SimpleWorldObjectProfile::NAVIGATION_OBSTACLE_SHAPE_SCENE_COLLISION ||
				p_profile->get_navigation_obstacle_shape_source() == SimpleWorldObjectProfile::NAVIGATION_OBSTACLE_SHAPE_MESH_AABB) &&
			p_profile->get_scene().is_valid()) {
		Node *scene_instance = p_profile->get_scene()->instantiate();
		Node3D *instance_3d = Object::cast_to<Node3D>(scene_instance);
		if (instance_3d != nullptr) {
			const Transform3D root_to_terrain = p_placement_transform * instance_3d->get_global_transform().affine_inverse();
			if (p_profile->get_navigation_obstacle_shape_source() == SimpleWorldObjectProfile::NAVIGATION_OBSTACLE_SHAPE_MESH_AABB) {
				_collect_mesh_aabb_obstructions(instance_3d, root_to_terrain, r_obstructions);
			} else {
				_collect_collision_obstructions(instance_3d, root_to_terrain, r_obstructions);
			}
		}
		if (scene_instance != nullptr) {
			memdelete(scene_instance);
		}
		return;
	}

	const real_t base_radius = p_profile->get_navigation_obstacle_radius();
	if (base_radius <= 0.0) {
		return;
	}
	const Vector3 safe_scale = p_placement_transform.basis.get_scale().abs().maxf((real_t)0.001);
	const real_t horizontal_scale = MAX(safe_scale.x, safe_scale.z);
	const real_t radius = base_radius * horizontal_scale;
	const real_t height = p_profile->get_navigation_obstacle_height() * safe_scale.y;

	Transform3D circle_transform;
	circle_transform.origin = p_placement_transform.origin;
	_append_circle_obstruction(r_obstructions, circle_transform, radius, height * 0.5);
}
} // namespace

void SimpleTerrain3D::_simple_terrain_data_changed() {
	if (syncing_data) {
		return;
	}
	rebuild_mesh();
}

void SimpleTerrain3D::_ensure_data() {
	if (simple_terrain_data.is_valid()) {
		simple_terrain_data->ensure_height_data_size(false);
		return;
	}

	// Lazily create terrain data after the node enters the world or is edited.
	// Creating default resources in the constructor makes Godot's default
	// property probing instantiate them, which is noisy for Mono/editor users.
	simple_terrain_data.instantiate();
	simple_terrain_data->fill_flat(flat_height);
	simple_terrain_data->connect_changed(callable_mp(this, &SimpleTerrain3D::_simple_terrain_data_changed));
}

Vector3 SimpleTerrain3D::_get_vertex_position(int p_x, int p_z) const {
	ERR_FAIL_COND_V(simple_terrain_data.is_null(), Vector3());
	const real_t half_size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size() * 0.5;
	// SimpleTerrainData stores only height values; X/Z are derived from grid indices.
	// Keeping this conversion centralized avoids subtle mismatches between mesh
	// building, picking, debug gizmos, and bounds calculation.
	return Vector3(
			(real_t)p_x * simple_terrain_data->get_cell_size() - half_size,
			simple_terrain_data->get_height(p_x, p_z),
			(real_t)p_z * simple_terrain_data->get_cell_size() - half_size);
}

Vector3 SimpleTerrain3D::_get_vertex_normal(int p_x, int p_z) const {
	ERR_FAIL_COND_V(simple_terrain_data.is_null(), Vector3(0, 1, 0));

	const int vertex_count = simple_terrain_data->get_vertex_count();
	const int left_x = MAX(0, p_x - 1);
	const int right_x = MIN(vertex_count - 1, p_x + 1);
	const int up_z = MAX(0, p_z - 1);
	const int down_z = MIN(vertex_count - 1, p_z + 1);

	const Vector3 tangent_x = _get_vertex_position(right_x, p_z) - _get_vertex_position(left_x, p_z);
	const Vector3 tangent_z = _get_vertex_position(p_x, down_z) - _get_vertex_position(p_x, up_z);
	Vector3 normal = tangent_z.cross(tangent_x);
	if (normal.is_zero_approx()) {
		return Vector3(0, 1, 0);
	}
	normal.normalize();
	if (normal.y < 0.0) {
		normal = -normal;
	}
	return normal;
}

real_t SimpleTerrain3D::_sample_nearest_height(real_t p_center_x, real_t p_center_z) const {
	// Flatten uses the closest existing vertex height as its target. This makes
	// the operation predictable when painting on slopes because it preserves the
	// clicked height instead of using a global flat_height setting.
	const int x = CLAMP(Math::round(p_center_x), 0, simple_terrain_data->get_vertex_count() - 1);
	const int z = CLAMP(Math::round(p_center_z), 0, simple_terrain_data->get_vertex_count() - 1);
	return simple_terrain_data->get_height(x, z);
}

real_t SimpleTerrain3D::_get_average_neighbor_height(const PackedFloat32Array &p_source_heights, int p_x, int p_z) const {
	// Smooth reads from a snapshot taken before the brush pass. Without this,
	// earlier edits in the same brush step would bias later vertices and create
	// directional smearing.
	const int vertex_count = simple_terrain_data->get_vertex_count();
	real_t total = 0.0;
	int count = 0;
	for (int z = MAX(0, p_z - 1); z <= MIN(vertex_count - 1, p_z + 1); z++) {
		for (int x = MAX(0, p_x - 1); x <= MIN(vertex_count - 1, p_x + 1); x++) {
			total += p_source_heights[simple_terrain_data->get_height_index(x, z)];
			count++;
		}
	}
	return total / (real_t)count;
}

real_t SimpleTerrain3D::_sample_value_noise(real_t p_x, real_t p_z, int p_seed) const {
	// Lightweight deterministic value noise used only for the "Random" button.
	// The initial terrain stays flat; random terrain is an explicit action.
	const real_t value = Math::sin(p_x * 12.9898 + p_z * 78.233 + (real_t)p_seed * 37.719) * 43758.5453;
	return Math::fposmod(value, (real_t)1.0) * 2.0 - 1.0;
}

Ref<ArrayMesh> SimpleTerrain3D::_build_chunk_mesh(int p_origin_x, int p_origin_z, int p_quad_width, int p_quad_depth) const {
	ERR_FAIL_COND_V(simple_terrain_data.is_null(), Ref<ArrayMesh>());

	const int grid_size = simple_terrain_data->get_grid_size();
	const int vertex_width = p_quad_width + 1;
	const int vertex_depth = p_quad_depth + 1;
	const int vertex_total = vertex_width * vertex_depth;

	Vector<Vector3> vertices;
	Vector<Vector3> normals;
	Vector<Vector2> uvs;
	Vector<int> indices;
	vertices.resize(vertex_total);
	normals.resize(vertex_total);
	uvs.resize(vertex_total);

	// Chunks intentionally share edge vertices by sampling the same SimpleTerrainData
	// coordinates. This prevents cracks; normals are also sampled from the global
	// height field so adjacent chunks shade smoothly across shared borders.
	Vector3 *vertices_w = vertices.ptrw();
	Vector3 *normals_w = normals.ptrw();
	Vector2 *uvs_w = uvs.ptrw();

	const real_t half_size = (real_t)grid_size * simple_terrain_data->get_cell_size() * 0.5;
	for (int local_z = 0; local_z < vertex_depth; local_z++) {
		for (int local_x = 0; local_x < vertex_width; local_x++) {
			const int terrain_x = p_origin_x + local_x;
			const int terrain_z = p_origin_z + local_z;
			const int local_index = local_z * vertex_width + local_x;
			vertices_w[local_index] = Vector3(
					(real_t)terrain_x * simple_terrain_data->get_cell_size() - half_size,
					simple_terrain_data->get_height(terrain_x, terrain_z),
					(real_t)terrain_z * simple_terrain_data->get_cell_size() - half_size);
			normals_w[local_index] = _get_vertex_normal(terrain_x, terrain_z);
			uvs_w[local_index] = Vector2((real_t)terrain_x / (real_t)grid_size, (real_t)terrain_z / (real_t)grid_size);
		}
	}

	auto get_local_index = [&](int p_x, int p_z) {
		return p_z * vertex_width + p_x;
	};

	auto add_triangle = [&](int p_a, int p_b, int p_c) {
		indices.push_back(p_a);
		indices.push_back(p_b);
		indices.push_back(p_c);
	};

	for (int z = 0; z < p_quad_depth; z++) {
		for (int x = 0; x < p_quad_width; x++) {
			const int top_left = get_local_index(x, z);
			const int top_right = get_local_index(x + 1, z);
			const int bottom_left = get_local_index(x, z + 1);
			const int bottom_right = get_local_index(x + 1, z + 1);
			add_triangle(top_left, top_right, bottom_left);
			add_triangle(top_right, bottom_right, bottom_left);
		}
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_INDEX] = indices;

	Ref<ArrayMesh> array_mesh;
	array_mesh.instantiate();
	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return array_mesh;
}

Ref<NavigationMesh> SimpleTerrain3D::_build_chunk_navigation_mesh(int p_origin_x, int p_origin_z, int p_quad_width, int p_quad_depth) const {
	ERR_FAIL_COND_V(simple_terrain_data.is_null(), Ref<NavigationMesh>());

	struct NavigationCell {
		bool walkable = false;
		bool used = false;
		Vector3 normal = Vector3(0.0, 1.0, 0.0);
		Plane plane;
	};

	Vector<Vector3> vertices;
	Vector<Vector<int>> polygons;
	vertices.resize((p_quad_width + 1) * (p_quad_depth + 1));
	Vector3 *vertices_w = vertices.ptrw();

	auto get_local_index = [&](int p_x, int p_z) {
		return p_z * (p_quad_width + 1) + p_x;
	};

	for (int local_z = 0; local_z <= p_quad_depth; local_z++) {
		for (int local_x = 0; local_x <= p_quad_width; local_x++) {
			vertices_w[get_local_index(local_x, local_z)] = _get_vertex_position(p_origin_x + local_x, p_origin_z + local_z);
		}
	}

	const real_t min_walkable_y = Math::cos(Math::deg_to_rad(navigation_max_slope));
	auto get_triangle_normal = [&](int p_a, int p_b, int p_c) {
		const Vector3 normal = (vertices[p_b] - vertices[p_a]).cross(vertices[p_c] - vertices[p_a]);
		if (normal.is_zero_approx()) {
			return Vector3();
		}
		return normal.normalized();
	};

	auto is_walkable_normal = [&](const Vector3 &p_normal) {
		return !p_normal.is_zero_approx() && Math::abs(p_normal.y) >= min_walkable_y;
	};

	auto add_oriented_polygon = [&](const Vector<int> &p_indices) {
		if (p_indices.size() < 3) {
			return;
		}
		const Vector3 normal = get_triangle_normal(p_indices[0], p_indices[1], p_indices[2]);
		if (normal.is_zero_approx() || Math::abs(normal.y) < min_walkable_y) {
			return;
		}

		Vector<int> polygon;
		polygon.resize(p_indices.size());
		if (normal.y >= 0.0) {
			for (int i = 0; i < p_indices.size(); i++) {
				polygon.write[i] = p_indices[i];
			}
		} else {
			polygon.write[0] = p_indices[0];
			for (int i = 1; i < p_indices.size(); i++) {
				polygon.write[i] = p_indices[p_indices.size() - i];
			}
		}
		polygons.push_back(polygon);
	};

	auto add_walkable_triangle = [&](int p_a, int p_b, int p_c) {
		Vector<int> triangle;
		triangle.resize(3);
		triangle.write[0] = p_a;
		triangle.write[1] = p_b;
		triangle.write[2] = p_c;
		add_oriented_polygon(triangle);
	};

	auto add_walkable_quad = [&](int p_top_left, int p_top_right, int p_bottom_left, int p_bottom_right, bool p_fallback_to_triangles) {
		const Vector3 first_normal = get_triangle_normal(p_top_left, p_top_right, p_bottom_left);
		const Vector3 second_normal = get_triangle_normal(p_top_right, p_bottom_right, p_bottom_left);
		const real_t normal_similarity = Math::cos(Math::deg_to_rad(navigation_quad_max_normal_angle));
		const bool triangles_walkable = is_walkable_normal(first_normal) && is_walkable_normal(second_normal);
		const bool normals_similar = triangles_walkable && Math::abs(first_normal.dot(second_normal)) >= normal_similarity;
		const Plane plane(vertices[p_top_left], vertices[p_top_right], vertices[p_bottom_left]);
		const bool planar = normals_similar && Math::abs(plane.distance_to(vertices[p_bottom_right])) <= navigation_quad_planar_tolerance;

		if (planar) {
			Vector<int> quad;
			quad.resize(4);
			quad.write[0] = p_top_left;
			quad.write[1] = p_top_right;
			quad.write[2] = p_bottom_right;
			quad.write[3] = p_bottom_left;
			add_oriented_polygon(quad);
			return;
		}

		if (p_fallback_to_triangles) {
			add_walkable_triangle(p_top_left, p_top_right, p_bottom_left);
			add_walkable_triangle(p_top_right, p_bottom_right, p_bottom_left);
		}
	};

	auto add_cell_as_current_mode = [&](int p_x, int p_z) {
		const int top_left = get_local_index(p_x, p_z);
		const int top_right = get_local_index(p_x + 1, p_z);
		const int bottom_left = get_local_index(p_x, p_z + 1);
		const int bottom_right = get_local_index(p_x + 1, p_z + 1);
		if (navigation_build_mode == NAVIGATION_BUILD_TRIANGLES) {
			add_walkable_triangle(top_left, top_right, bottom_left);
			add_walkable_triangle(top_right, bottom_right, bottom_left);
		} else {
			add_walkable_quad(top_left, top_right, bottom_left, bottom_right, true);
		}
	};

	if (navigation_build_mode == NAVIGATION_BUILD_MERGED_RECTS) {
		Vector<NavigationCell> cells;
		cells.resize(p_quad_width * p_quad_depth);

		auto get_cell_index = [&](int p_x, int p_z) {
			return p_z * p_quad_width + p_x;
		};

		const real_t merge_normal_similarity = Math::cos(Math::deg_to_rad(navigation_merge_max_normal_angle));
		for (int z = 0; z < p_quad_depth; z++) {
			for (int x = 0; x < p_quad_width; x++) {
				const int top_left = get_local_index(x, z);
				const int top_right = get_local_index(x + 1, z);
				const int bottom_left = get_local_index(x, z + 1);
				const int bottom_right = get_local_index(x + 1, z + 1);
				const Vector3 first_normal = get_triangle_normal(top_left, top_right, bottom_left);
				const Vector3 second_normal = get_triangle_normal(top_right, bottom_right, bottom_left);
				NavigationCell &cell = cells.write[get_cell_index(x, z)];
				if (!is_walkable_normal(first_normal) || !is_walkable_normal(second_normal) || Math::abs(first_normal.dot(second_normal)) < merge_normal_similarity) {
					continue;
				}
				cell.plane = Plane(vertices[top_left], vertices[top_right], vertices[bottom_left]);
				if (Math::abs(cell.plane.distance_to(vertices[bottom_right])) > navigation_merge_planar_tolerance) {
					continue;
				}
				Vector3 normal = first_normal + second_normal;
				if (normal.is_zero_approx()) {
					normal = first_normal;
				} else {
					normal.normalize();
				}
				if (normal.y < 0.0) {
					normal = -normal;
				}
				cell.walkable = true;
				cell.normal = normal;
			}
		}

		auto rect_fits_seed = [&](int p_start_x, int p_start_z, int p_width, int p_height, const NavigationCell &p_seed) {
			for (int z = p_start_z; z < p_start_z + p_height; z++) {
				for (int x = p_start_x; x < p_start_x + p_width; x++) {
					const NavigationCell &cell = cells[get_cell_index(x, z)];
					if (!cell.walkable || cell.used || p_seed.normal.dot(cell.normal) < merge_normal_similarity) {
						return false;
					}
				}
			}

			const int corner_indices[4] = {
				get_local_index(p_start_x, p_start_z),
				get_local_index(p_start_x + p_width, p_start_z),
				get_local_index(p_start_x + p_width, p_start_z + p_height),
				get_local_index(p_start_x, p_start_z + p_height),
			};
			for (int i = 0; i < 4; i++) {
				if (Math::abs(p_seed.plane.distance_to(vertices[corner_indices[i]])) > navigation_merge_planar_tolerance) {
					return false;
				}
			}

			for (int z = p_start_z; z <= p_start_z + p_height; z++) {
				real_t min_height = vertices[get_local_index(p_start_x, z)].y;
				real_t max_height = min_height;
				for (int x = p_start_x + 1; x <= p_start_x + p_width; x++) {
					const real_t height = vertices[get_local_index(x, z)].y;
					min_height = MIN(min_height, height);
					max_height = MAX(max_height, height);
				}
				if (max_height - min_height > navigation_merge_max_height_delta * (real_t)p_width) {
					return false;
				}
			}
			for (int x = p_start_x; x <= p_start_x + p_width; x++) {
				real_t min_height = vertices[get_local_index(x, p_start_z)].y;
				real_t max_height = min_height;
				for (int z = p_start_z + 1; z <= p_start_z + p_height; z++) {
					const real_t height = vertices[get_local_index(x, z)].y;
					min_height = MIN(min_height, height);
					max_height = MAX(max_height, height);
				}
				if (max_height - min_height > navigation_merge_max_height_delta * (real_t)p_height) {
					return false;
				}
			}

			return true;
		};

		for (int z = 0; z < p_quad_depth; z++) {
			for (int x = 0; x < p_quad_width; x++) {
				NavigationCell &seed = cells.write[get_cell_index(x, z)];
				if (seed.used) {
					continue;
				}
				if (!seed.walkable) {
					add_cell_as_current_mode(x, z);
					seed.used = true;
					continue;
				}

				int rect_width = 1;
				while (x + rect_width < p_quad_width && rect_width < navigation_merge_max_rect_size && rect_fits_seed(x, z, rect_width + 1, 1, seed)) {
					rect_width++;
				}

				int rect_height = 1;
				while (z + rect_height < p_quad_depth && rect_height < navigation_merge_max_rect_size && rect_fits_seed(x, z, rect_width, rect_height + 1, seed)) {
					rect_height++;
				}

				for (int rect_z = z; rect_z < z + rect_height; rect_z++) {
					for (int rect_x = x; rect_x < x + rect_width; rect_x++) {
						cells.write[get_cell_index(rect_x, rect_z)].used = true;
					}
				}

				Vector<int> rect;
				rect.resize(4);
				rect.write[0] = get_local_index(x, z);
				rect.write[1] = get_local_index(x + rect_width, z);
				rect.write[2] = get_local_index(x + rect_width, z + rect_height);
				rect.write[3] = get_local_index(x, z + rect_height);
				const int previous_polygon_count = polygons.size();
				add_oriented_polygon(rect);
				if (polygons.size() == previous_polygon_count) {
					for (int rect_z = z; rect_z < z + rect_height; rect_z++) {
						for (int rect_x = x; rect_x < x + rect_width; rect_x++) {
							add_cell_as_current_mode(rect_x, rect_z);
						}
					}
				}
			}
		}
	} else {
		for (int z = 0; z < p_quad_depth; z++) {
			for (int x = 0; x < p_quad_width; x++) {
				add_cell_as_current_mode(x, z);
			}
		}
	}

	Ref<NavigationMesh> navigation_mesh;
	navigation_mesh.instantiate();
	navigation_mesh->set_cell_size(simple_terrain_data->get_cell_size());
	navigation_mesh->set_data(vertices, polygons);
	return navigation_mesh;
}

void SimpleTerrain3D::_mark_navigation_bake_dirty() {
	if (navigation_bake_dirty) {
		return;
	}
	navigation_bake_dirty = true;
	notify_property_list_changed();
}

void SimpleTerrain3D::_navigation_bake_finished() {
	navigation_bake_dirty = false;
	_sync_chunk_navigation();
	notify_property_list_changed();
	emit_signal(SNAME("navigation_bake_finished"));
}

#ifdef DEBUG_ENABLED
void SimpleTerrain3D::_navigation_debug_changed() {
	if (is_inside_tree()) {
		_update_navigation_debug_mesh();
	}
}

void SimpleTerrain3D::_update_navigation_debug_mesh() {
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	RenderingServer *rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(ns);
	ERR_FAIL_NULL(rs);

	if (!ns->get_debug_enabled() || !ns->get_debug_navigation_enabled() || !navigation_enabled || !navigation_debug_visible) {
		if (navigation_debug_instance.is_valid()) {
			rs->instance_set_visible(navigation_debug_instance, false);
		}
		return;
	}

	Vector<Ref<NavigationMesh>> navigation_meshes;
	if (navigation_build_mode == NAVIGATION_BUILD_BAKED) {
		if (navigation_baked_mesh.is_valid()) {
			navigation_meshes.push_back(navigation_baked_mesh);
		}
	} else {
		for (const TerrainChunk &chunk : chunks) {
			if (chunk.navigation_mesh.is_valid()) {
				navigation_meshes.push_back(chunk.navigation_mesh);
			}
		}
	}

	Vector<SimpleTerrainNavigationObstruction> runtime_obstructions;
	if (navigation_debug_runtime_obstacles_visible && world_placement_library.is_valid() && world_placement_data.is_valid()) {
		const PackedStringArray profile_ids = world_placement_data->get_profile_ids();
		const PackedVector3Array positions = world_placement_data->get_positions();
		const PackedVector3Array rotations = world_placement_data->get_rotations();
		const PackedVector3Array scales = world_placement_data->get_scales();
		const PackedVector3Array normals = world_placement_data->get_terrain_normals();
		const int placement_count = MIN(profile_ids.size(), MIN(positions.size(), MIN(rotations.size(), MIN(scales.size(), normals.size()))));
		const Transform3D terrain_inverse = get_global_transform().affine_inverse();

		for (int i = 0; i < placement_count; i++) {
			Ref<SimpleWorldObjectProfile> profile = world_placement_library->get_profile_by_id(profile_ids[i]);
			if (profile.is_null() || !profile->uses_runtime_navigation_obstacle()) {
				continue;
			}

			const Vector3 local_position = terrain_inverse.xform(positions[i]);
			const Vector3 local_normal = terrain_inverse.basis.xform(normals[i]).normalized();
			const Transform3D placement_transform = _build_profile_placement_transform(local_position, rotations[i], scales[i], local_normal, profile->is_aligning_to_terrain_normal());
			_append_profile_placement_obstructions(profile, placement_transform, runtime_obstructions);
		}
	}

	if (navigation_meshes.is_empty() && runtime_obstructions.is_empty()) {
		if (navigation_debug_instance.is_valid()) {
			rs->instance_set_visible(navigation_debug_instance, false);
		}
		return;
	}

	int face_vertex_count = 0;
	int line_vertex_count = 0;
	for (const Ref<NavigationMesh> &navigation_mesh : navigation_meshes) {
		const int polygon_count = navigation_mesh->get_polygon_count();
		for (int polygon_index = 0; polygon_index < polygon_count; polygon_index++) {
			const Vector<int> polygon = navigation_mesh->get_polygon(polygon_index);
			const int polygon_size = polygon.size();
			if (polygon_size < 3) {
				continue;
			}
			face_vertex_count += (polygon_size - 2) * 3;
			line_vertex_count += polygon_size * 2;
		}
	}

	int obstacle_face_vertex_count = 0;
	int obstacle_line_vertex_count = 0;
	for (const SimpleTerrainNavigationObstruction &obstruction : runtime_obstructions) {
		const int vertex_count = obstruction.vertices.size();
		if (vertex_count < 3) {
			continue;
		}
		obstacle_face_vertex_count += (vertex_count - 2) * 3;
		obstacle_line_vertex_count += vertex_count * 2;
	}

	if (face_vertex_count == 0 && obstacle_face_vertex_count == 0) {
		if (navigation_debug_instance.is_valid()) {
			rs->instance_set_visible(navigation_debug_instance, false);
		}
		return;
	}

	const bool enabled_geometry_face_random_color = ns->get_debug_navigation_enable_geometry_face_random_color();
	const bool enabled_edge_lines = ns->get_debug_navigation_enable_edge_lines();

	Vector<Vector3> face_vertices;
	face_vertices.resize(face_vertex_count);
	Vector<Color> face_colors;
	if (enabled_geometry_face_random_color) {
		face_colors.resize(face_vertex_count);
	}
	Vector<Vector3> line_vertices;
	if (enabled_edge_lines) {
		line_vertices.resize(line_vertex_count);
	}
	Vector<Vector3> obstacle_face_vertices;
	if (obstacle_face_vertex_count > 0) {
		obstacle_face_vertices.resize(obstacle_face_vertex_count);
	}
	Vector<Vector3> obstacle_line_vertices;
	if (enabled_edge_lines && obstacle_line_vertex_count > 0) {
		obstacle_line_vertices.resize(obstacle_line_vertex_count);
	}

	Vector3 *face_vertices_w = face_vertices.ptrw();
	Color *face_colors_w = face_colors.ptrw();
	Vector3 *line_vertices_w = line_vertices.ptrw();
	Vector3 *obstacle_face_vertices_w = obstacle_face_vertices.ptrw();
	Vector3 *obstacle_line_vertices_w = obstacle_line_vertices.ptrw();
	int face_vertex_index = 0;
	int line_vertex_index = 0;
	int obstacle_face_vertex_index = 0;
	int obstacle_line_vertex_index = 0;

	const Color debug_face_color = ns->get_debug_navigation_geometry_face_color();
	Color polygon_color = debug_face_color;
	RandomPCG rng;

	for (const Ref<NavigationMesh> &navigation_mesh : navigation_meshes) {
		const Vector<Vector3> vertices = navigation_mesh->get_vertices();
		if (vertices.is_empty()) {
			continue;
		}

		const int polygon_count = navigation_mesh->get_polygon_count();
		for (int polygon_index = 0; polygon_index < polygon_count; polygon_index++) {
			const Vector<int> polygon_indices = navigation_mesh->get_polygon(polygon_index);
			const int polygon_size = polygon_indices.size();
			if (polygon_size < 3) {
				continue;
			}

			if (enabled_geometry_face_random_color) {
				polygon_color.set_hsv(debug_face_color.get_h() + rng.random(-1.0, 1.0) * 0.1, debug_face_color.get_s(), debug_face_color.get_v() + rng.random(-1.0, 1.0) * 0.2);
				polygon_color.a = debug_face_color.a;
			}

			for (int polygon_indices_index = 0; polygon_indices_index < polygon_size - 2; polygon_indices_index++) {
				face_vertices_w[face_vertex_index] = vertices[polygon_indices[0]];
				face_vertices_w[face_vertex_index + 1] = vertices[polygon_indices[polygon_indices_index + 1]];
				face_vertices_w[face_vertex_index + 2] = vertices[polygon_indices[polygon_indices_index + 2]];
				if (enabled_geometry_face_random_color) {
					face_colors_w[face_vertex_index] = polygon_color;
					face_colors_w[face_vertex_index + 1] = polygon_color;
					face_colors_w[face_vertex_index + 2] = polygon_color;
				}
				face_vertex_index += 3;
			}

			if (enabled_edge_lines) {
				for (int polygon_indices_index = 0; polygon_indices_index < polygon_size; polygon_indices_index++) {
					line_vertices_w[line_vertex_index++] = vertices[polygon_indices[polygon_indices_index]];
					line_vertices_w[line_vertex_index++] = vertices[polygon_indices[(polygon_indices_index + 1) % polygon_size]];
				}
			}
		}
	}

	for (const SimpleTerrainNavigationObstruction &obstruction : runtime_obstructions) {
		const Vector<Vector3> &vertices = obstruction.vertices;
		const int vertex_count = vertices.size();
		if (vertex_count < 3) {
			continue;
		}

		for (int vertex_index = 0; vertex_index < vertex_count - 2; vertex_index++) {
			obstacle_face_vertices_w[obstacle_face_vertex_index++] = vertices[0];
			obstacle_face_vertices_w[obstacle_face_vertex_index++] = vertices[vertex_index + 1];
			obstacle_face_vertices_w[obstacle_face_vertex_index++] = vertices[vertex_index + 2];
		}

		if (enabled_edge_lines) {
			for (int vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
				obstacle_line_vertices_w[obstacle_line_vertex_index++] = vertices[vertex_index];
				obstacle_line_vertices_w[obstacle_line_vertex_index++] = vertices[(vertex_index + 1) % vertex_count];
			}
		}
	}

	if (!navigation_debug_instance.is_valid()) {
		navigation_debug_instance = rs->instance_create();
	}
	if (navigation_debug_mesh.is_null()) {
		navigation_debug_mesh.instantiate();
	}
	navigation_debug_mesh->clear_surfaces();

	if (face_vertex_index > 0) {
		if (face_vertex_index != face_vertices.size()) {
			face_vertices.resize(face_vertex_index);
		}
		Array face_mesh_array;
		face_mesh_array.resize(Mesh::ARRAY_MAX);
		face_mesh_array[Mesh::ARRAY_VERTEX] = face_vertices;
		if (enabled_geometry_face_random_color) {
			if (face_vertex_index != face_colors.size()) {
				face_colors.resize(face_vertex_index);
			}
			face_mesh_array[Mesh::ARRAY_COLOR] = face_colors;
		}
		navigation_debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, face_mesh_array);
		navigation_debug_mesh->surface_set_material(navigation_debug_mesh->get_surface_count() - 1, ns->get_debug_navigation_geometry_face_material());
	}

	if (enabled_edge_lines && line_vertex_index > 0) {
		if (line_vertex_index != line_vertices.size()) {
			line_vertices.resize(line_vertex_index);
		}
		Array line_mesh_array;
		line_mesh_array.resize(Mesh::ARRAY_MAX);
		line_mesh_array[Mesh::ARRAY_VERTEX] = line_vertices;
		navigation_debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, line_mesh_array);
		navigation_debug_mesh->surface_set_material(navigation_debug_mesh->get_surface_count() - 1, ns->get_debug_navigation_geometry_edge_material());
	}

	if (obstacle_face_vertex_index > 0) {
		if (obstacle_face_vertex_index != obstacle_face_vertices.size()) {
			obstacle_face_vertices.resize(obstacle_face_vertex_index);
		}
		Array obstacle_face_mesh_array;
		obstacle_face_mesh_array.resize(Mesh::ARRAY_MAX);
		obstacle_face_mesh_array[Mesh::ARRAY_VERTEX] = obstacle_face_vertices;
		navigation_debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, obstacle_face_mesh_array);
		navigation_debug_mesh->surface_set_material(navigation_debug_mesh->get_surface_count() - 1, ns->get_debug_navigation_avoidance_obstacles_radius_material());
	}

	if (enabled_edge_lines && obstacle_line_vertex_index > 0) {
		if (obstacle_line_vertex_index != obstacle_line_vertices.size()) {
			obstacle_line_vertices.resize(obstacle_line_vertex_index);
		}
		Array obstacle_line_mesh_array;
		obstacle_line_mesh_array.resize(Mesh::ARRAY_MAX);
		obstacle_line_mesh_array[Mesh::ARRAY_VERTEX] = obstacle_line_vertices;
		navigation_debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, obstacle_line_mesh_array);
		navigation_debug_mesh->surface_set_material(navigation_debug_mesh->get_surface_count() - 1, ns->get_debug_navigation_avoidance_static_obstacle_pushout_edge_material());
	}

	rs->instance_set_base(navigation_debug_instance, navigation_debug_mesh->get_rid());
	if (is_inside_tree() && get_world_3d().is_valid()) {
		rs->instance_set_scenario(navigation_debug_instance, get_world_3d()->get_scenario());
		rs->instance_set_transform(navigation_debug_instance, get_global_transform());
		rs->instance_set_visible(navigation_debug_instance, is_visible_in_tree());
	}
}
#endif // DEBUG_ENABLED

void SimpleTerrain3D::_clear_chunks() {
	RenderingServer *rs = RenderingServer::get_singleton();
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	for (TerrainChunk &chunk : chunks) {
		if (chunk.instance.is_valid()) {
			// Internal instances are not scene nodes, so SimpleTerrain3D owns their RID
			// lifetime explicitly.
			rs->free_rid(chunk.instance);
		}
		if (ns && chunk.navigation_region.is_valid()) {
			ns->free_rid(chunk.navigation_region);
		}
	}
	chunks.clear();
}

void SimpleTerrain3D::_sync_chunk_instances() {
	RenderingServer *rs = RenderingServer::get_singleton();
	const RID scenario = is_inside_tree() && get_world_3d().is_valid() ? get_world_3d()->get_scenario() : RID();
	const Transform3D global_transform = is_inside_tree() ? get_global_transform() : Transform3D();
	for (TerrainChunk &chunk : chunks) {
		if (!chunk.instance.is_valid()) {
			chunk.instance = rs->instance_create();
		}
		// Keep every chunk instance in the node's world/scenario. The chunk
		// mesh vertices are already in SimpleTerrain3D local space, so all chunks use
		// the same node transform.
		rs->instance_set_base(chunk.instance, chunk.mesh.is_valid() ? chunk.mesh->get_rid() : RID());
		rs->instance_set_scenario(chunk.instance, scenario);
		rs->instance_set_transform(chunk.instance, global_transform);
	}
	_sync_chunk_materials();
}

void SimpleTerrain3D::_sync_chunk_navigation() {
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	ERR_FAIL_NULL(ns);

	const RID navigation_map = navigation_enabled && is_inside_tree() && get_world_3d().is_valid() ? get_world_3d()->get_navigation_map() : RID();
	const Transform3D global_transform = is_inside_tree() ? get_global_transform() : Transform3D();
	if (navigation_build_mode == NAVIGATION_BUILD_BAKED) {
		for (TerrainChunk &chunk : chunks) {
			if (chunk.navigation_region.is_valid()) {
				ns->region_set_map(chunk.navigation_region, RID());
			}
		}

		if (!navigation_enabled || navigation_baked_mesh.is_null()) {
			if (navigation_baked_region.is_valid()) {
				ns->region_set_map(navigation_baked_region, RID());
			}
			return;
		}

		if (!navigation_baked_region.is_valid()) {
			navigation_baked_region = ns->region_create();
			ns->region_set_owner_id(navigation_baked_region, get_instance_id());
		}
		ns->region_set_navigation_layers(navigation_baked_region, navigation_layers);
		ns->region_set_transform(navigation_baked_region, global_transform);
		ns->region_set_navigation_mesh(navigation_baked_region, navigation_baked_mesh);
		ns->region_set_map(navigation_baked_region, navigation_map);
#ifdef DEBUG_ENABLED
		_update_navigation_debug_mesh();
#endif // DEBUG_ENABLED
		return;
	}

	if (navigation_baked_region.is_valid()) {
		ns->region_set_map(navigation_baked_region, RID());
	}

	for (TerrainChunk &chunk : chunks) {
		if (!navigation_enabled) {
			if (chunk.navigation_region.is_valid()) {
				ns->region_set_map(chunk.navigation_region, RID());
			}
			continue;
		}
		if (!chunk.navigation_region.is_valid()) {
			chunk.navigation_region = ns->region_create();
			ns->region_set_owner_id(chunk.navigation_region, get_instance_id());
		}
		ns->region_set_navigation_layers(chunk.navigation_region, navigation_layers);
		ns->region_set_transform(chunk.navigation_region, global_transform);
		ns->region_set_navigation_mesh(chunk.navigation_region, chunk.navigation_mesh);
		ns->region_set_map(chunk.navigation_region, navigation_map);
	}
#ifdef DEBUG_ENABLED
	_update_navigation_debug_mesh();
#endif // DEBUG_ENABLED
}

void SimpleTerrain3D::_sync_chunk_materials() {
	RenderingServer *rs = RenderingServer::get_singleton();
	const Ref<Material> material = _get_active_chunk_material();
	const RID material_rid = material.is_valid() ? material->get_rid() : RID();
	// Material changes are cheap compared to mesh rebuilds. Applying the same
	// override to every chunk keeps the material path independent from chunk
	// generation and lets users swap materials at runtime.
	for (TerrainChunk &chunk : chunks) {
		if (chunk.instance.is_valid()) {
			rs->instance_geometry_set_material_override(chunk.instance, material_rid);
		}
	}
}

void SimpleTerrain3D::_rebuild_chunks_for_region(int p_min_x, int p_min_z, int p_max_x, int p_max_z) {
	if (simple_terrain_data.is_null()) {
		return;
	}
	_mark_navigation_bake_dirty();

	// Convert the edited vertex rectangle to chunk coordinates. The caller pads
	// the rectangle by one vertex so neighboring triangles that depend on edge
	// vertices are also rebuilt.
	const int min_chunk_x = CLAMP(p_min_x / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);
	const int max_chunk_x = CLAMP(p_max_x / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);
	const int min_chunk_z = CLAMP(p_min_z / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);
	const int max_chunk_z = CLAMP(p_max_z / chunk_size, 0, Math::ceil((real_t)simple_terrain_data->get_grid_size() / (real_t)chunk_size) - 1);

	RenderingServer *rs = RenderingServer::get_singleton();
	for (TerrainChunk &chunk : chunks) {
		const int chunk_x = chunk.origin_x / chunk_size;
		const int chunk_z = chunk.origin_z / chunk_size;
		if (chunk_x < min_chunk_x || chunk_x > max_chunk_x || chunk_z < min_chunk_z || chunk_z > max_chunk_z) {
			continue;
		}
		chunk.mesh = _build_chunk_mesh(chunk.origin_x, chunk.origin_z, chunk.quad_width, chunk.quad_depth);
		if (navigation_enabled && navigation_build_mode != NAVIGATION_BUILD_BAKED) {
			chunk.navigation_mesh = _build_chunk_navigation_mesh(chunk.origin_x, chunk.origin_z, chunk.quad_width, chunk.quad_depth);
		}
		if (chunk.instance.is_valid()) {
			rs->instance_set_base(chunk.instance, chunk.mesh->get_rid());
		}
	}
	_sync_chunk_navigation();
	if (show_chunk_gizmos) {
		update_gizmos();
	}
}

Ref<Material> SimpleTerrain3D::_get_active_chunk_material() {
	// Explicit user material wins because it is the least surprising behavior:
	// assigning terrain_material should fully override built-in shader choices.
	if (terrain_material.is_valid()) {
		return terrain_material;
	}
	if (use_builtin_triplanar_material) {
		_update_builtin_triplanar_material();
		return builtin_triplanar_material;
	}
	return get_material_override();
}

void SimpleTerrain3D::_update_builtin_triplanar_material() {
	if (builtin_triplanar_shader.is_null()) {
		// The shader is generated lazily so projects that use a custom material do
		// not pay the cost or carry shader state they never use.
		builtin_triplanar_shader.instantiate();
		builtin_triplanar_shader->set_code(_get_builtin_triplanar_shader_code());
	}
	if (builtin_triplanar_material.is_null()) {
		builtin_triplanar_material.instantiate();
		builtin_triplanar_material->set_shader(builtin_triplanar_shader);
	}

	builtin_triplanar_material->set_shader_parameter("low_albedo", triplanar_low_texture);
	builtin_triplanar_material->set_shader_parameter("mid_albedo", triplanar_mid_texture);
	builtin_triplanar_material->set_shader_parameter("high_albedo", triplanar_high_texture);
	builtin_triplanar_material->set_shader_parameter("low_color", triplanar_low_color);
	builtin_triplanar_material->set_shader_parameter("mid_color", triplanar_mid_color);
	builtin_triplanar_material->set_shader_parameter("high_color", triplanar_high_color);
	builtin_triplanar_material->set_shader_parameter("low_height", triplanar_low_height);
	builtin_triplanar_material->set_shader_parameter("high_height", triplanar_high_height);
	builtin_triplanar_material->set_shader_parameter("blend_width", triplanar_blend_width);
	builtin_triplanar_material->set_shader_parameter("texture_scale", triplanar_texture_scale);
	builtin_triplanar_material->set_shader_parameter("triplanar_sharpness", triplanar_blend_sharpness);
}

String SimpleTerrain3D::_get_builtin_triplanar_shader_code() {
	// The shader blends three albedo layers by world-space height and samples
	// them through triplanar projection. World-space projection avoids stretched
	// UVs on steep slopes and keeps texture scale stable across chunk boundaries.
	return R"(
shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D low_albedo : source_color, hint_default_white;
uniform sampler2D mid_albedo : source_color, hint_default_white;
uniform sampler2D high_albedo : source_color, hint_default_white;
uniform vec4 low_color : source_color = vec4(0.45, 0.34, 0.22, 1.0);
uniform vec4 mid_color : source_color = vec4(0.26, 0.45, 0.18, 1.0);
uniform vec4 high_color : source_color = vec4(0.55, 0.55, 0.52, 1.0);
uniform float low_height = 1.5;
uniform float high_height = 8.0;
uniform float blend_width = 2.0;
uniform float texture_scale = 0.08;
uniform float triplanar_sharpness = 4.0;

varying vec3 terrain_world_pos;
varying vec3 terrain_world_normal;

void vertex() {
	terrain_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	terrain_world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
}

vec4 triplanar_sample(sampler2D tex, vec3 world_pos, vec3 world_normal) {
	vec3 blend = pow(abs(world_normal), vec3(max(triplanar_sharpness, 0.001)));
	blend /= max(dot(blend, vec3(1.0)), 0.001);
	vec3 coord = world_pos * texture_scale;
	vec4 x_sample = texture(tex, coord.yz);
	vec4 y_sample = texture(tex, coord.xz);
	vec4 z_sample = texture(tex, coord.xy);
	return x_sample * blend.x + y_sample * blend.y + z_sample * blend.z;
}

void fragment() {
	float safe_blend = max(blend_width, 0.001);
	float low_to_mid = smoothstep(low_height - safe_blend, low_height + safe_blend, terrain_world_pos.y);
	float mid_to_high = smoothstep(high_height - safe_blend, high_height + safe_blend, terrain_world_pos.y);
	float low_weight = 1.0 - low_to_mid;
	float mid_weight = low_to_mid * (1.0 - mid_to_high);
	float high_weight = mid_to_high;
	float weight_sum = max(low_weight + mid_weight + high_weight, 0.001);
	low_weight /= weight_sum;
	mid_weight /= weight_sum;
	high_weight /= weight_sum;

	vec4 low_layer = triplanar_sample(low_albedo, terrain_world_pos, terrain_world_normal) * low_color;
	vec4 mid_layer = triplanar_sample(mid_albedo, terrain_world_pos, terrain_world_normal) * mid_color;
	vec4 high_layer = triplanar_sample(high_albedo, terrain_world_pos, terrain_world_normal) * high_color;
	vec4 albedo = low_layer * low_weight + mid_layer * mid_weight + high_layer * high_weight;

	ALBEDO = albedo.rgb;
	ROUGHNESS = 0.9;
}
)";
}

void SimpleTerrain3D::set_simple_terrain_data(const Ref<SimpleTerrainData> &p_simple_terrain_data) {
	if (simple_terrain_data == p_simple_terrain_data) {
		return;
	}
	if (simple_terrain_data.is_valid()) {
		// Disconnect from the previous resource so edits to an old SimpleTerrainData do
		// not continue rebuilding this node.
		simple_terrain_data->disconnect_changed(callable_mp(this, &SimpleTerrain3D::_simple_terrain_data_changed));
	}
	simple_terrain_data = p_simple_terrain_data;
	if (simple_terrain_data.is_valid()) {
		simple_terrain_data->ensure_height_data_size(false);
		simple_terrain_data->connect_changed(callable_mp(this, &SimpleTerrain3D::_simple_terrain_data_changed));
	}
	rebuild_mesh();
}

void SimpleTerrain3D::set_world_placement_library(const Ref<SimpleWorldPlacementLibrary> &p_library) {
	if (world_placement_library == p_library) {
		return;
	}
	world_placement_library = p_library;
	_mark_navigation_bake_dirty();
	notify_property_list_changed();
}

void SimpleTerrain3D::set_world_placement_data(const Ref<SimpleWorldPlacementData> &p_data) {
	if (world_placement_data == p_data) {
		return;
	}
	world_placement_data = p_data;
	_mark_navigation_bake_dirty();
	notify_property_list_changed();
}

void SimpleTerrain3D::set_grid_size(int p_grid_size) {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->set_grid_size(p_grid_size);
	syncing_data = false;
	rebuild_mesh();
}

int SimpleTerrain3D::get_grid_size() const {
	return simple_terrain_data.is_valid() ? simple_terrain_data->get_grid_size() : 64;
}

void SimpleTerrain3D::set_cell_size(real_t p_cell_size) {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->set_cell_size(p_cell_size);
	syncing_data = false;
	rebuild_mesh();
}

real_t SimpleTerrain3D::get_cell_size() const {
	return simple_terrain_data.is_valid() ? simple_terrain_data->get_cell_size() : 1.0;
}

void SimpleTerrain3D::set_chunk_size(int p_chunk_size) {
	const int new_chunk_size = MAX(1, p_chunk_size);
	if (chunk_size == new_chunk_size) {
		return;
	}
	chunk_size = new_chunk_size;
	rebuild_mesh();
}

void SimpleTerrain3D::set_show_chunk_gizmos(bool p_show) {
	if (show_chunk_gizmos == p_show) {
		return;
	}
	show_chunk_gizmos = p_show;
	update_gizmos();
}

void SimpleTerrain3D::set_navigation_enabled(bool p_enabled) {
	if (navigation_enabled == p_enabled) {
		return;
	}
	navigation_enabled = p_enabled;
	if (navigation_enabled) {
		rebuild_navigation();
	} else {
		_sync_chunk_navigation();
	}
}

void SimpleTerrain3D::set_navigation_layers(uint32_t p_layers) {
	navigation_layers = p_layers;
	_sync_chunk_navigation();
}

void SimpleTerrain3D::set_navigation_max_slope(real_t p_slope) {
	const real_t new_slope = CLAMP(p_slope, (real_t)0.0, (real_t)89.9);
	if (Math::is_equal_approx(navigation_max_slope, new_slope)) {
		return;
	}
	navigation_max_slope = new_slope;
	if (navigation_build_mode == NAVIGATION_BUILD_BAKED) {
		_mark_navigation_bake_dirty();
	} else {
		rebuild_navigation();
	}
}

void SimpleTerrain3D::set_navigation_build_mode(NavigationBuildMode p_mode) {
	if (navigation_build_mode == p_mode) {
		return;
	}
	navigation_build_mode = p_mode;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_baked_mesh(const Ref<NavigationMesh> &p_navigation_mesh) {
	if (navigation_baked_mesh == p_navigation_mesh) {
		return;
	}
	navigation_baked_mesh = p_navigation_mesh;
	navigation_bake_dirty = navigation_baked_mesh.is_null();
	_sync_chunk_navigation();
	notify_property_list_changed();
}

void SimpleTerrain3D::set_navigation_quad_max_normal_angle(real_t p_angle) {
	const real_t new_angle = CLAMP(p_angle, (real_t)0.0, (real_t)89.9);
	if (Math::is_equal_approx(navigation_quad_max_normal_angle, new_angle)) {
		return;
	}
	navigation_quad_max_normal_angle = new_angle;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_quad_planar_tolerance(real_t p_tolerance) {
	const real_t new_tolerance = MAX((real_t)0.0, p_tolerance);
	if (Math::is_equal_approx(navigation_quad_planar_tolerance, new_tolerance)) {
		return;
	}
	navigation_quad_planar_tolerance = new_tolerance;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_merge_max_normal_angle(real_t p_angle) {
	const real_t new_angle = CLAMP(p_angle, (real_t)0.0, (real_t)89.9);
	if (Math::is_equal_approx(navigation_merge_max_normal_angle, new_angle)) {
		return;
	}
	navigation_merge_max_normal_angle = new_angle;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_merge_max_height_delta(real_t p_height_delta) {
	const real_t new_height_delta = MAX((real_t)0.0, p_height_delta);
	if (Math::is_equal_approx(navigation_merge_max_height_delta, new_height_delta)) {
		return;
	}
	navigation_merge_max_height_delta = new_height_delta;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_merge_planar_tolerance(real_t p_tolerance) {
	const real_t new_tolerance = MAX((real_t)0.0, p_tolerance);
	if (Math::is_equal_approx(navigation_merge_planar_tolerance, new_tolerance)) {
		return;
	}
	navigation_merge_planar_tolerance = new_tolerance;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_merge_max_rect_size(int p_size) {
	const int new_size = MAX(1, p_size);
	if (navigation_merge_max_rect_size == new_size) {
		return;
	}
	navigation_merge_max_rect_size = new_size;
	rebuild_navigation();
}

void SimpleTerrain3D::set_navigation_debug_visible(bool p_visible) {
	if (navigation_debug_visible == p_visible) {
		return;
	}
	navigation_debug_visible = p_visible;
#ifdef DEBUG_ENABLED
	if (navigation_debug_instance.is_valid() && !navigation_debug_visible) {
		RenderingServer::get_singleton()->instance_set_visible(navigation_debug_instance, false);
	} else if (navigation_debug_visible && is_inside_tree()) {
		_update_navigation_debug_mesh();
	}
#endif // DEBUG_ENABLED
	notify_property_list_changed();
}

void SimpleTerrain3D::set_navigation_debug_runtime_obstacles_visible(bool p_visible) {
	if (navigation_debug_runtime_obstacles_visible == p_visible) {
		return;
	}
	navigation_debug_runtime_obstacles_visible = p_visible;
#ifdef DEBUG_ENABLED
	if (is_inside_tree()) {
		_update_navigation_debug_mesh();
	}
#endif // DEBUG_ENABLED
	notify_property_list_changed();
}

void SimpleTerrain3D::set_height_data(const PackedFloat32Array &p_height_data) {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->set_height_data(p_height_data);
	syncing_data = false;
	rebuild_mesh();
}

PackedFloat32Array SimpleTerrain3D::get_height_data() const {
	return simple_terrain_data.is_valid() ? simple_terrain_data->get_height_data() : PackedFloat32Array();
}

void SimpleTerrain3D::set_terrain_material(const Ref<Material> &p_material) {
	if (terrain_material == p_material) {
		return;
	}
	terrain_material = p_material;
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_use_builtin_triplanar_material(bool p_use) {
	if (use_builtin_triplanar_material == p_use) {
		return;
	}
	use_builtin_triplanar_material = p_use;
	if (use_builtin_triplanar_material) {
		_update_builtin_triplanar_material();
	}
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_low_texture(const Ref<Texture2D> &p_texture) {
	triplanar_low_texture = p_texture;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_mid_texture(const Ref<Texture2D> &p_texture) {
	triplanar_mid_texture = p_texture;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_high_texture(const Ref<Texture2D> &p_texture) {
	triplanar_high_texture = p_texture;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_low_color(const Color &p_color) {
	triplanar_low_color = p_color;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_mid_color(const Color &p_color) {
	triplanar_mid_color = p_color;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_high_color(const Color &p_color) {
	triplanar_high_color = p_color;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_low_height(real_t p_height) {
	triplanar_low_height = p_height;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_high_height(real_t p_height) {
	triplanar_high_height = p_height;
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_blend_width(real_t p_width) {
	triplanar_blend_width = MAX((real_t)0.001, p_width);
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_texture_scale(real_t p_scale) {
	triplanar_texture_scale = MAX((real_t)0.0001, p_scale);
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_triplanar_blend_sharpness(real_t p_sharpness) {
	triplanar_blend_sharpness = MAX((real_t)0.001, p_sharpness);
	_update_builtin_triplanar_material();
	_sync_chunk_materials();
}

void SimpleTerrain3D::set_flat_height(real_t p_flat_height) {
	flat_height = p_flat_height;
}

void SimpleTerrain3D::set_random_height_scale(real_t p_random_height_scale) {
	random_height_scale = p_random_height_scale;
}

void SimpleTerrain3D::set_random_frequency(real_t p_random_frequency) {
	random_frequency = MAX((real_t)0.0001, p_random_frequency);
}

void SimpleTerrain3D::set_random_octaves(int p_random_octaves) {
	random_octaves = CLAMP(p_random_octaves, 1, 12);
}

void SimpleTerrain3D::set_random_seed(int p_random_seed) {
	random_seed = p_random_seed;
}

void SimpleTerrain3D::reset_flat_terrain() {
	_ensure_data();
	syncing_data = true;
	simple_terrain_data->fill_flat(flat_height);
	syncing_data = false;
	rebuild_mesh();
}

void SimpleTerrain3D::generate_random_terrain() {
	_ensure_data();
	const int vertex_count = simple_terrain_data->get_vertex_count();
	PackedFloat32Array heights;
	heights.resize(vertex_count * vertex_count);

	// This is intentionally simple deterministic fractal value noise. It is a
	// quick generator for editor iteration, not a full terrain synthesis system.
	for (int z = 0; z < vertex_count; z++) {
		for (int x = 0; x < vertex_count; x++) {
			real_t amplitude = 1.0;
			real_t frequency = random_frequency;
			real_t value = 0.0;
			real_t amplitude_sum = 0.0;
			for (int octave = 0; octave < random_octaves; octave++) {
				value += _sample_value_noise((real_t)x * frequency, (real_t)z * frequency, random_seed + octave * 131) * amplitude;
				amplitude_sum += amplitude;
				amplitude *= 0.5;
				frequency *= 2.0;
			}
			heights.set(simple_terrain_data->get_height_index(x, z), (value / MAX((real_t)0.0001, amplitude_sum)) * random_height_scale);
		}
	}

	set_height_data(heights);
}

void SimpleTerrain3D::randomize_seed() {
	RandomPCG rng;
	rng.randomize();
	random_seed = (int)rng.rand();
	generate_random_terrain();
}

void SimpleTerrain3D::rebuild_mesh() {
	_ensure_data();
	const int grid_size = simple_terrain_data->get_grid_size();
	_clear_chunks();
	set_mesh(Ref<Mesh>());
	_mark_navigation_bake_dirty();

	// Build one mesh per chunk. The inherited MeshInstance3D mesh is kept empty
	// because RenderingServer instances below provide the actual renderables.
	for (int origin_z = 0; origin_z < grid_size; origin_z += chunk_size) {
		for (int origin_x = 0; origin_x < grid_size; origin_x += chunk_size) {
			TerrainChunk chunk;
			chunk.origin_x = origin_x;
			chunk.origin_z = origin_z;
			chunk.quad_width = MIN(chunk_size, grid_size - origin_x);
			chunk.quad_depth = MIN(chunk_size, grid_size - origin_z);
			chunk.mesh = _build_chunk_mesh(origin_x, origin_z, chunk.quad_width, chunk.quad_depth);
			if (navigation_enabled && navigation_build_mode != NAVIGATION_BUILD_BAKED) {
				chunk.navigation_mesh = _build_chunk_navigation_mesh(origin_x, origin_z, chunk.quad_width, chunk.quad_depth);
			}
			chunks.push_back(chunk);
		}
	}

	_sync_chunk_instances();
	_sync_chunk_navigation();
	notify_property_list_changed();
	update_gizmos();
}

void SimpleTerrain3D::rebuild_navigation() {
	_ensure_data();
	if (!navigation_enabled) {
		_sync_chunk_navigation();
		return;
	}
	if (navigation_build_mode == NAVIGATION_BUILD_BAKED) {
		_sync_chunk_navigation();
		return;
	}
	if (chunks.is_empty()) {
		rebuild_mesh();
		return;
	}
	for (TerrainChunk &chunk : chunks) {
		chunk.navigation_mesh = _build_chunk_navigation_mesh(chunk.origin_x, chunk.origin_z, chunk.quad_width, chunk.quad_depth);
	}
	_sync_chunk_navigation();
}

void SimpleTerrain3D::bake_navigation(bool p_on_thread) {
	_ensure_data();
	ERR_FAIL_COND(simple_terrain_data.is_null());
	ERR_FAIL_NULL(NavigationServer3D::get_singleton());

	navigation_build_mode = NAVIGATION_BUILD_BAKED;
	if (navigation_baked_mesh.is_null()) {
		navigation_baked_mesh.instantiate();
		navigation_baked_mesh->set_cell_size(MIN((float)simple_terrain_data->get_cell_size(), navigation_baked_mesh->get_cell_size()));
		navigation_baked_mesh->set_cell_height(MIN((float)simple_terrain_data->get_cell_size() * 0.5f, navigation_baked_mesh->get_cell_height()));
		navigation_baked_mesh->set_agent_max_slope((float)navigation_max_slope);
	}

	Ref<NavigationMeshSourceGeometryData3D> source_geometry_data;
	source_geometry_data.instantiate();

	const int grid_size = simple_terrain_data->get_grid_size();
	PackedVector3Array faces;
	faces.resize(grid_size * grid_size * 6);
	Vector3 *faces_w = faces.ptrw();
	int face_index = 0;

	for (int z = 0; z < grid_size; z++) {
		for (int x = 0; x < grid_size; x++) {
			const Vector3 top_left = _get_vertex_position(x, z);
			const Vector3 top_right = _get_vertex_position(x + 1, z);
			const Vector3 bottom_left = _get_vertex_position(x, z + 1);
			const Vector3 bottom_right = _get_vertex_position(x + 1, z + 1);

			faces_w[face_index++] = top_left;
			faces_w[face_index++] = top_right;
			faces_w[face_index++] = bottom_left;
			faces_w[face_index++] = top_right;
			faces_w[face_index++] = bottom_right;
			faces_w[face_index++] = bottom_left;
		}
	}

	source_geometry_data->add_faces(faces, Transform3D());

	if (world_placement_library.is_valid() && world_placement_data.is_valid()) {
		const PackedStringArray profile_ids = world_placement_data->get_profile_ids();
		const PackedVector3Array positions = world_placement_data->get_positions();
		const PackedVector3Array rotations = world_placement_data->get_rotations();
		const PackedVector3Array scales = world_placement_data->get_scales();
		const PackedVector3Array normals = world_placement_data->get_terrain_normals();
		const int placement_count = MIN(profile_ids.size(), MIN(positions.size(), MIN(rotations.size(), MIN(scales.size(), normals.size()))));
		const Transform3D terrain_inverse = get_global_transform().affine_inverse();

		for (int i = 0; i < placement_count; i++) {
			Ref<SimpleWorldObjectProfile> profile = world_placement_library->get_profile_by_id(profile_ids[i]);
			if (profile.is_null() || !profile->uses_baked_navigation_obstacle()) {
				continue;
			}

			const Vector3 local_position = terrain_inverse.xform(positions[i]);
			const Vector3 local_normal = terrain_inverse.basis.xform(normals[i]).normalized();
			const Transform3D placement_transform = _build_profile_placement_transform(local_position, rotations[i], scales[i], local_normal, profile->is_aligning_to_terrain_normal());
			Vector<SimpleTerrainNavigationObstruction> obstructions;
			_append_profile_placement_obstructions(profile, placement_transform, obstructions);
			for (const SimpleTerrainNavigationObstruction &obstruction : obstructions) {
				source_geometry_data->add_projected_obstruction(obstruction.vertices, obstruction.elevation, obstruction.height, profile->get_navigation_obstacle_carve());
			}
		}
	}

	navigation_baked_mesh->set_agent_max_slope((float)navigation_max_slope);

	if (p_on_thread) {
		NavigationServer3D::get_singleton()->bake_from_source_geometry_data_async(navigation_baked_mesh, source_geometry_data, callable_mp(this, &SimpleTerrain3D::_navigation_bake_finished));
	} else {
		NavigationServer3D::get_singleton()->bake_from_source_geometry_data(navigation_baked_mesh, source_geometry_data, callable_mp(this, &SimpleTerrain3D::_navigation_bake_finished));
	}
}

void SimpleTerrain3D::apply_brush(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	apply_brush_with_delta(p_world_position, p_radius, p_strength, p_operation);
}

Dictionary SimpleTerrain3D::apply_brush_with_delta(const Vector3 &p_world_position, real_t p_radius, real_t p_strength, BrushOperation p_operation) {
	Dictionary delta;
	_ensure_data();
	if (!is_inside_tree() || p_radius <= 0.0 || p_strength == 0.0) {
		return delta;
	}

	PackedFloat32Array &heights = simple_terrain_data->get_mutable_height_data();
	const PackedFloat32Array original_heights = p_operation == BRUSH_SMOOTH ? simple_terrain_data->get_height_data() : PackedFloat32Array();
	float *heights_w = heights.ptrw();
	PackedInt32Array changed_indices;
	PackedFloat32Array before_values;
	PackedFloat32Array after_values;
	const Vector3 local_position = get_global_transform().affine_inverse().xform(p_world_position);
	const int vertex_count = simple_terrain_data->get_vertex_count();
	const real_t half_size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size() * 0.5;
	const real_t center_x = (local_position.x + half_size) / simple_terrain_data->get_cell_size();
	const real_t center_z = (local_position.z + half_size) / simple_terrain_data->get_cell_size();
	const real_t radius_cells = p_radius / simple_terrain_data->get_cell_size();
	const int min_x = CLAMP(Math::floor(center_x - radius_cells), 0, vertex_count - 1);
	const int max_x = CLAMP(Math::ceil(center_x + radius_cells), 0, vertex_count - 1);
	const int min_z = CLAMP(Math::floor(center_z - radius_cells), 0, vertex_count - 1);
	const int max_z = CLAMP(Math::ceil(center_z + radius_cells), 0, vertex_count - 1);
	const real_t flatten_height = _sample_nearest_height(center_x, center_z);
	bool changed = false;

	// Brush radius is evaluated in world units but converted to heightmap cells.
	// This keeps painting behavior stable even when cell_size changes.
	for (int z = min_z; z <= max_z; z++) {
		for (int x = min_x; x <= max_x; x++) {
			const real_t dx = ((real_t)x - center_x) * simple_terrain_data->get_cell_size();
			const real_t dz = ((real_t)z - center_z) * simple_terrain_data->get_cell_size();
			const real_t distance = Vector2(dx, dz).length();
			if (distance > p_radius) {
				continue;
			}

			const real_t falloff = 1.0 - distance / p_radius;
			const int index = simple_terrain_data->get_height_index(x, z);
			const real_t before = heights_w[index];
			real_t after = before;
			switch (p_operation) {
				case BRUSH_RAISE:
					after = before + p_strength * falloff;
					break;
				case BRUSH_LOWER:
					after = before - p_strength * falloff;
					break;
				case BRUSH_SMOOTH:
					after = Math::lerp(before, _get_average_neighbor_height(original_heights, x, z), CLAMP(p_strength * falloff, (real_t)0.0, (real_t)1.0));
					break;
				case BRUSH_FLATTEN:
					after = Math::lerp(before, flatten_height, CLAMP(p_strength * falloff, (real_t)0.0, (real_t)1.0));
					break;
			}
			if (!Math::is_equal_approx(before, after)) {
				heights_w[index] = after;
				changed_indices.push_back(index);
				before_values.push_back(before);
				after_values.push_back(after);
				changed = true;
			}
		}
	}

	if (changed) {
		syncing_data = true;
		simple_terrain_data->notify_height_data_changed();
		syncing_data = false;
		_rebuild_chunks_for_region(MAX(0, min_x - 1), MAX(0, min_z - 1), MIN(vertex_count - 1, max_x + 1), MIN(vertex_count - 1, max_z + 1));
	}
	delta["indices"] = changed_indices;
	delta["before"] = before_values;
	delta["after"] = after_values;
	return delta;
}

void SimpleTerrain3D::apply_height_patch(const PackedInt32Array &p_indices, const PackedFloat32Array &p_heights) {
	_ensure_data();
	ERR_FAIL_COND(p_indices.size() != p_heights.size());
	if (p_indices.is_empty()) {
		return;
	}

	PackedFloat32Array &heights = simple_terrain_data->get_mutable_height_data();
	float *heights_w = heights.ptrw();
	const int vertex_count = simple_terrain_data->get_vertex_count();
	int min_x = vertex_count - 1;
	int min_z = vertex_count - 1;
	int max_x = 0;
	int max_z = 0;
	bool changed = false;

	for (int i = 0; i < p_indices.size(); i++) {
		const int index = p_indices[i];
		ERR_CONTINUE(index < 0 || index >= heights.size());
		const real_t height = p_heights[i];
		if (Math::is_equal_approx(heights_w[index], height)) {
			continue;
		}
		heights_w[index] = height;
		const int x = index % vertex_count;
		const int z = index / vertex_count;
		min_x = MIN(min_x, x);
		min_z = MIN(min_z, z);
		max_x = MAX(max_x, x);
		max_z = MAX(max_z, z);
		changed = true;
	}

	if (changed) {
		syncing_data = true;
		simple_terrain_data->notify_height_data_changed();
		syncing_data = false;
		_rebuild_chunks_for_region(MAX(0, min_x - 1), MAX(0, min_z - 1), MIN(vertex_count - 1, max_x + 1), MIN(vertex_count - 1, max_z + 1));
	}
}

Dictionary SimpleTerrain3D::get_brush_hit(const Vector3 &p_ray_origin, const Vector3 &p_ray_direction) const {
	Dictionary result;
	if (!is_inside_tree() || simple_terrain_data.is_null() || p_ray_direction.is_zero_approx()) {
		return result;
	}

	// Ray picking walks only the XZ cells crossed by the ray using a 2D DDA.
	// This is much cheaper than checking every triangle in the full terrain and
	// keeps brush interaction responsive as grid_size grows.
	const Transform3D inverse_transform = get_global_transform().affine_inverse();
	const Vector3 local_origin = inverse_transform.xform(p_ray_origin);
	const Vector3 local_direction = inverse_transform.basis.xform(p_ray_direction).normalized();
	const real_t half_size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size() * 0.5;
	const real_t min_bound = -half_size;
	const real_t max_bound = half_size;
	real_t entry_t = 0.0;
	real_t exit_t = Math::INF;

	auto clip_axis = [&](real_t p_origin, real_t p_direction) {
		if (Math::abs(p_direction) < CMP_EPSILON) {
			return p_origin >= min_bound && p_origin <= max_bound;
		}
		real_t t0 = (min_bound - p_origin) / p_direction;
		real_t t1 = (max_bound - p_origin) / p_direction;
		if (t0 > t1) {
			SWAP(t0, t1);
		}
		entry_t = MAX(entry_t, t0);
		exit_t = MIN(exit_t, t1);
		return entry_t <= exit_t;
	};

	if (!clip_axis(local_origin.x, local_direction.x) || !clip_axis(local_origin.z, local_direction.z)) {
		return result;
	}

	const int grid_size = simple_terrain_data->get_grid_size();
	const real_t cell_size = simple_terrain_data->get_cell_size();
	const Vector3 entry_position = local_origin + local_direction * MAX(entry_t, (real_t)0.0);
	int cell_x = CLAMP(Math::floor((entry_position.x + half_size) / cell_size), 0, grid_size - 1);
	int cell_z = CLAMP(Math::floor((entry_position.z + half_size) / cell_size), 0, grid_size - 1);
	const int end_x = CLAMP(Math::floor(((local_origin + local_direction * exit_t).x + half_size) / cell_size), 0, grid_size - 1);
	const int end_z = CLAMP(Math::floor(((local_origin + local_direction * exit_t).z + half_size) / cell_size), 0, grid_size - 1);

	const int step_x = local_direction.x > CMP_EPSILON ? 1 : (local_direction.x < -CMP_EPSILON ? -1 : 0);
	const int step_z = local_direction.z > CMP_EPSILON ? 1 : (local_direction.z < -CMP_EPSILON ? -1 : 0);
	const real_t next_boundary_x = min_bound + (real_t)(cell_x + (step_x > 0 ? 1 : 0)) * cell_size;
	const real_t next_boundary_z = min_bound + (real_t)(cell_z + (step_z > 0 ? 1 : 0)) * cell_size;
	real_t next_t_x = step_x == 0 ? Math::INF : (next_boundary_x - local_origin.x) / local_direction.x;
	real_t next_t_z = step_z == 0 ? Math::INF : (next_boundary_z - local_origin.z) / local_direction.z;
	const real_t delta_t_x = step_x == 0 ? Math::INF : cell_size / Math::abs(local_direction.x);
	const real_t delta_t_z = step_z == 0 ? Math::INF : cell_size / Math::abs(local_direction.z);
	real_t closest_distance = Math::INF;
	Vector3 closest_position;
	Vector3 closest_normal = Vector3(0.0, 1.0, 0.0);
	bool found = false;

	auto test_cell = [&](int p_x, int p_z) {
		const Vector3 top_left = _get_vertex_position(p_x, p_z);
		const Vector3 top_right = _get_vertex_position(p_x + 1, p_z);
		const Vector3 bottom_left = _get_vertex_position(p_x, p_z + 1);
		const Vector3 bottom_right = _get_vertex_position(p_x + 1, p_z + 1);
		Vector3 hit;
		if (Geometry3D::ray_intersects_triangle(local_origin, local_direction, top_left, top_right, bottom_left, &hit)) {
			const real_t distance = local_origin.distance_to(hit);
			if (distance < closest_distance) {
				Vector3 normal = (top_right - top_left).cross(bottom_left - top_left).normalized();
				if (normal.y < 0.0) {
					normal = -normal;
				}
				closest_distance = distance;
				closest_position = hit;
				closest_normal = normal;
				found = true;
			}
		}
		if (Geometry3D::ray_intersects_triangle(local_origin, local_direction, top_right, bottom_right, bottom_left, &hit)) {
			const real_t distance = local_origin.distance_to(hit);
			if (distance < closest_distance) {
				Vector3 normal = (bottom_right - top_right).cross(bottom_left - top_right).normalized();
				if (normal.y < 0.0) {
					normal = -normal;
				}
				closest_distance = distance;
				closest_position = hit;
				closest_normal = normal;
				found = true;
			}
		}
	};

	while (cell_x >= 0 && cell_x < grid_size && cell_z >= 0 && cell_z < grid_size) {
		test_cell(cell_x, cell_z);
		const real_t next_t = MIN(next_t_x, next_t_z);
		if (found && closest_distance <= next_t) {
			break;
		}
		if (cell_x == end_x && cell_z == end_z) {
			break;
		}
		if (next_t_x < next_t_z) {
			cell_x += step_x;
			next_t_x += delta_t_x;
		} else {
			cell_z += step_z;
			next_t_z += delta_t_z;
		}
	}

	if (found) {
		// Public API returns world-space data because editor tools and gameplay
		// scripts usually operate in scene coordinates.
		const Vector3 world_position = get_global_transform().xform(closest_position);
		const Vector3 world_normal = get_global_transform().basis.xform(closest_normal).normalized();
		result["position"] = world_position;
		result["local_position"] = closest_position;
		result["normal"] = world_normal;
		result["distance"] = p_ray_origin.distance_to(world_position);
	}
	return result;
}

PackedVector3Array SimpleTerrain3D::get_chunk_debug_lines() const {
	PackedVector3Array lines;
	if (simple_terrain_data.is_null() || chunks.is_empty()) {
		return lines;
	}

	// Lines are emitted as pairs. They are local-space points so the editor gizmo
	// can draw them under the SimpleTerrain3D node transform like any other gizmo.
	for (const TerrainChunk &chunk : chunks) {
		const int min_x = chunk.origin_x;
		const int min_z = chunk.origin_z;
		const int max_x = chunk.origin_x + chunk.quad_width;
		const int max_z = chunk.origin_z + chunk.quad_depth;

		lines.push_back(_get_vertex_position(min_x, min_z));
		lines.push_back(_get_vertex_position(max_x, min_z));
		lines.push_back(_get_vertex_position(max_x, min_z));
		lines.push_back(_get_vertex_position(max_x, max_z));
		lines.push_back(_get_vertex_position(max_x, max_z));
		lines.push_back(_get_vertex_position(min_x, max_z));
		lines.push_back(_get_vertex_position(min_x, max_z));
		lines.push_back(_get_vertex_position(min_x, min_z));
	}

	return lines;
}

void SimpleTerrain3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			if (simple_terrain_data.is_null()) {
				// First appearance in a world creates a flat terrain. Random noise
				// generation is intentionally never automatic.
				reset_flat_terrain();
			} else if (chunks.is_empty()) {
				rebuild_mesh();
			}
			_sync_chunk_instances();
			_sync_chunk_navigation();
#ifdef DEBUG_ENABLED
			_update_navigation_debug_mesh();
#endif // DEBUG_ENABLED
			set_notify_transform(true);
		} break;

		case NOTIFICATION_EXIT_WORLD: {
			NavigationServer3D *ns = NavigationServer3D::get_singleton();
			for (TerrainChunk &chunk : chunks) {
				if (chunk.instance.is_valid()) {
					RenderingServer::get_singleton()->instance_set_scenario(chunk.instance, RID());
				}
				if (ns && chunk.navigation_region.is_valid()) {
					ns->region_set_map(chunk.navigation_region, RID());
				}
			}
			if (ns && navigation_baked_region.is_valid()) {
				ns->region_set_map(navigation_baked_region, RID());
			}
#ifdef DEBUG_ENABLED
			if (navigation_debug_instance.is_valid()) {
				RenderingServer::get_singleton()->instance_set_visible(navigation_debug_instance, false);
			}
#endif // DEBUG_ENABLED
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			_sync_chunk_instances();
			_sync_chunk_navigation();
#ifdef DEBUG_ENABLED
			if (navigation_debug_instance.is_valid()) {
				RenderingServer::get_singleton()->instance_set_transform(navigation_debug_instance, get_global_transform());
			}
#endif // DEBUG_ENABLED
		} break;
	}
}

void SimpleTerrain3D::_bind_methods() {
	// Everything bound here becomes part of the public script/editor API:
	// GDScript, C#, the Inspector, UndoRedo, and documentation generation all
	// discover SimpleTerrain3D through these ClassDB registrations. Keep runtime-safe
	// methods here, and keep editor-only actions inside SimpleTerrainEditorPlugin.
	ClassDB::bind_method(D_METHOD("set_simple_terrain_data", "simple_terrain_data"), &SimpleTerrain3D::set_simple_terrain_data);
	ClassDB::bind_method(D_METHOD("get_simple_terrain_data"), &SimpleTerrain3D::get_simple_terrain_data);
	ClassDB::bind_method(D_METHOD("set_world_placement_library", "library"), &SimpleTerrain3D::set_world_placement_library);
	ClassDB::bind_method(D_METHOD("get_world_placement_library"), &SimpleTerrain3D::get_world_placement_library);
	ClassDB::bind_method(D_METHOD("set_world_placement_data", "data"), &SimpleTerrain3D::set_world_placement_data);
	ClassDB::bind_method(D_METHOD("get_world_placement_data"), &SimpleTerrain3D::get_world_placement_data);
	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &SimpleTerrain3D::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &SimpleTerrain3D::get_grid_size);
	ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &SimpleTerrain3D::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &SimpleTerrain3D::get_cell_size);
	ClassDB::bind_method(D_METHOD("set_chunk_size", "chunk_size"), &SimpleTerrain3D::set_chunk_size);
	ClassDB::bind_method(D_METHOD("get_chunk_size"), &SimpleTerrain3D::get_chunk_size);
	ClassDB::bind_method(D_METHOD("set_show_chunk_gizmos", "show"), &SimpleTerrain3D::set_show_chunk_gizmos);
	ClassDB::bind_method(D_METHOD("is_showing_chunk_gizmos"), &SimpleTerrain3D::is_showing_chunk_gizmos);
	ClassDB::bind_method(D_METHOD("set_navigation_enabled", "enabled"), &SimpleTerrain3D::set_navigation_enabled);
	ClassDB::bind_method(D_METHOD("is_navigation_enabled"), &SimpleTerrain3D::is_navigation_enabled);
	ClassDB::bind_method(D_METHOD("set_navigation_layers", "layers"), &SimpleTerrain3D::set_navigation_layers);
	ClassDB::bind_method(D_METHOD("get_navigation_layers"), &SimpleTerrain3D::get_navigation_layers);
	ClassDB::bind_method(D_METHOD("set_navigation_max_slope", "max_slope"), &SimpleTerrain3D::set_navigation_max_slope);
	ClassDB::bind_method(D_METHOD("get_navigation_max_slope"), &SimpleTerrain3D::get_navigation_max_slope);
	ClassDB::bind_method(D_METHOD("set_navigation_build_mode", "mode"), &SimpleTerrain3D::set_navigation_build_mode);
	ClassDB::bind_method(D_METHOD("get_navigation_build_mode"), &SimpleTerrain3D::get_navigation_build_mode);
	ClassDB::bind_method(D_METHOD("set_navigation_baked_mesh", "navigation_mesh"), &SimpleTerrain3D::set_navigation_baked_mesh);
	ClassDB::bind_method(D_METHOD("get_navigation_baked_mesh"), &SimpleTerrain3D::get_navigation_baked_mesh);
	ClassDB::bind_method(D_METHOD("is_navigation_bake_dirty"), &SimpleTerrain3D::is_navigation_bake_dirty);
	ClassDB::bind_method(D_METHOD("set_navigation_quad_max_normal_angle", "angle"), &SimpleTerrain3D::set_navigation_quad_max_normal_angle);
	ClassDB::bind_method(D_METHOD("get_navigation_quad_max_normal_angle"), &SimpleTerrain3D::get_navigation_quad_max_normal_angle);
	ClassDB::bind_method(D_METHOD("set_navigation_quad_planar_tolerance", "tolerance"), &SimpleTerrain3D::set_navigation_quad_planar_tolerance);
	ClassDB::bind_method(D_METHOD("get_navigation_quad_planar_tolerance"), &SimpleTerrain3D::get_navigation_quad_planar_tolerance);
	ClassDB::bind_method(D_METHOD("set_navigation_merge_max_normal_angle", "angle"), &SimpleTerrain3D::set_navigation_merge_max_normal_angle);
	ClassDB::bind_method(D_METHOD("get_navigation_merge_max_normal_angle"), &SimpleTerrain3D::get_navigation_merge_max_normal_angle);
	ClassDB::bind_method(D_METHOD("set_navigation_merge_max_height_delta", "height_delta"), &SimpleTerrain3D::set_navigation_merge_max_height_delta);
	ClassDB::bind_method(D_METHOD("get_navigation_merge_max_height_delta"), &SimpleTerrain3D::get_navigation_merge_max_height_delta);
	ClassDB::bind_method(D_METHOD("set_navigation_merge_planar_tolerance", "tolerance"), &SimpleTerrain3D::set_navigation_merge_planar_tolerance);
	ClassDB::bind_method(D_METHOD("get_navigation_merge_planar_tolerance"), &SimpleTerrain3D::get_navigation_merge_planar_tolerance);
	ClassDB::bind_method(D_METHOD("set_navigation_merge_max_rect_size", "size"), &SimpleTerrain3D::set_navigation_merge_max_rect_size);
	ClassDB::bind_method(D_METHOD("get_navigation_merge_max_rect_size"), &SimpleTerrain3D::get_navigation_merge_max_rect_size);
	ClassDB::bind_method(D_METHOD("set_navigation_debug_visible", "visible"), &SimpleTerrain3D::set_navigation_debug_visible);
	ClassDB::bind_method(D_METHOD("is_navigation_debug_visible"), &SimpleTerrain3D::is_navigation_debug_visible);
	ClassDB::bind_method(D_METHOD("set_navigation_debug_runtime_obstacles_visible", "visible"), &SimpleTerrain3D::set_navigation_debug_runtime_obstacles_visible);
	ClassDB::bind_method(D_METHOD("is_navigation_debug_runtime_obstacles_visible"), &SimpleTerrain3D::is_navigation_debug_runtime_obstacles_visible);
	ClassDB::bind_method(D_METHOD("set_height_data", "height_data"), &SimpleTerrain3D::set_height_data);
	ClassDB::bind_method(D_METHOD("get_height_data"), &SimpleTerrain3D::get_height_data);
	ClassDB::bind_method(D_METHOD("set_terrain_material", "material"), &SimpleTerrain3D::set_terrain_material);
	ClassDB::bind_method(D_METHOD("get_terrain_material"), &SimpleTerrain3D::get_terrain_material);
	ClassDB::bind_method(D_METHOD("set_use_builtin_triplanar_material", "use"), &SimpleTerrain3D::set_use_builtin_triplanar_material);
	ClassDB::bind_method(D_METHOD("is_using_builtin_triplanar_material"), &SimpleTerrain3D::is_using_builtin_triplanar_material);
	ClassDB::bind_method(D_METHOD("set_triplanar_low_texture", "texture"), &SimpleTerrain3D::set_triplanar_low_texture);
	ClassDB::bind_method(D_METHOD("get_triplanar_low_texture"), &SimpleTerrain3D::get_triplanar_low_texture);
	ClassDB::bind_method(D_METHOD("set_triplanar_mid_texture", "texture"), &SimpleTerrain3D::set_triplanar_mid_texture);
	ClassDB::bind_method(D_METHOD("get_triplanar_mid_texture"), &SimpleTerrain3D::get_triplanar_mid_texture);
	ClassDB::bind_method(D_METHOD("set_triplanar_high_texture", "texture"), &SimpleTerrain3D::set_triplanar_high_texture);
	ClassDB::bind_method(D_METHOD("get_triplanar_high_texture"), &SimpleTerrain3D::get_triplanar_high_texture);
	ClassDB::bind_method(D_METHOD("set_triplanar_low_color", "color"), &SimpleTerrain3D::set_triplanar_low_color);
	ClassDB::bind_method(D_METHOD("get_triplanar_low_color"), &SimpleTerrain3D::get_triplanar_low_color);
	ClassDB::bind_method(D_METHOD("set_triplanar_mid_color", "color"), &SimpleTerrain3D::set_triplanar_mid_color);
	ClassDB::bind_method(D_METHOD("get_triplanar_mid_color"), &SimpleTerrain3D::get_triplanar_mid_color);
	ClassDB::bind_method(D_METHOD("set_triplanar_high_color", "color"), &SimpleTerrain3D::set_triplanar_high_color);
	ClassDB::bind_method(D_METHOD("get_triplanar_high_color"), &SimpleTerrain3D::get_triplanar_high_color);
	ClassDB::bind_method(D_METHOD("set_triplanar_low_height", "height"), &SimpleTerrain3D::set_triplanar_low_height);
	ClassDB::bind_method(D_METHOD("get_triplanar_low_height"), &SimpleTerrain3D::get_triplanar_low_height);
	ClassDB::bind_method(D_METHOD("set_triplanar_high_height", "height"), &SimpleTerrain3D::set_triplanar_high_height);
	ClassDB::bind_method(D_METHOD("get_triplanar_high_height"), &SimpleTerrain3D::get_triplanar_high_height);
	ClassDB::bind_method(D_METHOD("set_triplanar_blend_width", "width"), &SimpleTerrain3D::set_triplanar_blend_width);
	ClassDB::bind_method(D_METHOD("get_triplanar_blend_width"), &SimpleTerrain3D::get_triplanar_blend_width);
	ClassDB::bind_method(D_METHOD("set_triplanar_texture_scale", "scale"), &SimpleTerrain3D::set_triplanar_texture_scale);
	ClassDB::bind_method(D_METHOD("get_triplanar_texture_scale"), &SimpleTerrain3D::get_triplanar_texture_scale);
	ClassDB::bind_method(D_METHOD("set_triplanar_blend_sharpness", "sharpness"), &SimpleTerrain3D::set_triplanar_blend_sharpness);
	ClassDB::bind_method(D_METHOD("get_triplanar_blend_sharpness"), &SimpleTerrain3D::get_triplanar_blend_sharpness);
	ClassDB::bind_method(D_METHOD("set_flat_height", "flat_height"), &SimpleTerrain3D::set_flat_height);
	ClassDB::bind_method(D_METHOD("get_flat_height"), &SimpleTerrain3D::get_flat_height);
	ClassDB::bind_method(D_METHOD("set_random_height_scale", "random_height_scale"), &SimpleTerrain3D::set_random_height_scale);
	ClassDB::bind_method(D_METHOD("get_random_height_scale"), &SimpleTerrain3D::get_random_height_scale);
	ClassDB::bind_method(D_METHOD("set_random_frequency", "random_frequency"), &SimpleTerrain3D::set_random_frequency);
	ClassDB::bind_method(D_METHOD("get_random_frequency"), &SimpleTerrain3D::get_random_frequency);
	ClassDB::bind_method(D_METHOD("set_random_octaves", "random_octaves"), &SimpleTerrain3D::set_random_octaves);
	ClassDB::bind_method(D_METHOD("get_random_octaves"), &SimpleTerrain3D::get_random_octaves);
	ClassDB::bind_method(D_METHOD("set_random_seed", "random_seed"), &SimpleTerrain3D::set_random_seed);
	ClassDB::bind_method(D_METHOD("get_random_seed"), &SimpleTerrain3D::get_random_seed);
	ClassDB::bind_method(D_METHOD("reset_flat_terrain"), &SimpleTerrain3D::reset_flat_terrain);
	ClassDB::bind_method(D_METHOD("generate_random_terrain"), &SimpleTerrain3D::generate_random_terrain);
	ClassDB::bind_method(D_METHOD("randomize_seed"), &SimpleTerrain3D::randomize_seed);
	ClassDB::bind_method(D_METHOD("rebuild_mesh"), &SimpleTerrain3D::rebuild_mesh);
	ClassDB::bind_method(D_METHOD("rebuild_navigation"), &SimpleTerrain3D::rebuild_navigation);
	ClassDB::bind_method(D_METHOD("bake_navigation", "on_thread"), &SimpleTerrain3D::bake_navigation, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("apply_brush", "world_position", "radius", "strength", "operation"), &SimpleTerrain3D::apply_brush);
	ClassDB::bind_method(D_METHOD("apply_brush_with_delta", "world_position", "radius", "strength", "operation"), &SimpleTerrain3D::apply_brush_with_delta);
	ClassDB::bind_method(D_METHOD("apply_height_patch", "indices", "heights"), &SimpleTerrain3D::apply_height_patch);
	ClassDB::bind_method(D_METHOD("get_brush_hit", "ray_origin", "ray_direction"), &SimpleTerrain3D::get_brush_hit);
	ClassDB::bind_method(D_METHOD("get_chunk_debug_lines"), &SimpleTerrain3D::get_chunk_debug_lines);

	// Core terrain data and render chunk controls.
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "simple_terrain_data", PROPERTY_HINT_RESOURCE_TYPE, "SimpleTerrainData"), "set_simple_terrain_data", "get_simple_terrain_data");
	ADD_GROUP("World Placement", "world_placement_");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_placement_library", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementLibrary"), "set_world_placement_library", "get_world_placement_library");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_placement_data", PROPERTY_HINT_RESOURCE_TYPE, "SimpleWorldPlacementData"), "set_world_placement_data", "get_world_placement_data");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size", PROPERTY_HINT_RANGE, "2,512,1,or_greater"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater"), "set_cell_size", "get_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "chunk_size", PROPERTY_HINT_RANGE, "1,256,1,or_greater"), "set_chunk_size", "get_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_chunk_gizmos"), "set_show_chunk_gizmos", "is_showing_chunk_gizmos");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "height_data"), "set_height_data", "get_height_data");
	ADD_GROUP("Navigation", "navigation_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "navigation_enabled"), "set_navigation_enabled", "is_navigation_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_layers", PROPERTY_HINT_LAYERS_3D_NAVIGATION), "set_navigation_layers", "get_navigation_layers");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_max_slope", PROPERTY_HINT_RANGE, "0,89.9,0.1,suffix:deg"), "set_navigation_max_slope", "get_navigation_max_slope");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_build_mode", PROPERTY_HINT_ENUM, "Triangles,Quads,Merged Rectangles,Baked"), "set_navigation_build_mode", "get_navigation_build_mode");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "navigation_baked_mesh", PROPERTY_HINT_RESOURCE_TYPE, NavigationMesh::get_class_static()), "set_navigation_baked_mesh", "get_navigation_baked_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_quad_max_normal_angle", PROPERTY_HINT_RANGE, "0,89.9,0.1,suffix:deg"), "set_navigation_quad_max_normal_angle", "get_navigation_quad_max_normal_angle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_quad_planar_tolerance", PROPERTY_HINT_RANGE, "0,10,0.001,or_greater,suffix:m"), "set_navigation_quad_planar_tolerance", "get_navigation_quad_planar_tolerance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_merge_max_normal_angle", PROPERTY_HINT_RANGE, "0,89.9,0.1,suffix:deg"), "set_navigation_merge_max_normal_angle", "get_navigation_merge_max_normal_angle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_merge_max_height_delta", PROPERTY_HINT_RANGE, "0,10,0.001,or_greater,suffix:m"), "set_navigation_merge_max_height_delta", "get_navigation_merge_max_height_delta");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "navigation_merge_planar_tolerance", PROPERTY_HINT_RANGE, "0,10,0.001,or_greater,suffix:m"), "set_navigation_merge_planar_tolerance", "get_navigation_merge_planar_tolerance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_merge_max_rect_size", PROPERTY_HINT_RANGE, "1,128,1,or_greater"), "set_navigation_merge_max_rect_size", "get_navigation_merge_max_rect_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "navigation_debug_visible"), "set_navigation_debug_visible", "is_navigation_debug_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "navigation_debug_runtime_obstacles_visible"), "set_navigation_debug_runtime_obstacles_visible", "is_navigation_debug_runtime_obstacles_visible");
	ADD_GROUP("", "");

	// Material controls are exposed as node properties so game projects can
	// either provide their own material or use the built-in height triplanar
	// shader without writing shader code.
	ADD_GROUP("Terrain Material", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "BaseMaterial3D,ShaderMaterial"), "set_terrain_material", "get_terrain_material");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_builtin_triplanar_material"), "set_use_builtin_triplanar_material", "is_using_builtin_triplanar_material");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "triplanar_low_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_triplanar_low_texture", "get_triplanar_low_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "triplanar_mid_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_triplanar_mid_texture", "get_triplanar_mid_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "triplanar_high_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_triplanar_high_texture", "get_triplanar_high_texture");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "triplanar_low_color"), "set_triplanar_low_color", "get_triplanar_low_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "triplanar_mid_color"), "set_triplanar_mid_color", "get_triplanar_mid_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "triplanar_high_color"), "set_triplanar_high_color", "get_triplanar_high_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_low_height"), "set_triplanar_low_height", "get_triplanar_low_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_high_height"), "set_triplanar_high_height", "get_triplanar_high_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_blend_width", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater"), "set_triplanar_blend_width", "get_triplanar_blend_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_texture_scale", PROPERTY_HINT_RANGE, "0.0001,10,0.0001,or_greater"), "set_triplanar_texture_scale", "get_triplanar_texture_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "triplanar_blend_sharpness", PROPERTY_HINT_RANGE, "0.001,32,0.001,or_greater"), "set_triplanar_blend_sharpness", "get_triplanar_blend_sharpness");

	// Generation settings are runtime-callable. The default terrain still starts
	// flat; random terrain is generated only when explicitly requested.
	ADD_GROUP("Generation", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flat_height"), "set_flat_height", "get_flat_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_height_scale"), "set_random_height_scale", "get_random_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_frequency", PROPERTY_HINT_RANGE, "0.0001,1,0.0001,or_greater"), "set_random_frequency", "get_random_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_octaves", PROPERTY_HINT_RANGE, "1,12,1"), "set_random_octaves", "get_random_octaves");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");

	BIND_ENUM_CONSTANT(BRUSH_RAISE);
	BIND_ENUM_CONSTANT(BRUSH_LOWER);
	BIND_ENUM_CONSTANT(BRUSH_SMOOTH);
	BIND_ENUM_CONSTANT(BRUSH_FLATTEN);
	BIND_ENUM_CONSTANT(NAVIGATION_BUILD_TRIANGLES);
	BIND_ENUM_CONSTANT(NAVIGATION_BUILD_QUADS);
	BIND_ENUM_CONSTANT(NAVIGATION_BUILD_MERGED_RECTS);
	BIND_ENUM_CONSTANT(NAVIGATION_BUILD_BAKED);

	ADD_SIGNAL(MethodInfo("navigation_bake_finished"));
}

SimpleTerrain3D::SimpleTerrain3D() {
#ifdef DEBUG_ENABLED
	NavigationServer3D::get_singleton()->connect(SNAME("navigation_debug_changed"), callable_mp(this, &SimpleTerrain3D::_navigation_debug_changed));
#endif // DEBUG_ENABLED
}

AABB SimpleTerrain3D::get_aabb() const {
	if (simple_terrain_data.is_null()) {
		return AABB();
	}

	// The visible renderables are internal chunk instances, but the editor and
	// culling code still ask the SimpleTerrain3D node for an overall local bounds.
	const int vertex_count = simple_terrain_data->get_vertex_count();
	const PackedFloat32Array heights = simple_terrain_data->get_height_data();
	real_t min_height = 0.0;
	real_t max_height = 0.0;
	if (!heights.is_empty()) {
		min_height = heights[0];
		max_height = heights[0];
		for (int i = 1; i < heights.size(); i++) {
			min_height = MIN(min_height, (real_t)heights[i]);
			max_height = MAX(max_height, (real_t)heights[i]);
		}
	}

	const real_t size = (real_t)simple_terrain_data->get_grid_size() * simple_terrain_data->get_cell_size();
	return AABB(Vector3(-size * 0.5, min_height, -size * 0.5), Vector3(size, MAX((real_t)0.01, max_height - min_height), size));
}

Ref<TriangleMesh> SimpleTerrain3D::generate_triangle_mesh() const {
	Vector<Vector3> faces;
	for (const TerrainChunk &chunk : chunks) {
		if (chunk.mesh.is_null()) {
			continue;
		}
		const Vector<Face3> chunk_faces = chunk.mesh->get_faces();
		const int old_size = faces.size();
		faces.resize(old_size + chunk_faces.size() * 3);
		Vector3 *faces_w = faces.ptrw();
		for (int i = 0; i < chunk_faces.size(); i++) {
			faces_w[old_size + i * 3 + 0] = chunk_faces[i].vertex[0];
			faces_w[old_size + i * 3 + 1] = chunk_faces[i].vertex[1];
			faces_w[old_size + i * 3 + 2] = chunk_faces[i].vertex[2];
		}
	}

	if (faces.is_empty()) {
		return Ref<TriangleMesh>();
	}

	Ref<TriangleMesh> triangle_mesh;
	triangle_mesh.instantiate();
	triangle_mesh->create_from_faces(faces);
	return triangle_mesh;
}

PackedStringArray SimpleTerrain3D::get_configuration_warnings() const {
	return GeometryInstance3D::get_configuration_warnings();
}

SimpleTerrain3D::~SimpleTerrain3D() {
	_clear_chunks();
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	if (ns && navigation_baked_region.is_valid()) {
		ns->free_rid(navigation_baked_region);
	}
#ifdef DEBUG_ENABLED
	if (ns) {
		ns->disconnect(SNAME("navigation_debug_changed"), callable_mp(this, &SimpleTerrain3D::_navigation_debug_changed));
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs && navigation_debug_instance.is_valid()) {
		rs->free_rid(navigation_debug_instance);
	}
	if (rs && navigation_debug_mesh.is_valid()) {
		rs->free_rid(navigation_debug_mesh->get_rid());
	}
#endif // DEBUG_ENABLED
}
