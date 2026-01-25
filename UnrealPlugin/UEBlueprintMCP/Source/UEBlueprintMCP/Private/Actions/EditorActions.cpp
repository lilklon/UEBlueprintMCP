// Copyright (c) 2025 zolnoor. All rights reserved.

#include "Actions/EditorActions.h"
#include "MCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "FileHelpers.h"
#include "UObject/SavePackage.h"


// Helper to find actor by name
static AActor* FindActorByName(UWorld* World, const FString& ActorName)
{
	if (!World) return nullptr;

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

	for (AActor* Actor : AllActors)
	{
		if (Actor && Actor->GetName() == ActorName)
		{
			return Actor;
		}
	}
	return nullptr;
}


// ============================================================================
// FGetActorsInLevelAction
// ============================================================================

TSharedPtr<FJsonObject> FGetActorsInLevelAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

	TArray<TSharedPtr<FJsonValue>> ActorArray;
	for (AActor* Actor : AllActors)
	{
		if (Actor)
		{
			ActorArray.Add(FMCPCommonUtils::ActorToJsonValue(Actor));
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("actors"), ActorArray);
	return CreateSuccessResponse(Result);
}


// ============================================================================
// FFindActorsByNameAction
// ============================================================================

bool FFindActorsByNameAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Pattern;
	return GetRequiredString(Params, TEXT("pattern"), Pattern, OutError);
}

TSharedPtr<FJsonObject> FFindActorsByNameAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Pattern, Error;
	GetRequiredString(Params, TEXT("pattern"), Pattern, Error);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

	TArray<TSharedPtr<FJsonValue>> MatchingActors;
	for (AActor* Actor : AllActors)
	{
		if (Actor && Actor->GetName().Contains(Pattern))
		{
			MatchingActors.Add(FMCPCommonUtils::ActorToJsonValue(Actor));
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("actors"), MatchingActors);
	return CreateSuccessResponse(Result);
}


// ============================================================================
// FSpawnActorAction
// ============================================================================

bool FSpawnActorAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Name, Type;
	if (!GetRequiredString(Params, TEXT("name"), Name, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("type"), Type, OutError)) return false;
	return true;
}

TSharedPtr<FJsonObject> FSpawnActorAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActorName, ActorType, Error;
	GetRequiredString(Params, TEXT("name"), ActorName, Error);
	GetRequiredString(Params, TEXT("type"), ActorType, Error);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	// Resolve actor class
	UClass* ActorClass = ResolveActorClass(ActorType);
	if (!ActorClass)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Unknown actor type: %s"), *ActorType),
			TEXT("invalid_type")
		);
	}

	// Delete existing actor with same name
	AActor* Existing = FindActorByName(World, ActorName);
	if (Existing)
	{
		World->DestroyActor(Existing);
	}

	// Parse transform
	FVector Location = FMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
	FRotator Rotation = FMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
	FVector Scale = Params->HasField(TEXT("scale")) ?
		FMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")) : FVector(1, 1, 1);

	// Spawn
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*ActorName);
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	AActor* NewActor = World->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParams);
	if (!NewActor)
	{
		return CreateErrorResponse(TEXT("Failed to spawn actor"), TEXT("spawn_failed"));
	}

	NewActor->SetActorScale3D(Scale);
	NewActor->SetActorLabel(*ActorName);
	Context.LastCreatedActorName = ActorName;

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Spawned actor '%s' of type '%s'"), *ActorName, *ActorType);

	return CreateSuccessResponse(FMCPCommonUtils::ActorToJsonObject(NewActor));
}

UClass* FSpawnActorAction::ResolveActorClass(const FString& TypeName) const
{
	if (TypeName == TEXT("StaticMeshActor")) return AStaticMeshActor::StaticClass();
	if (TypeName == TEXT("PointLight")) return APointLight::StaticClass();
	if (TypeName == TEXT("SpotLight")) return ASpotLight::StaticClass();
	if (TypeName == TEXT("DirectionalLight")) return ADirectionalLight::StaticClass();
	if (TypeName == TEXT("CameraActor")) return ACameraActor::StaticClass();
	if (TypeName == TEXT("Actor")) return AActor::StaticClass();
	return nullptr;
}


// ============================================================================
// FDeleteActorAction
// ============================================================================

bool FDeleteActorAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Name;
	return GetRequiredString(Params, TEXT("name"), Name, OutError);
}

TSharedPtr<FJsonObject> FDeleteActorAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActorName, Error;
	GetRequiredString(Params, TEXT("name"), ActorName, Error);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	AActor* Actor = FindActorByName(World, ActorName);
	if (!Actor)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Actor not found: %s"), *ActorName),
			TEXT("not_found")
		);
	}

	// Store info before deletion
	TSharedPtr<FJsonObject> ActorInfo = FMCPCommonUtils::ActorToJsonObject(Actor);

	// Delete
	Actor->Destroy();

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Deleted actor '%s'"), *ActorName);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetObjectField(TEXT("deleted_actor"), ActorInfo);
	return CreateSuccessResponse(Result);
}


