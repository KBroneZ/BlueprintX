// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "UI/FGInteractWidget.h"
#include "Types/SlateEnums.h"
#include "BXCornerMenuWidget.generated.h"

class ABXBoxPreview;
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
 * Client side only; the save itself runs on the server through UBXRemoteCallObject.
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
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	UTextBlock* AddLabel(UVerticalBox* Parent, const FString& Text);
	UButton* MakeButton(const FString& Text);

	/** Fills the corner list with every other marker this client knows, nearest first. */
	void RefreshCornerList();
	void RefreshPreview();
	void SetStatus(const FString& Message, const FLinearColor& Color);

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
	void OnCloseClicked();

	void OnCreateResult(bool bSuccess, int32 NumBuildables, const FString& Message);

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
	TObjectPtr<UTextBlock> mStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mSaveButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(Transient)
	TObjectPtr<ABXBoxPreview> mPreview;

	/** Same order as the entries of mCornerList. */
	TArray<TWeakObjectPtr<ABXCornerMarker>> mOtherCorners;

	TWeakObjectPtr<UBXRemoteCallObject> mBoundRemoteCall;
	FDelegateHandle mResultHandle;
};
