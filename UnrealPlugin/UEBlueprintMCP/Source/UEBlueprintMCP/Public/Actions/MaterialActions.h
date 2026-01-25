// Copyright (c) 2025 zolnoor. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorAction.h"

// Forward declarations
class UMaterial;
class UMaterialInstance;
class UMaterialExpression;

/**
 * FMaterialAction
 *
 * Base class for Material-related actions.
 * Provides common utilities for material manipulation.
 */
class UEBLUEPRINTMCP_API FMaterialAction : public FEditorAction
{
protected:
	/** Find Material by name */
	UMaterial* FindMaterial(const FString& MaterialName, FString& OutError) const;

	/** Get Material by name, or use current from context if name is empty */
	UMaterial* GetMaterialByNameOrCurrent(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) const;

	/** Clean up existing material with same name */
	void CleanupExistingMaterial(const FString& MaterialName, const FString& PackagePath) const;

	/** Resolve expression class from friendly name */
	UClass* ResolveExpressionClass(const FString& ExpressionClassName) const;

	/** Mark material as modified and trigger recompilation */
	void MarkMaterialModified(UMaterial* Material, FMCPEditorContext& Context) const;
};


/**
 * FCreateMaterialAction
 *
 * Creates a new Material asset with specified domain and blend mode.
 *
 * Parameters:
 *   - material_name (required): Name of the Material to create
 *   - path (optional): Content path (default: /Game/Materials)
 *   - domain (optional): Material domain (Surface, PostProcess, DeferredDecal, LightFunction, UI)
 *   - blend_mode (optional): Blend mode (Opaque, Masked, Translucent, Additive, Modulate)
 *
 * Returns:
 *   - name: Created material name
 *   - path: Asset path
 *   - domain: Material domain
 *   - blend_mode: Blend mode
 */
class UEBLUEPRINTMCP_API FCreateMaterialAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("create_material"); }

private:
	/** Resolve domain enum from string */
	TOptional<EMaterialDomain> ResolveDomain(const FString& DomainString) const;

	/** Resolve blend mode enum from string */
	TOptional<EBlendMode> ResolveBlendMode(const FString& BlendModeString) const;
};


/**
 * FAddMaterialExpressionAction
 *
 * Adds an expression node to a Material's graph.
 *
 * Parameters:
 *   - material_name (required): Name of the target Material
 *   - expression_class (required): Type of expression (SceneTexture, Time, Noise, Add, Multiply, etc.)
 *   - node_name (required): Unique name for this node (for later connection/reference)
 *   - position (optional): [X, Y] editor position (default: [0, 0])
 *   - properties (optional): Object with property name/value pairs to set
 *
 * Returns:
 *   - node_name: Registered node name
 *   - expression_class: Expression type
 */
class UEBLUEPRINTMCP_API FAddMaterialExpressionAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("add_material_expression"); }

private:
	/** Set properties on an expression from JSON object */
	void SetExpressionProperties(UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Properties) const;
};


/**
 * FConnectMaterialExpressionsAction
 *
 * Connects output of one expression to input of another.
 *
 * Parameters:
 *   - material_name (required): Name of the Material
 *   - source_node (required): Name of the source expression
 *   - source_output_index (optional): Output pin index (default: 0)
 *   - target_node (required): Name of the target expression
 *   - target_input (required): Input pin name (A, B, Alpha, etc.)
 *
 * Returns:
 *   - source_node: Source node name
 *   - target_node: Target node name
 *   - target_input: Input pin name
 */
class UEBLUEPRINTMCP_API FConnectMaterialExpressionsAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("connect_material_expressions"); }

private:
	/** Connect to a named input on an expression (handles type-specific input property mapping) */
	bool ConnectToExpressionInput(UMaterialExpression* SourceExpr, int32 OutputIndex,
		UMaterialExpression* TargetExpr, const FString& InputName, FString& OutError) const;
};


/**
 * FConnectToMaterialOutputAction
 *
 * Connects an expression to a material's main output (BaseColor, EmissiveColor, etc.).
 *
 * Parameters:
 *   - material_name (required): Name of the Material
 *   - source_node (required): Name of the source expression
 *   - source_output_index (optional): Output pin index (default: 0)
 *   - material_property (required): Material property (BaseColor, EmissiveColor, Metallic, Roughness, Normal, Opacity, etc.)
 *
 * Returns:
 *   - source_node: Source node name
 *   - material_property: Connected property
 */
class UEBLUEPRINTMCP_API FConnectToMaterialOutputAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("connect_to_material_output"); }

private:
	/** Connect expression to material property */
	bool ConnectToMaterialProperty(UMaterial* Material, UMaterialExpression* SourceExpr,
		int32 OutputIndex, const FString& PropertyName, FString& OutError) const;
};


/**
 * FSetMaterialExpressionPropertyAction
 *
 * Sets a property on an existing material expression.
 *
 * Parameters:
 *   - material_name (required): Name of the Material
 *   - node_name (required): Name of the expression node
 *   - property_name (required): Property to set
 *   - property_value (required): Value to set (as string, will be parsed)
 *
 * Returns:
 *   - node_name: Node name
 *   - property_name: Property that was set
 */
class UEBLUEPRINTMCP_API FSetMaterialExpressionPropertyAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("set_material_expression_property"); }

private:
	/** Set a single property on an expression */
	bool SetExpressionProperty(UMaterialExpression* Expression, const FString& PropertyName,
		const FString& PropertyValue, FString& OutError) const;
};


/**
 * FCompileMaterialAction
 *
 * Compiles a material and reports errors.
 *
 * Parameters:
 *   - material_name (required): Name of the Material to compile
 *
 * Returns:
 *   - name: Material name
 *   - success: Whether compilation succeeded
 *   - error_count: Number of errors
 *   - warning_count: Number of warnings
 */
class UEBLUEPRINTMCP_API FCompileMaterialAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("compile_material"); }
	virtual bool RequiresSave() const override { return true; } // Save after successful compilation
};


/**
 * FCreateMaterialInstanceAction
 *
 * Creates a Material Instance from a parent material with parameter overrides.
 *
 * Parameters:
 *   - instance_name (required): Name for the material instance
 *   - parent_material (required): Name of the parent material
 *   - path (optional): Content path (default: /Game/Materials)
 *   - scalar_parameters (optional): Object with scalar parameter overrides
 *   - vector_parameters (optional): Object with vector parameter overrides
 *
 * Returns:
 *   - name: Instance name
 *   - path: Asset path
 *   - parent: Parent material name
 */
class UEBLUEPRINTMCP_API FCreateMaterialInstanceAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("create_material_instance"); }
};


/**
 * FCreatePostProcessVolumeAction
 *
 * Creates a Post Process Volume actor in the level.
 *
 * Parameters:
 *   - name (required): Name for the volume actor
 *   - location (optional): [X, Y, Z] world location
 *   - infinite_extent (optional): Whether to apply everywhere (default: true)
 *   - priority (optional): Priority value (default: 0.0)
 *   - post_process_materials (optional): Array of material names to add
 *
 * Returns:
 *   - name: Actor name
 *   - location: [X, Y, Z]
 *   - infinite_extent: Boolean
 *   - priority: Priority value
 */
class UEBLUEPRINTMCP_API FCreatePostProcessVolumeAction : public FMaterialAction
{
public:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual FString GetActionName() const override { return TEXT("create_post_process_volume"); }

private:
	/** Parse location from params */
	FVector GetVectorFromParams(const TSharedPtr<FJsonObject>& Params, const FString& FieldName) const;
};
