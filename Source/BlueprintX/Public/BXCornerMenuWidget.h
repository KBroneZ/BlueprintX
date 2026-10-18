// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "UI/FGInteractWidget.h"
#include "Types/SlateEnums.h"
#include "Engine/TimerHandle.h"
#include "BXCornerMenuWidget.generated.h"

class ABXCornerMarker;
class UBXRemoteCallObject;
class UButton;
class UComboBoxString;
class UEditableTextBox;
class USpinBox;
class UTextBlock;
class UVerticalBox;

/**
 * Window opened by interacting (E) with a Blueprint-X Corner: pick the other corner, see the box,
 * name the blueprint and ask the server to save it.
 *
 * Built entirely in C++ (no widget blueprint) so the layout lives in source control.
 * Client side only; the building count and the save run on the server through
 * UBXRemoteCallObject. The box preview and adjust mode live in UBXAdjustSubsystem, so they
 * survive while the window is hidden.
 */
UCLASS()
class BLUEPRINTX_API UBXCornerMenuWidget : public UFGInteractWidget
{
	GENERATED_BODY()

public:
	UBXCornerMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual void Init_Implementation() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void AddHeightRow(UVerticalBox* Parent);
	void AddButtonRow(UVerticalBox* Parent);
	UTextBlock* AddLabel(UVerticalBox* Parent, const FString& Text);
	UEditableTextBox* AddTextBox(UVerticalBox* Parent, const FString& Hint);
	UButton* MakeButton(const FString& Text);

	/** The game UI pops interact widgets from its stack (Esc) but only removes Blueprint
	 *  windows from the screen, so this C++ window removes itself once it left the stack. */
	bool IsInGameUIStack();

	/** Removes the window. bRestoreGameInput: the player closed it with a button, so input
	 *  goes back to the game; false when the game itself closed it (Esc). */
	void CloseWindow(bool bRestoreGameInput = true);

	/** Gives keyboard and mouse back to the game once no window is left (fixes the T-005 bug:
	 *  after Close the player could not move until Esc was pressed twice). */
	void RestoreGameInput();

	/** Takes back the values of adjust mode, if it was on. */
	void RestoreAdjustValues();

	/** Fills the corner list with every other marker this client knows, nearest first. */
	void RefreshCornerList();
	void SelectCorner(const ABXCornerMarker* Corner);
	void RefreshPreview();
	void SetStatus(const FString& Message, const FLinearColor& Color);

	/** Asks the server for a new building count once the values stop changing for a moment. */
	void RequestCount();
	void SendCountRequest();

	ABXCornerMarker* GetThisCorner() const;
	ABXCornerMarker* GetSelectedCorner() const;
	UBXRemoteCallObject* GetRemoteCall() const;

	UFUNCTION()
	void OnCornerSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnHeightChanged(float NewValue);

	UFUNCTION()
	void OnSaveClicked();

	UFUNCTION()
	void OnAdjustClicked();

	UFUNCTION()
	void OnCloseClicked();

	void OnCreateResult(bool bSuccess, int32 NumBuildables, const FString& Message);
	void OnCountResult(int32 RequestId, int32 NumBuildables, int32 NumExcluded, const FString& Message);

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> mCornerList;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> mHeightBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> mNameBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> mDescriptionBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mBoxSizeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mCountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mSaveButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mAdjustButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mCloseButton;

	/** The marker the box is built from (its rotation orients the box). Usually the one the
	 *  player used, but the adjust mode corner when the window comes back from adjust mode. */
	TWeakObjectPtr<ABXCornerMarker> mThisCorner;

	/** Same order as the entries of mCornerList. */
	TArray<TWeakObjectPtr<ABXCornerMarker>> mOtherCorners;

	TWeakObjectPtr<UBXRemoteCallObject> mBoundRemoteCall;
	FDelegateHandle mResultHandle;
	FDelegateHandle mCountHandle;

	FTimerHandle mCountTimer;

	/** Id of the newest count request (unique across windows); older answers are ignored. */
	int32 mCountRequestId = 0;

	/** Set when the window hides for adjust mode, so closing it keeps the preview. */
	bool mIsEnteringAdjustMode = false;

	/** Set once the game UI has pushed this window onto its interact widget stack. */
	bool mWasInGameUIStack = false;
};
