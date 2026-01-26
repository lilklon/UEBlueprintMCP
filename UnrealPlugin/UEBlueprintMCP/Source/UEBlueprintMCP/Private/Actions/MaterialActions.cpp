// Copyright (c) 2025 zolnoor. All rights reserved.

#include "Actions/MaterialActions.h"
#include "MCPContext.h"

// Material system headers
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionIf.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionSquareRoot.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionCeil.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionCosine.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionCrossProduct.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionStep.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionMin.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionTextureSample.h"

// Factory and editing
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "MaterialEditingLibrary.h"

// Editor and asset utilities
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "UObject/SavePackage.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"

// Post process volume
#include "Engine/PostProcessVolume.h"
#include "Components/PostProcessComponent.h"
#include "EngineUtils.h"  // For TActorIterator
#include "ComponentReregisterContext.h"  // For FGlobalComponentReregisterContext

// =========================================================================
// Expression Class Mapping
// =========================================================================

static TMap<FString, UClass*> ExpressionClassMap;
static TMap<FString, EMaterialShadingModel> ShadingModelMap;
static TMap<FString, EBlendMode> BlendModeMap;

static void InitShadingModelMap()
{
	if (ShadingModelMap.Num() > 0) return;

	ShadingModelMap.Add(TEXT("Unlit"), MSM_Unlit);
	ShadingModelMap.Add(TEXT("MSM_Unlit"), MSM_Unlit);
	ShadingModelMap.Add(TEXT("DefaultLit"), MSM_DefaultLit);
	ShadingModelMap.Add(TEXT("MSM_DefaultLit"), MSM_DefaultLit);
	ShadingModelMap.Add(TEXT("Lit"), MSM_DefaultLit);
	ShadingModelMap.Add(TEXT("Subsurface"), MSM_Subsurface);
	ShadingModelMap.Add(TEXT("MSM_Subsurface"), MSM_Subsurface);
	ShadingModelMap.Add(TEXT("PreintegratedSkin"), MSM_PreintegratedSkin);
	ShadingModelMap.Add(TEXT("MSM_PreintegratedSkin"), MSM_PreintegratedSkin);
	ShadingModelMap.Add(TEXT("ClearCoat"), MSM_ClearCoat);
	ShadingModelMap.Add(TEXT("MSM_ClearCoat"), MSM_ClearCoat);
	ShadingModelMap.Add(TEXT("SubsurfaceProfile"), MSM_SubsurfaceProfile);
	ShadingModelMap.Add(TEXT("MSM_SubsurfaceProfile"), MSM_SubsurfaceProfile);
	ShadingModelMap.Add(TEXT("TwoSidedFoliage"), MSM_TwoSidedFoliage);
	ShadingModelMap.Add(TEXT("MSM_TwoSidedFoliage"), MSM_TwoSidedFoliage);
	ShadingModelMap.Add(TEXT("Hair"), MSM_Hair);
	ShadingModelMap.Add(TEXT("MSM_Hair"), MSM_Hair);
	ShadingModelMap.Add(TEXT("Cloth"), MSM_Cloth);
	ShadingModelMap.Add(TEXT("MSM_Cloth"), MSM_Cloth);
	ShadingModelMap.Add(TEXT("Eye"), MSM_Eye);
	ShadingModelMap.Add(TEXT("MSM_Eye"), MSM_Eye);
}

static void InitBlendModeMap()
{
	if (BlendModeMap.Num() > 0) return;

	BlendModeMap.Add(TEXT("Opaque"), BLEND_Opaque);
	BlendModeMap.Add(TEXT("BLEND_Opaque"), BLEND_Opaque);
	BlendModeMap.Add(TEXT("Masked"), BLEND_Masked);
	BlendModeMap.Add(TEXT("BLEND_Masked"), BLEND_Masked);
	BlendModeMap.Add(TEXT("Translucent"), BLEND_Translucent);
	BlendModeMap.Add(TEXT("BLEND_Translucent"), BLEND_Translucent);
	BlendModeMap.Add(TEXT("Additive"), BLEND_Additive);
	BlendModeMap.Add(TEXT("BLEND_Additive"), BLEND_Additive);
	BlendModeMap.Add(TEXT("Modulate"), BLEND_Modulate);
	BlendModeMap.Add(TEXT("BLEND_Modulate"), BLEND_Modulate);
	BlendModeMap.Add(TEXT("AlphaComposite"), BLEND_AlphaComposite);
	BlendModeMap.Add(TEXT("BLEND_AlphaComposite"), BLEND_AlphaComposite);
	BlendModeMap.Add(TEXT("AlphaHoldout"), BLEND_AlphaHoldout);
	BlendModeMap.Add(TEXT("BLEND_AlphaHoldout"), BLEND_AlphaHoldout);
}

