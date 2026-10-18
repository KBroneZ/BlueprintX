// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXChatCommand.h"

#include "BlueprintX.h"
#include "BXCornerMarker.h"
#include "BXMarkerCleaner.h"
#include "BXRemoteCallObject.h"
#include "Command/CommandSender.h"
#include "EngineUtils.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"

#define LOCTEXT_NAMESPACE "BlueprintX"

namespace BXCommand
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float DefaultHeightMeters = 8.0f;
	const FLinearColor InfoColor(1.0f, 0.55f, 0.1f);
	const FLinearColor SuccessColor(0.2f, 0.9f, 0.3f);
	const FLinearColor ErrorColor(1.0f, 0.3f, 0.2f);
	const TCHAR* const HeightPrefix = TEXT("h=");
	constexpr int32 HeightPrefixLength = 2;

	const TCHAR* const SaveAlias = TEXT("bxsave");
	const TCHAR* const SubHelp = TEXT("help");
	const TCHAR* const SubSave = TEXT("save");
	const TCHAR* const SubClear = TEXT("clear");
}

namespace
{
	/** The two corner markers closest to Location, nearest first. Fewer if fewer exist. */
	TArray<ABXCornerMarker*> FindNearestCorners(UWorld* World, const FVector& Location)
	{
		TArray<ABXCornerMarker*> Corners;
		for (TActorIterator<ABXCornerMarker> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				Corners.Add(*It);
			}
		}
		Corners.Sort([&Location](const ABXCornerMarker& A, const ABXCornerMarker& B)
		{
			return FVector::DistSquared(A.GetCornerLocation(), Location) < FVector::DistSquared(B.GetCornerLocation(), Location);
		});
		Corners.SetNum(FMath::Min(Corners.Num(), 2));
		return Corners;
	}

	/** "<name words...> [h=<metres>]": every word is part of the name except an "h=" option. */
	FBXSaveRequest ParseSaveRequest(const TArray<FString>& Arguments)
	{
		FBXSaveRequest Request;
		Request.ExtraHeight = BXCommand::DefaultHeightMeters * BXCommand::CentimetersPerMeter;

		TArray<FString> NameWords;
		for (const FString& Argument : Arguments)
		{
			const FString Value = Argument.Mid(BXCommand::HeightPrefixLength);
			if (Argument.StartsWith(BXCommand::HeightPrefix, ESearchCase::IgnoreCase) && Value.IsNumeric())
			{
				Request.ExtraHeight = FMath::Max(0.0f, FCString::Atof(*Value)) * BXCommand::CentimetersPerMeter;
				continue;
			}
			NameWords.Add(Argument);
		}
		Request.BlueprintName = FString::Join(NameWords, TEXT(" "));
		Request.Description = TEXT("Saved with Blueprint-X.");
		return Request;
	}

	/** SML passes the alias as typed, optionally qualified with the mod reference ("BlueprintX:bxsave"). */
	bool IsSaveAlias(const FString& Label)
	{
		FString Name = Label;
		int32 ColonIndex = INDEX_NONE;
		if (Name.FindLastChar(TEXT(':'), ColonIndex))
		{
			Name.RightChopInline(ColonIndex + 1);
		}
		return Name.Equals(BXCommand::SaveAlias, ESearchCase::IgnoreCase);
	}

	FString Plural(int32 Count, const TCHAR* Singular, const TCHAR* PluralWord)
	{
		return FString::Printf(TEXT("%d %s"), Count, Count == 1 ? Singular : PluralWord);
	}
}

ABXChatCommand::ABXChatCommand()
{
	CommandName = TEXT("bx");
	Aliases.Add(BXCommand::SaveAlias);
	MinNumberOfArguments = 0;
	bOnlyUsableByPlayer = true;
	Usage = LOCTEXT("BXUsage", "/bx help | /bx save <name> [h=<height in m>] | /bx clear");
}

