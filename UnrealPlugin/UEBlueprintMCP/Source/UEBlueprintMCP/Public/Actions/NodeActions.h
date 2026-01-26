// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorAction.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

// ============================================================================
// Graph Operations (connect, find, delete, inspect)
// ============================================================================

/** Connect two nodes in a Blueprint graph */
class UEBLUEPRINTMCP_API FConnectBlueprintNodesAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("connect_blueprint_nodes"); }
};


/** Find nodes in a Blueprint graph */
class UEBLUEPRINTMCP_API FFindBlueprintNodesAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("find_blueprint_nodes"); }
	virtual bool RequiresSave() const override { return false; }
};


/** Delete a node from a Blueprint graph */
class UEBLUEPRINTMCP_API FDeleteBlueprintNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("delete_blueprint_node"); }
};


/** Get all pins on a node (for debugging connections) */
class UEBLUEPRINTMCP_API FGetNodePinsAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("get_node_pins"); }
	virtual bool RequiresSave() const override { return false; }
};


// ============================================================================
// Event Nodes
// ============================================================================

/** Add an event node (ReceiveBeginPlay, ReceiveTick, etc.) */
class UEBLUEPRINTMCP_API FAddBlueprintEventNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_event_node"); }
};


/** Add an input action event node (legacy input) */
class UEBLUEPRINTMCP_API FAddBlueprintInputActionNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_input_action_node"); }
};


/** Add an Enhanced Input action event node */
class UEBLUEPRINTMCP_API FAddEnhancedInputActionNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_enhanced_input_action_node"); }
};


/** Add a custom event node */
class UEBLUEPRINTMCP_API FAddBlueprintCustomEventAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_custom_event"); }
};


// ============================================================================
// Variable Nodes
// ============================================================================

/** Add a variable to a Blueprint */
class UEBLUEPRINTMCP_API FAddBlueprintVariableAction : public FBlueprintAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_variable"); }
};


/** Add a variable get node */
class UEBLUEPRINTMCP_API FAddBlueprintVariableGetAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_variable_get"); }
};


/** Add a variable set node */
class UEBLUEPRINTMCP_API FAddBlueprintVariableSetAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_variable_set"); }
};


/** Set the default value of a pin */
class UEBLUEPRINTMCP_API FSetNodePinDefaultAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("set_node_pin_default"); }
};


// ============================================================================
// Function Nodes
// ============================================================================

/** Add a function call node */
class UEBLUEPRINTMCP_API FAddBlueprintFunctionNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_function_node"); }
};


/** Add a self reference node */
class UEBLUEPRINTMCP_API FAddBlueprintSelfReferenceAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_self_reference"); }
};


/** Add a component reference node */
class UEBLUEPRINTMCP_API FAddBlueprintGetSelfComponentReferenceAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_get_self_component_reference"); }
};


/** Add a branch (if/then/else) node */
class UEBLUEPRINTMCP_API FAddBlueprintBranchNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_branch_node"); }
};


/** Add a cast node */
class UEBLUEPRINTMCP_API FAddBlueprintCastNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_cast_node"); }
};


/** Add a subsystem getter node (e.g., EnhancedInputLocalPlayerSubsystem) */
class UEBLUEPRINTMCP_API FAddBlueprintGetSubsystemNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_blueprint_get_subsystem_node"); }
};


// ============================================================================
// Blueprint Function Graph
// ============================================================================

/** Create a new function in a Blueprint */
class UEBLUEPRINTMCP_API FCreateBlueprintFunctionAction : public FBlueprintAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("create_blueprint_function"); }
};


// ============================================================================
// Event Dispatchers
// ============================================================================

/** Add an event dispatcher to a Blueprint */
class UEBLUEPRINTMCP_API FAddEventDispatcherAction : public FBlueprintAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_event_dispatcher"); }
};


/** Add a call node for an event dispatcher */
class UEBLUEPRINTMCP_API FCallEventDispatcherAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("call_event_dispatcher"); }
};


/** Add a bind node for an event dispatcher */
class UEBLUEPRINTMCP_API FBindEventDispatcherAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("bind_event_dispatcher"); }
};


// ============================================================================
// Spawn Actor Nodes
// ============================================================================

/** Add a SpawnActorFromClass node */
class UEBLUEPRINTMCP_API FAddSpawnActorFromClassNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_spawn_actor_from_class_node"); }
};


/** Call a Blueprint function */
class UEBLUEPRINTMCP_API FCallBlueprintFunctionAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("call_blueprint_function"); }
};


// ============================================================================
// External Object Property Nodes
// ============================================================================

/** Set a property on an external object reference (e.g., bShowMouseCursor on PlayerController) */
class UEBLUEPRINTMCP_API FSetObjectPropertyAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("set_object_property"); }
};


// ============================================================================
// Macro Instance Nodes
// ============================================================================

/** Add a macro instance node (ForEachLoop, ForLoop, WhileLoop, etc.) */
class UEBLUEPRINTMCP_API FAddMacroInstanceNodeAction : public FBlueprintNodeAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_macro_instance_node"); }
private:
	UEdGraph* FindMacroGraph(const FString& MacroName) const;
};
