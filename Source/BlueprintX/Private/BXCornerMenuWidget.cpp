// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXCornerMenuWidget.h"

#include "BlueprintX.h"
#include "BXAreaCollector.h"
#include "BXBoxPreview.h"
#include "BXCornerMarker.h"
#include "BXRemoteCallObject.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "FGHUD.h"
#include "FGPlayerController.h"
#include "UI/FGGameUI.h"

namespace BXCornerMenu
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float DefaultHeightMeters = 8.0f;
	constexpr float WindowWidth = 460.0f;
	constexpr int32 TitleFontSize = 20;
	constexpr int32 TextFontSize = 12;
	const FMargin WindowPadding(18.0f);
	const FMargin RowPadding(0.0f, 4.0f);
	const FMargin ButtonGap(0.0f, 0.0f, 8.0f, 0.0f);
	const FLinearColor BackgroundColor(0.02f, 0.02f, 0.03f, 1.0f);
	const FLinearColor TitleColor(0.98f, 0.55f, 0.1f);
	const FLinearColor TextColor(0.9f, 0.9f, 0.9f);
	const FLinearColor FieldTextColor = FLinearColor::Black;
	const FLinearColor InfoColor(0.75f, 0.75f, 0.75f);
	const FLinearColor SuccessColor(0.2f, 0.9f, 0.3f);
	const FLinearColor ErrorColor(1.0f, 0.3f, 0.2f);
}

namespace
{
	UFGGameUI* FindGameUI(const APlayerController* Player)
	{
		const AFGHUD* HUD = Player != nullptr ? Cast<AFGHUD>(Player->GetHUD()) : nullptr;
		return HUD != nullptr ? HUD->GetGameUI() : nullptr;
	}
}

UBXCornerMenuWidget::UBXCornerMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Typing in the text boxes must not move or turn the player.
	mUseKeyboard = true;
	mUseMouse = true;
	mDesiredHorizontalAlignment = HAlign_Center;
	mDesiredVerticalAlignment = VAlign_Center;
	mSupportsCaching = false;
}

void UBXCornerMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UBXCornerMenuWidget::Init_Implementation()
{
	Super::Init_Implementation();

	if (GetThisCorner() == nullptr)
	{
		SetStatus(TEXT("This window must be opened from a Blueprint-X Corner."), BXCornerMenu::ErrorColor);
		return;
	}
	if (UBXRemoteCallObject* RemoteCall = GetRemoteCall())
	{
		// Init may run more than once; never keep two bindings.
		RemoteCall->OnCreateResult.Remove(mResultHandle);
		mBoundRemoteCall = RemoteCall;
		mResultHandle = RemoteCall->OnCreateResult.AddUObject(this, &UBXCornerMenuWidget::OnCreateResult);
	}
	RefreshCornerList();
	RefreshPreview();
	SetDefaultFocusWidget(mNameBox);
}

void UBXCornerMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsInGameUIStack())
	{
		mWasInGameUIStack = true;
	}
	else if (mWasInGameUIStack)
	{
		// The game closed this window (Esc) but left it on screen.
		CloseWindow();
	}
}

void UBXCornerMenuWidget::NativeDestruct()
{
	if (UBXRemoteCallObject* RemoteCall = mBoundRemoteCall.Get())
	{
		RemoteCall->OnCreateResult.Remove(mResultHandle);
	}
	mBoundRemoteCall.Reset();
	if (IsValid(mPreview))
	{
		mPreview->Destroy();
	}
	mPreview = nullptr;
	Super::NativeDestruct();
}

void UBXCornerMenuWidget::BuildLayout()
{
	using namespace BXCornerMenu;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(BackgroundColor);
	Background->SetPadding(WindowPadding);
	WidgetTree->RootWidget = Background;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
	Size->SetWidthOverride(WindowWidth);
	Background->SetContent(Size);

	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rows"));
	Size->AddChild(Rows);

	UTextBlock* Title = AddLabel(Rows, TEXT("Blueprint-X Corner"));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = TitleFontSize;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(TitleColor));

	AddLabel(Rows, TEXT("Other corner (nearest first)"));
	mCornerList = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("CornerList"));
	mCornerList->OnSelectionChanged.AddDynamic(this, &UBXCornerMenuWidget::OnCornerSelected);
	Rows->AddChildToVerticalBox(mCornerList)->SetPadding(RowPadding);

	AddHeightRow(Rows);
	mBoxSizeText = AddLabel(Rows, FString());
	mBoxSizeText->SetColorAndOpacity(FSlateColor(InfoColor));

	AddLabel(Rows, TEXT("Name"));
	mNameBox = AddTextBox(Rows, TEXT("Blueprint name"));
	AddLabel(Rows, TEXT("Description (optional)"));
	mDescriptionBox = AddTextBox(Rows, FString());

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	mSaveButton = MakeButton(TEXT("Save blueprint"));
	mSaveButton->OnClicked.AddDynamic(this, &UBXCornerMenuWidget::OnSaveClicked);
	Buttons->AddChildToHorizontalBox(mSaveButton)->SetPadding(ButtonGap);
	mCloseButton = MakeButton(TEXT("Close"));
	mCloseButton->OnClicked.AddDynamic(this, &UBXCornerMenuWidget::OnCloseClicked);
	Buttons->AddChildToHorizontalBox(mCloseButton);
	Rows->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 4.0f));

	mStatusText = AddLabel(Rows, FString());
}

