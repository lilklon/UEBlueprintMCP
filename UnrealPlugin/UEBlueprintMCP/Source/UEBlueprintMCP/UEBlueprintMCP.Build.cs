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
			"InputBlueprintNodes",
			"EditorScriptingUtilities",
			"AssetTools",
			"MaterialEditor",     // For UMaterialEditingLibrary and material expression manipulation
			"RenderCore",         // For material shader compilation
		});

		// RTTI/exceptions for crash handling (Windows SEH only - disable on Mac to avoid linker errors)
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			bUseRTTI = true;
			bEnableExceptions = true;
		}
	}
}
