// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "InputCoreTypes.h"
#include "BXAdjustSubsystem.generated.h"

class ABXBoxPreview;
class ABXCornerMarker;
class APlayerController;
class UBXAdjustHintWidget;

/** What the marker window shows; kept while the window is hidden in adjust mode. */
struct FBXWindowValues
{
	static constexpr float DefaultExtraHeightMeters = 8.0f;

	TWeakObjectPtr<ABXCornerMarker> ThisCorner;
	TWeakObjectPtr<ABXCornerMarker> OtherCorner;
	float ExtraHeightMeters = DefaultExtraHeightMeters;
	FString Name;
	FString Description;
};

/**
 * Client side: owns the box preview of the marker window, and runs adjust mode.
 *
 * Adjust mode: the window is hidden and the player can walk and look around while the box stays
 * visible. Page Up / Page Down change the extra height (the box follows), and the reopen key or
 * using a corner marker again (E) brings the window back with the same values. If a corner
 * disappears, the banner says so and the reopen key ends adjust mode.
 *
 * Not created on dedicated servers. Nothing here is replicated.
 */
UCLASS()
class BLUEPRINTX_API UBXAdjustSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Key that brings the window back in adjust mode (Page Up / Page Down change the height). */
	static FKey GetReopenKey() { return EKeys::Y; }

	static UBXAdjustSubsystem* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Draws the box outline between both corners; hides it when a corner is missing. */
	void ShowPreview(const ABXCornerMarker* ThisCorner, const ABXCornerMarker* OtherCorner, float ExtraHeightMeters);
	void HidePreview();

	/** The window is being hidden: keep the preview and the values, show the hint, listen for keys. */
	void BeginAdjust(APlayerController* Player, const FBXWindowValues& Values);

	/** The window is opening: if adjust mode is on, ends it and hands back its values. */
	bool TakeAdjustValues(FBXWindowValues& OutValues);

	/** The window closed for good: the preview goes away and adjust mode ends. */
	void EndSession();

	bool IsAdjusting() const { return mIsAdjusting; }

private:
	void StopAdjust();
	void ReopenWindow();
	void ChangeHeight(float DeltaMeters);
	void UpdateHint();

	/** True when no other window, chat or menu takes the keys, so a key press is meant for us. */
	bool IsPlayerFree(APlayerController* Player) const;

	UPROPERTY(Transient)
	TObjectPtr<ABXBoxPreview> mPreview;

	UPROPERTY(Transient)
	TObjectPtr<UBXAdjustHintWidget> mHint;

	TWeakObjectPtr<APlayerController> mPlayer;
	FBXWindowValues mValues;
	bool mIsAdjusting = false;

	/** A corner disappeared during adjust mode (dismantled, or out of this client's range). */
	bool mIsCornerLost = false;
};
