// Copyright (c) 2025 zolnoor. All rights reserved.

#include "Actions/UMGActions.h"
#include "MCPCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/Button.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Kismet2/KismetEditorUtilities.h"

// Helper to find widget blueprint in common paths
static UWidgetBlueprint* FindWidgetBlueprintByName(const FString& BlueprintName)
{
	TArray<FString> SearchPaths = {
		FString::Printf(TEXT("/Game/UI/%s"), *BlueprintName),
		FString::Printf(TEXT("/Game/Widgets/%s"), *BlueprintName),
		FString::Printf(TEXT("/Game/%s"), *BlueprintName)
	};

	for (const FString& Path : SearchPaths)
	{
		if (UEditorAssetLibrary::DoesAssetExist(Path))
		{
			UWidgetBlueprint* Widget = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(Path));
			if (Widget)
			{
				return Widget;
			}
		}
	}
	return nullptr;
}

// =============================================================================
// FCreateUMGWidgetBlueprintAction
// =============================================================================

bool FCreateUMGWidgetBlueprintAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing 'widget_name' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FCreateUMGWidgetBlueprintAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString BlueprintName = Params->GetStringField(TEXT("widget_name"));

	// Get optional path parameter
	FString PackagePath = TEXT("/Game/UI/");
	FString PathParam;
	if (Params->TryGetStringField(TEXT("path"), PathParam))
	{
		PackagePath = PathParam;
		if (!PackagePath.EndsWith(TEXT("/")))
		{
			PackagePath += TEXT("/");
		}
	}

	FString AssetName = BlueprintName;
	FString FullPath = PackagePath + AssetName;

	// Aggressive cleanup: remove any existing widget blueprint
	TArray<FString> PathsToCheck = {
		FullPath,
		TEXT("/Game/Widgets/") + AssetName,
		TEXT("/Game/UI/") + AssetName
	};

	for (const FString& CheckPath : PathsToCheck)
	{
		// Delete from disk first
		if (UEditorAssetLibrary::DoesAssetExist(CheckPath))
		{
			UE_LOG(LogTemp, Log, TEXT("Widget Blueprint exists at '%s', deleting from disk"), *CheckPath);
			UEditorAssetLibrary::DeleteAsset(CheckPath);
		}

		// Clean up from memory
		UPackage* ExistingPackage = FindPackage(nullptr, *CheckPath);
		if (ExistingPackage)
		{
			UBlueprint* ExistingBP = FindObject<UBlueprint>(ExistingPackage, *AssetName);
			if (!ExistingBP)
			{
				ExistingBP = FindObject<UBlueprint>(ExistingPackage, nullptr);
			}

			if (ExistingBP)
			{
				UE_LOG(LogTemp, Log, TEXT("Widget Blueprint '%s' found in memory, cleaning up"), *AssetName);
				FString TempName = FString::Printf(TEXT("%s_OLD_%d"), *AssetName, FMath::Rand());
				ExistingBP->Rename(*TempName, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
				ExistingBP->ClearFlags(RF_Public | RF_Standalone);
				ExistingBP->MarkAsGarbage();
			}

			ExistingPackage->ClearFlags(RF_Public | RF_Standalone);
			ExistingPackage->MarkAsGarbage();
		}
	}

	// Force garbage collection
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	// Create package
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Double-check cleanup worked
	if (FindObject<UBlueprint>(Package, *AssetName))
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Failed to clean up existing Widget Blueprint '%s'. Try restarting the editor."), *AssetName));
	}

	// Create Widget Blueprint
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		UUserWidget::StaticClass(),
		Package,
		FName(*AssetName),
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		FName("CreateUMGWidget")
	);

	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewBlueprint);
	if (!WidgetBlueprint)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Blueprint"));
	}

	// Add default Canvas Panel
	if (!WidgetBlueprint->WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
	}

	// Register and compile
	FAssetRegistryModule::AssetCreated(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

	// Save immediately
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	UE_LOG(LogTemp, Log, TEXT("Widget Blueprint '%s' created at '%s'"), *BlueprintName, *FullPath);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), BlueprintName);
	ResultObj->SetStringField(TEXT("path"), FullPath);
	return ResultObj;
}

// =============================================================================
// FAddTextBlockToWidgetAction
// =============================================================================

bool FAddTextBlockToWidgetAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing 'widget_name' parameter");
		return false;
	}
	if (!Params->HasField(TEXT("text_block_name")))
	{
		OutError = TEXT("Missing 'text_block_name' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FAddTextBlockToWidgetAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString BlueprintName = Params->GetStringField(TEXT("widget_name"));
	FString WidgetName = Params->GetStringField(TEXT("text_block_name"));

	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprintByName(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Widget Blueprint '%s' not found in /Game/UI, /Game/Widgets, or /Game"), *BlueprintName));
	}

	// Optional parameters
	FString InitialText = TEXT("New Text Block");
	Params->TryGetStringField(TEXT("text"), InitialText);

	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
		{
			Position.X = (*PosArray)[0]->AsNumber();
			Position.Y = (*PosArray)[1]->AsNumber();
		}
	}

	// Create Text Block
	UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *WidgetName);
	if (!TextBlock)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Text Block widget"));
	}

	TextBlock->SetText(FText::FromString(InitialText));

	// Add to canvas
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Root Canvas Panel not found"));
	}

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(TextBlock);
	PanelSlot->SetPosition(Position);

	// Compile and save
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("text"), InitialText);
	return ResultObj;
}

