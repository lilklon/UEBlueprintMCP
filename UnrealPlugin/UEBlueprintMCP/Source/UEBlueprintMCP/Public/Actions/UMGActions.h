// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actions/EditorAction.h"

/**
 * Create a UMG Widget Blueprint
 */
class UEBLUEPRINTMCP_API FCreateUMGWidgetBlueprintAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override { return TEXT("CreateUMGWidgetBlueprint"); }

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

/**
 * Add a Text Block to a Widget Blueprint
 */
class UEBLUEPRINTMCP_API FAddTextBlockToWidgetAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override { return TEXT("AddTextBlockToWidget"); }

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

/**
 * Add a Button to a Widget Blueprint
 */
class UEBLUEPRINTMCP_API FAddButtonToWidgetAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override { return TEXT("AddButtonToWidget"); }

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

/**
 * Bind a widget event to a function
 */
class UEBLUEPRINTMCP_API FBindWidgetEventAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override { return TEXT("BindWidgetEvent"); }

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

/**
 * Add widget to viewport (returns class path for Blueprint use)
 */
class UEBLUEPRINTMCP_API FAddWidgetToViewportAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override { return TEXT("AddWidgetToViewport"); }

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

/**
 * Set up text block binding to a variable
 */
class UEBLUEPRINTMCP_API FSetTextBlockBindingAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override { return TEXT("SetTextBlockBinding"); }

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};
