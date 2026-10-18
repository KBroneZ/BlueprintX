// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Command/ChatCommandInstance.h"
#include "BXSaveChatCommand.generated.h"

/**
 * Debug trigger until the marker window exists: "/bxsave <name> [h=<height in m>]".
 * Uses the two corner markers nearest to the player. Chat commands run on the server; the
 * command goes through UBXRemoteCallObject so the answer reaches the player like the window's.
 */
UCLASS()
class BLUEPRINTX_API ABXSaveChatCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ABXSaveChatCommand();

	virtual EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments,
		const FString& Label) override;
};
