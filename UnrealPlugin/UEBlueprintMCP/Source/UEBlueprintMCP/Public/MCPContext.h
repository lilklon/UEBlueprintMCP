// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"

/**
 * FMCPEditorContext
 *
 * Tracks the current editing context across MCP commands.
 * This allows commands to reference the "current" blueprint/graph
 * without specifying it each time.
 */
struct UEBLUEPRINTMCP_API FMCPEditorContext
{
public:
	FMCPEditorContext();

	// =========================================================================
	// Current Focus
	// =========================================================================

	/** Currently active Blueprint (weak reference to allow GC) */
	TWeakObjectPtr<UBlueprint> CurrentBlueprint;

	/** Name of the current graph (event graph, function graph, etc.) */
	FName CurrentGraphName;

	/** Currently active world */
	TWeakObjectPtr<UWorld> CurrentWorld;

	// =========================================================================
	// Recently Created Objects (for command chaining)
	// =========================================================================

	/** GUID of the last created node */
	FGuid LastCreatedNodeId;

	/** Name of the last created actor */
	FString LastCreatedActorName;

	/** Name of the last created widget */
	FString LastCreatedWidgetName;

	// =========================================================================
	// Dirty Tracking
	// =========================================================================

	/** Packages that have been modified and need saving */
	TSet<UPackage*> DirtyPackages;

	// =========================================================================
	// Methods
	// =========================================================================

	/** Set the current Blueprint focus */
	void SetCurrentBlueprint(UBlueprint* BP);

	/** Set the current graph by name */
	void SetCurrentGraph(const FName& GraphName);

	/** Get the current graph (event graph if none specified) */
	UEdGraph* GetCurrentGraph() const;

	/** Get the event graph for the current Blueprint */
	UEdGraph* GetEventGraph() const;

	/** Mark a package as dirty (needs saving) */
	void MarkPackageDirty(UPackage* Package);

	/** Save all dirty packages */
	void SaveDirtyPackages();

	/** Clear the context (reset to defaults) */
	void Clear();

	/** Convert context to JSON for Python inspection */
	TSharedPtr<FJsonObject> ToJson() const;

	// =========================================================================
	// Convenience Methods
	// =========================================================================

	/** Get Blueprint by name, or use current if name is empty */
	UBlueprint* GetBlueprintByNameOrCurrent(const FString& BlueprintName) const;

	/** Get graph by name, or use current/event graph if name is empty */
	UEdGraph* GetGraphByNameOrCurrent(const FString& GraphName) const;

	/** Resolve $last_node to actual node ID */
	FGuid ResolveNodeId(const FString& NodeIdOrAlias) const;
};
