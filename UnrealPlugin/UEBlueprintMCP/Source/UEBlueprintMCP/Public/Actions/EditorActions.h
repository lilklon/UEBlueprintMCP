// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorAction.h"

class AActor;

/**
 * FGetActorsInLevelAction
 * Returns all actors in the current level.
 */
class UEBLUEPRINTMCP_API FGetActorsInLevelAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("get_actors_in_level"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FFindActorsByNameAction
 * Finds actors matching a name pattern.
 */
class UEBLUEPRINTMCP_API FFindActorsByNameAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("find_actors_by_name"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FSpawnActorAction
 * Spawns a basic actor type in the level.
 */
class UEBLUEPRINTMCP_API FSpawnActorAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("spawn_actor"); }

private:
	UClass* ResolveActorClass(const FString& TypeName) const;
};


/**
 * FDeleteActorAction
 * Deletes an actor from the level.
 */
class UEBLUEPRINTMCP_API FDeleteActorAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("delete_actor"); }
};


/**
 * FSetActorTransformAction
 * Sets the transform (location/rotation/scale) of an actor.
 */
class UEBLUEPRINTMCP_API FSetActorTransformAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("set_actor_transform"); }
};


/**
 * FGetActorPropertiesAction
 * Gets all properties of an actor.
 */
class UEBLUEPRINTMCP_API FGetActorPropertiesAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("get_actor_properties"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FSetActorPropertyAction
 * Sets a property on an actor.
 */
class UEBLUEPRINTMCP_API FSetActorPropertyAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("set_actor_property"); }
};


/**
 * FFocusViewportAction
 * Focuses the viewport on an actor or location.
 */
class UEBLUEPRINTMCP_API FFocusViewportAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("focus_viewport"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FGetViewportTransformAction
 * Gets the current viewport camera location and rotation.
 */
class UEBLUEPRINTMCP_API FGetViewportTransformAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("get_viewport_transform"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FSetViewportTransformAction
 * Sets the viewport camera location and/or rotation.
 */
class UEBLUEPRINTMCP_API FSetViewportTransformAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("set_viewport_transform"); }
	virtual bool RequiresSave() const override { return false; }
};


/**
 * FSaveAllAction
 * Saves all dirty packages (blueprints, levels, assets).
 */
class UEBLUEPRINTMCP_API FSaveAllAction : public FEditorAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override { return true; }
	virtual FString GetActionName() const override { return TEXT("save_all"); }
	virtual bool RequiresSave() const override { return false; }
};