static void InitExpressionClassMap()
{
	if (ExpressionClassMap.Num() > 0) return; // Already initialized

	// Scene/Texture access
	ExpressionClassMap.Add(TEXT("SceneTexture"), UMaterialExpressionSceneTexture::StaticClass());
	ExpressionClassMap.Add(TEXT("SceneDepth"), UMaterialExpressionSceneDepth::StaticClass());
	ExpressionClassMap.Add(TEXT("ScreenPosition"), UMaterialExpressionScreenPosition::StaticClass());
	ExpressionClassMap.Add(TEXT("TextureCoordinate"), UMaterialExpressionTextureCoordinate::StaticClass());
	ExpressionClassMap.Add(TEXT("TextureSample"), UMaterialExpressionTextureSample::StaticClass());
	ExpressionClassMap.Add(TEXT("PixelDepth"), UMaterialExpressionPixelDepth::StaticClass());
	ExpressionClassMap.Add(TEXT("WorldPosition"), UMaterialExpressionWorldPosition::StaticClass());
	ExpressionClassMap.Add(TEXT("CameraPosition"), UMaterialExpressionCameraPositionWS::StaticClass());

	// Math operations
	ExpressionClassMap.Add(TEXT("Add"), UMaterialExpressionAdd::StaticClass());
	ExpressionClassMap.Add(TEXT("Subtract"), UMaterialExpressionSubtract::StaticClass());
	ExpressionClassMap.Add(TEXT("Multiply"), UMaterialExpressionMultiply::StaticClass());
	ExpressionClassMap.Add(TEXT("Divide"), UMaterialExpressionDivide::StaticClass());
	ExpressionClassMap.Add(TEXT("Power"), UMaterialExpressionPower::StaticClass());
	ExpressionClassMap.Add(TEXT("SquareRoot"), UMaterialExpressionSquareRoot::StaticClass());
	ExpressionClassMap.Add(TEXT("Abs"), UMaterialExpressionAbs::StaticClass());
	ExpressionClassMap.Add(TEXT("Min"), UMaterialExpressionMin::StaticClass());
	ExpressionClassMap.Add(TEXT("Max"), UMaterialExpressionMax::StaticClass());
	ExpressionClassMap.Add(TEXT("Clamp"), UMaterialExpressionClamp::StaticClass());
	ExpressionClassMap.Add(TEXT("Saturate"), UMaterialExpressionSaturate::StaticClass());
	ExpressionClassMap.Add(TEXT("Floor"), UMaterialExpressionFloor::StaticClass());
	ExpressionClassMap.Add(TEXT("Ceil"), UMaterialExpressionCeil::StaticClass());
	ExpressionClassMap.Add(TEXT("Frac"), UMaterialExpressionFrac::StaticClass());
	ExpressionClassMap.Add(TEXT("OneMinus"), UMaterialExpressionOneMinus::StaticClass());
	ExpressionClassMap.Add(TEXT("Step"), UMaterialExpressionStep::StaticClass());
	ExpressionClassMap.Add(TEXT("SmoothStep"), UMaterialExpressionSmoothStep::StaticClass());

	// Trigonometry
	ExpressionClassMap.Add(TEXT("Sin"), UMaterialExpressionSine::StaticClass());
	ExpressionClassMap.Add(TEXT("Cos"), UMaterialExpressionCosine::StaticClass());

	// Vector operations
	ExpressionClassMap.Add(TEXT("DotProduct"), UMaterialExpressionDotProduct::StaticClass());
	ExpressionClassMap.Add(TEXT("CrossProduct"), UMaterialExpressionCrossProduct::StaticClass());
	ExpressionClassMap.Add(TEXT("Normalize"), UMaterialExpressionNormalize::StaticClass());
	ExpressionClassMap.Add(TEXT("AppendVector"), UMaterialExpressionAppendVector::StaticClass());
	ExpressionClassMap.Add(TEXT("ComponentMask"), UMaterialExpressionComponentMask::StaticClass());

	// Constants
	ExpressionClassMap.Add(TEXT("Constant"), UMaterialExpressionConstant::StaticClass());
	ExpressionClassMap.Add(TEXT("Constant2Vector"), UMaterialExpressionConstant2Vector::StaticClass());
	ExpressionClassMap.Add(TEXT("Constant3Vector"), UMaterialExpressionConstant3Vector::StaticClass());
	ExpressionClassMap.Add(TEXT("Constant4Vector"), UMaterialExpressionConstant4Vector::StaticClass());

	// Parameters
	ExpressionClassMap.Add(TEXT("ScalarParameter"), UMaterialExpressionScalarParameter::StaticClass());
	ExpressionClassMap.Add(TEXT("VectorParameter"), UMaterialExpressionVectorParameter::StaticClass());

	// Procedural
	ExpressionClassMap.Add(TEXT("Noise"), UMaterialExpressionNoise::StaticClass());
	ExpressionClassMap.Add(TEXT("Time"), UMaterialExpressionTime::StaticClass());
	ExpressionClassMap.Add(TEXT("Panner"), UMaterialExpressionPanner::StaticClass());

	// Derivatives
	ExpressionClassMap.Add(TEXT("DDX"), UMaterialExpressionDDX::StaticClass());
	ExpressionClassMap.Add(TEXT("DDY"), UMaterialExpressionDDY::StaticClass());

	// Control
	ExpressionClassMap.Add(TEXT("If"), UMaterialExpressionIf::StaticClass());
	ExpressionClassMap.Add(TEXT("Lerp"), UMaterialExpressionLinearInterpolate::StaticClass());
	ExpressionClassMap.Add(TEXT("LinearInterpolate"), UMaterialExpressionLinearInterpolate::StaticClass());

	// Custom HLSL
	ExpressionClassMap.Add(TEXT("Custom"), UMaterialExpressionCustom::StaticClass());
}

// =========================================================================
// FMaterialAction - Base Class
// =========================================================================

UMaterial* FMaterialAction::FindMaterial(const FString& MaterialName, FString& OutError) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UMaterial::StaticClass()->GetClassPathName(), AssetList);

	for (const FAssetData& AssetData : AssetList)
	{
		if (AssetData.AssetName.ToString() == MaterialName)
		{
			return Cast<UMaterial>(AssetData.GetAsset());
		}
	}

	OutError = FString::Printf(TEXT("Material '%s' not found"), *MaterialName);
	return nullptr;
}

UMaterial* FMaterialAction::GetMaterialByNameOrCurrent(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) const
{
	FString MaterialName = GetOptionalString(Params, TEXT("material_name"));

	if (MaterialName.IsEmpty())
	{
		UMaterial* Current = Context.CurrentMaterial.Get();
		if (!Current)
		{
			OutError = TEXT("No current material set. Specify material_name or create a material first.");
		}
		return Current;
	}

	return FindMaterial(MaterialName, OutError);
}

void FMaterialAction::CleanupExistingMaterial(const FString& MaterialName, const FString& PackagePath) const
{
	UPackage* ExistingPackage = FindPackage(nullptr, *PackagePath);
	if (ExistingPackage)
	{
		UMaterial* ExistingMaterial = FindObject<UMaterial>(ExistingPackage, *MaterialName);
		if (ExistingMaterial)
		{
			FString TempName = FString::Printf(TEXT("%s_TEMP_%d"), *MaterialName, FMath::Rand());
			ExistingMaterial->Rename(*TempName, GetTransientPackage(),
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
			ExistingMaterial->MarkAsGarbage();
			ExistingPackage->MarkAsGarbage();
		}
	}

	if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
	{
		UEditorAssetLibrary::DeleteAsset(PackagePath);
	}
}

UClass* FMaterialAction::ResolveExpressionClass(const FString& ExpressionClassName) const
{
	InitExpressionClassMap();

	if (UClass** Found = ExpressionClassMap.Find(ExpressionClassName))
	{
		return *Found;
	}

	// Try direct class lookup
	FString ClassName = TEXT("MaterialExpression") + ExpressionClassName;
	UClass* FoundClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *ClassName));
	return FoundClass;
}

void FMaterialAction::MarkMaterialModified(UMaterial* Material, FMCPEditorContext& Context) const
{
	if (Material)
	{
		Material->PreEditChange(nullptr);
		Material->PostEditChange();

		// Reregister components to apply changes
		{
			FGlobalComponentReregisterContext RecreateComponents;
		}

		Material->MarkPackageDirty();
		Context.MarkPackageDirty(Material->GetOutermost());
	}
}

// =========================================================================
// FCreateMaterialAction
// =========================================================================

bool FCreateMaterialAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString MaterialName;
	return GetRequiredString(Params, TEXT("material_name"), MaterialName, OutError);
}

TOptional<EMaterialDomain> FCreateMaterialAction::ResolveDomain(const FString& DomainString) const
{
	if (DomainString.IsEmpty() || DomainString.Equals(TEXT("Surface"), ESearchCase::IgnoreCase))
		return MD_Surface;
	if (DomainString.Equals(TEXT("PostProcess"), ESearchCase::IgnoreCase))
		return MD_PostProcess;
	if (DomainString.Equals(TEXT("DeferredDecal"), ESearchCase::IgnoreCase))
		return MD_DeferredDecal;
	if (DomainString.Equals(TEXT("LightFunction"), ESearchCase::IgnoreCase))
		return MD_LightFunction;
	if (DomainString.Equals(TEXT("UI"), ESearchCase::IgnoreCase))
		return MD_UI;
	if (DomainString.Equals(TEXT("Volume"), ESearchCase::IgnoreCase))
		return MD_Volume;

	return TOptional<EMaterialDomain>();
}

TOptional<EBlendMode> FCreateMaterialAction::ResolveBlendMode(const FString& BlendModeString) const
{
	if (BlendModeString.IsEmpty() || BlendModeString.Equals(TEXT("Opaque"), ESearchCase::IgnoreCase))
		return BLEND_Opaque;
	if (BlendModeString.Equals(TEXT("Masked"), ESearchCase::IgnoreCase))
		return BLEND_Masked;
	if (BlendModeString.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase))
		return BLEND_Translucent;
	if (BlendModeString.Equals(TEXT("Additive"), ESearchCase::IgnoreCase))
		return BLEND_Additive;
	if (BlendModeString.Equals(TEXT("Modulate"), ESearchCase::IgnoreCase))
		return BLEND_Modulate;
	if (BlendModeString.Equals(TEXT("AlphaComposite"), ESearchCase::IgnoreCase))
		return BLEND_AlphaComposite;
	if (BlendModeString.Equals(TEXT("AlphaHoldout"), ESearchCase::IgnoreCase))
		return BLEND_AlphaHoldout;

	return TOptional<EBlendMode>();
}

