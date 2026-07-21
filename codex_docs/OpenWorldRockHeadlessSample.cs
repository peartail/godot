using Godot;

public static class OpenWorldRockHeadlessSample
{
    public static Error GenerateAndSave(string path)
    {
        var profile = new OpenWorldRockGenerationProfile { PointCount = 36, StrataStrength = 0.18f };
        var request = new OpenWorldRockGenerationRequest {
            Profile = profile,
            Mode = OpenWorldRockGenerationRequest.RockMode.Boulder,
            Seed = 8801,
            Size = new Vector3(2.4f, 1.7f, 2.1f),
            StableId = "alpine-rock-8801"
        };
        using var generator = new OpenWorldRockGenerator3D { AutoGenerate = false, GenerationRequest = request };
        Godot.Collections.Dictionary validation = generator.ValidateRequest();
        if (!(bool)validation["success"])
            return Error.InvalidData;
        generator.GenerateRock();
        Godot.Collections.Dictionary report = generator.GetGenerationReport();
        GD.Print(Json.Stringify(report));
        if (!(bool)report["success"])
            return Error.CantCreate;
        return ResourceSaver.Save(generator.CreateBakedVariant(), path);
    }
}
