// Copyright (c) 2025 zolnoor. All rights reserved.

#include "MCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Self.h"
#include "K2Node_InputAction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/Actor.h"
#include "EditorAssetLibrary.h"

// =========================================================================
// JSON Parsing Utilities
// =========================================================================

FVector FMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
	FVector Result(0.0f, 0.0f, 0.0f);

	if (!JsonObject->HasField(FieldName))
	{
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
	{
		Result.X = (float)(*JsonArray)[0]->AsNumber();
		Result.Y = (float)(*JsonArray)[1]->AsNumber();
		Result.Z = (float)(*JsonArray)[2]->AsNumber();
	}

	return Result;
}

FRotator FMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
	FRotator Result(0.0f, 0.0f, 0.0f);

	if (!JsonObject->HasField(FieldName))
	{
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
	{
		Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
		Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
		Result.Roll = (float)(*JsonArray)[2]->AsNumber();
	}

	return Result;
}

FVector2D FMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
	FVector2D Result(0.0f, 0.0f);

	if (!JsonObject->HasField(FieldName))
	{
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
	{
		Result.X = (float)(*JsonArray)[0]->AsNumber();
		Result.Y = (float)(*JsonArray)[1]->AsNumber();
	}

	return Result;
}

// =========================================================================
// Blueprint Utilities
// =========================================================================

UBlueprint* FMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
	FString AssetPath = TEXT("/Game/Blueprints/") + BlueprintName;
	return LoadObject<UBlueprint>(nullptr, *AssetPath);
}

UEdGraph* FMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// Try to find the event graph
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph->GetName().Contains(TEXT("EventGraph")))
		{
			return Graph;
		}
	}

	// Create a new event graph if none exists
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint, FName(TEXT("EventGraph")),
		UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
	return NewGraph;
}

UEdGraph* FMCPCommonUtils::FindFunctionGraph(UBlueprint* Blueprint, const FString& FunctionName)
{
	if (!Blueprint || FunctionName.IsEmpty())
	{
		return nullptr;
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetFName() == FName(*FunctionName))
		{
			return Graph;
		}
	}

	return nullptr;
}

UEdGraph* FMCPCommonUtils::FindGraphByName(UBlueprint* Blueprint, const FString& GraphName)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// If no graph name specified, return the event graph (default behavior)
	if (GraphName.IsEmpty())
	{
		return FindOrCreateEventGraph(Blueprint);
	}

	// First check if it's a function graph
	UEdGraph* FunctionGraph = FindFunctionGraph(Blueprint, GraphName);
	if (FunctionGraph)
	{
		return FunctionGraph;
	}

	// Check ubergraph pages (event graphs can have different names)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph->GetFName() == FName(*GraphName) || Graph->GetName() == GraphName)
		{
			return Graph;
		}
	}

	return nullptr;
}

USCS_Node* FMCPCommonUtils::FindComponentNode(UBlueprint* Blueprint, const FString& ComponentName)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// Traverse inheritance hierarchy to find component
	UBlueprint* SearchBP = Blueprint;
	while (SearchBP)
	{
		if (SearchBP->SimpleConstructionScript)
		{
			for (USCS_Node* Node : SearchBP->SimpleConstructionScript->GetAllNodes())
			{
				if (Node && Node->GetVariableName().ToString() == ComponentName)
				{
					return Node;
				}
			}
		}

		// Walk up to parent Blueprint
		UClass* ParentClass = SearchBP->ParentClass;
		SearchBP = (ParentClass) ? Cast<UBlueprint>(ParentClass->ClassGeneratedBy) : nullptr;
	}

	return nullptr;
}

// =========================================================================
// Property Setting Utilities
// =========================================================================

