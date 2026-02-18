// Copyright (c) 2025 zolnoor. All rights reserved.

#include "MCPBridge.h"
#include "MCPServer.h"
#include "Actions/EditorAction.h"
#include "Actions/BlueprintActions.h"
#include "Actions/EditorActions.h"
#include "Actions/NodeActions.h"
#include "Actions/ProjectActions.h"
#include "Actions/UMGActions.h"
#include "Actions/MaterialActions.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"

// NOTE: SEH crash protection is deferred to Phase 2
// For now, using defensive programming (validation before execution)

UMCPBridge::UMCPBridge()
	: Server(nullptr)
{
}

void UMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Bridge initializing"));

	// Register action handlers
	RegisterActions();

	// Start the TCP server
	Server = new FMCPServer(this, DefaultPort);
	if (Server->Start())
	{
		UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Server started on port %d"), DefaultPort);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: Failed to start server"));
	}
}

void UMCPBridge::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Bridge deinitializing"));

	// Stop the server
	if (Server)
	{
		Server->Stop();
		delete Server;
		Server = nullptr;
	}

	// Clear action handlers
	ActionHandlers.Empty();

	Super::Deinitialize();
}

void UMCPBridge::RegisterActions()
{
	// =========================================================================
	// Blueprint Actions
	// =========================================================================
	ActionHandlers.Add(TEXT("create_blueprint"), MakeShared<FCreateBlueprintAction>());
	ActionHandlers.Add(TEXT("compile_blueprint"), MakeShared<FCompileBlueprintAction>());
	ActionHandlers.Add(TEXT("add_component_to_blueprint"), MakeShared<FAddComponentToBlueprintAction>());
	ActionHandlers.Add(TEXT("spawn_blueprint_actor"), MakeShared<FSpawnBlueprintActorAction>());
	ActionHandlers.Add(TEXT("set_component_property"), MakeShared<FSetComponentPropertyAction>());
	ActionHandlers.Add(TEXT("set_static_mesh_properties"), MakeShared<FSetStaticMeshPropertiesAction>());
	ActionHandlers.Add(TEXT("set_physics_properties"), MakeShared<FSetPhysicsPropertiesAction>());
	ActionHandlers.Add(TEXT("set_blueprint_property"), MakeShared<FSetBlueprintPropertyAction>());
	ActionHandlers.Add(TEXT("create_colored_material"), MakeShared<FCreateColoredMaterialAction>());

	// =========================================================================
	// Editor Actions (actors, viewport, save)
	// =========================================================================
	ActionHandlers.Add(TEXT("get_actors_in_level"), MakeShared<FGetActorsInLevelAction>());
	ActionHandlers.Add(TEXT("find_actors_by_name"), MakeShared<FFindActorsByNameAction>());
	ActionHandlers.Add(TEXT("spawn_actor"), MakeShared<FSpawnActorAction>());
	ActionHandlers.Add(TEXT("delete_actor"), MakeShared<FDeleteActorAction>());
	ActionHandlers.Add(TEXT("set_actor_transform"), MakeShared<FSetActorTransformAction>());
	ActionHandlers.Add(TEXT("get_actor_properties"), MakeShared<FGetActorPropertiesAction>());
	ActionHandlers.Add(TEXT("set_actor_property"), MakeShared<FSetActorPropertyAction>());
	ActionHandlers.Add(TEXT("focus_viewport"), MakeShared<FFocusViewportAction>());
	ActionHandlers.Add(TEXT("get_viewport_transform"), MakeShared<FGetViewportTransformAction>());
	ActionHandlers.Add(TEXT("set_viewport_transform"), MakeShared<FSetViewportTransformAction>());
	ActionHandlers.Add(TEXT("save_all"), MakeShared<FSaveAllAction>());

	// =========================================================================
	// Node Actions - Graph Operations
	// =========================================================================
	ActionHandlers.Add(TEXT("connect_blueprint_nodes"), MakeShared<FConnectBlueprintNodesAction>());
	ActionHandlers.Add(TEXT("find_blueprint_nodes"), MakeShared<FFindBlueprintNodesAction>());
	ActionHandlers.Add(TEXT("delete_blueprint_node"), MakeShared<FDeleteBlueprintNodeAction>());
	ActionHandlers.Add(TEXT("get_node_pins"), MakeShared<FGetNodePinsAction>());
	ActionHandlers.Add(TEXT("set_node_position"), MakeShared<FSetNodePositionAction>());

	// =========================================================================
	// Node Actions - Event Nodes
	// =========================================================================
	ActionHandlers.Add(TEXT("add_blueprint_event_node"), MakeShared<FAddBlueprintEventNodeAction>());
	ActionHandlers.Add(TEXT("add_blueprint_input_action_node"), MakeShared<FAddBlueprintInputActionNodeAction>());
	ActionHandlers.Add(TEXT("add_enhanced_input_action_node"), MakeShared<FAddEnhancedInputActionNodeAction>());
	ActionHandlers.Add(TEXT("add_blueprint_custom_event"), MakeShared<FAddBlueprintCustomEventAction>());

	// =========================================================================
	// Node Actions - Variable Nodes
	// =========================================================================
	ActionHandlers.Add(TEXT("add_blueprint_variable"), MakeShared<FAddBlueprintVariableAction>());
	ActionHandlers.Add(TEXT("add_blueprint_variable_get"), MakeShared<FAddBlueprintVariableGetAction>());
	ActionHandlers.Add(TEXT("add_blueprint_variable_set"), MakeShared<FAddBlueprintVariableSetAction>());
	ActionHandlers.Add(TEXT("set_node_pin_default"), MakeShared<FSetNodePinDefaultAction>());

	// =========================================================================
	// Node Actions - Function Nodes
	// =========================================================================
	ActionHandlers.Add(TEXT("add_blueprint_function_node"), MakeShared<FAddBlueprintFunctionNodeAction>());
	ActionHandlers.Add(TEXT("add_blueprint_self_reference"), MakeShared<FAddBlueprintSelfReferenceAction>());
	ActionHandlers.Add(TEXT("add_blueprint_get_self_component_reference"), MakeShared<FAddBlueprintGetSelfComponentReferenceAction>());
	ActionHandlers.Add(TEXT("add_blueprint_branch_node"), MakeShared<FAddBlueprintBranchNodeAction>());
	ActionHandlers.Add(TEXT("add_blueprint_cast_node"), MakeShared<FAddBlueprintCastNodeAction>());
	ActionHandlers.Add(TEXT("add_blueprint_get_subsystem_node"), MakeShared<FAddBlueprintGetSubsystemNodeAction>());

	// =========================================================================
	// Node Actions - Blueprint Function Graph
	// =========================================================================
	ActionHandlers.Add(TEXT("create_blueprint_function"), MakeShared<FCreateBlueprintFunctionAction>());

	// =========================================================================
	// Node Actions - Event Dispatchers
	// =========================================================================
	ActionHandlers.Add(TEXT("add_event_dispatcher"), MakeShared<FAddEventDispatcherAction>());
	ActionHandlers.Add(TEXT("call_event_dispatcher"), MakeShared<FCallEventDispatcherAction>());
	ActionHandlers.Add(TEXT("bind_event_dispatcher"), MakeShared<FBindEventDispatcherAction>());

	// =========================================================================
	// Node Actions - Spawn Actor Nodes
	// =========================================================================
	ActionHandlers.Add(TEXT("add_spawn_actor_from_class_node"), MakeShared<FAddSpawnActorFromClassNodeAction>());
	ActionHandlers.Add(TEXT("call_blueprint_function"), MakeShared<FCallBlueprintFunctionAction>());

	// =========================================================================
	// Node Actions - External Object Property Nodes
	// =========================================================================
	ActionHandlers.Add(TEXT("set_object_property"), MakeShared<FSetObjectPropertyAction>());

	// =========================================================================
	// Node Actions - Macro Instance Nodes
	// =========================================================================
	ActionHandlers.Add(TEXT("add_macro_instance_node"), MakeShared<FAddMacroInstanceNodeAction>());

	// =========================================================================
	// Project Actions (Input Mappings, Enhanced Input)
	// =========================================================================
	ActionHandlers.Add(TEXT("create_input_mapping"), MakeShared<FCreateInputMappingAction>());
	ActionHandlers.Add(TEXT("create_input_action"), MakeShared<FCreateInputActionAction>());
	ActionHandlers.Add(TEXT("create_input_mapping_context"), MakeShared<FCreateInputMappingContextAction>());
	ActionHandlers.Add(TEXT("add_key_mapping_to_context"), MakeShared<FAddKeyMappingToContextAction>());

	// =========================================================================
	// UMG Actions (Widget Blueprints)
	// =========================================================================
	ActionHandlers.Add(TEXT("create_umg_widget_blueprint"), MakeShared<FCreateUMGWidgetBlueprintAction>());
	ActionHandlers.Add(TEXT("add_text_block_to_widget"), MakeShared<FAddTextBlockToWidgetAction>());
	ActionHandlers.Add(TEXT("add_button_to_widget"), MakeShared<FAddButtonToWidgetAction>());
	ActionHandlers.Add(TEXT("bind_widget_event"), MakeShared<FBindWidgetEventAction>());
	ActionHandlers.Add(TEXT("add_widget_to_viewport"), MakeShared<FAddWidgetToViewportAction>());
	ActionHandlers.Add(TEXT("set_text_block_binding"), MakeShared<FSetTextBlockBindingAction>());

	// =========================================================================
	// Material Actions (Materials, Shaders, Post-Process)
	// =========================================================================
	ActionHandlers.Add(TEXT("create_material"), MakeShared<FCreateMaterialAction>());
	ActionHandlers.Add(TEXT("set_material_property"), MakeShared<FSetMaterialPropertyAction>());
	ActionHandlers.Add(TEXT("add_material_expression"), MakeShared<FAddMaterialExpressionAction>());
	ActionHandlers.Add(TEXT("connect_material_expressions"), MakeShared<FConnectMaterialExpressionsAction>());
	ActionHandlers.Add(TEXT("connect_to_material_output"), MakeShared<FConnectToMaterialOutputAction>());
	ActionHandlers.Add(TEXT("set_material_expression_property"), MakeShared<FSetMaterialExpressionPropertyAction>());
	ActionHandlers.Add(TEXT("compile_material"), MakeShared<FCompileMaterialAction>());
	ActionHandlers.Add(TEXT("create_material_instance"), MakeShared<FCreateMaterialInstanceAction>());
	ActionHandlers.Add(TEXT("create_post_process_volume"), MakeShared<FCreatePostProcessVolumeAction>());

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Registered %d action handlers"), ActionHandlers.Num());
}

