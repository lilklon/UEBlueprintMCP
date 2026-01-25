// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

// Forward declarations
class UMCPBridge;

/**
 * FMCPServer
 *
 * TCP server that accepts connections from MCP clients and routes
 * commands to the Bridge for execution.
 *
 * Key differences from original UnrealMCP:
 * - Persistent connections (socket stays open between commands)
 * - ping/close commands handled without game thread
 * - Timeout handling for stale connections
 */
class UEBLUEPRINTMCP_API FMCPServer : public FRunnable
{
public:
	FMCPServer(UMCPBridge* InBridge, int32 InPort = 55557);
	virtual ~FMCPServer();

	/** Start the server thread */
	bool Start();

	/** Stop the server thread */
	void Stop();

	/** Check if server is running */
	bool IsRunning() const { return bIsRunning; }

	// =========================================================================
	// FRunnable Interface
	// =========================================================================

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

private:
	/** Handle a single client connection */
	void HandleClient(FSocket* ClientSocket);

	/** Receive a message from client (length-prefixed JSON) */
	bool ReceiveMessage(FSocket* ClientSocket, FString& OutMessage);

	/** Send a response to client (length-prefixed JSON) */
	bool SendResponse(FSocket* ClientSocket, const FString& Response);

	/** Handle ping command (no game thread needed) */
	FString HandlePing();

	/** Handle close command */
	void HandleClose(FSocket* ClientSocket);

	/** Handle get_context command (no game thread needed) */
	FString HandleGetContext();

	/** Execute command on game thread and get response */
	FString ExecuteOnGameThread(const FString& CommandType, TSharedPtr<FJsonObject> Params);

	/** The bridge that owns this server */
	UMCPBridge* Bridge;

	/** Listener socket */
	FSocket* ListenerSocket;

	/** Port to listen on */
	int32 Port;

	/** Server thread */
	FRunnableThread* Thread;

	/** Flag to signal thread to stop */
	TAtomic<bool> bShouldStop;

	/** Flag indicating if server is running */
	TAtomic<bool> bIsRunning;

	/** Connection timeout in seconds */
	static constexpr float ConnectionTimeout = 60.0f;

	/** Receive buffer size */
	static constexpr int32 RecvBufferSize = 1024 * 1024;  // 1MB
};
