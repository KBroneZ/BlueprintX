// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXAdjustHintWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

namespace BXAdjustHint
{
	constexpr int32 FontSize = 14;
	constexpr float TopOffset = 96.0f;
	const FMargin BannerPadding(16.0f, 8.0f);
	const FLinearColor BackgroundColor(0.02f, 0.02f, 0.03f, 0.8f);
	const FLinearColor TextColor(0.98f, 0.55f, 0.1f);
}

void UBXAdjustHintWidget::NativeOnInitialized()
{
	using namespace BXAdjustHint;

	Super::NativeOnInitialized();

	// Full-screen overlay that lets every click through; the banner sits at the top centre.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Root;

	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Banner"));
	Banner->SetBrushColor(BackgroundColor);
	Banner->SetPadding(BannerPadding);
	UOverlaySlot* BannerSlot = Root->AddChildToOverlay(Banner);
	BannerSlot->SetHorizontalAlignment(HAlign_Center);
	BannerSlot->SetVerticalAlignment(VAlign_Top);
	BannerSlot->SetPadding(FMargin(0.0f, TopOffset, 0.0f, 0.0f));

	mText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text"));
	FSlateFontInfo Font = mText->GetFont();
	Font.Size = FontSize;
	mText->SetFont(Font);
	mText->SetColorAndOpacity(FSlateColor(TextColor));
	mText->SetJustification(ETextJustify::Center);
	Banner->SetContent(mText);
}

void UBXAdjustHintWidget::SetHint(const FString& Text)
{
	if (mText != nullptr)
	{
		mText->SetText(FText::FromString(Text));
	}
}
