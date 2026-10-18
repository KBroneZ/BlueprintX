// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Command/ChatCommandInstance.h"
#include "BXChatCommand.generated.h"

/**
 * The /bx chat command group (chat commands run on the server):
 *   /bx help                      - list the commands
 *   /bx save <name> [h=<metres>]  - save everything between the two Corners nearest to the player
 *   /bx clear                     - remove every Blueprint-X Corner and refund its cost to the player
 * "/bxsave <name> [h=<metres>]" stays as an alias of "/bx save" (0.3.0 - 0.6.0 used it).
 * Any player may run every command, also on a dedicated server (D-021).
 */
UCLASS()
class BLUEPRINTX_API ABXChatCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ABXChatCommand();

	virtual EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments,
		const FString& Label) override;

private:
	EExecutionStatus RunSave(UCommandSender* Sender, const TArray<FString>& Arguments);
	EExecutionStatus RunClear(UCommandSender* Sender);
	EExecutionStatus RunHelp(UCommandSender* Sender);
};
