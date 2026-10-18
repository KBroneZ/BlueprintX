// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXAdjustSubsystem.h"

#include "BlueprintX.h"
#include "BXAdjustHintWidget.h"
#include "BXAreaCollector.h"
#include "BXBlueprintSaver.h"
#include "BXBoxPreview.h"
#include "BXCornerMarker.h"
#include "BXCornerMenuWidget.h"
#include "Engine/World.h"
#include "FGHUD.h"
#include "GameFramework/PlayerController.h"
#include "UI/FGGameUI.h"

namespace BXAdjust
{
	constexpr float HeightStepMeters = 1.0f;
	constexpr float MaxExtraHeightMeters = FBXSaveRequest::MaxExtraHeight / 100.0f;
	// Above the game HUD, below menus.
	constexpr int32 HintZOrder = 5;
}

UBXAdjustSubsystem* UBXAdjustSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext != nullptr ? WorldContext->GetWorld() : nullptr;
	return World != nullptr ? World->GetSubsystem<UBXAdjustSubsystem>() : nullptr;
}

bool UBXAdjustSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// The preview and the window only exist where a player looks at the screen.
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld() && !IsRunningDedicatedServer()
		&& Super::ShouldCreateSubsystem(Outer);
}

void UBXAdjustSubsystem::Deinitialize()
{
	EndSession();
	Super::Deinitialize();
}

TStatId UBXAdjustSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBXAdjustSubsystem, STATGROUP_Tickables);
}

void UBXAdjustSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!mIsAdjusting)
	{
		return;
	}

	APlayerController* Player = mPlayer.Get();
	if (Player == nullptr)
	{
		EndSession();
		return;
	}
	const bool bHasCorners = mValues.ThisCorner.IsValid() && mValues.OtherCorner.IsValid();
	if (!bHasCorners && !mIsCornerLost)
	{
		// Dismantled, or (multiplayer) too far away for this client to know the marker.
		UE_LOG(LogBlueprintX, Display, TEXT("Adjust mode: a corner marker is gone."));
		mIsCornerLost = true;
		HidePreview();
		UpdateHint();
	}
	if (!IsPlayerFree(Player))
	{
		return;
	}
	if (Player->WasInputKeyJustPressed(GetReopenKey()))
	{
		if (bHasCorners)
		{
			ReopenWindow();
		}
		else
		{
			EndSession();
		}
		return;
	}
	if (!bHasCorners)
	{
		return;
	}
	if (Player->WasInputKeyJustPressed(EKeys::PageUp))
	{
		ChangeHeight(BXAdjust::HeightStepMeters);
	}
	else if (Player->WasInputKeyJustPressed(EKeys::PageDown))
	{
		ChangeHeight(-BXAdjust::HeightStepMeters);
	}
}

void UBXAdjustSubsystem::ShowPreview(const ABXCornerMarker* ThisCorner, const ABXCornerMarker* OtherCorner,
	float ExtraHeightMeters)
{
	if (!IsValid(ThisCorner) || !IsValid(OtherCorner))
	{
		HidePreview();
		return;
	}
	const FBXArea Area = FBXArea::FromCorners(*ThisCorner, *OtherCorner, ExtraHeightMeters * 100.0f,
		FBXArea::DefaultBelowMargin);
	if (!IsValid(mPreview))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		mPreview = GetWorld()->SpawnActor<ABXBoxPreview>(Area.Frame.GetLocation(), FRotator::ZeroRotator, Params);
	}
	if (IsValid(mPreview))
	{
		mPreview->ShowArea(Area, ThisCorner->GetPreviewBeamMesh(), ThisCorner->GetMarkerMaterial());
		mPreview->SetActorHiddenInGame(false);
	}
}

void UBXAdjustSubsystem::HidePreview()
{
	if (IsValid(mPreview))
	{
		mPreview->SetActorHiddenInGame(true);
	}
}

