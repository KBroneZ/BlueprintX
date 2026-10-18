// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXRemoteCallObject.h"

#include "BlueprintX.h"
#include "BXCornerMarker.h"
#include "FGChatManager.h"
#include "FGPlayerController.h"
#include "HAL/PlatformTime.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace BXRemoteCall
{
	const FLinearColor SuccessColor(0.2f, 0.9f, 0.3f);
	const FLinearColor ErrorColor(1.0f, 0.3f, 0.2f);

	// One count per player at most every this many seconds (each count walks every buildable
	// of the world); newer requests wait and only the newest one is answered.
	constexpr double MinCountInterval = 1.0;
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

void UBXRemoteCallObject::Server_CountBuildables_Implementation(ABXCornerMarker* CornerA, ABXCornerMarker* CornerB,
	float ExtraHeight, int32 RequestId)
{
	mPendingCount = {CornerA, CornerB, ExtraHeight, RequestId};

	// Too soon after the last count: answer only the newest request, a moment later.
	const double Wait = mLastCountSeconds + BXRemoteCall::MinCountInterval - FPlatformTime::Seconds();
	UWorld* World = GetWorld();
	if (Wait > 0.0 && World != nullptr)
	{
		if (!World->GetTimerManager().IsTimerActive(mCountTimer))
		{
			World->GetTimerManager().SetTimer(mCountTimer, this, &UBXRemoteCallObject::RunPendingCount, Wait, false);
		}
		return;
	}
	RunPendingCount();
}

void UBXRemoteCallObject::RunPendingCount()
{
	const double StartSeconds = FPlatformTime::Seconds();
	mLastCountSeconds = StartSeconds;
	const FBXPendingCount Request = mPendingCount;

	const FBXCountResult Result = FBXBlueprintSaver::CountArea(GetWorld(), Request.CornerA.Get(), Request.CornerB.Get(),
		Request.ExtraHeight);
	UE_LOG(LogBlueprintX, Display, TEXT("Count request %d: %d buildings, %d excluded, %.2f ms. %s"), Request.RequestId,
		Result.NumBuildables, Result.NumExcluded, (FPlatformTime::Seconds() - StartSeconds) * 1000.0, *Result.Message);
	Client_CountResult(Request.RequestId, Result.bSuccess ? Result.NumBuildables : INDEX_NONE, Result.NumExcluded,
		Result.Message);
}

void UBXRemoteCallObject::Client_CountResult_Implementation(int32 RequestId, int32 NumBuildables, int32 NumExcluded,
	const FString& Message)
{
	OnCountResult.Broadcast(RequestId, NumBuildables, NumExcluded, Message);
}

void UBXRemoteCallObject::Client_CreateResult_Implementation(bool bSuccess, int32 NumBuildables, const FString& Message)
{
	UE_LOG(LogBlueprintX, Display, TEXT("Save result: %s (%d buildings) %s"),
		bSuccess ? TEXT("OK") : TEXT("FAILED"), NumBuildables, *Message);
	OnCreateResult.Broadcast(bSuccess, NumBuildables, Message);

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
