// Copyright (c) 2025 zolnoor. All rights reserved.

#include "Actions/NodeActions.h"
#include "MCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_EnhancedInputAction.h"
#include "K2Node_GetSubsystem.h"
#include "K2Node_MacroInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

// ============================================================================
// Graph Operations (connect, find, delete, inspect)
// ============================================================================

bool FConnectBlueprintNodesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString SourceNodeId, TargetNodeId, SourcePin, TargetPin;
	if (!GetRequiredString(Params, TEXT("source_node_id"), SourceNodeId, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("target_node_id"), TargetNodeId, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("source_pin"), SourcePin, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("target_pin"), TargetPin, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FConnectBlueprintNodesAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString SourceNodeId = Params->GetStringField(TEXT("source_node_id"));
	FString TargetNodeId = Params->GetStringField(TEXT("target_node_id"));
	FString SourcePinName = Params->GetStringField(TEXT("source_pin"));
	FString TargetPinName = Params->GetStringField(TEXT("target_pin"));

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the nodes
	UEdGraphNode* SourceNode = nullptr;
	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (Node->NodeGuid.ToString() == SourceNodeId)
		{
			SourceNode = Node;
		}
		else if (Node->NodeGuid.ToString() == TargetNodeId)
		{
			TargetNode = Node;
		}
	}

	if (!SourceNode || !TargetNode)
	{
		return CreateErrorResponse(TEXT("Source or target node not found"));
	}

	// Find pins and provide detailed error messages
	UEdGraphPin* SourcePin = FMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
	UEdGraphPin* TargetPin = FMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);

	auto GetAvailablePins = [](UEdGraphNode* Node, EEdGraphPinDirection Direction) -> FString
	{
		TArray<FString> PinNames;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == Direction && !Pin->bHidden)
			{
				PinNames.Add(FString::Printf(TEXT("'%s' (%s)"),
					*Pin->PinName.ToString(),
					*Pin->PinType.PinCategory.ToString()));
			}
		}
		return FString::Join(PinNames, TEXT(", "));
	};

	if (!SourcePin)
	{
		FString AvailablePins = GetAvailablePins(SourceNode, EGPD_Output);
		return CreateErrorResponse(FString::Printf(
			TEXT("Source pin '%s' not found on node. Available OUTPUT pins: [%s]"),
			*SourcePinName, *AvailablePins));
	}

	if (!TargetPin)
	{
		FString AvailablePins = GetAvailablePins(TargetNode, EGPD_Input);
		return CreateErrorResponse(FString::Printf(
			TEXT("Target pin '%s' not found on node. Available INPUT pins: [%s]"),
			*TargetPinName, *AvailablePins));
	}

	// Connect using the schema
	const UEdGraphSchema* Schema = TargetGraph->GetSchema();
	if (Schema)
	{
		bool bResult = Schema->TryCreateConnection(SourcePin, TargetPin);
		if (bResult)
		{
			SourceNode->PinConnectionListChanged(SourcePin);
			TargetNode->PinConnectionListChanged(TargetPin);
			MarkBlueprintModified(Blueprint, Context);

			TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
			ResultData->SetStringField(TEXT("source_node_id"), SourceNodeId);
			ResultData->SetStringField(TEXT("target_node_id"), TargetNodeId);
			return CreateSuccessResponse(ResultData);
		}
		else
		{
			return CreateErrorResponse(FString::Printf(
				TEXT("Schema refused connection: '%s' (%s) -> '%s' (%s). Types may be incompatible."),
				*SourcePin->PinName.ToString(), *SourcePin->PinType.PinCategory.ToString(),
				*TargetPin->PinName.ToString(), *TargetPin->PinType.PinCategory.ToString()));
		}
	}

	return CreateErrorResponse(TEXT("Failed to get graph schema"));
}


bool FFindBlueprintNodesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FFindBlueprintNodesAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString NodeType = GetOptionalString(Params, TEXT("node_type"));
	FString EventName = GetOptionalString(Params, TEXT("event_name"));

	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	TArray<TSharedPtr<FJsonValue>> NodesArray;

	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (!Node) continue;

		bool bMatch = false;

		if (NodeType.IsEmpty())
		{
			// No filter - include all nodes
			bMatch = true;
		}
		else if (NodeType == TEXT("Event"))
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (EventNode)
			{
				if (EventName.IsEmpty())
				{
					bMatch = true;
				}
				else if (EventNode->EventReference.GetMemberName() == FName(*EventName))
				{
					bMatch = true;
				}
			}
			UK2Node_CustomEvent* CustomNode = Cast<UK2Node_CustomEvent>(Node);
			if (CustomNode)
			{
				if (EventName.IsEmpty())
				{
					bMatch = true;
				}
				else if (CustomNode->CustomFunctionName == EventName)
				{
					bMatch = true;
				}
			}
		}
		else if (NodeType == TEXT("Function"))
		{
			if (Cast<UK2Node_CallFunction>(Node))
			{
				bMatch = true;
			}
		}
		else if (NodeType == TEXT("Variable"))
		{
			if (Cast<UK2Node_VariableGet>(Node) || Cast<UK2Node_VariableSet>(Node))
			{
				bMatch = true;
			}
		}

		if (bMatch)
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
			NodeObj->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString());
			NodeObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
			NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
		}
	}

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetArrayField(TEXT("nodes"), NodesArray);
	ResultData->SetNumberField(TEXT("count"), NodesArray.Num());
	return CreateSuccessResponse(ResultData);
}


