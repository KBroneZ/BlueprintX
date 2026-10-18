// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXBlueprintSaver.h"

#include "BlueprintX.h"
#include "BXAreaCollector.h"
#include "BXCornerMarker.h"
#include "Buildables/FGBuildableFoundation.h"
#include "FGBlueprintSubsystem.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"

namespace BXSaver
{
	// Blueprint dimensions are counted in 8 m cells, like the vanilla Blueprint Designer
	// (designer size = mDimensions * 800).
	constexpr double CellSize = 800.0;

	constexpr float BelowMargin = FBXArea::DefaultBelowMargin;

	constexpr int32 MaxNameLength = 64;

	// Sanity limits so one request cannot make the server walk the whole map in one frame.
	constexpr float MaxExtraHeight = FBXSaveRequest::MaxExtraHeight;
	constexpr double MaxSideLength = 1200.0 * 100.0;
	const TCHAR* const FileExtension = TEXT(".sbp");
}

namespace
{
	/** Lowest point (Frame space Z) of what the blueprint stands on: foundation bottoms, other pivots. */
	double FindFloorZ(const FBXArea& Area, const TArray<AFGBuildable*>& Buildables, double LowerCornerZ)
	{
		double FloorZ = LowerCornerZ;
		for (const AFGBuildable* Buildable : Buildables)
		{
			double BottomZ = Area.Frame.InverseTransformPosition(Buildable->GetActorLocation()).Z;
			if (const AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(Buildable))
			{
				BottomZ -= Foundation->mHeight * 0.5;
			}
			FloorZ = FMath::Min(FloorZ, BottomZ);
		}
		return FloorZ;
	}

	int32 ToCells(double Length)
	{
		return FMath::Max(1, FMath::CeilToInt32(Length / BXSaver::CellSize - KINDA_SMALL_NUMBER));
	}

	FString ValidateRequest(const UWorld* World, const ABXCornerMarker* CornerA, const ABXCornerMarker* CornerB,
		const FString& SafeName, AFGBlueprintSubsystem* BlueprintSubsystem)
	{
		if (!IsValid(CornerA) || !IsValid(CornerB) || CornerA->GetWorld() != World || CornerB->GetWorld() != World)
		{
			return TEXT("Both corner markers must exist.");
		}
		if (CornerA == CornerB)
		{
			return TEXT("Pick two different corner markers.");
		}
		if (SafeName.IsEmpty())
		{
			return TEXT("The blueprint needs a name.");
		}
		if (SafeName.Len() > BXSaver::MaxNameLength)
		{
			return FString::Printf(TEXT("The name is too long (max %d characters)."), BXSaver::MaxNameLength);
		}
		if (BlueprintSubsystem == nullptr)
		{
			return TEXT("Blueprint system not ready.");
		}
		const FString FilePath = FPaths::Combine(BlueprintSubsystem->GetSessionBlueprintPath(), SafeName + BXSaver::FileExtension);
		if (BlueprintSubsystem->GetBlueprintDescriptorByNameString(SafeName) != nullptr || IFileManager::Get().FileExists(*FilePath))
		{
			return FString::Printf(TEXT("A blueprint named \"%s\" already exists."), *SafeName);
		}
		return FString();
	}
}