// =============================================================================
// FAddButtonToWidgetAction
// =============================================================================

bool FAddButtonToWidgetAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing 'widget_name' parameter");
		return false;
	}
	if (!Params->HasField(TEXT("button_name")))
	{
		OutError = TEXT("Missing 'button_name' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FAddButtonToWidgetAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString BlueprintName = Params->GetStringField(TEXT("widget_name"));
	FString WidgetName = Params->GetStringField(TEXT("button_name"));

	FString ButtonText = TEXT("Button");
	Params->TryGetStringField(TEXT("text"), ButtonText);

	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprintByName(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Widget Blueprint '%s' not found in /Game/UI, /Game/Widgets, or /Game"), *BlueprintName));
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Root widget is not a Canvas Panel"));
	}

	// Create Button
	UButton* Button = WidgetBlueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *WidgetName);
	if (!Button)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Button widget"));
	}

	// Create text block for button label
	FString TextBlockName = WidgetName + TEXT("_Text");
	UTextBlock* ButtonTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *TextBlockName);
	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetText(FText::FromString(ButtonText));
		Button->AddChild(ButtonTextBlock);
	}

	// Add to canvas
	UCanvasPanelSlot* ButtonSlot = RootCanvas->AddChildToCanvas(Button);
	if (ButtonSlot)
	{
		const TArray<TSharedPtr<FJsonValue>>* Position;
		if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
		{
			FVector2D Pos((*Position)[0]->AsNumber(), (*Position)[1]->AsNumber());
			ButtonSlot->SetPosition(Pos);
		}

		const TArray<TSharedPtr<FJsonValue>>* Size;
		if (Params->TryGetArrayField(TEXT("size"), Size) && Size->Num() >= 2)
		{
			FVector2D SizeVec((*Size)[0]->AsNumber(), (*Size)[1]->AsNumber());
			ButtonSlot->SetSize(SizeVec);
			ButtonSlot->SetAutoSize(false);
		}
	}

	// Compile and save
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	return ResultObj;
}

// =============================================================================
// FBindWidgetEventAction
// =============================================================================

bool FBindWidgetEventAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing 'widget_name' parameter");
		return false;
	}
	if (!Params->HasField(TEXT("widget_component_name")))
	{
		OutError = TEXT("Missing 'widget_component_name' parameter");
		return false;
	}
	if (!Params->HasField(TEXT("event_name")))
	{
		OutError = TEXT("Missing 'event_name' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FBindWidgetEventAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString BlueprintName = Params->GetStringField(TEXT("widget_name"));
	FString WidgetComponentName = Params->GetStringField(TEXT("widget_component_name"));
	FString EventName = Params->GetStringField(TEXT("event_name"));

	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprintByName(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Find the widget in the WidgetTree
	UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(*WidgetComponentName);
	if (!Widget)
	{
		TArray<FString> AvailableWidgets;
		WidgetBlueprint->WidgetTree->ForEachWidget([&AvailableWidgets](UWidget* W) {
			if (W) AvailableWidgets.Add(W->GetName());
		});
		FString WidgetList = FString::Join(AvailableWidgets, TEXT(", "));
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Widget '%s' not found. Available: %s"), *WidgetComponentName, *WidgetList));
	}

	// Verify delegate exists
	FMulticastDelegateProperty* DelegateProp = nullptr;
	for (TFieldIterator<FMulticastDelegateProperty> It(Widget->GetClass()); It; ++It)
	{
		if (It->GetFName() == FName(*EventName))
		{
			DelegateProp = *It;
			break;
		}
	}

	if (!DelegateProp)
	{
		TArray<FString> AvailableDelegates;
		for (TFieldIterator<FMulticastDelegateProperty> It(Widget->GetClass()); It; ++It)
		{
			AvailableDelegates.Add(It->GetName());
		}
		FString DelegateList = FString::Join(AvailableDelegates, TEXT(", "));
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Delegate '%s' not found. Available: %s"), *EventName, *DelegateList));
	}

	// Get event graph
	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
	if (!EventGraph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find event graph"));
	}

	// Check if Component Bound Event node already exists for this widget/delegate combo
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		UK2Node_ComponentBoundEvent* ExistingEvent = Cast<UK2Node_ComponentBoundEvent>(Node);
		if (ExistingEvent &&
			ExistingEvent->ComponentPropertyName == FName(*WidgetComponentName) &&
			ExistingEvent->DelegatePropertyName == DelegateProp->GetFName())
		{
			// Already exists - return the existing node
			TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
			ResultObj->SetBoolField(TEXT("success"), true);
			ResultObj->SetBoolField(TEXT("already_exists"), true);
			ResultObj->SetStringField(TEXT("widget_name"), WidgetComponentName);
			ResultObj->SetStringField(TEXT("event_name"), EventName);
			ResultObj->SetStringField(TEXT("node_id"), ExistingEvent->NodeGuid.ToString());
			return ResultObj;
		}
	}

	// Calculate position for new node (below existing nodes)
	float MaxY = 0.0f;
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		MaxY = FMath::Max(MaxY, (float)Node->NodePosY);
	}

	// Create Component Bound Event node - this is the proper way to handle widget events
	UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(EventGraph);
	EventNode->ComponentPropertyName = FName(*WidgetComponentName);
	EventNode->DelegatePropertyName = DelegateProp->GetFName();
	EventNode->DelegateOwnerClass = Widget->GetClass();

	EventGraph->AddNode(EventNode, false, false);
	EventNode->CreateNewGuid();
	EventNode->NodePosX = 200;
	EventNode->NodePosY = (int32)(MaxY + 200);
	EventNode->AllocateDefaultPins();

	UE_LOG(LogTemp, Log, TEXT("Created Component Bound Event: %s.%s"), *WidgetComponentName, *EventName);

	// Compile and save
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetComponentName);
	ResultObj->SetStringField(TEXT("event_name"), EventName);
	ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
	return ResultObj;
}

