// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXRemoteCallObject.h"

#include "BlueprintX.h"
#include "BXCornerMarker.h"
#include "FGChatManager.h"
#include "FGPlayerController.h"
#include "Net/UnrealNetwork.h"

namespace BXRemoteCall
{
	const FLinearColor SuccessColor(0.2f, 0.9f, 0.3f);
	const FLinearColor ErrorColor(1.0f, 0.3f, 0.2f);
}

void UBXRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBXRemoteCallObject, mForceNetField_UBXRemoteCallObject);
}

void UBXRemoteCallObject::Server_CreateBlueprint_Implementation(ABXCornerMarker* CornerA, ABXCornerMarker* CornerB,
	const FBXSaveRequest& Request)
{
	const AFGPlayerController* Player = GetOwnerPlayerController();
	UE_LOG(LogBlueprintX, Display, TEXT("Save request '%s' from %s."), *Request.BlueprintName,
		Player != nullptr ? *Player->GetName() : TEXT("unknown player"));

	const FBXSaveResult Result = FBXBlueprintSaver::SaveArea(GetWorld(), CornerA, CornerB, Request);
	if (!Result.bSuccess)
	{
		UE_LOG(LogBlueprintX, Warning, TEXT("Save request '%s' failed: %s"), *Request.BlueprintName, *Result.Message);
	}
	Client_CreateResult(Result.bSuccess, Result.NumBuildables, Result.Message);
}

void UBXRemoteCallObject::Client_CreateResult_Implementation(bool bSuccess, int32 NumBuildables, const FString& Message)
{
	UE_LOG(LogBlueprintX, Display, TEXT("Save result: %s (%d buildings) %s"),
		bSuccess ? TEXT("OK") : TEXT("FAILED"), NumBuildables, *Message);

	AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld());
	if (ChatManager == nullptr)
	{
		return;
	}
	FChatMessageStruct ChatMessage;
	ChatMessage.MessageType = EFGChatMessageType::CMT_CustomMessage;
	ChatMessage.MessageSender = FText::FromString(TEXT("Blueprint-X"));
	ChatMessage.MessageSenderColor = bSuccess ? BXRemoteCall::SuccessColor : BXRemoteCall::ErrorColor;
	ChatMessage.MessageText = FText::FromString(Message);
	ChatManager->AddChatMessageToReceived(ChatMessage);
}
