// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class ABXCornerMarker;
class AFGBuildable;
class UWorld;

/** Box between two corner markers, in the local frame of corner A (yaw only). */
struct BLUEPRINTX_API FBXArea
{
	/** Foundations have their pivot half way up, so the box reaches below the markers far enough
	 *  to catch the floor they stand on (half of the tallest 4 m foundation). */
	static constexpr float DefaultBelowMargin = 200.0f;

	/** Corner A location with corner A's yaw. LocalBox is expressed in this frame. */
	FTransform Frame;

	/** Selection box in Frame space (cm). A buildable is inside when its pivot is inside. */
	FBox LocalBox = FBox(ForceInit);

	/** Builds the box: horizontal extent between both corners, from the lower corner minus
	 *  BelowMargin up to the higher corner plus ExtraHeight. */
	static FBXArea FromCorners(const ABXCornerMarker& CornerA, const ABXCornerMarker& CornerB,
		float ExtraHeight, float BelowMargin);

	bool ContainsWorldLocation(const FVector& WorldLocation) const;

	/** "Box: 6 x 2 m, 10 m high", for the window and the adjust mode banner. */
	FString DescribeSize() const;
};

/** Lightweight buildable that was turned into a temporary actor for the blueprint writer. */
struct FBXSpawnedTemporary
{
	TSubclassOf<AFGBuildable> BuildableClass;
	int32 InstanceIndex = INDEX_NONE;
};

/** Result of collecting the buildables inside an area. Call Release() when done. */
struct BLUEPRINTX_API FBXCollection
{
	TArray<AFGBuildable*> Buildables;
	int32 NumLightweights = 0;
	int32 NumExcluded = 0;

	/** Temporaries this collection spawned (and must remove again). */
	TArray<FBXSpawnedTemporary> SpawnedTemporaries;

	/** Removes the temporary actors spawned for lightweight buildables. Server only. */
	void Release(UWorld* World);
};

/** How many buildables an area holds, without spawning anything. */
struct FBXAreaCount
{
	/** Buildables a save of this area would contain. */
	int32 NumBuildables = 0;

	/** Buildables inside that a blueprint never contains (markers, HUB, elevators, ...). */
	int32 NumExcluded = 0;
};

/** Server-side helper: finds every buildable whose pivot is inside an area. */
class BLUEPRINTX_API FBXAreaCollector
{
public:
	static FBXCollection Collect(UWorld* World, const FBXArea& Area);

	/** Same selection as Collect, but only counts (cheap: lightweights are not spawned). Can be
	 *  higher than a save if the game fails to spawn the temporary of a lightweight (rare). */
	static FBXAreaCount Count(UWorld* World, const FBXArea& Area);

	/** True for buildables a blueprint must never contain (markers, HUB, elevators, designers...). */
	static bool IsExcluded(UWorld* World, const AFGBuildable* Buildable);
	static bool IsExcludedClass(UWorld* World, const UClass* BuildableClass);
};
