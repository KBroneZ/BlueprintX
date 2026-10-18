// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "BXSubsystem.generated.h"

class UFGSchematic;

/**
 * Server-side Blueprint-X subsystem.
 *
 * Gives the session the starting schematics (the Corner marker recipe) so the marker is
 * buildable from the start, in new and existing saves, without visiting the HUB.
 */
UCLASS(Blueprintable)
class BLUEPRINTX_API ABXSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	ABXSubsystem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Schematics unlocked automatically when a game starts or loads. */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint-X")
	TArray<TSubclassOf<UFGSchematic>> mStartingSchematics;

private:
	/** Tries to unlock the starting schematics; stops retrying once all are unlocked. */
	void TryUnlockStartingSchematics();

	FTimerHandle mUnlockTimer;
	int32 mUnlockAttempts = 0;
};