EExecutionStatus ABXChatCommand::ExecuteCommand_Implementation(UCommandSender* Sender,
	const TArray<FString>& Arguments, const FString& Label)
{
	if (Sender == nullptr)
	{
		return EExecutionStatus::UNCOMPLETED;
	}
	if (IsSaveAlias(Label))
	{
		return RunSave(Sender, Arguments);
	}

	const FString SubCommand = Arguments.IsEmpty() ? FString(BXCommand::SubHelp) : Arguments[0];
	const TArray<FString> SubArguments(Arguments.GetData() + FMath::Min(1, Arguments.Num()),
		FMath::Max(0, Arguments.Num() - 1));
	if (SubCommand.Equals(BXCommand::SubSave, ESearchCase::IgnoreCase))
	{
		return RunSave(Sender, SubArguments);
	}
	if (SubCommand.Equals(BXCommand::SubClear, ESearchCase::IgnoreCase))
	{
		return RunClear(Sender);
	}
	if (!SubCommand.Equals(BXCommand::SubHelp, ESearchCase::IgnoreCase))
	{
		Sender->SendChatMessage(FString::Printf(TEXT("Blueprint-X: unknown command '%s'."), *SubCommand),
			BXCommand::ErrorColor);
		RunHelp(Sender);
		return EExecutionStatus::BAD_ARGUMENTS;
	}
	return RunHelp(Sender);
}

EExecutionStatus ABXChatCommand::RunHelp(UCommandSender* Sender)
{
	Sender->SendChatMessage(TEXT("Blueprint-X commands:"), BXCommand::InfoColor);
	Sender->SendChatMessage(TEXT("/bx save <name> [h=<metres>] - save everything between the two Corners nearest to you"),
		BXCommand::InfoColor);
	Sender->SendChatMessage(TEXT("/bx clear - remove every Blueprint-X Corner in the world (their Iron Plates go to you)"),
		BXCommand::InfoColor);
	Sender->SendChatMessage(TEXT("/bx help - this list"), BXCommand::InfoColor);
	return EExecutionStatus::COMPLETED;
}

EExecutionStatus ABXChatCommand::RunSave(UCommandSender* Sender, const TArray<FString>& Arguments)
{
	const FBXSaveRequest Request = ParseSaveRequest(Arguments);
	if (Request.BlueprintName.IsEmpty())
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: usage /bx save <name> [h=<metres>]"), BXCommand::ErrorColor);
		return EExecutionStatus::BAD_ARGUMENTS;
	}
	AFGPlayerController* Player = Sender->GetPlayer();
	const APawn* Pawn = Player != nullptr ? Player->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: no player character."), BXCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}
	UBXRemoteCallObject* RemoteCall = Player->GetRemoteCallObjectOfClass<UBXRemoteCallObject>();
	if (RemoteCall == nullptr)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: remote call object not registered (mod install problem)."),
			BXCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}

	const TArray<ABXCornerMarker*> Corners = FindNearestCorners(GetWorld(), Pawn->GetActorLocation());
	if (Corners.Num() < 2)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: build two Blueprint-X Corners first."), BXCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}

	// Chat commands already run on the server, so this executes locally; the answer goes back
	// to the player through the same client call the marker window uses.
	RemoteCall->Server_CreateBlueprint(Corners[0], Corners[1], Request);
	return EExecutionStatus::COMPLETED;
}

EExecutionStatus ABXChatCommand::RunClear(UCommandSender* Sender)
{
	AFGPlayerController* Player = Sender->GetPlayer();
	AFGCharacterPlayer* Character = Player != nullptr ? Cast<AFGCharacterPlayer>(Player->GetPawn()) : nullptr;
	if (Character == nullptr)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: no player character to give the refund to."), BXCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}
	const AFGPlayerState* PlayerState = Player->GetPlayerState<AFGPlayerState>();
	const bool bNoBuildCost = PlayerState != nullptr && PlayerState->GetPlayerRules().NoBuildCost;

	UE_LOG(LogBlueprintX, Display, TEXT("Clear requested by %s."), *Sender->GetSenderName());
	const FBXClearResult Result = FBXMarkerCleaner::RemoveAllMarkers(GetWorld(), Character, bNoBuildCost);
	if (Result.NumRemoved == 0)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: no markers to remove."), BXCommand::InfoColor);
		return EExecutionStatus::UNCOMPLETED;
	}

	FString Message = FString::Printf(TEXT("Blueprint-X: Removed %s."), *Plural(Result.NumRemoved, TEXT("marker"), TEXT("markers")));
	if (Result.NumItemsRefunded > 0)
	{
		Message += FString::Printf(TEXT(" %s refunded."), *Plural(Result.NumItemsRefunded, TEXT("item"), TEXT("items")));
	}
	if (Result.NumItemsDropped > 0)
	{
		Message += FString::Printf(TEXT(" Inventory full: %s in a crate next to you."),
			*Plural(Result.NumItemsDropped, TEXT("item"), TEXT("items")));
	}
	Sender->SendChatMessage(Message, BXCommand::SuccessColor);
	return EExecutionStatus::COMPLETED;
}

#undef LOCTEXT_NAMESPACE
