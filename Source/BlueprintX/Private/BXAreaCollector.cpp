// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXAreaCollector.h"

#include "BXCornerMarker.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableBlueprintDesigner.h"
#include "Buildables/FGBuildableElevator.h"
#include "Buildables/FGBuildableElevatorFloorStop.h"
#include "Buildables/FGBuildableHubTerminal.h"
#include "Buildables/FGBuildableSpaceElevator.h"
#include "Buildables/FGBuildableTradingPost.h"
#include "FGBlueprintSubsystem.h"
#include "FGBuildableSubsystem.h"
#include "FGLightweightBuildableBlueprintLibrary.h"
#include "FGLightweightBuildableSubsystem.h"

namespace BXAreaCollector
{
	// Pivots exactly on a box face (snapped to the same grid as the markers) count as inside.
	constexpr double EdgeTolerance = 1.0;
}

FBXArea FBXArea::FromCorners(const ABXCornerMarker& CornerA, const ABXCornerMarker& CornerB,
	float ExtraHeight, float BelowMargin)
{
	FBXArea Area;
	Area.Frame = FTransform(FRotator(0.0, CornerA.GetActorRotation().Yaw, 0.0), CornerA.GetCornerLocation());

	const FVector LocalB = Area.Frame.InverseTransformPosition(CornerB.GetCornerLocation());
	Area.LocalBox = FBox(FVector::Min(FVector::ZeroVector, LocalB), FVector::Max(FVector::ZeroVector, LocalB));
	Area.LocalBox.Min.Z -= FMath::Max(BelowMargin, 0.0f);
	Area.LocalBox.Max.Z += FMath::Max(ExtraHeight, 0.0f);
	return Area;
}

bool FBXArea::ContainsWorldLocation(const FVector& WorldLocation) const
{
	const FVector Local = Frame.InverseTransformPosition(WorldLocation);
	return LocalBox.ExpandBy(BXAreaCollector::EdgeTolerance).IsInsideOrOn(Local);
}

bool FBXAreaCollector::IsExcludedClass(UWorld* World, const UClass* BuildableClass)
{
	if (BuildableClass == nullptr)
	{
		return true;
	}

	static const UClass* const NeverInBlueprint[] = {
		ABXCornerMarker::StaticClass(),
		AFGBuildableBlueprintDesigner::StaticClass(),
		AFGBuildableTradingPost::StaticClass(),
		AFGBuildableHubTerminal::StaticClass(),
		AFGBuildableSpaceElevator::StaticClass(),
		AFGBuildableElevator::StaticClass(),
		AFGBuildableElevatorFloorStop::StaticClass(),
	};
	for (const UClass* Excluded : NeverInBlueprint)
	{
		if (BuildableClass->IsChildOf(Excluded))
		{
			return true;
		}
	}

	// Same list the vanilla Blueprint Designer refuses.
	if (const AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World))
	{
		for (const TSubclassOf<AFGBuildable>& Blacklisted : BlueprintSubsystem->mBlacklistedDesignerBuildables)
		{
			if (Blacklisted != nullptr && BuildableClass->IsChildOf(Blacklisted))
			{
				return true;
			}
		}
	}
	return false;
}

bool FBXAreaCollector::IsExcluded(UWorld* World, const AFGBuildable* Buildable)
{
	return !IsValid(Buildable)
		|| Buildable->IsBuildableInsideBlueprintDesigner()
		|| IsExcludedClass(World, Buildable->GetClass());
}

FBXCollection FBXAreaCollector::Collect(UWorld* World, const FBXArea& Area)
{
	FBXCollection Result;

	// Actor buildables. Temporaries that mirror lightweights are skipped here and handled below,
	// so each lightweight is collected exactly once.
	if (const AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World))
	{
		for (AFGBuildable* Buildable : BuildableSubsystem->GetAllBuildablesRef())
		{
			if (!IsValid(Buildable) || !Area.ContainsWorldLocation(Buildable->GetActorLocation()))
			{
				continue;
			}
			FLightweightBuildableInstanceRef LightweightRef;
			if (UFGLightweightBuildableBlueprintLibrary::GetLightweightBuildableInstanceFromConvertedBuildableOrTemporary(Buildable, LightweightRef))
			{
				continue;
			}
			if (IsExcluded(World, Buildable))
			{
				++Result.NumExcluded;
				continue;
			}
			Result.Buildables.Add(Buildable);
		}
	}

	// Lightweight buildables (foundations, walls, ...): pick the instances first, spawn after,
	// so spawning cannot change the containers being iterated.
	AFGLightweightBuildableSubsystem* LightweightSubsystem = AFGLightweightBuildableSubsystem::Get(World);
	if (LightweightSubsystem == nullptr)
	{
		return Result;
	}

	// Lightweights carry no "inside a designer" flag, so test the location instead.
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
	TArray<FBXSpawnedTemporary> Picked;
	for (const auto& ClassAndInstances : LightweightSubsystem->GetAllLightweightBuildableInstances())
	{
		const TSubclassOf<AFGBuildable> BuildableClass = ClassAndInstances.Key;
		const bool bExcludedClass = IsExcludedClass(World, BuildableClass);
		const TArray<FRuntimeBuildableInstanceData>& Instances = ClassAndInstances.Value;
		for (int32 Index = 0; Index < Instances.Num(); ++Index)
		{
			const FRuntimeBuildableInstanceData& Instance = Instances[Index];
			if (!Instance.IsValid() || !Area.ContainsWorldLocation(Instance.Transform.GetLocation()))
			{
				continue;
			}
			if (bExcludedClass || (BlueprintSubsystem != nullptr
				&& BlueprintSubsystem->IsLocationInsideABlueprintDesigner(Instance.Transform.GetLocation()) != nullptr))
			{
				++Result.NumExcluded;
				continue;
			}
			Picked.Add({BuildableClass, Index});
		}
	}

	for (const FBXSpawnedTemporary& Pick : Picked)
	{
		const bool bAlreadySpawned =
			LightweightSubsystem->FindTemporaryByBuildableClassAndIndex(Pick.BuildableClass, Pick.InstanceIndex) != nullptr;

		FLightweightBuildableInstanceRef Ref;
		Ref.Initialize(LightweightSubsystem, Pick.BuildableClass, Pick.InstanceIndex);
		AFGBuildable* Temporary = UFGLightweightBuildableBlueprintLibrary::SpawnTemporaryFromLightweight(Ref);
		if (Temporary == nullptr)
		{
			++Result.NumExcluded;
			continue;
		}
		if (!bAlreadySpawned)
		{
			Result.SpawnedTemporaries.Add(Pick);
		}
		Result.Buildables.Add(Temporary);
		++Result.NumLightweights;
	}
	return Result;
}

void FBXCollection::Release(UWorld* World)
{
	AFGLightweightBuildableSubsystem* LightweightSubsystem = AFGLightweightBuildableSubsystem::Get(World);
	if (LightweightSubsystem != nullptr)
	{
		// Removes only the temporary actor; the lightweight instance itself stays in the world.
		for (const FBXSpawnedTemporary& Spawned : SpawnedTemporaries)
		{
			LightweightSubsystem->RemoveTemporaryBuildableForInstanceIndex(Spawned.BuildableClass, Spawned.InstanceIndex);
		}
	}
	SpawnedTemporaries.Reset();
	Buildables.Reset();
}
