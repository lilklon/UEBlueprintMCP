// Copyright (c) 2025 zolnoor. All rights reserved.

#include "MCPContext.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"

FMCPEditorContext::FMCPEditorContext()
	: CurrentGraphName(NAME_None)
{
}

void FMCPEditorContext::SetCurrentBlueprint(UBlueprint* BP)
{
	CurrentBlueprint = BP;

	// Reset graph to event graph when changing blueprints
	CurrentGraphName = NAME_None;
}

void FMCPEditorContext::SetCurrentGraph(const FName& GraphName)
{
	CurrentGraphName = GraphName;
}

UEdGraph* FMCPEditorContext::GetCurrentGraph() const
{
	UBlueprint* BP = CurrentBlueprint.Get();
	if (!BP)
	{
		return nullptr;
	}

	// If a specific graph is set, find it
	if (CurrentGraphName != NAME_None)
	{
		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (Graph && Graph->GetFName() == CurrentGraphName)
			{
				return Graph;
			}
		}
	}

	// Default to event graph
	return GetEventGraph();
}

UEdGraph* FMCPEditorContext::GetEventGraph() const
{
	UBlueprint* BP = CurrentBlueprint.Get();
	if (!BP)
	{
		return nullptr;
	}

	// Find the main event graph
	for (UEdGraph* Graph : BP->UbergraphPages)
	{
		if (Graph && Graph->GetFName() == TEXT("EventGraph"))
		{
			return Graph;
		}
	}

	// Fallback to first ubergraph
	if (BP->UbergraphPages.Num() > 0)
	{
		return BP->UbergraphPages[0];
	}

	return nullptr;
}

void FMCPEditorContext::MarkPackageDirty(UPackage* Package)
{
	if (Package)
	{
		Package->MarkPackageDirty();
		DirtyPackages.Add(Package);
	}
}

void FMCPEditorContext::SaveDirtyPackages()
{
	for (UPackage* Package : DirtyPackages)
	{
		if (Package && Package->IsDirty())
		{
			FString PackageFilename;
			if (FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()))
			{
				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
				SaveArgs.Error = GError;
				UPackage::SavePackage(Package, nullptr, *PackageFilename, SaveArgs);
			}
		}
	}
	DirtyPackages.Empty();
}

void FMCPEditorContext::Clear()
{
	CurrentBlueprint = nullptr;
	CurrentGraphName = NAME_None;
	CurrentWorld = nullptr;
	LastCreatedNodeId.Invalidate();
	LastCreatedActorName.Empty();
	LastCreatedWidgetName.Empty();
	DirtyPackages.Empty();
}

TSharedPtr<FJsonObject> FMCPEditorContext::ToJson() const
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();

	// Current Blueprint
	if (UBlueprint* BP = CurrentBlueprint.Get())
	{
		JsonObj->SetStringField(TEXT("current_blueprint"), BP->GetName());
	}
	else
	{
		JsonObj->SetField(TEXT("current_blueprint"), MakeShared<FJsonValueNull>());
	}

	// Current Graph
	if (CurrentGraphName != NAME_None)
	{
		JsonObj->SetStringField(TEXT("current_graph"), CurrentGraphName.ToString());
	}
	else
	{
		JsonObj->SetStringField(TEXT("current_graph"), TEXT("EventGraph"));
	}

	// Last created objects
	if (LastCreatedNodeId.IsValid())
	{
		JsonObj->SetStringField(TEXT("last_node_id"), LastCreatedNodeId.ToString());
	}
	if (!LastCreatedActorName.IsEmpty())
	{
		JsonObj->SetStringField(TEXT("last_actor_name"), LastCreatedActorName);
	}
	if (!LastCreatedWidgetName.IsEmpty())
	{
		JsonObj->SetStringField(TEXT("last_widget_name"), LastCreatedWidgetName);
	}

	// Dirty packages count
	JsonObj->SetNumberField(TEXT("dirty_packages_count"), DirtyPackages.Num());

	return JsonObj;
}

UBlueprint* FMCPEditorContext::GetBlueprintByNameOrCurrent(const FString& BlueprintName) const
{
	// If name is empty, use current
	if (BlueprintName.IsEmpty())
	{
		return CurrentBlueprint.Get();
	}

	// Search for Blueprint by name
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetList);

	for (const FAssetData& AssetData : AssetList)
	{
		if (AssetData.AssetName.ToString() == BlueprintName)
		{
			return Cast<UBlueprint>(AssetData.GetAsset());
		}
	}

	return nullptr;
}

UEdGraph* FMCPEditorContext::GetGraphByNameOrCurrent(const FString& GraphName) const
{
	UBlueprint* BP = CurrentBlueprint.Get();
	if (!BP)
	{
		return nullptr;
	}

	// If name is empty, use current graph
	if (GraphName.IsEmpty())
	{
		return GetCurrentGraph();
	}

	// Search function graphs
	for (UEdGraph* Graph : BP->FunctionGraphs)
	{
		if (Graph && Graph->GetFName().ToString() == GraphName)
		{
			return Graph;
		}
	}

	// Search ubergraph pages
	for (UEdGraph* Graph : BP->UbergraphPages)
	{
		if (Graph && Graph->GetFName().ToString() == GraphName)
		{
			return Graph;
		}
	}

	return nullptr;
}

FGuid FMCPEditorContext::ResolveNodeId(const FString& NodeIdOrAlias) const
{
	// Handle special alias
	if (NodeIdOrAlias == TEXT("$last_node") || NodeIdOrAlias == TEXT("$last"))
	{
		return LastCreatedNodeId;
	}

	// Try to parse as GUID
	FGuid NodeId;
	if (FGuid::Parse(NodeIdOrAlias, NodeId))
	{
		return NodeId;
	}

	// Invalid
	return FGuid();
}