TOptional<EBlendableLocation> FCreateMaterialAction::ResolveBlendableLocation(const FString& LocationString) const
{
	if (LocationString.IsEmpty() || LocationString.Equals(TEXT("AfterTonemapping"), ESearchCase::IgnoreCase) ||
		LocationString.Equals(TEXT("BL_AfterTonemapping"), ESearchCase::IgnoreCase))
		return BL_AfterTonemapping;
	if (LocationString.Equals(TEXT("BeforeTonemapping"), ESearchCase::IgnoreCase) ||
		LocationString.Equals(TEXT("BL_BeforeTonemapping"), ESearchCase::IgnoreCase))
		return BL_BeforeTonemapping;
	if (LocationString.Equals(TEXT("BeforeTranslucency"), ESearchCase::IgnoreCase) ||
		LocationString.Equals(TEXT("BL_BeforeTranslucency"), ESearchCase::IgnoreCase))
		return BL_BeforeTranslucency;
	if (LocationString.Equals(TEXT("ReplacingTonemapper"), ESearchCase::IgnoreCase) ||
		LocationString.Equals(TEXT("BL_ReplacingTonemapper"), ESearchCase::IgnoreCase))
		return BL_ReplacingTonemapper;
	if (LocationString.Equals(TEXT("SSRInput"), ESearchCase::IgnoreCase) ||
		LocationString.Equals(TEXT("BL_SSRInput"), ESearchCase::IgnoreCase))
		return BL_SSRInput;

	return TOptional<EBlendableLocation>();
}

TSharedPtr<FJsonObject> FCreateMaterialAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;
	FString MaterialName;
	GetRequiredString(Params, TEXT("material_name"), MaterialName, Error);

	FString Path = GetOptionalString(Params, TEXT("path"), TEXT("/Game/Materials"));
	FString DomainStr = GetOptionalString(Params, TEXT("domain"), TEXT("Surface"));
	FString BlendModeStr = GetOptionalString(Params, TEXT("blend_mode"), TEXT("Opaque"));
	FString BlendableLocationStr = GetOptionalString(Params, TEXT("blendable_location"), TEXT(""));

	// Resolve domain
	TOptional<EMaterialDomain> Domain = ResolveDomain(DomainStr);
	if (!Domain.IsSet())
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Invalid domain '%s'. Valid: Surface, PostProcess, DeferredDecal, LightFunction, UI, Volume"), *DomainStr),
			TEXT("invalid_domain"));
	}

	// Resolve blend mode
	TOptional<EBlendMode> BlendMode = ResolveBlendMode(BlendModeStr);
	if (!BlendMode.IsSet())
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Invalid blend_mode '%s'. Valid: Opaque, Masked, Translucent, Additive, Modulate"), *BlendModeStr),
			TEXT("invalid_blend_mode"));
	}

	// Build package path
	FString MaterialPackagePath = Path / MaterialName;

	// Clean up existing material
	CleanupExistingMaterial(MaterialName, MaterialPackagePath);

	// Create package
	UPackage* Package = CreatePackage(*MaterialPackagePath);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package for material"), TEXT("package_creation_failed"));
	}
	Package->FullyLoad();

	// Create material using factory
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(), Package, *MaterialName,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!NewMaterial)
	{
		return CreateErrorResponse(TEXT("Failed to create material"), TEXT("material_creation_failed"));
	}

	// Set domain and blend mode
	NewMaterial->MaterialDomain = Domain.GetValue();
	NewMaterial->BlendMode = BlendMode.GetValue();

	// Set blendable location for post-process materials
	if (!BlendableLocationStr.IsEmpty())
	{
		TOptional<EBlendableLocation> BlendableLocation = ResolveBlendableLocation(BlendableLocationStr);
		if (BlendableLocation.IsSet())
		{
			NewMaterial->BlendableLocation = BlendableLocation.GetValue();
		}
	}
	else if (Domain.GetValue() == MD_PostProcess)
	{
		// Default to BeforeTonemapping for post-process materials (needed for depth access)
		NewMaterial->BlendableLocation = BL_BeforeTonemapping;
	}

	// Trigger compilation
	NewMaterial->PreEditChange(nullptr);
	NewMaterial->PostEditChange();

	// Register and mark dirty
	Package->SetDirtyFlag(true);
	NewMaterial->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewMaterial);

	// Update context
	Context.SetCurrentMaterial(NewMaterial);
	Context.MarkPackageDirty(Package);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("name"), MaterialName);
	Result->SetStringField(TEXT("path"), MaterialPackagePath);
	Result->SetStringField(TEXT("domain"), DomainStr);
	Result->SetStringField(TEXT("blend_mode"), BlendModeStr);

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FAddMaterialExpressionAction
// =========================================================================

bool FAddMaterialExpressionAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString ExpressionClass, NodeName;
	if (!GetRequiredString(Params, TEXT("expression_class"), ExpressionClass, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("node_name"), NodeName, OutError)) return false;
	return true;
}