// ============================================================================
// FSetActorTransformAction
// ============================================================================

bool FSetActorTransformAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Name;
	return GetRequiredString(Params, TEXT("name"), Name, OutError);
}

TSharedPtr<FJsonObject> FSetActorTransformAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActorName, Error;
	GetRequiredString(Params, TEXT("name"), ActorName, Error);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	AActor* Actor = FindActorByName(World, ActorName);
	if (!Actor)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Actor not found: %s"), *ActorName),
			TEXT("not_found")
		);
	}

	// Update transform
	FTransform Transform = Actor->GetTransform();

	if (Params->HasField(TEXT("location")))
	{
		Transform.SetLocation(FMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
	}
	if (Params->HasField(TEXT("rotation")))
	{
		Transform.SetRotation(FQuat(FMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
	}
	if (Params->HasField(TEXT("scale")))
	{
		Transform.SetScale3D(FMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
	}

	Actor->SetActorTransform(Transform);
	Actor->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Set transform on actor '%s'"), *ActorName);

	return CreateSuccessResponse(FMCPCommonUtils::ActorToJsonObject(Actor));
}


// ============================================================================
// FGetActorPropertiesAction
// ============================================================================

bool FGetActorPropertiesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Name;
	return GetRequiredString(Params, TEXT("name"), Name, OutError);
}

TSharedPtr<FJsonObject> FGetActorPropertiesAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActorName, Error;
	GetRequiredString(Params, TEXT("name"), ActorName, Error);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	AActor* Actor = FindActorByName(World, ActorName);
	if (!Actor)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Actor not found: %s"), *ActorName),
			TEXT("not_found")
		);
	}

	return CreateSuccessResponse(FMCPCommonUtils::ActorToJsonObject(Actor));
}


// ============================================================================
// FSetActorPropertyAction
// ============================================================================

bool FSetActorPropertyAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Name, PropertyName;
	if (!GetRequiredString(Params, TEXT("name"), Name, OutError)) return false;
	if (!GetRequiredString(Params, TEXT("property_name"), PropertyName, OutError)) return false;
	if (!Params->HasField(TEXT("property_value")))
	{
		OutError = TEXT("Missing 'property_value' parameter");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FSetActorPropertyAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString ActorName, PropertyName, Error;
	GetRequiredString(Params, TEXT("name"), ActorName, Error);
	GetRequiredString(Params, TEXT("property_name"), PropertyName, Error);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	AActor* Actor = FindActorByName(World, ActorName);
	if (!Actor)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Actor not found: %s"), *ActorName),
			TEXT("not_found")
		);
	}

	TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));

	FString ErrorMessage;
	if (!FMCPCommonUtils::SetObjectProperty(Actor, PropertyName, JsonValue, ErrorMessage))
	{
		return CreateErrorResponse(ErrorMessage, TEXT("property_set_failed"));
	}

	Actor->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP: Set property '%s' on actor '%s'"), *PropertyName, *ActorName);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("actor"), ActorName);
	Result->SetStringField(TEXT("property"), PropertyName);
	Result->SetBoolField(TEXT("success"), true);
	return CreateSuccessResponse(Result);
}


// ============================================================================
// FFocusViewportAction
// ============================================================================

bool FFocusViewportAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	bool HasTarget = Params->HasField(TEXT("target"));
	bool HasLocation = Params->HasField(TEXT("location"));

	if (!HasTarget && !HasLocation)
	{
		OutError = TEXT("Either 'target' or 'location' must be provided");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FFocusViewportAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FLevelEditorViewportClient* ViewportClient = nullptr;
	if (GEditor && GEditor->GetActiveViewport())
	{
		ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	}

	if (!ViewportClient)
	{
		return CreateErrorResponse(TEXT("Failed to get active viewport"), TEXT("no_viewport"));
	}

	float Distance = GetOptionalNumber(Params, TEXT("distance"), 1000.0f);
	FVector TargetLocation(0, 0, 0);

	if (Params->HasField(TEXT("target")))
	{
		FString TargetActorName = Params->GetStringField(TEXT("target"));
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		AActor* TargetActor = FindActorByName(World, TargetActorName);

		if (!TargetActor)
		{
			return CreateErrorResponse(
				FString::Printf(TEXT("Actor not found: %s"), *TargetActorName),
				TEXT("not_found")
			);
		}
		TargetLocation = TargetActor->GetActorLocation();
	}
	else
	{
		TargetLocation = FMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
	}

	ViewportClient->SetViewLocation(TargetLocation - FVector(Distance, 0, 0));

	if (Params->HasField(TEXT("orientation")))
	{
		ViewportClient->SetViewRotation(FMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation")));
	}

	ViewportClient->Invalidate();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	return CreateSuccessResponse(Result);
}


// ============================================================================
// FGetViewportTransformAction
// ============================================================================

TSharedPtr<FJsonObject> FGetViewportTransformAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FLevelEditorViewportClient* ViewportClient = nullptr;
	if (GEditor && GEditor->GetActiveViewport())
	{
		ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	}

	if (!ViewportClient)
	{
		return CreateErrorResponse(TEXT("Failed to get active viewport"), TEXT("no_viewport"));
	}

	FVector Location = ViewportClient->GetViewLocation();
	FRotator Rotation = ViewportClient->GetViewRotation();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	Result->SetArrayField(TEXT("location"), LocationArray);

	TArray<TSharedPtr<FJsonValue>> RotationArray;
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
	Result->SetArrayField(TEXT("rotation"), RotationArray);

	return CreateSuccessResponse(Result);
}


