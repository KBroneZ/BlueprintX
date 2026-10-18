// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXSaveChatCommand.h"

#include "BXCornerMarker.h"
#include "BXRemoteCallObject.h"
#include "Command/CommandSender.h"
#include "EngineUtils.h"
#include "FGPlayerController.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "BlueprintX"

namespace BXSaveCommand
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float DefaultHeightMeters = 8.0f;
	const FLinearColor ErrorColor(1.0f, 0.3f, 0.2f);
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

	/** "<name words...> [height]": a trailing number is the height in metres. */
	FBXSaveRequest ParseRequest(const TArray<FString>& Arguments)
	{
		FBXSaveRequest Request;
		Request.ExtraHeight = BXSaveCommand::DefaultHeightMeters * BXSaveCommand::CentimetersPerMeter;

		int32 NumNameWords = Arguments.Num();
		if (Arguments.Num() > 1 && Arguments.Last().IsNumeric())
		{
			Request.ExtraHeight = FMath::Max(0.0f, FCString::Atof(*Arguments.Last())) * BXSaveCommand::CentimetersPerMeter;
			--NumNameWords;
		}
		for (int32 Index = 0; Index < NumNameWords; ++Index)
		{
			Request.BlueprintName += (Index > 0 ? TEXT(" ") : TEXT("")) + Arguments[Index];
		}
		Request.Description = TEXT("Saved with Blueprint-X.");
		return Request;
	}
}

ABXSaveChatCommand::ABXSaveChatCommand()
{
	CommandName = TEXT("bxsave");
	MinNumberOfArguments = 1;
	bOnlyUsableByPlayer = true;
	Usage = LOCTEXT("BXSaveUsage", "/bxsave <name> [height in m] - save everything between the two nearest Blueprint-X Corners");
}

EExecutionStatus ABXSaveChatCommand::ExecuteCommand_Implementation(UCommandSender* Sender,
	const TArray<FString>& Arguments, const FString& Label)
{
	if (Sender == nullptr)
	{
		return EExecutionStatus::UNCOMPLETED;
	}
	AFGPlayerController* Player = Sender->GetPlayer();
	const APawn* Pawn = Player != nullptr ? Player->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: no player character."), BXSaveCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}

	UBXRemoteCallObject* RemoteCall = Player->GetRemoteCallObjectOfClass<UBXRemoteCallObject>();
	if (RemoteCall == nullptr)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: remote call object not registered (mod install problem)."),
			BXSaveCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}

	const TArray<ABXCornerMarker*> Corners = FindNearestCorners(GetWorld(), Pawn->GetActorLocation());
	if (Corners.Num() < 2)
	{
		Sender->SendChatMessage(TEXT("Blueprint-X: build two Blueprint-X Corners first."), BXSaveCommand::ErrorColor);
		return EExecutionStatus::UNCOMPLETED;
	}

	// Chat commands already run on the server, so this executes locally; the window will call it from clients.
	RemoteCall->Server_CreateBlueprint(Corners[0], Corners[1], ParseRequest(Arguments));
	return EExecutionStatus::COMPLETED;
}

#undef LOCTEXT_NAMESPACE