void FAddMaterialExpressionAction::SetExpressionProperties(UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Properties) const
{
	if (!Properties.IsValid() || !Expression) return;

	for (const auto& Pair : Properties->Values)
	{
		const FString& PropName = Pair.Key;
		const TSharedPtr<FJsonValue>& PropValue = Pair.Value;

		// Handle specific expression types and their properties
		// SceneTexture
		if (UMaterialExpressionSceneTexture* SceneTex = Cast<UMaterialExpressionSceneTexture>(Expression))
		{
			if (PropName == TEXT("SceneTextureId"))
			{
				FString IdStr = PropValue->AsString();
				if (IdStr == TEXT("PPI_SceneColor")) SceneTex->SceneTextureId = PPI_SceneColor;
				else if (IdStr == TEXT("PPI_SceneDepth")) SceneTex->SceneTextureId = PPI_SceneDepth;
				else if (IdStr == TEXT("PPI_WorldNormal")) SceneTex->SceneTextureId = PPI_WorldNormal;
				else if (IdStr == TEXT("PPI_PostProcessInput0")) SceneTex->SceneTextureId = PPI_PostProcessInput0;
			}
		}
		// Scalar Parameter
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expression))
		{
			if (PropName == TEXT("ParameterName")) ScalarParam->ParameterName = FName(*PropValue->AsString());
			else if (PropName == TEXT("DefaultValue")) ScalarParam->DefaultValue = PropValue->AsNumber();
		}
		// Vector Parameter
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expression))
		{
			if (PropName == TEXT("ParameterName")) VectorParam->ParameterName = FName(*PropValue->AsString());
			else if (PropName == TEXT("DefaultValue"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Arr;
				if (PropValue->TryGetArray(Arr) && Arr->Num() >= 3)
				{
					VectorParam->DefaultValue = FLinearColor(
						(*Arr)[0]->AsNumber(),
						(*Arr)[1]->AsNumber(),
						(*Arr)[2]->AsNumber(),
						Arr->Num() > 3 ? (*Arr)[3]->AsNumber() : 1.0f);
				}
			}
		}
		// Constant
		else if (UMaterialExpressionConstant* Const = Cast<UMaterialExpressionConstant>(Expression))
		{
			if (PropName == TEXT("R") || PropName == TEXT("Value")) Const->R = PropValue->AsNumber();
		}
		// Constant3Vector
		else if (UMaterialExpressionConstant3Vector* Const3 = Cast<UMaterialExpressionConstant3Vector>(Expression))
		{
			if (PropName == TEXT("Constant"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Arr;
				if (PropValue->TryGetArray(Arr) && Arr->Num() >= 3)
				{
					Const3->Constant = FLinearColor(
						(*Arr)[0]->AsNumber(),
						(*Arr)[1]->AsNumber(),
						(*Arr)[2]->AsNumber());
				}
			}
		}
		// Custom HLSL
		else if (UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression))
		{
			if (PropName == TEXT("Code")) Custom->Code = PropValue->AsString();
			else if (PropName == TEXT("Description")) Custom->Description = PropValue->AsString();
			else if (PropName == TEXT("OutputType"))
			{
				FString TypeStr = PropValue->AsString();
				if (TypeStr == TEXT("CMOT_Float1")) Custom->OutputType = CMOT_Float1;
				else if (TypeStr == TEXT("CMOT_Float2")) Custom->OutputType = CMOT_Float2;
				else if (TypeStr == TEXT("CMOT_Float3")) Custom->OutputType = CMOT_Float3;
				else if (TypeStr == TEXT("CMOT_Float4")) Custom->OutputType = CMOT_Float4;
			}
		}
		// Noise
		else if (UMaterialExpressionNoise* Noise = Cast<UMaterialExpressionNoise>(Expression))
		{
			if (PropName == TEXT("NoiseFunction"))
			{
				FString FuncStr = PropValue->AsString();
				if (FuncStr == TEXT("NOISEFUNCTION_SimplexTex")) Noise->NoiseFunction = NOISEFUNCTION_SimplexTex;
				else if (FuncStr == TEXT("NOISEFUNCTION_GradientTex")) Noise->NoiseFunction = NOISEFUNCTION_GradientTex;
				else if (FuncStr == TEXT("NOISEFUNCTION_VoronoiALU")) Noise->NoiseFunction = NOISEFUNCTION_VoronoiALU;
			}
			else if (PropName == TEXT("Scale")) Noise->Scale = PropValue->AsNumber();
			else if (PropName == TEXT("Levels")) Noise->Levels = PropValue->AsNumber();
		}
		// ComponentMask
		else if (UMaterialExpressionComponentMask* Mask = Cast<UMaterialExpressionComponentMask>(Expression))
		{
			if (PropName == TEXT("R")) Mask->R = PropValue->AsBool();
			else if (PropName == TEXT("G")) Mask->G = PropValue->AsBool();
			else if (PropName == TEXT("B")) Mask->B = PropValue->AsBool();
			else if (PropName == TEXT("A")) Mask->A = PropValue->AsBool();
		}
		// Clamp
		else if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(Expression))
		{
			if (PropName == TEXT("MinDefault")) Clamp->MinDefault = PropValue->AsNumber();
			else if (PropName == TEXT("MaxDefault")) Clamp->MaxDefault = PropValue->AsNumber();
		}
	}
}

TSharedPtr<FJsonObject> FAddMaterialExpressionAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;

	// Get material
	UMaterial* Material = GetMaterialByNameOrCurrent(Params, Context, Error);
	if (!Material)
	{
		return CreateErrorResponse(Error, TEXT("material_not_found"));
	}

	// Get expression class
	FString ExpressionClassName;
	GetRequiredString(Params, TEXT("expression_class"), ExpressionClassName, Error);

	UClass* ExprClass = ResolveExpressionClass(ExpressionClassName);
	if (!ExprClass)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Unknown expression class '%s'. Common types: SceneTexture, Time, Noise, Add, Multiply, Lerp, Constant, ScalarParameter, VectorParameter, Custom"), *ExpressionClassName),
			TEXT("unknown_expression_class"));
	}

	// Get node name
	FString NodeName;
	GetRequiredString(Params, TEXT("node_name"), NodeName, Error);

	// Check for duplicate name
	if (Context.GetMaterialNode(NodeName))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node name '%s' already exists. Use a unique name."), *NodeName),
			TEXT("duplicate_node_name"));
	}

	// Get position
	int32 PosX = 0, PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PosArray = GetOptionalArray(Params, TEXT("position"));
	if (PosArray && PosArray->Num() >= 2)
	{
		PosX = (*PosArray)[0]->AsNumber();
		PosY = (*PosArray)[1]->AsNumber();
	}

	// Create the expression
	UMaterialExpression* NewExpr = NewObject<UMaterialExpression>(Material, ExprClass);
	if (!NewExpr)
	{
		return CreateErrorResponse(TEXT("Failed to create material expression"), TEXT("creation_failed"));
	}

	// Set editor position
	NewExpr->MaterialExpressionEditorX = PosX;
	NewExpr->MaterialExpressionEditorY = PosY;

	// Add to material's expression collection
	Material->GetExpressionCollection().AddExpression(NewExpr);

	// Set properties if provided
	if (Params->HasField(TEXT("properties")))
	{
		TSharedPtr<FJsonObject> PropsObj = Params->GetObjectField(TEXT("properties"));
		SetExpressionProperties(NewExpr, PropsObj);
	}

	// Register in context
	Context.RegisterMaterialNode(NodeName, NewExpr);
	Context.SetCurrentMaterial(Material);

	// Mark modified
	MarkMaterialModified(Material, Context);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("node_name"), NodeName);
	Result->SetStringField(TEXT("expression_class"), ExpressionClassName);
	Result->SetStringField(TEXT("material"), Material->GetName());

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FConnectMaterialExpressionsAction
// =========================================================================

bool FConnectMaterialExpressionsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString SourceNode, TargetNode, TargetInput;
	if (!GetRequiredString(Params, TEXT("source_node"), SourceNode, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("target_node"), TargetNode, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("target_input"), TargetInput, OutError)) return false;
	return true;
}

