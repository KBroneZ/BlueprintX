// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildable.h"
#include "BXCornerMarker.generated.h"

class UFGColoredInstanceMeshProxy;

/**
 * Blueprint-X Corner: a marker post the player builds at two opposite corners of an area.
 * The actor location (base centre of the post) is the corner point of the box; the box is
 * aligned with the first marker's rotation. The model is symmetric (arrows in all four
 * directions) because the box can grow from the corner either way.
 *
 * Meshes and materials are assigned in the Blueprint child (Build_BXCorner), because C++
 * constructors cannot safely load mod content. The model is built in Blender by
 * SourceArt/Marker/build_corner_marker.py.
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

	/** 1 m cube used for the beams of the box preview. */
	UStaticMesh* GetPreviewBeamMesh() const { return mPreviewBeamMesh; }

	/** Orange material of the marker (slot 0), reused by the box preview beams. */
	UMaterialInterface* GetMarkerMaterial() const;

protected:
	/** The marker post (pivot at the base centre, 3 m high, fits one 1 m grid cell). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blueprint-X")
	TObjectPtr<UFGColoredInstanceMeshProxy> mMarkerMesh;

	/** Mesh for the box preview beams; must be a 1 m cube centred on its origin. */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint-X")
	TObjectPtr<UStaticMesh> mPreviewBeamMesh;
};