bool FMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName,
	const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
	if (!Object)
	{
		OutErrorMessage = TEXT("Invalid object");
		return false;
	}

	FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
	if (!Property)
	{
		OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
		return false;
	}

	void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);

	// Handle different property types
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		BoolProp->SetPropertyValue(PropertyAddr, Value->AsBool());
		return true;
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		IntProp->SetPropertyValue_InContainer(Object, static_cast<int32>(Value->AsNumber()));
		return true;
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		FloatProp->SetPropertyValue(PropertyAddr, Value->AsNumber());
		return true;
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		DoubleProp->SetPropertyValue(PropertyAddr, Value->AsNumber());
		return true;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		StrProp->SetPropertyValue(PropertyAddr, Value->AsString());
		return true;
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		UEnum* EnumDef = EnumProp->GetEnum();
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();

		if (EnumDef && UnderlyingProp)
		{
			if (Value->Type == EJson::Number)
			{
				UnderlyingProp->SetIntPropertyValue(PropertyAddr, static_cast<int64>(Value->AsNumber()));
				return true;
			}
			else if (Value->Type == EJson::String)
			{
				FString EnumValueName = Value->AsString();
				if (EnumValueName.Contains(TEXT("::")))
				{
					EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
				}

				int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
				if (EnumValue != INDEX_NONE)
				{
					UnderlyingProp->SetIntPropertyValue(PropertyAddr, EnumValue);
					return true;
				}
				OutErrorMessage = FString::Printf(TEXT("Invalid enum value: %s"), *EnumValueName);
				return false;
			}
		}
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		if (StructProp->Struct == TBaseStructure<FVector>::Get())
		{
			if (Value->Type == EJson::Array)
			{
				TArray<TSharedPtr<FJsonValue>> Arr = Value->AsArray();
				if (Arr.Num() >= 3)
				{
					FVector* VecPtr = (FVector*)PropertyAddr;
					VecPtr->X = Arr[0]->AsNumber();
					VecPtr->Y = Arr[1]->AsNumber();
					VecPtr->Z = Arr[2]->AsNumber();
					return true;
				}
			}
		}
		else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
		{
			if (Value->Type == EJson::Array)
			{
				TArray<TSharedPtr<FJsonValue>> Arr = Value->AsArray();
				if (Arr.Num() >= 3)
				{
					FRotator* RotPtr = (FRotator*)PropertyAddr;
					RotPtr->Pitch = Arr[0]->AsNumber();
					RotPtr->Yaw = Arr[1]->AsNumber();
					RotPtr->Roll = Arr[2]->AsNumber();
					return true;
				}
			}
		}
	}
	else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		FString ClassPath = Value->AsString();
		UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);

		if (!LoadedClass)
		{
			// Try constructing path from Blueprint name
			FString BlueprintPath = FString::Printf(TEXT("/Game/Blueprints/%s.%s_C"), *ClassPath, *ClassPath);
			LoadedClass = LoadObject<UClass>(nullptr, *BlueprintPath);
		}

		if (LoadedClass)
		{
			ClassProp->SetPropertyValue(PropertyAddr, LoadedClass);
			return true;
		}

		OutErrorMessage = FString::Printf(TEXT("Could not load class: %s"), *ClassPath);
		return false;
	}

	OutErrorMessage = FString::Printf(TEXT("Unsupported property type for: %s"), *PropertyName);
	return false;
}

// =========================================================================
// Graph Node Utilities
// =========================================================================

UEdGraphPin* FMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}

	// Exact match first
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
		{
			return Pin;
		}
	}

	// Case-insensitive match
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) &&
			(Direction == EGPD_MAX || Pin->Direction == Direction))
		{
			return Pin;
		}
	}

	return nullptr;
}

UK2Node_Event* FMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
	if (!Graph)
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
		if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
		{
			return EventNode;
		}
	}

	return nullptr;
}