bool FDeleteBlueprintNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString NodeId;
	if (!GetRequiredString(Params, TEXT("node_id"), NodeId, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FDeleteBlueprintNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString NodeId = Params->GetStringField(TEXT("node_id"));

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the node
	UEdGraphNode* NodeToDelete = nullptr;
	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (Node->NodeGuid.ToString() == NodeId)
		{
			NodeToDelete = Node;
			break;
		}
	}

	if (!NodeToDelete)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found with ID: %s"), *NodeId));
	}

	FString NodeClass = NodeToDelete->GetClass()->GetName();
	FString NodeTitle = NodeToDelete->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

	// Break all pin connections
	for (UEdGraphPin* Pin : NodeToDelete->Pins)
	{
		Pin->BreakAllPinLinks();
	}

	// Remove from graph
	TargetGraph->RemoveNode(NodeToDelete);
	MarkBlueprintModified(Blueprint, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("deleted_node_id"), NodeId);
	ResultData->SetStringField(TEXT("deleted_node_class"), NodeClass);
	ResultData->SetStringField(TEXT("deleted_node_title"), NodeTitle);
	return CreateSuccessResponse(ResultData);
}


bool FGetNodePinsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString NodeId;
	if (!GetRequiredString(Params, TEXT("node_id"), NodeId, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FGetNodePinsAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString NodeId = Params->GetStringField(TEXT("node_id"));
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the node
	UEdGraphNode* FoundNode = nullptr;
	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (Node->NodeGuid.ToString() == NodeId)
		{
			FoundNode = Node;
			break;
		}
	}

	if (!FoundNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found with ID: %s"), *NodeId));
	}

	// Build array of pin info
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : FoundNode->Pins)
	{
		TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
		PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
		PinObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
		if (Pin->PinType.PinSubCategory != NAME_None)
		{
			PinObj->SetStringField(TEXT("sub_category"), Pin->PinType.PinSubCategory.ToString());
		}
		if (Pin->PinType.PinSubCategoryObject.Get())
		{
			PinObj->SetStringField(TEXT("sub_category_object"), Pin->PinType.PinSubCategoryObject->GetName());
		}
		PinObj->SetBoolField(TEXT("is_hidden"), Pin->bHidden);
		PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
	}

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_class"), FoundNode->GetClass()->GetName());
	ResultData->SetArrayField(TEXT("pins"), PinsArray);
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Event Nodes
// ============================================================================

