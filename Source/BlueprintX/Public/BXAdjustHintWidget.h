// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BXAdjustHintWidget.generated.h"

class UTextBlock;

/**
 * Small banner at the top of the screen while adjust mode is on: box size and the keys to use.
 * Pure C++ like the marker window; never takes the mouse or the keyboard.
 */
UCLASS()
class BLUEPRINTX_API UBXAdjustHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHint(const FString& Text);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mText;
};