void UBXCornerMenuWidget::AddHeightRow(UVerticalBox* Parent)
{
	using namespace BXCornerMenu;

	AddLabel(Parent, TEXT("Extra height above the higher corner (m)"));
	mHeightBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("Height"));
	mHeightBox->SetMinValue(0.0f);
	mHeightBox->SetMaxValue(FBXSaveRequest::MaxExtraHeight / CentimetersPerMeter);
	mHeightBox->SetMinSliderValue(0.0f);
	mHeightBox->SetMaxSliderValue(FBXSaveRequest::MaxExtraHeight / CentimetersPerMeter);
	mHeightBox->SetDelta(1.0f);
	mHeightBox->SetMinFractionalDigits(0);
	mHeightBox->SetMaxFractionalDigits(0);
	mHeightBox->SetValue(DefaultHeightMeters);
	FSlateFontInfo Font = mHeightBox->GetFont();
	Font.Size = TextFontSize;
	mHeightBox->SetFont(Font);
	mHeightBox->SetForegroundColor(FSlateColor(FieldTextColor));
	mHeightBox->OnValueChanged.AddDynamic(this, &UBXCornerMenuWidget::OnHeightChanged);
	Parent->AddChildToVerticalBox(mHeightBox)->SetPadding(RowPadding);
}

UTextBlock* UBXCornerMenuWidget::AddLabel(UVerticalBox* Parent, const FString& Text)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = BXCornerMenu::TextFontSize;
	Label->SetFont(Font);
	Label->SetColorAndOpacity(FSlateColor(BXCornerMenu::TextColor));
	Label->SetAutoWrapText(true);
	Label->SetText(FText::FromString(Text));
	Parent->AddChildToVerticalBox(Label)->SetPadding(BXCornerMenu::RowPadding);
	return Label;
}

UEditableTextBox* UBXCornerMenuWidget::AddTextBox(UVerticalBox* Parent, const FString& Hint)
{
	UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	// The style must be set before the Slate widget exists; BuildLayout runs before that.
	Box->WidgetStyle.TextStyle.Font.Size = BXCornerMenu::TextFontSize;
	Box->SetForegroundColor(BXCornerMenu::FieldTextColor);
	Box->SetHintText(FText::FromString(Hint));
	Parent->AddChildToVerticalBox(Box)->SetPadding(BXCornerMenu::RowPadding);
	return Box;
}

UButton* UBXCornerMenuWidget::MakeButton(const FString& Text)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = BXCornerMenu::TextFontSize;
	Label->SetFont(Font);
	Label->SetColorAndOpacity(FSlateColor(BXCornerMenu::FieldTextColor));
	Label->SetText(FText::FromString(Text));
	Button->AddChild(Label);
	return Button;
}

void UBXCornerMenuWidget::RefreshCornerList()
{
	const ABXCornerMarker* ThisCorner = GetThisCorner();
	mOtherCorners.Reset();
	mCornerList->ClearOptions();
	if (ThisCorner == nullptr)
	{
		return;
	}

	// Clients only know the markers the server replicated to them (usually the nearby ones).
	const FVector Here = ThisCorner->GetCornerLocation();
	TArray<ABXCornerMarker*> Others;
	for (TActorIterator<ABXCornerMarker> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && *It != ThisCorner)
		{
			Others.Add(*It);
		}
	}
	Others.Sort([&Here](const ABXCornerMarker& A, const ABXCornerMarker& B)
	{
		return FVector::DistSquared(A.GetCornerLocation(), Here) < FVector::DistSquared(B.GetCornerLocation(), Here);
	});

	for (int32 Index = 0; Index < Others.Num(); ++Index)
	{
		const FVector Offset = (Others[Index]->GetCornerLocation() - Here) / BXCornerMenu::CentimetersPerMeter;
		mCornerList->AddOption(FString::Printf(TEXT("#%d - %.0f m away, %+.0f m height"),
			Index + 1, Offset.Size2D(), Offset.Z));
		mOtherCorners.Add(Others[Index]);
	}
	if (Others.IsEmpty())
	{
		SetStatus(TEXT("Build a second Blueprint-X Corner at the opposite corner of the area."), BXCornerMenu::ErrorColor);
		return;
	}
	mCornerList->SetSelectedIndex(0);
}

