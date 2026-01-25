// Copyright (c) 2025 zolnoor. All rights reserved.

#include "MCPServer.h"
#include "MCPBridge.h"
#include "Async/Async.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

FMCPServer::FMCPServer(UMCPBridge* InBridge, int32 InPort)
	: Bridge(InBridge)
	, ListenerSocket(nullptr)
	, Port(InPort)
	, Thread(nullptr)
	, bShouldStop(false)
	, bIsRunning(false)
{
}

FMCPServer::~FMCPServer()
{
	Stop();
}

bool FMCPServer::Start()
{
	if (bIsRunning)
	{
		return true;
	}

	// Create the listener socket
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: Failed to get socket subsystem"));
		return false;
	}

	ListenerSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UEBlueprintMCP Listener"), false);
	if (!ListenerSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: Failed to create listener socket"));
		return false;
	}

	// Bind to port
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	Addr->SetAnyAddress();
	Addr->SetPort(Port);

	if (!ListenerSocket->Bind(*Addr))
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: Failed to bind to port %d"), Port);
		SocketSubsystem->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
		return false;
	}

	// Start listening
	if (!ListenerSocket->Listen(8))
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: Failed to listen on socket"));
		SocketSubsystem->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
		return false;
	}

	// Start the worker thread
	bShouldStop = false;
	Thread = FRunnableThread::Create(this, TEXT("UEBlueprintMCP Server Thread"));
	if (!Thread)
	{
		UE_LOG(LogTemp, Error, TEXT("UEBlueprintMCP: Failed to create server thread"));
		SocketSubsystem->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
		return false;
	}

	return true;
}

void FMCPServer::Stop()
{
	bShouldStop = true;

	if (ListenerSocket)
	{
		ListenerSocket->Close();
	}

	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}

	if (ListenerSocket)
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(ListenerSocket);
		}
		ListenerSocket = nullptr;
	}

	bIsRunning = false;
}

bool FMCPServer::Init()
{
	return true;
}

uint32 FMCPServer::Run()
{
	bIsRunning = true;

	while (!bShouldStop)
	{
		// Wait for connection (with timeout so we can check bShouldStop)
		bool bPendingConnection = false;
		if (ListenerSocket->WaitForPendingConnection(bPendingConnection, FTimespan::FromSeconds(0.5)))
		{
			if (bPendingConnection)
			{
				// Accept the connection
				FSocket* ClientSocket = ListenerSocket->Accept(TEXT("UEBlueprintMCP Client"));
				if (ClientSocket)
				{
					UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Client connected"));
					HandleClient(ClientSocket);
					UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Client disconnected"));

					ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
					if (SocketSubsystem)
					{
						SocketSubsystem->DestroySocket(ClientSocket);
					}
				}
			}
		}
	}

	bIsRunning = false;
	return 0;
}

void FMCPServer::Exit()
{
	bIsRunning = false;
}

void FMCPServer::HandleClient(FSocket* ClientSocket)
{
	// Set socket options
	ClientSocket->SetNonBlocking(false);
	ClientSocket->SetNoDelay(true);

	float LastActivityTime = FPlatformTime::Seconds();

	// Keep connection alive until client disconnects or timeout
	while (!bShouldStop)
	{
		// Check for timeout
		float CurrentTime = FPlatformTime::Seconds();
		if (CurrentTime - LastActivityTime > ConnectionTimeout)
		{
			UE_LOG(LogTemp, Warning, TEXT("UEBlueprintMCP: Client connection timed out"));
			break;
		}

		// Check if data is available
		uint32 PendingDataSize = 0;
		if (!ClientSocket->HasPendingData(PendingDataSize))
		{
			// No data, sleep briefly
			FPlatformProcess::Sleep(0.01f);
			continue;
		}

		// Receive message
		FString Message;
		if (!ReceiveMessage(ClientSocket, Message))
		{
			UE_LOG(LogTemp, Warning, TEXT("UEBlueprintMCP: Failed to receive message"));
			break;
		}

		LastActivityTime = FPlatformTime::Seconds();

		// Parse JSON
		TSharedPtr<FJsonObject> JsonObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
		if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
		{
			FString ErrorResponse = TEXT("{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
			SendResponse(ClientSocket, ErrorResponse);
			continue;
		}

		// Get command type
		FString CommandType;
		if (!JsonObj->TryGetStringField(TEXT("type"), CommandType))
		{
			FString ErrorResponse = TEXT("{\"status\":\"error\",\"error\":\"Missing 'type' field\"}");
			SendResponse(ClientSocket, ErrorResponse);
			continue;
		}

		// Handle special commands that don't need game thread
		if (CommandType == TEXT("ping"))
		{
			SendResponse(ClientSocket, HandlePing());
			continue;
		}

		if (CommandType == TEXT("close"))
		{
			HandleClose(ClientSocket);
			break;
		}

		if (CommandType == TEXT("get_context"))
		{
			SendResponse(ClientSocket, HandleGetContext());
			continue;
		}

		// Get params (optional)
		TSharedPtr<FJsonObject> Params;
		const TSharedPtr<FJsonObject>* ParamsPtr;
		if (JsonObj->TryGetObjectField(TEXT("params"), ParamsPtr))
		{
			Params = *ParamsPtr;
		}
		else
		{
			Params = MakeShared<FJsonObject>();
		}

		// Execute on game thread and get response
		FString Response = ExecuteOnGameThread(CommandType, Params);
		SendResponse(ClientSocket, Response);
	}
}

bool FMCPServer::ReceiveMessage(FSocket* ClientSocket, FString& OutMessage)
{
	// Receive length prefix (4 bytes, big endian)
	uint8 LengthBytes[4];
	int32 BytesRead = 0;
	if (!ClientSocket->Recv(LengthBytes, 4, BytesRead) || BytesRead != 4)
	{
		return false;
	}

	int32 Length = (LengthBytes[0] << 24) | (LengthBytes[1] << 16) | (LengthBytes[2] << 8) | LengthBytes[3];

	// Sanity check
	if (Length <= 0 || Length > RecvBufferSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEBlueprintMCP: Invalid message length: %d"), Length);
		return false;
	}

	// Receive message
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(Length);

	int32 TotalReceived = 0;
	while (TotalReceived < Length)
	{
		int32 Received = 0;
		if (!ClientSocket->Recv(Buffer.GetData() + TotalReceived, Length - TotalReceived, Received) || Received <= 0)
		{
			return false;
		}
		TotalReceived += Received;
	}

	// Convert to string
	OutMessage = FString(Length, UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())));
	return true;
}