UK2Node_Event* FMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, FVector2D Position)
{
	if (!Graph)
	{
		return nullptr;
	}

	// Get the blueprint for context
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	if (!Blueprint)
	{
		return nullptr;
	}

	// Create the event node
	UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
	if (!EventNode)
	{
		return nullptr;
	}

	// Find the event function in AActor or parent class
	UClass* OwnerClass = Blueprint->GeneratedClass ? Blueprint->GeneratedClass : Blueprint->ParentClass;
	if (OwnerClass)
	{
		UFunction* EventFunc = OwnerClass->FindFunctionByName(FName(*EventName), EIncludeSuperFlag::IncludeSuper);
		if (EventFunc)
		{
			EventNode->EventReference.SetFromField<UFunction>(EventFunc, false);
		}
		else
		{
			// Try with Receive prefix for implementable events
			FString ReceiveEventName = TEXT("Receive") + EventName.Replace(TEXT("Receive"), TEXT(""));
			EventFunc = OwnerClass->FindFunctionByName(FName(*ReceiveEventName), EIncludeSuperFlag::IncludeSuper);
			if (EventFunc)
			{
				EventNode->EventReference.SetFromField<UFunction>(EventFunc, false);
			}
		}
	}

	EventNode->NodePosX = Position.X;
	EventNode->NodePosY = Position.Y;

	Graph->AddNode(EventNode);
	EventNode->CreateNewGuid();
	EventNode->PostPlacedNewNode();
	EventNode->AllocateDefaultPins();

	return EventNode;
}

UK2Node_InputAction* FMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, FVector2D Position)
{
	if (!Graph)
	{
		return nullptr;
	}

	UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
	if (!InputActionNode)
	{
		return nullptr;
	}

	InputActionNode->InputActionName = FName(*ActionName);
	InputActionNode->NodePosX = Position.X;
	InputActionNode->NodePosY = Position.Y;

	Graph->AddNode(InputActionNode);
	InputActionNode->CreateNewGuid();
	InputActionNode->PostPlacedNewNode();
	InputActionNode->AllocateDefaultPins();

	return InputActionNode;
}

UK2Node_CallFunction* FMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, FVector2D Position)
{
	if (!Graph || !Function)
	{
		return nullptr;
	}

	UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!FunctionNode)
	{
		return nullptr;
	}

	FunctionNode->SetFromFunction(Function);
	FunctionNode->NodePosX = Position.X;
	FunctionNode->NodePosY = Position.Y;

	Graph->AddNode(FunctionNode);
	FunctionNode->CreateNewGuid();
	FunctionNode->PostPlacedNewNode();
	FunctionNode->AllocateDefaultPins();

	return FunctionNode;
}

UK2Node_Self* FMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, FVector2D Position)
{
	if (!Graph)
	{
		return nullptr;
	}

	UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
	if (!SelfNode)
	{
		return nullptr;
	}

	SelfNode->NodePosX = Position.X;
	SelfNode->NodePosY = Position.Y;

	Graph->AddNode(SelfNode);
	SelfNode->CreateNewGuid();
	SelfNode->PostPlacedNewNode();
	SelfNode->AllocateDefaultPins();

	return SelfNode;
}

TSharedPtr<FJsonObject> FMCPCommonUtils::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

// =========================================================================
// Actor Utilities
// =========================================================================

TSharedPtr<FJsonObject> FMCPCommonUtils::ActorToJsonObject(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
	ActorObject->SetStringField(TEXT("name"), Actor->GetName());
	ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

	FVector Location = Actor->GetActorLocation();
	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	ActorObject->SetArrayField(TEXT("location"), LocationArray);

	FRotator Rotation = Actor->GetActorRotation();
	TArray<TSharedPtr<FJsonValue>> RotationArray;
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
	ActorObject->SetArrayField(TEXT("rotation"), RotationArray);

	FVector Scale = Actor->GetActorScale3D();
	TArray<TSharedPtr<FJsonValue>> ScaleArray;
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
	ActorObject->SetArrayField(TEXT("scale"), ScaleArray);

	return ActorObject;
}

TSharedPtr<FJsonValue> FMCPCommonUtils::ActorToJsonValue(AActor* Actor)
{
	TSharedPtr<FJsonObject> Obj = ActorToJsonObject(Actor);
	if (Obj.IsValid())
	{
		return MakeShared<FJsonValueObject>(Obj);
	}
	return MakeShared<FJsonValueNull>();
}
