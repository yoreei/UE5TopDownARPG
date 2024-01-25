// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE5TopDownARPGTarget : TargetRules
{
	public UE5TopDownARPGTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        //DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        // IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
        // ExtraModuleNames.Add("UE5TopDownARPG");
        ExtraModuleNames.AddRange(new string[] { "UE5TopDownARPG", "CrowdPF" });
    }
}
