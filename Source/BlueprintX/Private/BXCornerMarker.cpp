// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXCornerMarker.h"

#include "BXCornerMenuWidget.h"
#include "FGColoredInstanceMeshProxy.h"
#include "Hologram/FGBuildableHologram.h"

namespace BXCorner
{
	// Sizes in cm; must match SourceArt/Marker/build_corner_marker.py.
	constexpr float FootprintWidth = 100.0f;
	constexpr float MarkerHeight = 300.0f;
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

	mMarkerMesh = CreateDefaultSubobject<UFGColoredInstanceMeshProxy>(TEXT("MarkerMesh"));
	mMarkerMesh->SetupAttachment(RootComponent);
	mMarkerMesh->SetMobility(EComponentMobility::Static);

	FFGClearanceData Clearance;
	Clearance.ClearanceBox = FBox(
		FVector(-FootprintWidth * 0.5f, -FootprintWidth * 0.5f, ClearanceGap),
		FVector(FootprintWidth * 0.5f, FootprintWidth * 0.5f, MarkerHeight));
	mClearanceData.Add(Clearance);
}

UMaterialInterface* ABXCornerMarker::GetMarkerMaterial() const
{
	return mMarkerMesh != nullptr ? mMarkerMesh->GetMaterial(0) : nullptr;
}
