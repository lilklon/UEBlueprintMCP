// Copyright (c) 2025 zolnoor. All rights reserved.

#include "MCPBridge.h"
#include "MCPServer.h"
#include "Actions/EditorAction.h"

UMCPBridge::UMCPBridge()
{
}

void UMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Bridge initializing"));

	// Register action handlers
	RegisterActions();

	// Start the TCP server
	Server = MakeUnique<FMCPServer>(this, DefaultPort);
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
		Server.Reset();
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
	// Platform-specific crash protection
#if PLATFORM_WINDOWS
	__try
	{
		return ExecuteCommandInternal(CommandType, Params);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: CRASH PREVENTED in command '%s'"), *CommandType);
		return CreateErrorResponse(
			FString::Printf(TEXT("CRASH PREVENTED: Access violation in command '%s'. Operation aborted safely."), *CommandType),
			TEXT("crash_prevented")
		);
	}
#else
	// On non-Windows platforms, just execute directly
	// TODO: Add signal handler for SIGSEGV on Mac/Linux
	return ExecuteCommandInternal(CommandType, Params);
#endif
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
