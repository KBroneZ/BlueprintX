// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "BXBlueprintSaver.generated.h"

class ABXCornerMarker;
class UWorld;

/** What the player asks for. Small on purpose: this is all that travels from client to server. */
USTRUCT(BlueprintType)
struct BLUEPRINTX_API FBXSaveRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Blueprint-X")
	FString BlueprintName;

	UPROPERTY(BlueprintReadWrite, Category = "Blueprint-X")
	FString Description;

	/** Height (cm) added above the higher corner marker. */
	UPROPERTY(BlueprintReadWrite, Category = "Blueprint-X")
	float ExtraHeight = 800.0f;
};

struct BLUEPRINTX_API FBXSaveResult
{
	bool bSuccess = false;
	int32 NumBuildables = 0;
	FString Message;

	static FBXSaveResult Fail(const FString& InMessage) { return {false, 0, InMessage}; }
};

/**
 * Server side: collects everything between two corner markers and saves it as a blueprint,
 * with the same calls the vanilla Blueprint Designer uses (file, server manifest, descriptor).
 */
class BLUEPRINTX_API FBXBlueprintSaver
{
public:
	static FBXSaveResult SaveArea(UWorld* World, ABXCornerMarker* CornerA, ABXCornerMarker* CornerB,
		const FBXSaveRequest& Request);
};