void UBXCornerMenuWidget::RefreshPreview()
{
	ABXCornerMarker* ThisCorner = GetThisCorner();
	ABXCornerMarker* OtherCorner = GetSelectedCorner();
	if (ThisCorner == nullptr || OtherCorner == nullptr)
	{
		mBoxSizeText->SetText(FText::GetEmpty());
		if (IsValid(mPreview))
		{
			mPreview->SetActorHiddenInGame(true);
		}
		return;
	}

	const float ExtraHeight = mHeightBox->GetValue() * BXCornerMenu::CentimetersPerMeter;
	const FBXArea Area = FBXArea::FromCorners(*ThisCorner, *OtherCorner, ExtraHeight, FBXArea::DefaultBelowMargin);
	const FVector Size = Area.LocalBox.GetSize() / BXCornerMenu::CentimetersPerMeter;
	mBoxSizeText->SetText(FText::FromString(FString::Printf(TEXT("Box: %.0f x %.0f m, %.0f m high"),
		Size.X, Size.Y, Size.Z)));

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

void UBXCornerMenuWidget::SetStatus(const FString& Message, const FLinearColor& Color)
{
	mStatusText->SetText(FText::FromString(Message));
	mStatusText->SetColorAndOpacity(FSlateColor(Color));
}

ABXCornerMarker* UBXCornerMenuWidget::GetThisCorner() const
{
	ABXCornerMarker* Corner = Cast<ABXCornerMarker>(mInteractObject);
	return IsValid(Corner) ? Corner : nullptr;
}

ABXCornerMarker* UBXCornerMenuWidget::GetSelectedCorner() const
{
	const int32 Index = mCornerList->GetSelectedIndex();
	return mOtherCorners.IsValidIndex(Index) ? mOtherCorners[Index].Get() : nullptr;
}

UBXRemoteCallObject* UBXCornerMenuWidget::GetRemoteCall() const
{
	AFGPlayerController* Player = Cast<AFGPlayerController>(GetOwningPlayer());
	return Player != nullptr ? Player->GetRemoteCallObjectOfClass<UBXRemoteCallObject>() : nullptr;
}

bool UBXCornerMenuWidget::IsInGameUIStack()
{
	UFGGameUI* GameUI = FindGameUI(GetOwningPlayer());
	return GameUI != nullptr && GameUI->GetInteractWidgetStack().Contains(this);
}

void UBXCornerMenuWidget::CloseWindow()
{
	UFGGameUI* GameUI = FindGameUI(GetOwningPlayer());
	if (GameUI != nullptr && IsInGameUIStack())
	{
		// Gives the player back movement and the mouse, like Esc does.
		GameUI->RemoveInteractWidget(this);
	}
	mWasInGameUIStack = false;
	RemoveFromParent();
}

void UBXCornerMenuWidget::OnCornerSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	RefreshPreview();
}

void UBXCornerMenuWidget::OnHeightChanged(float NewValue)
{
	RefreshPreview();
}

void UBXCornerMenuWidget::OnSaveClicked()
{
	ABXCornerMarker* ThisCorner = GetThisCorner();
	ABXCornerMarker* OtherCorner = GetSelectedCorner();
	if (ThisCorner == nullptr || OtherCorner == nullptr)
	{
		SetStatus(TEXT("Pick the other corner first."), BXCornerMenu::ErrorColor);
		return;
	}
	FBXSaveRequest Request;
	Request.BlueprintName = mNameBox->GetText().ToString().TrimStartAndEnd().Left(FBXSaveRequest::MaxNameLength);
	Request.Description = mDescriptionBox->GetText().ToString().TrimStartAndEnd().Left(FBXSaveRequest::MaxDescriptionLength);
	Request.ExtraHeight = mHeightBox->GetValue() * BXCornerMenu::CentimetersPerMeter;
	if (Request.BlueprintName.IsEmpty())
	{
		SetStatus(TEXT("Type a name for the blueprint."), BXCornerMenu::ErrorColor);
		return;
	}
	UBXRemoteCallObject* RemoteCall = GetRemoteCall();
	if (RemoteCall == nullptr)
	{
		SetStatus(TEXT("Remote call object not registered (mod install problem)."), BXCornerMenu::ErrorColor);
		return;
	}

	UE_LOG(LogBlueprintX, Display, TEXT("Window: asking the server to save '%s'."), *Request.BlueprintName);
	mSaveButton->SetIsEnabled(false);
	SetStatus(TEXT("Saving on the server..."), BXCornerMenu::InfoColor);
	RemoteCall->Server_CreateBlueprint(ThisCorner, OtherCorner, Request);
}

void UBXCornerMenuWidget::OnCloseClicked()
{
	CloseWindow();
}

void UBXCornerMenuWidget::OnCreateResult(bool bSuccess, int32 NumBuildables, const FString& Message)
{
	mSaveButton->SetIsEnabled(true);
	SetStatus(Message, bSuccess ? BXCornerMenu::SuccessColor : BXCornerMenu::ErrorColor);
}
