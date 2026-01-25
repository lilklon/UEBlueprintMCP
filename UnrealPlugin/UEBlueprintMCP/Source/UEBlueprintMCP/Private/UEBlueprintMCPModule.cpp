// Copyright (c) 2025 zolnoor. All rights reserved.

#include "UEBlueprintMCPModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FUEBlueprintMCPModule"

void FUEBlueprintMCPModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Module starting up"));

	// The Bridge is an EditorSubsystem and will be automatically created
	// when the editor starts. It handles server startup internally.
}

void FUEBlueprintMCPModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Module shutting down"));

	// The Bridge will be automatically destroyed as an EditorSubsystem.
}

bool FUEBlueprintMCPModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("UEBlueprintMCP");
}

FUEBlueprintMCPModule& FUEBlueprintMCPModule::Get()
{
	return FModuleManager::LoadModuleChecked<FUEBlueprintMCPModule>("UEBlueprintMCP");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUEBlueprintMCPModule, UEBlueprintMCP)
