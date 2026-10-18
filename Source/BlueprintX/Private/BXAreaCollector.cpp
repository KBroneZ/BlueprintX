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

FString FBXArea::DescribeSize() const
{
	const FVector Size = LocalBox.GetSize() / 100.0;
	return FString::Printf(TEXT("Box: %.0f x %.0f m, %.0f m high"), Size.X, Size.Y, Size.Z);
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

namespace
{
	/**
	 * Walks every buildable whose pivot is inside Area and that a blueprint may contain.
	 * OnActor(AFGBuildable*) gets actor buildables, OnLightweight(Class, InstanceIndex) gets
	 * lightweight instances (foundations, walls, ...). Returns how many were excluded.
	 * Temporaries that mirror lightweights are skipped on the actor side, so each lightweight is
	 * visited exactly once. Nothing is spawned here.
	 */
	template <typename FOnActor, typename FOnLightweight>
	int32 VisitArea(UWorld* World, const FBXArea& Area, FOnActor&& OnActor, FOnLightweight&& OnLightweight)
	{
		int32 NumExcluded = 0;
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
				if (Buildable->IsA<ABXCornerMarker>())
				{
					// The corners themselves are always inside; never saved and not worth reporting.
					continue;
				}
				if (FBXAreaCollector::IsExcluded(World, Buildable))
				{
					++NumExcluded;
					continue;
				}
				OnActor(Buildable);
			}
		}

		const AFGLightweightBuildableSubsystem* LightweightSubsystem = AFGLightweightBuildableSubsystem::Get(World);
		if (LightweightSubsystem == nullptr)
		{
			return NumExcluded;
		}

		// Lightweights carry no "inside a designer" flag, so test the location instead.
		AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
		for (const auto& ClassAndInstances : LightweightSubsystem->GetAllLightweightBuildableInstances())
		{
			const TSubclassOf<AFGBuildable> BuildableClass = ClassAndInstances.Key;
			const bool bExcludedClass = FBXAreaCollector::IsExcludedClass(World, BuildableClass);
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
					++NumExcluded;
					continue;
				}
				OnLightweight(BuildableClass, Index);
			}
		}
		return NumExcluded;
	}
}

FBXAreaCount FBXAreaCollector::Count(UWorld* World, const FBXArea& Area)
{
	FBXAreaCount Result;
	Result.NumExcluded = VisitArea(World, Area,
		[&Result](AFGBuildable*) { ++Result.NumBuildables; },
		[&Result](TSubclassOf<AFGBuildable>, int32) { ++Result.NumBuildables; });
	return Result;
}

FBXCollection FBXAreaCollector::Collect(UWorld* World, const FBXArea& Area)
{
	FBXCollection Result;

	// Pick the lightweight instances first and spawn after, so spawning cannot change the
	// containers being iterated.
	TArray<FBXSpawnedTemporary> Picked;
	Result.NumExcluded = VisitArea(World, Area,
		[&Result](AFGBuildable* Buildable) { Result.Buildables.Add(Buildable); },
		[&Picked](TSubclassOf<AFGBuildable> BuildableClass, int32 Index) { Picked.Add({BuildableClass, Index}); });

	AFGLightweightBuildableSubsystem* LightweightSubsystem = AFGLightweightBuildableSubsystem::Get(World);
	if (LightweightSubsystem == nullptr)
	{
		return Result;
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
