// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BXBoxPreview.generated.h"

struct FBXArea;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Client-only outline of the box a save would cover: 12 thin beams along the box edges.
 * Never replicated; the marker window spawns it locally and destroys it when it closes.
 */
UCLASS(NotBlueprintable, Transient)
class BLUEPRINTX_API ABXBoxPreview : public AActor
{
	GENERATED_BODY()

public:
	ABXBoxPreview();

	/** Draws the outline of Area. Mesh must be a 1 m cube centred on its origin. */
	void ShowArea(const FBXArea& Area, UStaticMesh* Mesh, UMaterialInterface* Material);

private:
	static constexpr int32 NumEdges = 12;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> mBeams;
};
