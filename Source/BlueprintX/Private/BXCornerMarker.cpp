// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXCornerMarker.h"

#include "BXCornerMenuWidget.h"
#include "FGColoredInstanceMeshProxy.h"
#include "Hologram/FGBuildableHologram.h"

namespace BXCorner
{
	// Sizes in cm. The meshes are 1 m engine cubes centred on their origin, scaled in place.
	constexpr float CubeSize = 100.0f;
	constexpr float BaseWidth = 100.0f;
	constexpr float BaseHeight = 20.0f;
	constexpr float PoleWidth = 20.0f;
	constexpr float PoleHeight = 300.0f;
	// Keeps the clearance box just above the surface the marker stands on.
	constexpr float ClearanceGap = 1.0f;
}

ABXCornerMarker::ABXCornerMarker()
{
	using namespace BXCorner;

	// FactoryGame leaves this empty on AFGBuildable; without it the build gun cannot place the marker.
	mHologramClass = AFGBuildableHologram::StaticClass();

	// Interacting (E) opens the Blueprint-X window; the widget is pure C++, so no asset path is needed.
	mIsUseable = true;
	mInteractWidgetSoftClass = UBXCornerMenuWidget::StaticClass();

	mBaseMesh = CreateDefaultSubobject<UFGColoredInstanceMeshProxy>(TEXT("BaseMesh"));
	mBaseMesh->SetupAttachment(RootComponent);
	mBaseMesh->SetMobility(EComponentMobility::Static);
	mBaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BaseHeight * 0.5f));
	mBaseMesh->SetRelativeScale3D(FVector(BaseWidth, BaseWidth, BaseHeight) / CubeSize);

	mPoleMesh = CreateDefaultSubobject<UFGColoredInstanceMeshProxy>(TEXT("PoleMesh"));
	mPoleMesh->SetupAttachment(RootComponent);
	mPoleMesh->SetMobility(EComponentMobility::Static);
	mPoleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BaseHeight + PoleHeight * 0.5f));
	mPoleMesh->SetRelativeScale3D(FVector(PoleWidth, PoleWidth, PoleHeight) / CubeSize);

	FFGClearanceData Clearance;
	Clearance.ClearanceBox = FBox(
		FVector(-BaseWidth * 0.5f, -BaseWidth * 0.5f, ClearanceGap),
		FVector(BaseWidth * 0.5f, BaseWidth * 0.5f, BaseHeight + PoleHeight));
	mClearanceData.Add(Clearance);
}

UStaticMesh* ABXCornerMarker::GetMarkerMesh() const
{
	return mBaseMesh != nullptr ? mBaseMesh->GetStaticMesh() : nullptr;
}

UMaterialInterface* ABXCornerMarker::GetMarkerMaterial() const
{
	return mBaseMesh != nullptr ? mBaseMesh->GetMaterial(0) : nullptr;
}