bool FConnectMaterialExpressionsAction::ConnectToExpressionInput(UMaterialExpression* SourceExpr, int32 OutputIndex,
	UMaterialExpression* TargetExpr, const FString& InputName, FString& OutError) const
{
	// Handle common expression types with named inputs
	// Most binary operations have A and B inputs

	// Add
	if (UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Add->A.Expression = SourceExpr; Add->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Add->B.Expression = SourceExpr; Add->B.OutputIndex = OutputIndex; return true; }
	}
	// Subtract
	else if (UMaterialExpressionSubtract* Sub = Cast<UMaterialExpressionSubtract>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Sub->A.Expression = SourceExpr; Sub->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Sub->B.Expression = SourceExpr; Sub->B.OutputIndex = OutputIndex; return true; }
	}
	// Multiply
	else if (UMaterialExpressionMultiply* Mul = Cast<UMaterialExpressionMultiply>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Mul->A.Expression = SourceExpr; Mul->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Mul->B.Expression = SourceExpr; Mul->B.OutputIndex = OutputIndex; return true; }
	}
	// Divide
	else if (UMaterialExpressionDivide* Div = Cast<UMaterialExpressionDivide>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Div->A.Expression = SourceExpr; Div->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Div->B.Expression = SourceExpr; Div->B.OutputIndex = OutputIndex; return true; }
	}
	// Power
	else if (UMaterialExpressionPower* Pow = Cast<UMaterialExpressionPower>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Base"), ESearchCase::IgnoreCase)) { Pow->Base.Expression = SourceExpr; Pow->Base.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("Exponent"), ESearchCase::IgnoreCase) || InputName.Equals(TEXT("Exp"), ESearchCase::IgnoreCase))
		{
			Pow->Exponent.Expression = SourceExpr; Pow->Exponent.OutputIndex = OutputIndex; return true;
		}
	}
	// Lerp
	else if (UMaterialExpressionLinearInterpolate* Lerp = Cast<UMaterialExpressionLinearInterpolate>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Lerp->A.Expression = SourceExpr; Lerp->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Lerp->B.Expression = SourceExpr; Lerp->B.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("Alpha"), ESearchCase::IgnoreCase)) { Lerp->Alpha.Expression = SourceExpr; Lerp->Alpha.OutputIndex = OutputIndex; return true; }
	}
	// Clamp
	else if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase)) { Clamp->Input.Expression = SourceExpr; Clamp->Input.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("Min"), ESearchCase::IgnoreCase)) { Clamp->Min.Expression = SourceExpr; Clamp->Min.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("Max"), ESearchCase::IgnoreCase)) { Clamp->Max.Expression = SourceExpr; Clamp->Max.OutputIndex = OutputIndex; return true; }
	}
	// If
	else if (UMaterialExpressionIf* If = Cast<UMaterialExpressionIf>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { If->A.Expression = SourceExpr; If->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { If->B.Expression = SourceExpr; If->B.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("AGreaterThanB"), ESearchCase::IgnoreCase)) { If->AGreaterThanB.Expression = SourceExpr; If->AGreaterThanB.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("AEqualsB"), ESearchCase::IgnoreCase)) { If->AEqualsB.Expression = SourceExpr; If->AEqualsB.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("ALessThanB"), ESearchCase::IgnoreCase)) { If->ALessThanB.Expression = SourceExpr; If->ALessThanB.OutputIndex = OutputIndex; return true; }
	}
	// ComponentMask
	else if (UMaterialExpressionComponentMask* Mask = Cast<UMaterialExpressionComponentMask>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase)) { Mask->Input.Expression = SourceExpr; Mask->Input.OutputIndex = OutputIndex; return true; }
	}
	// Noise
	else if (UMaterialExpressionNoise* Noise = Cast<UMaterialExpressionNoise>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Position"), ESearchCase::IgnoreCase)) { Noise->Position.Expression = SourceExpr; Noise->Position.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("FilterWidth"), ESearchCase::IgnoreCase)) { Noise->FilterWidth.Expression = SourceExpr; Noise->FilterWidth.OutputIndex = OutputIndex; return true; }
	}
	// Panner
	else if (UMaterialExpressionPanner* Panner = Cast<UMaterialExpressionPanner>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Coordinate"), ESearchCase::IgnoreCase)) { Panner->Coordinate.Expression = SourceExpr; Panner->Coordinate.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("Time"), ESearchCase::IgnoreCase)) { Panner->Time.Expression = SourceExpr; Panner->Time.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("Speed"), ESearchCase::IgnoreCase)) { Panner->Speed.Expression = SourceExpr; Panner->Speed.OutputIndex = OutputIndex; return true; }
	}
	// AppendVector
	else if (UMaterialExpressionAppendVector* Append = Cast<UMaterialExpressionAppendVector>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Append->A.Expression = SourceExpr; Append->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Append->B.Expression = SourceExpr; Append->B.OutputIndex = OutputIndex; return true; }
	}
	// DotProduct
	else if (UMaterialExpressionDotProduct* Dot = Cast<UMaterialExpressionDotProduct>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Dot->A.Expression = SourceExpr; Dot->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Dot->B.Expression = SourceExpr; Dot->B.OutputIndex = OutputIndex; return true; }
	}
	// Min
	else if (UMaterialExpressionMin* Min = Cast<UMaterialExpressionMin>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Min->A.Expression = SourceExpr; Min->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Min->B.Expression = SourceExpr; Min->B.OutputIndex = OutputIndex; return true; }
	}
	// Max
	else if (UMaterialExpressionMax* Max = Cast<UMaterialExpressionMax>(TargetExpr))
	{
		if (InputName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) { Max->A.Expression = SourceExpr; Max->A.OutputIndex = OutputIndex; return true; }
		if (InputName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) { Max->B.Expression = SourceExpr; Max->B.OutputIndex = OutputIndex; return true; }
	}
	// SquareRoot (single input)
	else if (UMaterialExpressionSquareRoot* Sqrt = Cast<UMaterialExpressionSquareRoot>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase)) { Sqrt->Input.Expression = SourceExpr; Sqrt->Input.OutputIndex = OutputIndex; return true; }
	}
	// Abs
	else if (UMaterialExpressionAbs* Abs = Cast<UMaterialExpressionAbs>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase)) { Abs->Input.Expression = SourceExpr; Abs->Input.OutputIndex = OutputIndex; return true; }
	}
	// SceneDepth
	else if (UMaterialExpressionSceneDepth* SceneDepth = Cast<UMaterialExpressionSceneDepth>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase) || InputName.Equals(TEXT("UV"), ESearchCase::IgnoreCase) || InputName.Equals(TEXT("Coordinates"), ESearchCase::IgnoreCase))
		{
			SceneDepth->Input.Expression = SourceExpr;
			SceneDepth->Input.OutputIndex = OutputIndex;
			return true;
		}
	}
	// SceneTexture
	else if (UMaterialExpressionSceneTexture* SceneTexture = Cast<UMaterialExpressionSceneTexture>(TargetExpr))
	{
		if (InputName.Equals(TEXT("UV"), ESearchCase::IgnoreCase) || InputName.Equals(TEXT("Coordinates"), ESearchCase::IgnoreCase))
		{
			SceneTexture->Coordinates.Expression = SourceExpr;
			SceneTexture->Coordinates.OutputIndex = OutputIndex;
			return true;
		}
	}
	// DDX
	else if (UMaterialExpressionDDX* DDX = Cast<UMaterialExpressionDDX>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Value"), ESearchCase::IgnoreCase) || InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase))
		{ DDX->Value.Expression = SourceExpr; DDX->Value.OutputIndex = OutputIndex; return true; }
	}
	// DDY
	else if (UMaterialExpressionDDY* DDY = Cast<UMaterialExpressionDDY>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Value"), ESearchCase::IgnoreCase) || InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase))
		{ DDY->Value.Expression = SourceExpr; DDY->Value.OutputIndex = OutputIndex; return true; }
	}
	// Saturate
	else if (UMaterialExpressionSaturate* Saturate = Cast<UMaterialExpressionSaturate>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase)) { Saturate->Input.Expression = SourceExpr; Saturate->Input.OutputIndex = OutputIndex; return true; }
	}
	// OneMinus
	else if (UMaterialExpressionOneMinus* OneMinus = Cast<UMaterialExpressionOneMinus>(TargetExpr))
	{
		if (InputName.Equals(TEXT("Input"), ESearchCase::IgnoreCase)) { OneMinus->Input.Expression = SourceExpr; OneMinus->Input.OutputIndex = OutputIndex; return true; }
	}
	// Custom (inputs array)
	else if (UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(TargetExpr))
	{
		// Custom expressions have a dynamic inputs array
		// Try to find matching input by name
		for (FCustomInput& Input : Custom->Inputs)
		{
			if (Input.InputName.ToString().Equals(InputName, ESearchCase::IgnoreCase))
			{
				Input.Input.Expression = SourceExpr;
				Input.Input.OutputIndex = OutputIndex;
				return true;
			}
		}
		// Add new input if not found
		FCustomInput NewInput;
		NewInput.InputName = FName(*InputName);
		NewInput.Input.Expression = SourceExpr;
		NewInput.Input.OutputIndex = OutputIndex;
		Custom->Inputs.Add(NewInput);
		return true;
	}

	OutError = FString::Printf(TEXT("Unknown input '%s' for expression type '%s'"), *InputName, *TargetExpr->GetClass()->GetName());
	return false;
}

