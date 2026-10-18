// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildable.h"
#include "BXCornerMarker.generated.h"

class UFGColoredInstanceMeshProxy;

/**
 * Blueprint-X Corner: a small marker the player builds at two opposite corners of an area.
 * The actor location (bottom centre of the base plate) is the corner point of the box.
 *
 * Meshes and materials are assigned in the Blueprint child (Build_BXCorner), because C++
 * constructors cannot safely load mod content.
 */
UCLASS()
class BLUEPRINTX_API ABXCornerMarker : public AFGBuildable
{
	GENERATED_BODY()

public:
	ABXCornerMarker();

	/** World location of the corner point this marker defines. */
	UFUNCTION(BlueprintPure, Category = "Blueprint-X")
	FVector GetCornerLocation() const { return GetActorLocation(); }

protected:
	/** Flat base plate that sits on the ground or foundation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blueprint-X")
	TObjectPtr<UFGColoredInstanceMeshProxy> mBaseMesh;

	/** Tall thin pole so the marker can be seen from far away. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blueprint-X")
	TObjectPtr<UFGColoredInstanceMeshProxy> mPoleMesh;
};
