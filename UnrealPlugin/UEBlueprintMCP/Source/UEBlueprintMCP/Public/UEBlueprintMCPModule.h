// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * UE Blueprint MCP Module
 *
 * Provides an MCP bridge for external tools (like AI assistants) to
 * manipulate Blueprints and the Unreal Editor via TCP commands.
 */
class FUEBlueprintMCPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Check if module is loaded */
	static bool IsAvailable();

	/** Get the module instance */
	static FUEBlueprintMCPModule& Get();
};