TSharedPtr<FJsonObject> FConnectMaterialExpressionsAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;

	// Get material
	UMaterial* Material = GetMaterialByNameOrCurrent(Params, Context, Error);
	if (!Material)
	{
		return CreateErrorResponse(Error, TEXT("material_not_found"));
	}

	// Get nodes
	FString SourceNodeName, TargetNodeName, TargetInput;
	GetRequiredString(Params, TEXT("source_node"), SourceNodeName, Error);
	GetRequiredString(Params, TEXT("target_node"), TargetNodeName, Error);
	GetRequiredString(Params, TEXT("target_input"), TargetInput, Error);

	int32 SourceOutputIndex = GetOptionalNumber(Params, TEXT("source_output_index"), 0);

	// Find source node
	UMaterialExpression* SourceExpr = Context.GetMaterialNode(SourceNodeName);
	if (!SourceExpr)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Source node '%s' not found. Make sure to use add_material_expression first."), *SourceNodeName),
			TEXT("source_not_found"));
	}

	// Find target node
	UMaterialExpression* TargetExpr = Context.GetMaterialNode(TargetNodeName);
	if (!TargetExpr)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Target node '%s' not found. Make sure to use add_material_expression first."), *TargetNodeName),
			TEXT("target_not_found"));
	}

	// Connect
	if (!ConnectToExpressionInput(SourceExpr, SourceOutputIndex, TargetExpr, TargetInput, Error))
	{
		return CreateErrorResponse(Error, TEXT("connection_failed"));
	}

	// Mark modified
	MarkMaterialModified(Material, Context);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("source_node"), SourceNodeName);
	Result->SetStringField(TEXT("target_node"), TargetNodeName);
	Result->SetStringField(TEXT("target_input"), TargetInput);

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FConnectToMaterialOutputAction
// =========================================================================

bool FConnectToMaterialOutputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString SourceNode, MaterialProperty;
	if (!GetRequiredString(Params, TEXT("source_node"), SourceNode, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("material_property"), MaterialProperty, OutError)) return false;
	return true;
}

bool FConnectToMaterialOutputAction::ConnectToMaterialProperty(UMaterial* Material, UMaterialExpression* SourceExpr,
	int32 OutputIndex, const FString& PropertyName, FString& OutError) const
{
	// Get editor-only data for main material outputs
	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	if (!EditorData)
	{
		OutError = TEXT("Could not access material editor data");
		return false;
	}

	// Map property name to material output
	if (PropertyName.Equals(TEXT("BaseColor"), ESearchCase::IgnoreCase))
	{
		EditorData->BaseColor.Expression = SourceExpr;
		EditorData->BaseColor.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("EmissiveColor"), ESearchCase::IgnoreCase) ||
		PropertyName.Equals(TEXT("Emissive"), ESearchCase::IgnoreCase))
	{
		EditorData->EmissiveColor.Expression = SourceExpr;
		EditorData->EmissiveColor.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("Metallic"), ESearchCase::IgnoreCase))
	{
		EditorData->Metallic.Expression = SourceExpr;
		EditorData->Metallic.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("Roughness"), ESearchCase::IgnoreCase))
	{
		EditorData->Roughness.Expression = SourceExpr;
		EditorData->Roughness.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("Specular"), ESearchCase::IgnoreCase))
	{
		EditorData->Specular.Expression = SourceExpr;
		EditorData->Specular.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("Normal"), ESearchCase::IgnoreCase))
	{
		EditorData->Normal.Expression = SourceExpr;
		EditorData->Normal.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("Opacity"), ESearchCase::IgnoreCase))
	{
		EditorData->Opacity.Expression = SourceExpr;
		EditorData->Opacity.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("OpacityMask"), ESearchCase::IgnoreCase))
	{
		EditorData->OpacityMask.Expression = SourceExpr;
		EditorData->OpacityMask.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("AmbientOcclusion"), ESearchCase::IgnoreCase) ||
		PropertyName.Equals(TEXT("AO"), ESearchCase::IgnoreCase))
	{
		EditorData->AmbientOcclusion.Expression = SourceExpr;
		EditorData->AmbientOcclusion.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("WorldPositionOffset"), ESearchCase::IgnoreCase) ||
		PropertyName.Equals(TEXT("WPO"), ESearchCase::IgnoreCase))
	{
		EditorData->WorldPositionOffset.Expression = SourceExpr;
		EditorData->WorldPositionOffset.OutputIndex = OutputIndex;
		return true;
	}
	if (PropertyName.Equals(TEXT("Refraction"), ESearchCase::IgnoreCase))
	{
		EditorData->Refraction.Expression = SourceExpr;
		EditorData->Refraction.OutputIndex = OutputIndex;
		return true;
	}

	OutError = FString::Printf(TEXT("Unknown material property '%s'. Valid: BaseColor, EmissiveColor, Metallic, Roughness, Specular, Normal, Opacity, OpacityMask, AmbientOcclusion, WorldPositionOffset, Refraction"), *PropertyName);
	return false;
}

TSharedPtr<FJsonObject> FConnectToMaterialOutputAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;

	// Get material
	UMaterial* Material = GetMaterialByNameOrCurrent(Params, Context, Error);
	if (!Material)
	{
		return CreateErrorResponse(Error, TEXT("material_not_found"));
	}

	// Get parameters
	FString SourceNodeName, MaterialProperty;
	GetRequiredString(Params, TEXT("source_node"), SourceNodeName, Error);
	GetRequiredString(Params, TEXT("material_property"), MaterialProperty, Error);

	int32 SourceOutputIndex = GetOptionalNumber(Params, TEXT("source_output_index"), 0);

	// Find source node
	UMaterialExpression* SourceExpr = Context.GetMaterialNode(SourceNodeName);
	if (!SourceExpr)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Source node '%s' not found"), *SourceNodeName),
			TEXT("source_not_found"));
	}

	// Connect to property
	if (!ConnectToMaterialProperty(Material, SourceExpr, SourceOutputIndex, MaterialProperty, Error))
	{
		return CreateErrorResponse(Error, TEXT("connection_failed"));
	}

	// Mark modified
	MarkMaterialModified(Material, Context);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("source_node"), SourceNodeName);
	Result->SetStringField(TEXT("material_property"), MaterialProperty);

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FSetMaterialExpressionPropertyAction
// =========================================================================

bool FSetMaterialExpressionPropertyAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString NodeName, PropertyName, PropertyValue;
	if (!GetRequiredString(Params, TEXT("node_name"), NodeName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("property_name"), PropertyName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("property_value"), PropertyValue, OutError)) return false;
	return true;
}