void UBXAdjustSubsystem::BeginAdjust(APlayerController* Player, const FBXWindowValues& Values)
{
	mPlayer = Player;
	mValues = Values;
	mIsAdjusting = true;
	mIsCornerLost = false;
	ShowPreview(mValues.ThisCorner.Get(), mValues.OtherCorner.Get(), mValues.ExtraHeightMeters);

	if (mHint == nullptr && Player != nullptr)
	{
		mHint = CreateWidget<UBXAdjustHintWidget>(Player, UBXAdjustHintWidget::StaticClass());
	}
	if (mHint != nullptr && !mHint->IsInViewport())
	{
		mHint->AddToViewport(BXAdjust::HintZOrder);
	}
	UpdateHint();
	UE_LOG(LogBlueprintX, Display, TEXT("Adjust mode started (extra height %.0f m)."), mValues.ExtraHeightMeters);
}

bool UBXAdjustSubsystem::TakeAdjustValues(FBXWindowValues& OutValues)
{
	if (!mIsAdjusting)
	{
		return false;
	}
	OutValues = mValues;
	StopAdjust();
	return true;
}

void UBXAdjustSubsystem::EndSession()
{
	StopAdjust();
	if (IsValid(mPreview))
	{
		mPreview->Destroy();
	}
	mPreview = nullptr;
	mValues = FBXWindowValues();
}

void UBXAdjustSubsystem::StopAdjust()
{
	mIsAdjusting = false;
	if (mHint != nullptr)
	{
		mHint->RemoveFromParent();
	}
}

void UBXAdjustSubsystem::ReopenWindow()
{
	APlayerController* Player = mPlayer.Get();
	AFGHUD* HUD = Player != nullptr ? Cast<AFGHUD>(Player->GetHUD()) : nullptr;
	ABXCornerMarker* Corner = mValues.ThisCorner.Get();
	if (HUD == nullptr || Corner == nullptr)
	{
		return;
	}
	// Same call the game makes when the player uses (E) the marker; the window then takes the
	// adjust values back in Init.
	HUD->OpenInteractUI(UBXCornerMenuWidget::StaticClass(), Corner);
}

void UBXAdjustSubsystem::ChangeHeight(float DeltaMeters)
{
	mValues.ExtraHeightMeters = FMath::Clamp(mValues.ExtraHeightMeters + DeltaMeters, 0.0f, BXAdjust::MaxExtraHeightMeters);
	ShowPreview(mValues.ThisCorner.Get(), mValues.OtherCorner.Get(), mValues.ExtraHeightMeters);
	UpdateHint();
}

void UBXAdjustSubsystem::UpdateHint()
{
	if (mHint == nullptr)
	{
		return;
	}
	const FString ReopenKeyName = GetReopenKey().GetDisplayName().ToString();
	const ABXCornerMarker* ThisCorner = mValues.ThisCorner.Get();
	const ABXCornerMarker* OtherCorner = mValues.OtherCorner.Get();
	if (ThisCorner == nullptr || OtherCorner == nullptr)
	{
		mHint->SetHint(FString::Printf(
			TEXT("Blueprint-X adjust mode - a corner marker was dismantled or is out of range\n")
			TEXT("%s: end adjust mode    use a corner (E): open the window again"), *ReopenKeyName));
		return;
	}
	const FBXArea Area = FBXArea::FromCorners(*ThisCorner, *OtherCorner, mValues.ExtraHeightMeters * 100.0f,
		FBXArea::DefaultBelowMargin);
	mHint->SetHint(FString::Printf(
		TEXT("Blueprint-X adjust mode - %s (extra height %.0f m)\n")
		TEXT("Page Up / Page Down: extra height    %s or use a corner (E): back to the window"),
		*Area.DescribeSize(), mValues.ExtraHeightMeters, *ReopenKeyName));
}

bool UBXAdjustSubsystem::IsPlayerFree(APlayerController* Player) const
{
	const AFGHUD* HUD = Cast<AFGHUD>(Player->GetHUD());
	UFGGameUI* GameUI = HUD != nullptr ? HUD->GetGameUI() : nullptr;
	return GameUI != nullptr && !GameUI->HasActiveInteractWidget();
}