bool FAddBlueprintEventNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString EventName;
	if (!GetRequiredString(Params, TEXT("event_name"), EventName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintEventNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString EventName = Params->GetStringField(TEXT("event_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = GetTargetGraph(Params, Context);

	UK2Node_Event* EventNode = FMCPCommonUtils::CreateEventNode(EventGraph, EventName, Position);
	if (!EventNode)
	{
		return CreateErrorResponse(TEXT("Failed to create event node"));
	}

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(EventNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintInputActionNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString ActionName;
	if (!GetRequiredString(Params, TEXT("action_name"), ActionName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintInputActionNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActionName = Params->GetStringField(TEXT("action_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = FMCPCommonUtils::FindOrCreateEventGraph(Blueprint);

	UK2Node_InputAction* InputActionNode = FMCPCommonUtils::CreateInputActionNode(EventGraph, ActionName, Position);
	if (!InputActionNode)
	{
		return CreateErrorResponse(TEXT("Failed to create input action node"));
	}

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(InputActionNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddEnhancedInputActionNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString ActionName;
	if (!GetRequiredString(Params, TEXT("action_name"), ActionName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddEnhancedInputActionNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActionName = Params->GetStringField(TEXT("action_name"));
	FString ActionPath = GetOptionalString(Params, TEXT("action_path"), TEXT("/Game/Input"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = FMCPCommonUtils::FindOrCreateEventGraph(Blueprint);

	// Load the UInputAction asset
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *ActionPath, *ActionName, *ActionName);
	UInputAction* InputActionAsset = LoadObject<UInputAction>(nullptr, *AssetPath);
	if (!InputActionAsset)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Input Action asset not found: %s"), *AssetPath));
	}

	// Create the Enhanced Input Action node using editor's spawn API
	UK2Node_EnhancedInputAction* ActionNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_EnhancedInputAction>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[InputActionAsset](UK2Node_EnhancedInputAction* Node) { Node->InputAction = InputActionAsset; }
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(ActionNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), ActionNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintCustomEventAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString EventName;
	if (!GetRequiredString(Params, TEXT("event_name"), EventName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintCustomEventAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString EventName = Params->GetStringField(TEXT("event_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = GetTargetGraph(Params, Context);

	// Create Custom Event node using editor's spawn API
	UK2Node_CustomEvent* CustomEventNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CustomEvent>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[&EventName](UK2Node_CustomEvent* Node) { Node->CustomFunctionName = FName(*EventName); }
	);

	// Add parameters if provided
	const TArray<TSharedPtr<FJsonValue>>* ParametersArray = GetOptionalArray(Params, TEXT("parameters"));
	if (ParametersArray)
	{
		for (const TSharedPtr<FJsonValue>& ParamValue : *ParametersArray)
		{
			const TSharedPtr<FJsonObject>* ParamObj;
			if (ParamValue->TryGetObject(ParamObj) && ParamObj)
			{
				FString ParamName, ParamType;
				if ((*ParamObj)->TryGetStringField(TEXT("name"), ParamName) &&
					(*ParamObj)->TryGetStringField(TEXT("type"), ParamType))
				{
					FEdGraphPinType PinType;
					PinType.PinCategory = UEdGraphSchema_K2::PC_Float;

					if (ParamType.Equals(TEXT("Float"), ESearchCase::IgnoreCase) ||
						ParamType.Equals(TEXT("Double"), ESearchCase::IgnoreCase))
					{
						PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
						PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
					}
					else if (ParamType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase) ||
							 ParamType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
					{
						PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
					}
					else if (ParamType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) ||
							 ParamType.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
					{
						PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
					}
					else if (ParamType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
					{
						PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
						PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
					}
					else if (ParamType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
					{
						PinType.PinCategory = UEdGraphSchema_K2::PC_String;
					}

					TSharedPtr<FUserPinInfo> NewPinInfo = MakeShared<FUserPinInfo>();
					NewPinInfo->PinName = FName(*ParamName);
					NewPinInfo->PinType = PinType;
					NewPinInfo->DesiredPinDirection = EGPD_Output;
					CustomEventNode->UserDefinedPins.Add(NewPinInfo);
				}
			}
		}
		CustomEventNode->ReconstructNode();
	}

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(CustomEventNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), CustomEventNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("event_name"), EventName);
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Variable Nodes
// ============================================================================

bool FAddBlueprintVariableAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString VariableName, VariableType;
	if (!GetRequiredString(Params, TEXT("variable_name"), VariableName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("variable_type"), VariableType, OutError)) return false;
	return ValidateBlueprint(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintVariableAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString VariableName = Params->GetStringField(TEXT("variable_name"));
	FString VariableType = Params->GetStringField(TEXT("variable_type"));
	bool IsExposed = GetOptionalBool(Params, TEXT("is_exposed"), false);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);

	// Create variable based on type
	FEdGraphPinType PinType;
	if (VariableType == TEXT("Boolean"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (VariableType == TEXT("Float"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
	}
	else if (VariableType == TEXT("String"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (VariableType == TEXT("Vector"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (VariableType == TEXT("Vector2D"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
	}
	else if (VariableType == TEXT("EventDispatcher") || VariableType == TEXT("MulticastDelegate"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
	}
	else
	{
		return CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
	}

	FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

	// Set variable properties
	for (FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		if (Variable.VarName == FName(*VariableName))
		{
			if (IsExposed)
			{
				Variable.PropertyFlags |= CPF_Edit;
			}
			break;
		}
	}

	MarkBlueprintModified(Blueprint, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("variable_name"), VariableName);
	ResultData->SetStringField(TEXT("variable_type"), VariableType);
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintVariableGetAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString VariableName;
	if (!GetRequiredString(Params, TEXT("variable_name"), VariableName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintVariableGetAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString VariableName = Params->GetStringField(TEXT("variable_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	UK2Node_VariableGet* VarGetNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_VariableGet>(
		TargetGraph, Position, EK2NewNodeFlags::None,
		[&VariableName](UK2Node_VariableGet* Node) { Node->VariableReference.SetSelfMember(FName(*VariableName)); }
	);
	VarGetNode->ReconstructNode();

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(VarGetNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), VarGetNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintVariableSetAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString VariableName;
	if (!GetRequiredString(Params, TEXT("variable_name"), VariableName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintVariableSetAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString VariableName = Params->GetStringField(TEXT("variable_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	UK2Node_VariableSet* VarSetNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_VariableSet>(
		TargetGraph, Position, EK2NewNodeFlags::None,
		[&VariableName](UK2Node_VariableSet* Node) { Node->VariableReference.SetSelfMember(FName(*VariableName)); }
	);
	VarSetNode->ReconstructNode();

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(VarSetNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), VarSetNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FSetNodePinDefaultAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString NodeId, PinName, DefaultValue;
	if (!GetRequiredString(Params, TEXT("node_id"), NodeId, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("pin_name"), PinName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("default_value"), DefaultValue, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FSetNodePinDefaultAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString NodeId = Params->GetStringField(TEXT("node_id"));
	FString PinName = Params->GetStringField(TEXT("pin_name"));
	FString DefaultValue = Params->GetStringField(TEXT("default_value"));

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the node
	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (Node->NodeGuid.ToString() == NodeId)
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// Find the pin
	UEdGraphPin* TargetPin = FMCPCommonUtils::FindPin(TargetNode, PinName, EGPD_Input);
	if (!TargetPin)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
	}

	// Set the default value - handle object pins differently
	if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
		TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
		TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
		TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
	{
		UObject* LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *DefaultValue);
		if (LoadedObject)
		{
			TargetPin->DefaultObject = LoadedObject;
			TargetPin->DefaultValue.Empty();
		}
		else
		{
			return CreateErrorResponse(FString::Printf(TEXT("Failed to load object: %s"), *DefaultValue));
		}
	}
	else
	{
		TargetPin->DefaultValue = DefaultValue;
	}

	MarkBlueprintModified(Blueprint, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("pin_name"), PinName);
	ResultData->SetStringField(TEXT("default_value"), DefaultValue);
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Function Nodes
// ============================================================================

bool FAddBlueprintFunctionNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Target, FunctionName;
	if (!GetRequiredString(Params, TEXT("target"), Target, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("function_name"), FunctionName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintFunctionNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Target = Params->GetStringField(TEXT("target"));
	FString FunctionName = Params->GetStringField(TEXT("function_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the function
	UFunction* Function = nullptr;
	UClass* TargetClass = nullptr;

	// Direct mapping for known library classes
	FString TargetLower = Target.ToLower();
	if (TargetLower.Contains(TEXT("kismetmathlibrary")) || TargetLower.Contains(TEXT("math")))
	{
		TargetClass = UKismetMathLibrary::StaticClass();
	}
	else if (TargetLower.Contains(TEXT("kismetsystemlibrary")) || TargetLower.Contains(TEXT("systemlibrary")))
	{
		TargetClass = UKismetSystemLibrary::StaticClass();
	}
	else if (TargetLower.Contains(TEXT("gameplaystatics")))
	{
		TargetClass = UGameplayStatics::StaticClass();
	}
	else if (TargetLower.Contains(TEXT("enhancedinputlocalplayersubsystem")) || TargetLower.Contains(TEXT("inputsubsystem")))
	{
		TargetClass = UEnhancedInputLocalPlayerSubsystem::StaticClass();
	}

	// If not a known class, try module paths
	if (!TargetClass)
	{
		TArray<FString> CandidateNames;
		CandidateNames.Add(Target);
		if (!Target.StartsWith(TEXT("U")))
		{
			CandidateNames.Add(TEXT("U") + Target);
		}

		static const FString ModulePaths[] = {
			TEXT("/Script/Engine"),
			TEXT("/Script/CoreUObject"),
			TEXT("/Script/UMG"),
		};

		for (const FString& Candidate : CandidateNames)
		{
			if (TargetClass) break;
			for (const FString& ModulePath : ModulePaths)
			{
				FString FullPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *Candidate);
				TargetClass = LoadClass<UObject>(nullptr, *FullPath);
				if (TargetClass) break;
			}
		}
	}

	// Look for function in target class
	if (TargetClass)
	{
		Function = TargetClass->FindFunctionByName(*FunctionName);
		// Try case-insensitive match
		if (!Function)
		{
			for (TFieldIterator<UFunction> FuncIt(TargetClass); FuncIt; ++FuncIt)
			{
				if ((*FuncIt)->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
				{
					Function = *FuncIt;
					break;
				}
			}
		}
	}

	// Fallback to blueprint class
	if (!Function)
	{
		Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
	}

	if (!Function)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, *Target));
	}

	UK2Node_CallFunction* FunctionNode = FMCPCommonUtils::CreateFunctionCallNode(TargetGraph, Function, Position);
	if (!FunctionNode)
	{
		return CreateErrorResponse(TEXT("Failed to create function call node"));
	}

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(FunctionNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintSelfReferenceAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintSelfReferenceAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = FMCPCommonUtils::FindOrCreateEventGraph(Blueprint);

	UK2Node_Self* SelfNode = FMCPCommonUtils::CreateSelfReferenceNode(EventGraph, Position);
	if (!SelfNode)
	{
		return CreateErrorResponse(TEXT("Failed to create self node"));
	}

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(SelfNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintGetSelfComponentReferenceAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString ComponentName;
	if (!GetRequiredString(Params, TEXT("component_name"), ComponentName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintGetSelfComponentReferenceAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ComponentName = Params->GetStringField(TEXT("component_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = FMCPCommonUtils::FindOrCreateEventGraph(Blueprint);

	UK2Node_VariableGet* GetComponentNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_VariableGet>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[&ComponentName](UK2Node_VariableGet* Node) { Node->VariableReference.SetSelfMember(FName(*ComponentName)); }
	);
	GetComponentNode->ReconstructNode();

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(GetComponentNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintBranchNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintBranchNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	UK2Node_IfThenElse* BranchNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_IfThenElse>(
		TargetGraph, Position, EK2NewNodeFlags::None
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(BranchNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), BranchNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FAddBlueprintCastNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString TargetClass;
	if (!GetRequiredString(Params, TEXT("target_class"), TargetClass, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintCastNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString TargetClassName = Params->GetStringField(TEXT("target_class"));
	bool bPureCast = GetOptionalBool(Params, TEXT("pure_cast"), false);
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = FMCPCommonUtils::FindOrCreateEventGraph(Blueprint);

	// Find the target class
	UClass* TargetClass = nullptr;

	// Check if it's a content path
	if (TargetClassName.StartsWith(TEXT("/Game/")))
	{
		FString BPPath = TargetClassName;
		if (!BPPath.EndsWith(TEXT("_C")))
		{
			BPPath += TEXT("_C");
		}
		TargetClass = LoadClass<UObject>(nullptr, *BPPath);
		if (!TargetClass)
		{
			TargetClass = LoadClass<UObject>(nullptr, *TargetClassName);
		}
	}

	// Try to find as a blueprint name
	if (!TargetClass)
	{
		UBlueprint* TargetBP = FMCPCommonUtils::FindBlueprint(TargetClassName);
		if (TargetBP && TargetBP->GeneratedClass)
		{
			TargetClass = TargetBP->GeneratedClass;
		}
	}

	// Try engine classes
	if (!TargetClass)
	{
		static const FString ModulePaths[] = {
			TEXT("/Script/Engine"),
			TEXT("/Script/CoreUObject"),
		};
		for (const FString& ModulePath : ModulePaths)
		{
			FString FullPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *TargetClassName);
			TargetClass = LoadClass<UObject>(nullptr, *FullPath);
			if (TargetClass) break;
		}
	}

	if (!TargetClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target class not found: %s"), *TargetClassName));
	}

	UK2Node_DynamicCast* CastNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_DynamicCast>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[TargetClass, bPureCast](UK2Node_DynamicCast* Node) {
			Node->TargetType = TargetClass;
			Node->SetPurity(bPureCast);
		}
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(CastNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), CastNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("target_class"), TargetClass->GetName());
	ResultData->SetBoolField(TEXT("pure_cast"), bPureCast);
	return CreateSuccessResponse(ResultData);
}


// =============================================================================
// Subsystem Nodes
// =============================================================================

bool FAddBlueprintGetSubsystemNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString SubsystemClass;
	if (!GetRequiredString(Params, TEXT("subsystem_class"), SubsystemClass, OutError)) return false;
	return ValidateBlueprint(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddBlueprintGetSubsystemNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString SubsystemClassName = Params->GetStringField(TEXT("subsystem_class"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = FMCPCommonUtils::FindOrCreateEventGraph(Blueprint);

	// Find the subsystem class
	UClass* FoundClass = nullptr;

	// Try to load the class directly (for paths like /Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem)
	if (SubsystemClassName.StartsWith(TEXT("/Script/")))
	{
		FoundClass = LoadClass<USubsystem>(nullptr, *SubsystemClassName);
	}
	else
	{
		// Try common subsystem class names
		static const FString ModulePaths[] = {
			TEXT("/Script/EnhancedInput"),
			TEXT("/Script/Engine"),
			TEXT("/Script/GameplayAbilities"),
		};

		for (const FString& ModulePath : ModulePaths)
		{
			FString FullPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *SubsystemClassName);
			FoundClass = LoadClass<USubsystem>(nullptr, *FullPath);
			if (FoundClass) break;
		}
	}

	if (!FoundClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Subsystem class not found: %s. Try full path like /Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem"), *SubsystemClassName));
	}

	// Create the K2Node_GetSubsystemFromPC node (gets subsystem from PlayerController)
	UK2Node_GetSubsystemFromPC* SubsystemNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_GetSubsystemFromPC>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[FoundClass](UK2Node_GetSubsystemFromPC* Node) { Node->Initialize(FoundClass); }
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(SubsystemNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), SubsystemNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("subsystem_class"), FoundClass->GetName());
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Blueprint Function Graph
// ============================================================================

bool FCreateBlueprintFunctionAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString FunctionName;
	if (!GetRequiredString(Params, TEXT("function_name"), FunctionName, OutError)) return false;
	return ValidateBlueprint(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FCreateBlueprintFunctionAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString FunctionName = Params->GetStringField(TEXT("function_name"));
	bool bIsPure = GetOptionalBool(Params, TEXT("is_pure"), false);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);

	// Check if function already exists
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetFName() == FName(*FunctionName))
		{
			// Find entry node
			FString EntryNodeId;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node);
				if (EntryNode)
				{
					EntryNodeId = EntryNode->NodeGuid.ToString();
					break;
				}
			}

			TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
			ResultData->SetBoolField(TEXT("already_exists"), true);
			ResultData->SetStringField(TEXT("function_name"), FunctionName);
			ResultData->SetStringField(TEXT("graph_name"), Graph->GetName());
			ResultData->SetStringField(TEXT("entry_node_id"), EntryNodeId);
			return CreateSuccessResponse(ResultData);
		}
	}

	// Create the function graph
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint, FName(*FunctionName),
		UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());

	if (!NewGraph)
	{
		return CreateErrorResponse(TEXT("Failed to create function graph"));
	}

	FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, NewGraph, true, nullptr);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	K2Schema->CreateDefaultNodesForGraph(*NewGraph);

	// Find entry and result nodes
	UK2Node_FunctionEntry* EntryNode = nullptr;
	UK2Node_FunctionResult* ResultNode = nullptr;
	for (UEdGraphNode* Node : NewGraph->Nodes)
	{
		if (!EntryNode) EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		if (!ResultNode) ResultNode = Cast<UK2Node_FunctionResult>(Node);
	}

	// Add input parameters
	const TArray<TSharedPtr<FJsonValue>>* InputsArray = GetOptionalArray(Params, TEXT("inputs"));
	if (InputsArray && EntryNode)
	{
		for (const TSharedPtr<FJsonValue>& InputValue : *InputsArray)
		{
			const TSharedPtr<FJsonObject>& InputObj = InputValue->AsObject();
			if (!InputObj) continue;

			FString ParamName, ParamType;
			if (!InputObj->TryGetStringField(TEXT("name"), ParamName)) continue;
			if (!InputObj->TryGetStringField(TEXT("type"), ParamType)) continue;

			FEdGraphPinType PinType;
			if (ParamType.Equals(TEXT("Float"), ESearchCase::IgnoreCase) ||
				ParamType.Equals(TEXT("Double"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
				PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			}
			else if (ParamType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			}
			else if (ParamType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
			}
			else if (ParamType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			}
			else if (ParamType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_String;
			}
			else
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
				PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			}

			EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
		}
		EntryNode->ReconstructNode();
	}

	// Add output parameters
	const TArray<TSharedPtr<FJsonValue>>* OutputsArray = GetOptionalArray(Params, TEXT("outputs"));
	if (OutputsArray)
	{
		if (!ResultNode)
		{
			ResultNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_FunctionResult>(
				NewGraph, FVector2D(400, 0), EK2NewNodeFlags::None
			);
		}

		for (const TSharedPtr<FJsonValue>& OutputValue : *OutputsArray)
		{
			const TSharedPtr<FJsonObject>& OutputObj = OutputValue->AsObject();
			if (!OutputObj) continue;

			FString ParamName, ParamType;
			if (!OutputObj->TryGetStringField(TEXT("name"), ParamName)) continue;
			if (!OutputObj->TryGetStringField(TEXT("type"), ParamType)) continue;

			FEdGraphPinType PinType;
			if (ParamType.Equals(TEXT("Float"), ESearchCase::IgnoreCase) ||
				ParamType.Equals(TEXT("Double"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
				PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			}
			else if (ParamType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			}
			else if (ParamType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
			}
			else if (ParamType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			}
			else if (ParamType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_String;
			}
			else
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
				PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			}

			ResultNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Input);
		}
		ResultNode->ReconstructNode();
	}

	if (bIsPure && EntryNode)
	{
		K2Schema->AddExtraFunctionFlags(NewGraph, FUNC_BlueprintPure);
	}

	MarkBlueprintModified(Blueprint, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("function_name"), FunctionName);
	ResultData->SetStringField(TEXT("graph_name"), NewGraph->GetName());
	if (EntryNode) ResultData->SetStringField(TEXT("entry_node_id"), EntryNode->NodeGuid.ToString());
	if (ResultNode) ResultData->SetStringField(TEXT("result_node_id"), ResultNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Event Dispatchers
// ============================================================================

bool FAddEventDispatcherAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString DispatcherName;
	if (!GetRequiredString(Params, TEXT("dispatcher_name"), DispatcherName, OutError)) return false;
	return ValidateBlueprint(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddEventDispatcherAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString DispatcherName = Params->GetStringField(TEXT("dispatcher_name"));

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);

	// Add the delegate variable
	FEdGraphPinType DelegateType;
	DelegateType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
	FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*DispatcherName), DelegateType);

	// Create the delegate signature graph
	FName GraphName = FName(*DispatcherName);
	UEdGraph* SignatureGraph = nullptr;

	for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs)
	{
		if (Graph->GetFName() == GraphName)
		{
			SignatureGraph = Graph;
			break;
		}
	}

	if (!SignatureGraph)
	{
		SignatureGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, GraphName,
			UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());

		if (!SignatureGraph)
		{
			return CreateErrorResponse(TEXT("Failed to create delegate signature graph"));
		}

		SignatureGraph->bEditable = false;

		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		K2Schema->CreateDefaultNodesForGraph(*SignatureGraph);
		K2Schema->CreateFunctionGraphTerminators(*SignatureGraph, (UClass*)nullptr);
		K2Schema->AddExtraFunctionFlags(SignatureGraph, (FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public));
		K2Schema->MarkFunctionEntryAsEditable(SignatureGraph, true);

		Blueprint->DelegateSignatureGraphs.Add(SignatureGraph);
	}

	// Find entry node and add parameters
	UK2Node_FunctionEntry* EntryNode = nullptr;
	for (UEdGraphNode* Node : SignatureGraph->Nodes)
	{
		EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		if (EntryNode) break;
	}

	const TArray<TSharedPtr<FJsonValue>>* ParamsArray = GetOptionalArray(Params, TEXT("parameters"));
	if (EntryNode && ParamsArray)
	{
		for (const TSharedPtr<FJsonValue>& ParamValue : *ParamsArray)
		{
			const TSharedPtr<FJsonObject>& ParamObj = ParamValue->AsObject();
			if (!ParamObj) continue;

			FString ParamName, ParamType;
			if (!ParamObj->TryGetStringField(TEXT("name"), ParamName)) continue;
			if (!ParamObj->TryGetStringField(TEXT("type"), ParamType)) continue;

			FEdGraphPinType PinType;
			if (ParamType == TEXT("Float"))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
			}
			else if (ParamType == TEXT("Boolean"))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			}
			else if (ParamType == TEXT("Integer") || ParamType == TEXT("Int"))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
			}
			else if (ParamType == TEXT("Vector"))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			}
			else
			{
				continue;
			}

			EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
		}
		EntryNode->ReconstructNode();
	}

	MarkBlueprintModified(Blueprint, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("dispatcher_name"), DispatcherName);
	return CreateSuccessResponse(ResultData);
}


bool FCallEventDispatcherAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString DispatcherName;
	if (!GetRequiredString(Params, TEXT("dispatcher_name"), DispatcherName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FCallEventDispatcherAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString DispatcherName = Params->GetStringField(TEXT("dispatcher_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* EventGraph = GetTargetGraph(Params, Context);

	// Find the delegate property
	FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
		Blueprint->GeneratedClass, FName(*DispatcherName));
	if (!DelegateProp)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Delegate property '%s' not found. Compile the blueprint first."), *DispatcherName));
	}

	// Create CallDelegate node
	UClass* GenClass = Blueprint->GeneratedClass;
	UK2Node_CallDelegate* CallNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CallDelegate>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[DelegateProp, GenClass](UK2Node_CallDelegate* Node) { Node->SetFromProperty(DelegateProp, false, GenClass); }
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(CallNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), CallNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}


bool FBindEventDispatcherAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString DispatcherName;
	if (!GetRequiredString(Params, TEXT("dispatcher_name"), DispatcherName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FBindEventDispatcherAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString DispatcherName = Params->GetStringField(TEXT("dispatcher_name"));
	FString TargetBlueprintName = GetOptionalString(Params, TEXT("target_blueprint"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);

	// Get target blueprint (defaults to self)
	UBlueprint* TargetBlueprint = Blueprint;
	if (!TargetBlueprintName.IsEmpty())
	{
		TargetBlueprint = FMCPCommonUtils::FindBlueprint(TargetBlueprintName);
		if (!TargetBlueprint)
		{
			return CreateErrorResponse(FString::Printf(TEXT("Target blueprint not found: %s"), *TargetBlueprintName));
		}
	}

	UEdGraph* EventGraph = GetTargetGraph(Params, Context);

	// Find the delegate property
	FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
		TargetBlueprint->GeneratedClass, FName(*DispatcherName));
	if (!DelegateProp)
	{
		TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
		ResultData->SetStringField(TEXT("message"), TEXT("Dispatcher not found. Compile the target blueprint first."));
		return CreateErrorResponse(TEXT("Dispatcher not found in compiled class. Compile the target blueprint first."));
	}

	UFunction* SignatureFunc = DelegateProp->SignatureFunction;

	// Create UK2Node_AddDelegate
	UClass* TargetGenClass = TargetBlueprint->GeneratedClass;
	UK2Node_AddDelegate* BindNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_AddDelegate>(
		EventGraph, Position, EK2NewNodeFlags::None,
		[DelegateProp, TargetGenClass](UK2Node_AddDelegate* Node) { Node->SetFromProperty(DelegateProp, false, TargetGenClass); }
	);

	// Create matching Custom Event
	FString EventName = FString::Printf(TEXT("On%s"), *DispatcherName);
	FVector2D EventPosition(Position.X + 300, Position.Y);
	UK2Node_CustomEvent* CustomEventNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CustomEvent>(
		EventGraph, EventPosition, EK2NewNodeFlags::None,
		[&EventName](UK2Node_CustomEvent* Node) { Node->CustomFunctionName = FName(*EventName); }
	);

	// Add parameters matching dispatcher signature
	if (SignatureFunc)
	{
		for (TFieldIterator<FProperty> PropIt(SignatureFunc); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			FProperty* Param = *PropIt;
			if (!(Param->PropertyFlags & CPF_ReturnParm))
			{
				FEdGraphPinType PinType;
				const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
				if (K2Schema->ConvertPropertyToPinType(Param, PinType))
				{
					TSharedPtr<FUserPinInfo> NewPinInfo = MakeShared<FUserPinInfo>();
					NewPinInfo->PinName = Param->GetFName();
					NewPinInfo->PinType = PinType;
					NewPinInfo->DesiredPinDirection = EGPD_Output;
					CustomEventNode->UserDefinedPins.Add(NewPinInfo);
				}
			}
		}
		CustomEventNode->ReconstructNode();
	}

	// Connect custom event to bind node
	UEdGraphPin* EventDelegatePin = nullptr;
	for (UEdGraphPin* Pin : CustomEventNode->Pins)
	{
		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Delegate && Pin->Direction == EGPD_Output)
		{
			EventDelegatePin = Pin;
			break;
		}
	}

	UEdGraphPin* BindDelegatePin = BindNode->GetDelegatePin();
	if (EventDelegatePin && BindDelegatePin)
	{
		EventDelegatePin->MakeLinkTo(BindDelegatePin);
	}

	MarkBlueprintModified(Blueprint, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("bind_node_id"), BindNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("event_node_id"), CustomEventNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("event_name"), EventName);
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Spawn Actor Nodes
// ============================================================================

bool FAddSpawnActorFromClassNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString ClassToSpawn;
	if (!GetRequiredString(Params, TEXT("class_to_spawn"), ClassToSpawn, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddSpawnActorFromClassNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ClassToSpawn = Params->GetStringField(TEXT("class_to_spawn"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the class to spawn
	UClass* SpawnClass = nullptr;

	// First, try to find as a blueprint
	UBlueprint* SpawnBP = FMCPCommonUtils::FindBlueprint(ClassToSpawn);
	if (SpawnBP && SpawnBP->GeneratedClass)
	{
		SpawnClass = SpawnBP->GeneratedClass;
	}

	// Try content path
	if (!SpawnClass && ClassToSpawn.StartsWith(TEXT("/Game/")))
	{
		FString BPPath = ClassToSpawn;
		if (!BPPath.EndsWith(TEXT("_C")))
		{
			BPPath += TEXT("_C");
		}
		SpawnClass = LoadClass<AActor>(nullptr, *BPPath);
		if (!SpawnClass)
		{
			SpawnClass = LoadClass<AActor>(nullptr, *ClassToSpawn);
		}
	}

	// Try engine classes
	if (!SpawnClass)
	{
		static const FString ModulePaths[] = {
			TEXT("/Script/Engine"),
			TEXT("/Script/CoreUObject")
		};
		for (const FString& ModulePath : ModulePaths)
		{
			FString FullPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *ClassToSpawn);
			SpawnClass = LoadClass<AActor>(nullptr, *FullPath);
			if (SpawnClass) break;
		}
	}

	if (!SpawnClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Class to spawn not found: %s"), *ClassToSpawn));
	}

	// Pre-allocate pins in lambda so PostPlacedNewNode() doesn't crash.
	// UE's SpawnNode calls PostPlacedNewNode BEFORE AllocateDefaultPins, but
	// UK2Node_SpawnActorFromClass::PostPlacedNewNode uses FindPinChecked which
	// crashes if pins don't exist. This is an Epic bug we work around here.
	UK2Node_SpawnActorFromClass* SpawnNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_SpawnActorFromClass>(
		TargetGraph, Position, EK2NewNodeFlags::None,
		[](UK2Node_SpawnActorFromClass* Node)
		{
			Node->AllocateDefaultPins();
		}
	);

	// Set the class to spawn via the class pin
	UEdGraphPin* ClassPin = SpawnNode->GetClassPin();
	if (ClassPin)
	{
		const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(TargetGraph->GetSchema());
		if (K2Schema)
		{
			K2Schema->TrySetDefaultObject(*ClassPin, SpawnClass);
		}
	}
	SpawnNode->ReconstructNode();

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(SpawnNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), SpawnNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("class_to_spawn"), SpawnClass->GetName());
	return CreateSuccessResponse(ResultData);
}


bool FCallBlueprintFunctionAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString TargetBlueprint, FunctionName;
	if (!GetRequiredString(Params, TEXT("target_blueprint"), TargetBlueprint, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("function_name"), FunctionName, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FCallBlueprintFunctionAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString TargetBlueprintName = Params->GetStringField(TEXT("target_blueprint"));
	FString FunctionName = Params->GetStringField(TEXT("function_name"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the target blueprint
	UBlueprint* TargetBlueprint = FMCPCommonUtils::FindBlueprint(TargetBlueprintName);
	if (!TargetBlueprint)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target blueprint not found: %s"), *TargetBlueprintName));
	}

	// Ensure target is compiled
	if (!TargetBlueprint->GeneratedClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target blueprint not compiled: %s"), *TargetBlueprintName));
	}

	// Find the function
	UFunction* Function = TargetBlueprint->GeneratedClass->FindFunctionByName(*FunctionName);
	if (!Function)
	{
		// Check if graph exists
		bool bFoundGraph = false;
		for (UEdGraph* Graph : TargetBlueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetFName() == FName(*FunctionName))
			{
				bFoundGraph = true;
				break;
			}
		}

		if (bFoundGraph)
		{
			return CreateErrorResponse(FString::Printf(
				TEXT("Function '%s' exists but is not compiled. Compile '%s' first."),
				*FunctionName, *TargetBlueprintName));
		}
		else
		{
			// List available functions
			TArray<FString> AvailableFunctions;
			for (TFieldIterator<UFunction> FuncIt(TargetBlueprint->GeneratedClass); FuncIt; ++FuncIt)
			{
				if ((*FuncIt)->HasAnyFunctionFlags(FUNC_BlueprintCallable))
				{
					AvailableFunctions.Add((*FuncIt)->GetName());
				}
			}
			FString AvailableStr = FString::Join(AvailableFunctions, TEXT(", "));
			return CreateErrorResponse(FString::Printf(
				TEXT("Function '%s' not found in '%s'. Available: %s"),
				*FunctionName, *TargetBlueprintName, *AvailableStr));
		}
	}

	UClass* TargetGenClass = TargetBlueprint->GeneratedClass;
	UK2Node_CallFunction* FunctionNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CallFunction>(
		TargetGraph, Position, EK2NewNodeFlags::None,
		[Function, TargetGenClass](UK2Node_CallFunction* Node) {
			Node->FunctionReference.SetExternalMember(Function->GetFName(), TargetGenClass);
		}
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(FunctionNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("function_name"), FunctionName);
	ResultData->SetStringField(TEXT("target_blueprint"), TargetBlueprintName);
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// External Object Property Nodes
// ============================================================================

bool FSetObjectPropertyAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString PropertyName, OwnerClass;
	if (!GetRequiredString(Params, TEXT("property_name"), PropertyName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("owner_class"), OwnerClass, OutError)) return false;
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FSetObjectPropertyAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	FString OwnerClassName = Params->GetStringField(TEXT("owner_class"));
	FVector2D Position = GetNodePosition(Params);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	// Find the owner class - try /Script/Engine first (most common), then blueprint
	UClass* OwnerClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *OwnerClassName));
	if (!OwnerClass)
	{
		UBlueprint* OwnerBP = FMCPCommonUtils::FindBlueprint(OwnerClassName);
		if (OwnerBP) OwnerClass = OwnerBP->GeneratedClass;
	}

	if (!OwnerClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *OwnerClassName));
	}

	// Verify property exists
	FProperty* Property = OwnerClass->FindPropertyByName(FName(*PropertyName));
	if (!Property)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Property '%s' not found on '%s'"), *PropertyName, *OwnerClassName));
	}

	// Create Set node with external member reference
	UK2Node_VariableSet* VarSetNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_VariableSet>(
		TargetGraph, Position, EK2NewNodeFlags::None,
		[&PropertyName, OwnerClass](UK2Node_VariableSet* Node) {
			Node->VariableReference.SetExternalMember(FName(*PropertyName), OwnerClass);
		}
	);
	VarSetNode->ReconstructNode();

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(VarSetNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), VarSetNode->NodeGuid.ToString());
	ResultData->SetStringField(TEXT("property_name"), PropertyName);
	ResultData->SetStringField(TEXT("owner_class"), OwnerClass->GetName());
	return CreateSuccessResponse(ResultData);
}


// ============================================================================
// Macro Instance Nodes
// ============================================================================

UEdGraph* FAddMacroInstanceNodeAction::FindMacroGraph(const FString& MacroName) const
{
	UBlueprint* MacroBP = LoadObject<UBlueprint>(nullptr,
		TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
	if (!MacroBP) return nullptr;

	for (UEdGraph* Graph : MacroBP->MacroGraphs)
	{
		if (Graph && Graph->GetFName().ToString().Equals(MacroName, ESearchCase::IgnoreCase))
			return Graph;
	}
	return nullptr;
}

bool FAddMacroInstanceNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString MacroName;
	if (!GetRequiredString(Params, TEXT("macro_name"), MacroName, OutError)) return false;
	if (!FindMacroGraph(MacroName))
	{
		OutError = FString::Printf(TEXT("Macro '%s' not found in StandardMacros"), *MacroName);
		return false;
	}
	return ValidateGraph(Params, Context, OutError);
}

TSharedPtr<FJsonObject> FAddMacroInstanceNodeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString MacroName = Params->GetStringField(TEXT("macro_name"));
	UEdGraph* MacroGraph = FindMacroGraph(MacroName);

	UBlueprint* Blueprint = GetTargetBlueprint(Params, Context);
	UEdGraph* TargetGraph = GetTargetGraph(Params, Context);

	UK2Node_MacroInstance* MacroNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_MacroInstance>(
		TargetGraph, GetNodePosition(Params), EK2NewNodeFlags::None,
		[MacroGraph](UK2Node_MacroInstance* Node) { Node->SetMacroGraph(MacroGraph); }
	);

	MarkBlueprintModified(Blueprint, Context);
	RegisterCreatedNode(MacroNode, Context);

	TSharedPtr<FJsonObject> ResultData = MakeShared<FJsonObject>();
	ResultData->SetStringField(TEXT("node_id"), MacroNode->NodeGuid.ToString());
	return CreateSuccessResponse(ResultData);
}