bool FSetMaterialExpressionPropertyAction::SetExpressionProperty(UMaterialExpression* Expression,
	const FString& PropertyName, const FString& PropertyValue, FString& OutError) const
{
	// Use reflection to set property
	FProperty* Prop = Expression->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (Prop)
	{
		void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Expression);

		// Handle different property types
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
		{
			FloatProp->SetPropertyValue(ValuePtr, FCString::Atof(*PropertyValue));
			return true;
		}
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
		{
			DoubleProp->SetPropertyValue(ValuePtr, FCString::Atod(*PropertyValue));
			return true;
		}
		if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			IntProp->SetPropertyValue(ValuePtr, FCString::Atoi(*PropertyValue));
			return true;
		}
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
		{
			BoolProp->SetPropertyValue(ValuePtr, PropertyValue.ToBool());
			return true;
		}
		if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			StrProp->SetPropertyValue(ValuePtr, PropertyValue);
			return true;
		}
		if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			NameProp->SetPropertyValue(ValuePtr, FName(*PropertyValue));
			return true;
		}
	}

	OutError = FString::Printf(TEXT("Property '%s' not found or unsupported type on expression"), *PropertyName);
	return false;
}

TSharedPtr<FJsonObject> FSetMaterialExpressionPropertyAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;

	// Get material
	UMaterial* Material = GetMaterialByNameOrCurrent(Params, Context, Error);
	if (!Material)
	{
		return CreateErrorResponse(Error, TEXT("material_not_found"));
	}

	// Get parameters
	FString NodeName, PropertyName, PropertyValue;
	GetRequiredString(Params, TEXT("node_name"), NodeName, Error);
	GetRequiredString(Params, TEXT("property_name"), PropertyName, Error);
	GetRequiredString(Params, TEXT("property_value"), PropertyValue, Error);

	// Find node
	UMaterialExpression* Expr = Context.GetMaterialNode(NodeName);
	if (!Expr)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node '%s' not found"), *NodeName),
			TEXT("node_not_found"));
	}

	// Set property
	if (!SetExpressionProperty(Expr, PropertyName, PropertyValue, Error))
	{
		return CreateErrorResponse(Error, TEXT("property_set_failed"));
	}

	// Mark modified
	MarkMaterialModified(Material, Context);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("node_name"), NodeName);
	Result->SetStringField(TEXT("property_name"), PropertyName);

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FCompileMaterialAction
// =========================================================================

bool FCompileMaterialAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString MaterialName;
	return GetRequiredString(Params, TEXT("material_name"), MaterialName, OutError);
}

TSharedPtr<FJsonObject> FCompileMaterialAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;
	FString MaterialName;
	GetRequiredString(Params, TEXT("material_name"), MaterialName, Error);

	// Find material
	UMaterial* Material = FindMaterial(MaterialName, Error);
	if (!Material)
	{
		return CreateErrorResponse(Error, TEXT("material_not_found"));
	}

	// Force recompilation
	Material->PreEditChange(nullptr);
	Material->PostEditChange();

	// Get compilation result
	bool bSuccess = true;
	int32 ErrorCount = 0;
	int32 WarningCount = 0;

	// Force recompile for rendering (async shader compilation)
	Material->ForceRecompileForRendering();

	// Reregister all components using this material
	FGlobalComponentReregisterContext RecreateComponents;

	// Save material
	Material->MarkPackageDirty();
	Context.MarkPackageDirty(Material->GetOutermost());

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("name"), MaterialName);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetNumberField(TEXT("error_count"), ErrorCount);
	Result->SetNumberField(TEXT("warning_count"), WarningCount);

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FCreateMaterialInstanceAction
// =========================================================================

bool FCreateMaterialInstanceAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString InstanceName, ParentMaterial;
	if (!GetRequiredString(Params, TEXT("instance_name"), InstanceName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("parent_material"), ParentMaterial, OutError)) return false;
	return true;
}

TSharedPtr<FJsonObject> FCreateMaterialInstanceAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;
	FString InstanceName, ParentMaterialName;
	GetRequiredString(Params, TEXT("instance_name"), InstanceName, Error);
	GetRequiredString(Params, TEXT("parent_material"), ParentMaterialName, Error);

	FString Path = GetOptionalString(Params, TEXT("path"), TEXT("/Game/Materials"));

	// Find parent material
	UMaterial* ParentMaterial = FindMaterial(ParentMaterialName, Error);
	if (!ParentMaterial)
	{
		return CreateErrorResponse(Error, TEXT("parent_not_found"));
	}

	// Build package path
	FString InstancePackagePath = Path / InstanceName;

	// Clean up existing
	UPackage* ExistingPackage = FindPackage(nullptr, *InstancePackagePath);
	if (ExistingPackage)
	{
		UMaterialInstanceConstant* ExistingInstance = FindObject<UMaterialInstanceConstant>(ExistingPackage, *InstanceName);
		if (ExistingInstance)
		{
			FString TempName = FString::Printf(TEXT("%s_TEMP_%d"), *InstanceName, FMath::Rand());
			ExistingInstance->Rename(*TempName, GetTransientPackage(),
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
			ExistingInstance->MarkAsGarbage();
			ExistingPackage->MarkAsGarbage();
		}
	}

	if (UEditorAssetLibrary::DoesAssetExist(InstancePackagePath))
	{
		UEditorAssetLibrary::DeleteAsset(InstancePackagePath);
	}

	// Create package
	UPackage* Package = CreatePackage(*InstancePackagePath);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package"), TEXT("package_creation_failed"));
	}
	Package->FullyLoad();

	// Create material instance using factory
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = ParentMaterial;

	UMaterialInstanceConstant* NewInstance = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(
		UMaterialInstanceConstant::StaticClass(), Package, *InstanceName,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!NewInstance)
	{
		return CreateErrorResponse(TEXT("Failed to create material instance"), TEXT("creation_failed"));
	}

	// Set scalar parameters
	if (Params->HasField(TEXT("scalar_parameters")))
	{
		TSharedPtr<FJsonObject> ScalarParams = Params->GetObjectField(TEXT("scalar_parameters"));
		for (const auto& Pair : ScalarParams->Values)
		{
			NewInstance->SetScalarParameterValueEditorOnly(FName(*Pair.Key), Pair.Value->AsNumber());
		}
	}

	// Set vector parameters
	if (Params->HasField(TEXT("vector_parameters")))
	{
		TSharedPtr<FJsonObject> VectorParams = Params->GetObjectField(TEXT("vector_parameters"));
		for (const auto& Pair : VectorParams->Values)
		{
			const TArray<TSharedPtr<FJsonValue>>* Arr;
			if (Pair.Value->TryGetArray(Arr) && Arr->Num() >= 3)
			{
				FLinearColor Color(
					(*Arr)[0]->AsNumber(),
					(*Arr)[1]->AsNumber(),
					(*Arr)[2]->AsNumber(),
					Arr->Num() > 3 ? (*Arr)[3]->AsNumber() : 1.0f);
				NewInstance->SetVectorParameterValueEditorOnly(FName(*Pair.Key), Color);
			}
		}
	}

	// Register and mark dirty
	Package->SetDirtyFlag(true);
	NewInstance->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewInstance);
	Context.MarkPackageDirty(Package);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("name"), InstanceName);
	Result->SetStringField(TEXT("path"), InstancePackagePath);
	Result->SetStringField(TEXT("parent"), ParentMaterialName);

	return CreateSuccessResponse(Result);
}

// =========================================================================
// FSetMaterialPropertyAction
// =========================================================================

bool FSetMaterialPropertyAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString MaterialName, PropertyName, PropertyValue;
	if (!GetRequiredString(Params, TEXT("material_name"), MaterialName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("property_name"), PropertyName, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("property_value"), PropertyValue, OutError)) return false;
	return true;
}