TSharedRef<FEditorAction>* UMCPBridge::FindAction(const FString& CommandType)
{
	return ActionHandlers.Find(CommandType);
}

TSharedPtr<FJsonObject> UMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	// =========================================================================
	// Action Handlers (modular actions - check these first)
	// =========================================================================
	TSharedRef<FEditorAction>* ActionPtr = FindAction(CommandType);
	if (ActionPtr)
	{
		return (*ActionPtr)->Execute(Params, Context);
	}

	// =========================================================================
	// Unknown Command (all handlers should be registered as actions now)
	// =========================================================================
	return CreateErrorResponse(
		FString::Printf(TEXT("Unknown command type: %s"), *CommandType),
		TEXT("unknown_command")
	);
}

TSharedPtr<FJsonObject> UMCPBridge::ExecuteCommandSafe(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	// TODO Phase 2: Add SEH crash protection here
	// For now, rely on defensive programming (validation in actions)
	return ExecuteCommandInternal(CommandType, Params);
}

TSharedPtr<FJsonObject> UMCPBridge::ExecuteCommandInternal(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	return ExecuteCommand(CommandType, Params);
}

TSharedPtr<FJsonObject> UMCPBridge::CreateSuccessResponse(const TSharedPtr<FJsonObject>& ResultData)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));

	if (ResultData.IsValid())
	{
		Response->SetObjectField(TEXT("result"), ResultData);
	}
	else
	{
		Response->SetObjectField(TEXT("result"), MakeShared<FJsonObject>());
	}

	return Response;
}

TSharedPtr<FJsonObject> UMCPBridge::CreateErrorResponse(const FString& ErrorMessage, const FString& ErrorType)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("error"));
	Response->SetStringField(TEXT("error"), ErrorMessage);
	Response->SetStringField(TEXT("error_type"), ErrorType);

	return Response;
}
