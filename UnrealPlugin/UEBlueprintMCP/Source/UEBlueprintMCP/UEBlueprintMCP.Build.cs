// Copyright (c) 2025 zolnoor. All rights reserved.

using UnrealBuildTool;

public class UEBlueprintMCP : ModuleRules
{
	public UEBlueprintMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"EditorSubsystem",
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"GraphEditor",
			"Json",
			"JsonUtilities",
			"Networking",
			"Sockets",
			"UMG",
			"UMGEditor",
			"EnhancedInput",
		});

		// Ensure proper RTTI/exceptions for crash handling
		bUseRTTI = true;
		bEnableExceptions = true;
	}
}
