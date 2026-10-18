// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXBoxPreview.h"

#include "BXAreaCollector.h"
#include "Components/StaticMeshComponent.h"

namespace BXBoxPreview
{
	// Sizes in cm. The beam mesh is a 1 m cube centred on its origin, scaled in place.
	constexpr double CubeSize = 100.0;
	constexpr double BeamThickness = 15.0;
}

ABXBoxPreview::ABXBoxPreview()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);

	for (int32 Index = 0; Index < NumEdges; ++Index)
	{
		UStaticMeshComponent* Beam = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Beam%d"), Index));
		Beam->SetupAttachment(RootComponent);
		Beam->SetMobility(EComponentMobility::Movable);
		Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Beam->SetGenerateOverlapEvents(false);
		Beam->SetCastShadow(false);
		Beam->SetCanEverAffectNavigation(false);
		mBeams.Add(Beam);
	}
}

void ABXBoxPreview::ShowArea(const FBXArea& Area, UStaticMesh* Mesh, UMaterialInterface* Material)
{
	using namespace BXBoxPreview;

	SetActorTransform(Area.Frame);

	const FBox& Box = Area.LocalBox;
	const FVector Size = Box.GetSize();
	const FVector Center = Box.GetCenter();
	const double Min[3] = {Box.Min.X, Box.Min.Y, Box.Min.Z};
	const double Max[3] = {Box.Max.X, Box.Max.Y, Box.Max.Z};

	// For each axis, 4 beams run along it, one at every combination of min/max on the other two axes.
	int32 BeamIndex = 0;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const int32 AxisU = (Axis + 1) % 3;
		const int32 AxisV = (Axis + 2) % 3;
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			FVector Location = Center;
			Location[AxisU] = (Corner & 1) ? Max[AxisU] : Min[AxisU];
			Location[AxisV] = (Corner & 2) ? Max[AxisV] : Min[AxisV];

			FVector Scale(BeamThickness / CubeSize);
			Scale[Axis] = (Size[Axis] + BeamThickness) / CubeSize;

			UStaticMeshComponent* Beam = mBeams[BeamIndex++];
			Beam->SetStaticMesh(Mesh);
			Beam->SetMaterial(0, Material);
			Beam->SetRelativeLocationAndRotation(Location, FRotator::ZeroRotator);
			Beam->SetRelativeScale3D(Scale);
		}
	}
}
