// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXSubsystem.h"

#include "BlueprintX.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "TimerManager.h"

namespace BXSubsystem
{
	// The schematic manager and SML's content registry finish setting up a little after
	// BeginPlay (and after the save is loaded), so the unlock is retried for a while.
	constexpr float UnlockRetrySeconds = 1.0f;
	constexpr int32 MaxUnlockAttempts = 60;
}

ABXSubsystem::ABXSubsystem()
{
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

void ABXSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || mStartingSchematics.IsEmpty())
	{
		return;
	}
	GetWorldTimerManager().SetTimer(mUnlockTimer, this, &ABXSubsystem::TryUnlockStartingSchematics,
		BXSubsystem::UnlockRetrySeconds, true);
}

void ABXSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(mUnlockTimer);
	Super::EndPlay(EndPlayReason);
}

void ABXSubsystem::TryUnlockStartingSchematics()
{
	++mUnlockAttempts;
	const bool bLastAttempt = mUnlockAttempts >= BXSubsystem::MaxUnlockAttempts;

	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	if (SchematicManager == nullptr)
	{
		if (bLastAttempt)
		{
			UE_LOG(LogBlueprintX, Error, TEXT("Schematic manager not found; starting schematics not unlocked."));
			GetWorldTimerManager().ClearTimer(mUnlockTimer);
		}
		return;
	}

	bool bAllUnlocked = true;
	for (const TSubclassOf<UFGSchematic>& Schematic : mStartingSchematics)
	{
		if (Schematic == nullptr || SchematicManager->IsSchematicPurchased(Schematic))
		{
			continue;
		}
		if (!SchematicManager->CanGiveAccessToSchematic(Schematic))
		{
			bAllUnlocked = false;
			if (bLastAttempt)
			{
				UE_LOG(LogBlueprintX, Error, TEXT("Could not unlock starting schematic %s."), *Schematic->GetName());
			}
			continue;
		}
		SchematicManager->GiveAccessToSchematic(Schematic, nullptr, ESchematicUnlockFlags::SuppressNarrativeMessages);
		UE_LOG(LogBlueprintX, Display, TEXT("Unlocked starting schematic %s."), *Schematic->GetName());
	}

	if (bAllUnlocked || bLastAttempt)
	{
		GetWorldTimerManager().ClearTimer(mUnlockTimer);
	}
}
