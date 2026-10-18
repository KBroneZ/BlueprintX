// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "BXBlueprintSaver.h"
#include "Engine/TimerHandle.h"
#include "BXRemoteCallObject.generated.h"

class ABXCornerMarker;

/** Client side: the server answered a save request (bSuccess, NumBuildables, Message). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FBXOnCreateResult, bool, int32, const FString&);

/** Client side: the server counted an area (RequestId, NumBuildables, NumExcluded, Message).
 *  NumBuildables is INDEX_NONE when the count failed; Message then says why. */
DECLARE_MULTICAST_DELEGATE_FourParams(FBXOnCountResult, int32, int32, int32, const FString&);

/**
 * Client <-> server calls for Blueprint-X. The client only sends two marker references and the
 * request text (a few hundred bytes), so the 64 KB RPC limit can never be reached (bug 001).
 * Registered with SML through RootGameInstance_BlueprintX.
 */
UCLASS()
class BLUEPRINTX_API UBXRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Asks the server to save everything between the two corner markers as a blueprint. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Blueprint-X")
	void Server_CreateBlueprint(ABXCornerMarker* CornerA, ABXCornerMarker* CornerB, const FBXSaveRequest& Request);

	/** Server answer to Server_CreateBlueprint. */
	UFUNCTION(Client, Reliable)
	void Client_CreateResult(bool bSuccess, int32 NumBuildables, const FString& Message);

	/** Asks the server how many buildings a save between the two markers would contain.
	 *  In: two marker references + height; out: one number (Client_CountResult). */
	UFUNCTION(Server, Reliable)
	void Server_CountBuildables(ABXCornerMarker* CornerA, ABXCornerMarker* CornerB, float ExtraHeight, int32 RequestId);

	/** Server answer to Server_CountBuildables. */
	UFUNCTION(Client, Reliable)
	void Client_CountResult(int32 RequestId, int32 NumBuildables, int32 NumExcluded, const FString& Message);

	/** Broadcast on the client when Client_CreateResult arrives (the marker window listens). */
	FBXOnCreateResult OnCreateResult;

	/** Broadcast on the client when Client_CountResult arrives (the marker window listens). */
	FBXOnCountResult OnCountResult;

	/** RCOs need at least one replicated property to get a net channel. */
	UPROPERTY(Replicated, Meta = (NoAutoJson))
	bool mForceNetField_UBXRemoteCallObject = false;

private:
	/** Server: count request waiting for its turn (only the newest one is kept). */
	struct FBXPendingCount
	{
		TWeakObjectPtr<ABXCornerMarker> CornerA;
		TWeakObjectPtr<ABXCornerMarker> CornerB;
		float ExtraHeight = 0.0f;
		int32 RequestId = 0;
	};

	/** Server: counts mPendingCount and answers the client. */
	void RunPendingCount();

	FBXPendingCount mPendingCount;
	FTimerHandle mCountTimer;

	/** Server: time of the last count, so a client cannot keep the server busy counting. */
	double mLastCountSeconds = 0.0;
};
