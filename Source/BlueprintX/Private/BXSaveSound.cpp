// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXSaveSound.h"

#include "BlueprintX.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

namespace BXSaveSound
{
	const TCHAR* const PersonalPackage = TEXT("/BlueprintX/Audio/Personal/S_BXSavePersonal");
	const TCHAR* const PersonalObject = TEXT("/BlueprintX/Audio/Personal/S_BXSavePersonal.S_BXSavePersonal");
	const TCHAR* const PublicObject = TEXT("/BlueprintX/Audio/S_BXSave.S_BXSave");
}

void FBXSaveSound::Play(const UObject* WorldContext)
{
	using namespace BXSaveSound;

	// Loaded by path (the sound is cooked with the rest of the mod content); C++ cannot hold a
	// hard reference to mod content.
	USoundBase* Sound = nullptr;
	if (FPackageName::DoesPackageExist(PersonalPackage))
	{
		Sound = LoadObject<USoundBase>(nullptr, PersonalObject);
	}
	if (Sound == nullptr)
	{
		Sound = LoadObject<USoundBase>(nullptr, PublicObject);
	}
	if (Sound == nullptr)
	{
		UE_LOG(LogBlueprintX, Warning, TEXT("Save sound %s not found (mod install problem)."), PublicObject);
		return;
	}
	UGameplayStatics::PlaySound2D(WorldContext, Sound);
}