TOptional<EMaterialShadingModel> FSetMaterialPropertyAction::ResolveShadingModel(const FString& ShadingModelString) const
{
	InitShadingModelMap();
	if (EMaterialShadingModel* Found = ShadingModelMap.Find(ShadingModelString))
	{
		return *Found;
	}
	return TOptional<EMaterialShadingModel>();
}

TSharedPtr<FJsonObject> FSetMaterialPropertyAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;
	FString MaterialName, PropertyName, PropertyValue;
	GetRequiredString(Params, TEXT("material_name"), MaterialName, Error);
	GetRequiredString(Params, TEXT("property_name"), PropertyName, Error);
	GetRequiredString(Params, TEXT("property_value"), PropertyValue, Error);

	// Find material
	UMaterial* Material = FindMaterial(MaterialName, Error);
	if (!Material)
	{
		return CreateErrorResponse(Error, TEXT("material_not_found"));
	}

	// Property handlers map (property name -> handler lambda)
	using FPropertyHandler = TFunction<bool(UMaterial*, const FString&, FString&)>;
	static TMap<FString, FPropertyHandler> PropertyHandlers;

	if (PropertyHandlers.Num() == 0)
	{
		PropertyHandlers.Add(TEXT("ShadingModel"), [this](UMaterial* Mat, const FString& Value, FString& OutErr) -> bool
		{
			TOptional<EMaterialShadingModel> Model = ResolveShadingModel(Value);
			if (!Model.IsSet())
			{
				OutErr = FString::Printf(TEXT("Invalid ShadingModel '%s'. Valid: Unlit, DefaultLit, Subsurface, PreintegratedSkin, ClearCoat, SubsurfaceProfile, TwoSidedFoliage, Hair, Cloth, Eye"), *Value);
				return false;
			}
			Mat->SetShadingModel(Model.GetValue());
			return true;
		});

		PropertyHandlers.Add(TEXT("TwoSided"), [](UMaterial* Mat, const FString& Value, FString& OutErr) -> bool
		{
			Mat->TwoSided = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1");
			return true;
		});

		PropertyHandlers.Add(TEXT("BlendMode"), [](UMaterial* Mat, const FString& Value, FString& OutErr) -> bool
		{
			InitBlendModeMap();
			if (EBlendMode* Found = BlendModeMap.Find(Value))
			{
				Mat->BlendMode = *Found;
				return true;
			}
			OutErr = FString::Printf(TEXT("Invalid BlendMode '%s'. Valid: Opaque, Masked, Translucent, Additive, Modulate, AlphaComposite, AlphaHoldout"), *Value);
			return false;
		});

		PropertyHandlers.Add(TEXT("DitheredLODTransition"), [](UMaterial* Mat, const FString& Value, FString& OutErr) -> bool
		{
			Mat->DitheredLODTransition = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1");
			return true;
		});

		PropertyHandlers.Add(TEXT("AllowNegativeEmissiveColor"), [](UMaterial* Mat, const FString& Value, FString& OutErr) -> bool
		{
			Mat->bAllowNegativeEmissiveColor = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1");
			return true;
		});

		PropertyHandlers.Add(TEXT("OpacityMaskClipValue"), [](UMaterial* Mat, const FString& Value, FString& OutErr) -> bool
		{
			Mat->OpacityMaskClipValue = FCString::Atof(*Value);
			return true;
		});
	}

	// Find and execute the property handler
	FPropertyHandler* Handler = PropertyHandlers.Find(PropertyName);
	if (!Handler)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Unknown material property '%s'. Supported: ShadingModel, TwoSided, BlendMode, DitheredLODTransition, AllowNegativeEmissiveColor, OpacityMaskClipValue"), *PropertyName),
			TEXT("unknown_property"));
	}

	FString HandlerError;
	if (!(*Handler)(Material, PropertyValue, HandlerError))
	{
		return CreateErrorResponse(HandlerError, TEXT("property_set_failed"));
	}

	// Mark material as modified and trigger recompilation
	MarkMaterialModified(Material, Context);

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("material_name"), MaterialName);
	Result->SetStringField(TEXT("property_name"), PropertyName);
	Result->SetStringField(TEXT("property_value"), PropertyValue);

	return CreateSuccessResponse(Result);
}


// =========================================================================
// FCreatePostProcessVolumeAction
// =========================================================================

bool FCreatePostProcessVolumeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Name;
	return GetRequiredString(Params, TEXT("name"), Name, OutError);
}

FVector FCreatePostProcessVolumeAction::GetVectorFromParams(const TSharedPtr<FJsonObject>& Params, const FString& FieldName) const
{
	FVector Result = FVector::ZeroVector;
	const TArray<TSharedPtr<FJsonValue>>* Arr = GetOptionalArray(Params, FieldName);
	if (Arr && Arr->Num() >= 3)
	{
		Result.X = (*Arr)[0]->AsNumber();
		Result.Y = (*Arr)[1]->AsNumber();
		Result.Z = (*Arr)[2]->AsNumber();
	}
	return Result;
}

TSharedPtr<FJsonObject> FCreatePostProcessVolumeAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;
	FString ActorName;
	GetRequiredString(Params, TEXT("name"), ActorName, Error);

	FVector Location = GetVectorFromParams(Params, TEXT("location"));
	bool bInfiniteExtent = GetOptionalBool(Params, TEXT("infinite_extent"), true);
	float Priority = GetOptionalNumber(Params, TEXT("priority"), 0.0);

	// Get world
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No world available"), TEXT("no_world"));
	}

	// Find and delete existing actor with same name using safe method
	TArray<AActor*> AllPPVs;
	UGameplayStatics::GetAllActorsOfClass(World, APostProcessVolume::StaticClass(), AllPPVs);
	for (AActor* Actor : AllPPVs)
	{
		if (Actor && (Actor->GetActorLabel() == ActorName || Actor->GetName() == ActorName))
		{
			// Deselect before destroying to avoid editor issues
			GEditor->SelectNone(true, true);
			World->DestroyActor(Actor);
			break;
		}
	}

	// Spawn post process volume
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*ActorName);
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>(Location, FRotator::ZeroRotator, SpawnParams);
	if (!Volume)
	{
		return CreateErrorResponse(TEXT("Failed to spawn post process volume"), TEXT("spawn_failed"));
	}

	// Set properties
	Volume->bUnbound = bInfiniteExtent;
	Volume->Priority = Priority;
	Volume->SetActorLabel(ActorName);

	// Add materials
	const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = GetOptionalArray(Params, TEXT("post_process_materials"));
	if (MaterialsArray)
	{
		for (const TSharedPtr<FJsonValue>& MatValue : *MaterialsArray)
		{
			FString MatName = MatValue->AsString();
			FString MatError;
			UMaterial* Mat = FindMaterial(MatName, MatError);
			if (Mat)
			{
				// Add as weighted blendable
				Volume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Mat));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FCreatePostProcessVolumeAction: Material '%s' not found"), *MatName);
			}
		}
	}

	// Update context
	Context.LastCreatedActorName = ActorName;

	// Mark level dirty
	World->MarkPackageDirty();

	// Build response
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("name"), ActorName);

	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	Result->SetArrayField(TEXT("location"), LocationArray);

	Result->SetBoolField(TEXT("infinite_extent"), bInfiniteExtent);
	Result->SetNumberField(TEXT("priority"), Priority);

	return CreateSuccessResponse(Result);
}
