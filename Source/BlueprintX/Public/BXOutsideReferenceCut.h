// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"

class AActor;
class AFGBuildable;
class FArrayProperty;
class FObjectPropertyBase;
class FProperty;
class UClass;
class UObject;
class UPackage;
class UWorld;

/**
 * Bug 002: while a blueprint is written, hides every saved reference that leaves the saved set
 * (wires on poles, belt/pipe/rail/hypertube ends, hidden circuit links to player equipment,
 * blueprint proxies, conveyor chains...), so the file stores "not connected" there.
 *
 * Same "clear, serialize, set back" pattern as the game's own PreSerializedToBlueprint /
 * PostSerializedToBlueprint: only memory changes, no game logic runs (no disconnect, no circuit or
 * belt rebuild), and the destructor puts every value back. Server only: create it right before
 * AFGBlueprintSubsystem::WriteBlueprintToArchive and let it go right after, in the same frame.
 */
class BLUEPRINTX_API FBXOutsideReferenceCut
{
public:
	FBXOutsideReferenceCut(UWorld* World, const TArray<AFGBuildable*>& Buildables);
	~FBXOutsideReferenceCut();

	FBXOutsideReferenceCut(const FBXOutsideReferenceCut&) = delete;
	FBXOutsideReferenceCut& operator=(const FBXOutsideReferenceCut&) = delete;

	/** How many references are hidden right now. */
	int32 GetNumCut() const { return NumCut; }

	/** "8x Build_PowerLine_C, 2x Equip_HoverPack_C" (most common first), for the log. */
	FString DescribeCut(int32 MaxClasses = 6) const;

private:
	struct FCutObject
	{
		const FObjectPropertyBase* Property;
		void* Address;
		UObject* Original;
	};

	struct FCutArray
	{
		const FArrayProperty* Property;
		void* Address;
		TArray<UObject*> Original;
	};

	void CutValue(const FProperty* Property, void* Address);
	bool MayHoldReferences(const FProperty* Property);
	void CutObjectArray(const FArrayProperty* ArrayProperty, const FObjectPropertyBase* Inner, void* Address);
	bool IsSaved(const UObject* Target) const;
	void NoteCut(const UObject* Target);

	/** Actors the blueprint writer saves (buildables plus what it collects with them). */
	TSet<const AActor*> SavedActors;

	/** Everything the blueprint writer saves (actors and their components). */
	TSet<const UObject*> SavedObjects;

	/** Package of the running world: objects outside it (and the transient package) are assets. */
	const UPackage* WorldPackage = nullptr;

	TMap<const FProperty*, bool> MayHoldReferencesCache;
	TSet<const FProperty*> WarnedProperties;

	TArray<FCutObject> CutObjects;
	TArray<FCutArray> CutArrays;
	TMap<const UClass*, int32> CutByClass;
	int32 NumCut = 0;
};