// =============================================================================
// FAddWidgetToViewportAction
// =============================================================================

bool FAddWidgetToViewportAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing 'widget_name' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FAddWidgetToViewportAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString BlueprintName = Params->GetStringField(TEXT("widget_name"));

	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprintByName(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	int32 ZOrder = 0;
	Params->TryGetNumberField(TEXT("z_order"), ZOrder);

	UClass* WidgetClass = WidgetBlueprint->GeneratedClass;
	if (!WidgetClass)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get widget class"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("class_path"), WidgetClass->GetPathName());
	ResultObj->SetNumberField(TEXT("z_order"), ZOrder);
	ResultObj->SetStringField(TEXT("note"), TEXT("Widget class ready. Use CreateWidget and AddToViewport nodes in Blueprint."));
	return ResultObj;
}

// =============================================================================
// FSetTextBlockBindingAction
// =============================================================================

bool FSetTextBlockBindingAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing 'widget_name' parameter");
		return false;
	}
	if (!Params->HasField(TEXT("text_block_name")))
	{
		OutError = TEXT("Missing 'text_block_name' parameter");
		return false;
	}
	if (!Params->HasField(TEXT("binding_property")))
	{
		OutError = TEXT("Missing 'binding_property' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FSetTextBlockBindingAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString BlueprintName = Params->GetStringField(TEXT("widget_name"));
	FString WidgetName = Params->GetStringField(TEXT("text_block_name"));
	FString BindingName = Params->GetStringField(TEXT("binding_property"));

	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprintByName(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Create variable for binding
	FBlueprintEditorUtils::AddMemberVariable(
		WidgetBlueprint,
		FName(*BindingName),
		FEdGraphPinType(UEdGraphSchema_K2::PC_Text, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType())
	);

	// Find the TextBlock widget
	UTextBlock* TextBlock = Cast<UTextBlock>(WidgetBlueprint->WidgetTree->FindWidget(FName(*WidgetName)));
	if (!TextBlock)
	{
		return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("TextBlock '%s' not found"), *WidgetName));
	}

	// Create binding function
	const FString FunctionName = FString::Printf(TEXT("Get%s"), *BindingName);
	UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
		WidgetBlueprint,
		FName(*FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (FuncGraph)
	{
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, FuncGraph, false, nullptr);

		// Create entry node
		UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
		FuncGraph->AddNode(EntryNode, false, false);
		EntryNode->NodePosX = 0;
		EntryNode->NodePosY = 0;
		EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBlueprint->GeneratedClass);
		EntryNode->AllocateDefaultPins();

		// Create get variable node
		UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
		GetVarNode->VariableReference.SetSelfMember(FName(*BindingName));
		FuncGraph->AddNode(GetVarNode, false, false);
		GetVarNode->NodePosX = 200;
		GetVarNode->NodePosY = 0;
		GetVarNode->AllocateDefaultPins();

		// Connect nodes
		UEdGraphPin* EntryThenPin = EntryNode->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* GetVarOutPin = GetVarNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
		if (EntryThenPin && GetVarOutPin)
		{
			EntryThenPin->MakeLinkTo(GetVarOutPin);
		}
	}

	// Compile
	WidgetBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("binding_name"), BindingName);
	return ResultObj;
}