// ============================================================================
// FSetViewportTransformAction
// ============================================================================

bool FSetViewportTransformAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	if (!Params->HasField(TEXT("location")) && !Params->HasField(TEXT("rotation")))
	{
		OutError = TEXT("At least 'location' or 'rotation' must be provided");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FSetViewportTransformAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FLevelEditorViewportClient* ViewportClient = nullptr;
	if (GEditor && GEditor->GetActiveViewport())
	{
		ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	}

	if (!ViewportClient)
	{
		return CreateErrorResponse(TEXT("Failed to get active viewport"), TEXT("no_viewport"));
	}

	if (Params->HasField(TEXT("location")))
	{
		ViewportClient->SetViewLocation(FMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
	}
	if (Params->HasField(TEXT("rotation")))
	{
		ViewportClient->SetViewRotation(FMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
	}

	ViewportClient->Invalidate();

	// Return new state
	FVector Location = ViewportClient->GetViewLocation();
	FRotator Rotation = ViewportClient->GetViewRotation();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	Result->SetArrayField(TEXT("location"), LocationArray);

	TArray<TSharedPtr<FJsonValue>> RotationArray;
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
	Result->SetArrayField(TEXT("rotation"), RotationArray);

	return CreateSuccessResponse(Result);
}


// ============================================================================
// FSaveAllAction
// ============================================================================

TSharedPtr<FJsonObject> FSaveAllAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	bool bOnlyMaps = GetOptionalBool(Params, TEXT("only_maps"), false);

	int32 SavedCount = 0;
	TArray<FString> SavedPackages;

	if (bOnlyMaps)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (World)
		{
			UPackage* WorldPackage = World->GetOutermost();
			if (WorldPackage && WorldPackage->IsDirty())
			{
				FString PackageFilename;
				if (FPackageName::TryConvertLongPackageNameToFilename(
					WorldPackage->GetName(), PackageFilename, FPackageName::GetMapPackageExtension()))
				{
					FSavePackageArgs SaveArgs;
					SaveArgs.TopLevelFlags = RF_Standalone;
					if (UPackage::SavePackage(WorldPackage, World, *PackageFilename, SaveArgs))
					{
						SavedCount++;
						SavedPackages.Add(WorldPackage->GetName());
					}
				}
			}
		}
	}
	else
	{
		TArray<UPackage*> DirtyPackages;
		FEditorFileUtils::GetDirtyPackages(DirtyPackages);

		for (UPackage* Package : DirtyPackages)
		{
			if (!Package) continue;

			FString PackageFilename;
			FString PackageName = Package->GetName();
			bool bIsMap = Package->ContainsMap();
			FString Extension = bIsMap ?
				FPackageName::GetMapPackageExtension() :
				FPackageName::GetAssetPackageExtension();

			if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, PackageFilename, Extension))
			{
				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Standalone;

				UObject* AssetToSave = bIsMap ? Package->FindAssetInPackage() : nullptr;

				if (UPackage::SavePackage(Package, AssetToSave, *PackageFilename, SaveArgs))
				{
					SavedCount++;
					SavedPackages.Add(PackageName);
					UE_LOG(LogTemp, Log, TEXT("UEBlueprintMCP SaveAll: Saved %s"), *PackageName);
				}
			}
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("saved_count"), SavedCount);

	TArray<TSharedPtr<FJsonValue>> PackagesArray;
	for (const FString& PkgName : SavedPackages)
	{
		PackagesArray.Add(MakeShared<FJsonValueString>(PkgName));
	}
	Result->SetArrayField(TEXT("saved_packages"), PackagesArray);

	return CreateSuccessResponse(Result);
}
