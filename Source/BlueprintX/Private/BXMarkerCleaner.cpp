// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXMarkerCleaner.h"

#include "BlueprintX.h"
#include "BXCornerMarker.h"
#include "EngineUtils.h"
#include "FGCharacterPlayer.h"
#include "FGDismantleInterface.h"
#include "FGInventoryComponent.h"
#include "FGInventoryLibrary.h"
#include "FGItemPickup_Spawnable.h"
#include "Resources/FGItemDescriptor.h"

namespace
{
	/** Splits consolidated stacks (which ignore stack limits) into stacks an inventory accepts. */
	TArray<FInventoryStack> SplitIntoStackSizes(const TArray<FInventoryStack>& Stacks)
	{
		TArray<FInventoryStack> Result;
		for (const FInventoryStack& Stack : Stacks)
		{
			const TSubclassOf<UFGItemDescriptor> ItemClass = Stack.Item.GetItemClass();
			const int32 StackSize = FMath::Max(1, UFGItemDescriptor::GetStackSize(ItemClass));
			for (int32 Left = Stack.NumItems; Left > 0; Left -= StackSize)
			{
				Result.Emplace(FMath::Min(Left, StackSize), ItemClass);
			}
		}
		return Result;
	}

	/** Puts the refund into the player's inventory; what does not fit goes into a crate next to them. */
	void GiveRefund(UWorld* World, AFGCharacterPlayer* Refundee, TArray<FInventoryStack> Refund, FBXClearResult& Result)
	{
		UFGInventoryLibrary::ConsolidateInventoryItems(Refund);
		UFGInventoryComponent* Inventory = Refundee->GetInventory();

		TArray<FInventoryStack> Overflow;
		for (const FInventoryStack& Stack : SplitIntoStackSizes(Refund))
		{
			const int32 Added = Inventory != nullptr ? Inventory->AddStack(Stack, true) : 0;
			Result.NumItemsRefunded += Added;
			if (Added < Stack.NumItems)
			{
				Overflow.Emplace(Stack.NumItems - Added, Stack.Item.GetItemClass());
			}
		}
		if (Overflow.IsEmpty())
		{
			return;
		}

		// Same as vanilla dismantle with a full inventory: the rest goes into a crate next to the player.
		FSpawnInventoryCrateParameters CrateParameters;
		CrateParameters.ActorsToIgnore.Add(Refundee);
		if (AFGItemPickup_Spawnable::SpawnInventoryCrate(World, Overflow, Refundee->GetActorLocation(), CrateParameters) == nullptr)
		{
			UE_LOG(LogBlueprintX, Error, TEXT("Clear: could not spawn the crate for the refund that did not fit."));
			return;
		}
		for (const FInventoryStack& Stack : Overflow)
		{
			Result.NumItemsDropped += Stack.NumItems;
		}
	}
}

FBXClearResult FBXMarkerCleaner::RemoveAllMarkers(UWorld* World, AFGCharacterPlayer* Refundee, bool bNoBuildCost)
{
	FBXClearResult Result;
	if (World == nullptr)
	{
		return Result;
	}

	// Collect first: dismantling while iterating the actor list would skip actors. A dismantled
	// marker lives on during its dismantle effect; skipping it avoids refunding it twice when
	// /bx clear runs again (or a player dismantles it by hand) in that moment.
	TArray<ABXCornerMarker*> Markers;
	for (TActorIterator<ABXCornerMarker> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->GetIsDismantled() && !It->IsAboutToBeDismantled() &&
			IFGDismantleInterface::Execute_CanDismantle(*It))
		{
			Markers.Add(*It);
		}
	}

	TArray<FInventoryStack> Refund;
	for (ABXCornerMarker* Marker : Markers)
	{
		TArray<FInventoryStack> MarkerRefund;
		IFGDismantleInterface::Execute_GetDismantleRefund(Marker, MarkerRefund, bNoBuildCost);
		Refund.Append(MarkerRefund);
		IFGDismantleInterface::Execute_Dismantle(Marker);
		++Result.NumRemoved;
	}

	if (Refundee != nullptr && !Refund.IsEmpty())
	{
		GiveRefund(World, Refundee, MoveTemp(Refund), Result);
	}
	UE_LOG(LogBlueprintX, Display, TEXT("Clear: removed %d markers, refunded %d items, dropped %d items in a crate."),
		Result.NumRemoved, Result.NumItemsRefunded, Result.NumItemsDropped);
	return Result;
}
