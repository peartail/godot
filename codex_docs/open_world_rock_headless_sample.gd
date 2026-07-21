extends SceneTree

func _initialize() -> void:
	var profile := OpenWorldRockGenerationProfile.new()
	profile.point_count = 36
	profile.strata_strength = 0.18
	var request := OpenWorldRockGenerationRequest.new()
	request.profile = profile
	request.mode = OpenWorldRockGenerationRequest.MODE_BOULDER
	request.seed = 8801
	request.size = Vector3(2.4, 1.7, 2.1)
	request.stable_id = "alpine-rock-8801"
	var generator := OpenWorldRockGenerator3D.new()
	generator.auto_generate = false
	generator.generation_request = request
	var validation: Dictionary = generator.validate_request()
	print(JSON.stringify(validation))
	if not validation.success:
		quit(2)
		return
	generator.generate_rock()
	var report: Dictionary = generator.get_generation_report()
	print(JSON.stringify(report))
	if not report.success:
		quit(3)
		return
	var path := "user://open_world_rock_8801.tres"
	var baked := generator.create_baked_variant()
	if ResourceSaver.save(baked, path) != OK:
		quit(4)
		return
	var loaded := ResourceLoader.load(path, "OpenWorldRockVariant", ResourceLoader.CACHE_MODE_IGNORE) as OpenWorldRockVariant
	if loaded == null or loaded.source_seed != request.seed or loaded.lod2_mesh == null or loaded.collision_shape == null:
		quit(5)
		return
	quit(0)
