// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"

class AFGCharacterPlayer;
class UWorld;

/** What /bx clear did. */
struct FBXClearResult
{
	int32 NumRemoved = 0;

	/** Items put into the player's inventory. */
	int32 NumItemsRefunded = 0;

	/** Items that did not fit and went into a dismantle crate next to the player. */
	int32 NumItemsDropped = 0;
};

/**
 * Removes every Blueprint-X Corner in the world (only ABXCornerMarker actors, never other
 * buildings) and refunds their build cost to one player, like dismantling them by hand.
 * Server only. Open marker windows close by themselves once their marker is gone.
 */
class BLUEPRINTX_API FBXMarkerCleaner
{
public:
	/** bNoBuildCost: the player's "No Build Cost" rule is on, so nothing is refunded (vanilla rule). */
	static FBXClearResult RemoveAllMarkers(UWorld* World, AFGCharacterPlayer* Refundee, bool bNoBuildCost);
};
