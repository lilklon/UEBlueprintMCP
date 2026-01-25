// Copyright (c) 2025 zolnoor. All rights reserved.

#include "MCPBridge.h"
#include "MCPServer.h"
#include "Actions/EditorAction.h"

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
	// TODO: Register action handlers here
	// Example:
	// ActionHandlers.Add(TEXT("create_blueprint"), MakeShared<FCreateBlueprintAction>());
	// ActionHandlers.Add(TEXT("add_component_to_blueprint"), MakeShared<FAddComponentAction>());
	// etc.

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Registered %d action handlers"), ActionHandlers.Num());
}

TSharedRef<FEditorAction>* UMCPBridge::FindAction(const FString& CommandType)
{
	return ActionHandlers.Find(CommandType);
}

TSharedPtr<FJsonObject> UMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	// Find action handler
	TSharedRef<FEditorAction>* ActionPtr = FindAction(CommandType);
	if (!ActionPtr)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Unknown command type: %s"), *CommandType),
			TEXT("unknown_command")
		);
	}

	// Execute the action
	return (*ActionPtr)->Execute(Params, Context);
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
