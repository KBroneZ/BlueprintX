// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "BXBlueprintSaver.h"
#include "BXRemoteCallObject.generated.h"

class ABXCornerMarker;

/** Client side: the server answered a save request (bSuccess, NumBuildables, Message). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FBXOnCreateResult, bool, int32, const FString&);

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

	/** Broadcast on the client when Client_CreateResult arrives (the marker window listens). */
	FBXOnCreateResult OnCreateResult;

	/** RCOs need at least one replicated property to get a net channel. */
	UPROPERTY(Replicated, Meta = (NoAutoJson))
	bool mForceNetField_UBXRemoteCallObject = false;
};