bool FMCPServer::SendResponse(FSocket* ClientSocket, const FString& Response)
{
	// Convert to UTF-8
	FTCHARToUTF8 Converter(*Response);
	int32 Length = Converter.Length();

	// Send length prefix (4 bytes, big endian)
	uint8 LengthBytes[4] = {
		static_cast<uint8>((Length >> 24) & 0xFF),
		static_cast<uint8>((Length >> 16) & 0xFF),
		static_cast<uint8>((Length >> 8) & 0xFF),
		static_cast<uint8>(Length & 0xFF)
	};

	int32 BytesSent = 0;
	if (!ClientSocket->Send(LengthBytes, 4, BytesSent) || BytesSent != 4)
	{
		return false;
	}

	// Send message
	int32 TotalSent = 0;
	while (TotalSent < Length)
	{
		int32 Sent = 0;
		if (!ClientSocket->Send(reinterpret_cast<const uint8*>(Converter.Get()) + TotalSent, Length - TotalSent, Sent) || Sent <= 0)
		{
			return false;
		}
		TotalSent += Sent;
	}

	return true;
}

FString FMCPServer::HandlePing()
{
	return TEXT("{\"status\":\"success\",\"result\":{\"pong\":true}}");
}

void FMCPServer::HandleClose(FSocket* ClientSocket)
{
	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Client requested disconnect"));
	SendResponse(ClientSocket, TEXT("{\"status\":\"success\",\"result\":{\"closed\":true}}"));
}

FString FMCPServer::HandleGetContext()
{
	// Execute on game thread to safely access context
	FString Result;

	FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool(false);

	AsyncTask(ENamedThreads::GameThread, [this, &Result, DoneEvent]()
	{
		if (Bridge)
		{
			TSharedPtr<FJsonObject> ContextJson = Bridge->GetContext().ToJson();

			TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
			Response->SetStringField(TEXT("status"), TEXT("success"));
			Response->SetObjectField(TEXT("result"), ContextJson);

			FString ResponseStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseStr);
			FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
			Result = ResponseStr;
		}
		else
		{
			Result = TEXT("{\"status\":\"error\",\"error\":\"Bridge not available\"}");
		}

		DoneEvent->Trigger();
	});

	DoneEvent->Wait();
	FPlatformProcess::ReturnSynchEventToPool(DoneEvent);

	return Result;
}

FString FMCPServer::ExecuteOnGameThread(const FString& CommandType, TSharedPtr<FJsonObject> Params)
{
	FString Result;
	FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool(false);

	AsyncTask(ENamedThreads::GameThread, [this, &CommandType, Params, &Result, DoneEvent]()
	{
		if (Bridge)
		{
			// Execute with crash protection
			TSharedPtr<FJsonObject> Response = Bridge->ExecuteCommandSafe(CommandType, Params);

			// Serialize response
			FString ResponseStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseStr);
			FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
			Result = ResponseStr;
		}
		else
		{
			Result = TEXT("{\"status\":\"error\",\"error\":\"Bridge not available\"}");
		}

		DoneEvent->Trigger();
	});

	// Wait for game thread to complete
	DoneEvent->Wait();
	FPlatformProcess::ReturnSynchEventToPool(DoneEvent);

	return Result;
}