FBXSaveResult FBXBlueprintSaver::SaveArea(UWorld* World, ABXCornerMarker* CornerA, ABXCornerMarker* CornerB,
	const FBXSaveRequest& Request)
{
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		return FBXSaveResult::Fail(TEXT("Blueprints are saved by the server."));
	}

	const double StartSeconds = FPlatformTime::Seconds();
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
	const FString SafeName = AFGBlueprintSubsystem::SanitizeBlueprintFileName(Request.BlueprintName.TrimStartAndEnd());
	const FString Error = ValidateRequest(World, CornerA, CornerB, SafeName, BlueprintSubsystem);
	if (!Error.IsEmpty())
	{
		return FBXSaveResult::Fail(Error);
	}

	const float ExtraHeight = FMath::IsFinite(Request.ExtraHeight)
		? FMath::Clamp(Request.ExtraHeight, 0.0f, BXSaver::MaxExtraHeight) : 0.0f;
	const FBXArea Area = FBXArea::FromCorners(*CornerA, *CornerB, ExtraHeight, BXSaver::BelowMargin);
	const FVector AreaSize = Area.LocalBox.GetSize();
	if (FMath::Max3(AreaSize.X, AreaSize.Y, AreaSize.Z) > BXSaver::MaxSideLength)
	{
		return FBXSaveResult::Fail(FString::Printf(TEXT("The area is too big (max %.0f m per side)."),
			BXSaver::MaxSideLength / 100.0));
	}
	FBXCollection Collection = FBXAreaCollector::Collect(World, Area);
	if (Collection.Buildables.IsEmpty())
	{
		Collection.Release(World);
		return FBXSaveResult::Fail(TEXT("No buildings found between the two corner markers."));
	}

	// Origin like the vanilla designer: centre of the footprint, at floor level, box rotation.
	const FBox& Box = Area.LocalBox;
	const double LowerCornerZ = Box.Min.Z + BXSaver::BelowMargin;
	const double FloorZ = FindFloorZ(Area, Collection.Buildables, LowerCornerZ);
	const FVector LocalOrigin(Box.GetCenter().X, Box.GetCenter().Y, FloorZ);
	const FTransform Origin(Area.Frame.GetRotation(), Area.Frame.TransformPosition(LocalOrigin));
	const FVector BoxSize = Box.GetSize();
	const FIntVector Dimensions(ToCells(BoxSize.X), ToCells(BoxSize.Y), ToCells(Box.Max.Z - FloorZ));

	FBlueprintRecord Record;
	Record.BlueprintName = SafeName;
	Record.BlueprintDescription = Request.Description.Left(1024);
	BlueprintSubsystem->AssignCategoryAndSubcategoryToBlueprintRecord(Record);

	// Writes the .sbp/.sbpcfg files and updates the server manifest (clients download from it).
	FBlueprintHeader Header = BlueprintSubsystem->WriteBlueprintToArchive(Record, Origin, Collection.Buildables, Dimensions);
	const int32 NumSaved = Collection.Buildables.Num();
	const int32 NumLightweights = Collection.NumLightweights;
	const int32 NumExcluded = Collection.NumExcluded;
	Collection.Release(World);

	const FString FilePath = FPaths::Combine(BlueprintSubsystem->GetSessionBlueprintPath(), SafeName + BXSaver::FileExtension);
	if (!Header.IsValid() || !IFileManager::Get().FileExists(*FilePath))
	{
		UE_LOG(LogBlueprintX, Error, TEXT("Blueprint '%s' was not written (expected %s)."), *SafeName, *FilePath);
		return FBXSaveResult::Fail(TEXT("The game could not write the blueprint file. See the log."));
	}

	if (UFGBlueprintDescriptor* Descriptor = BlueprintSubsystem->CreateBlueprintDescriptor(Record, Header))
	{
		BlueprintSubsystem->FindSubcategoryAndAddBlueprintDescriptor(Descriptor, Record);
	}

	UE_LOG(LogBlueprintX, Display,
		TEXT("Saved blueprint '%s': %d buildables (%d lightweight, %d excluded), dimensions %s, origin %s, file %s (%lld bytes), %.1f ms."),
		*SafeName, NumSaved, NumLightweights, NumExcluded, *Dimensions.ToString(), *Origin.GetLocation().ToString(),
		*FilePath, IFileManager::Get().FileSize(*FilePath), (FPlatformTime::Seconds() - StartSeconds) * 1000.0);

	return {true, NumSaved, FString::Printf(TEXT("Blueprint saved: \"%s\", %d buildings."), *SafeName, NumSaved)};
}
