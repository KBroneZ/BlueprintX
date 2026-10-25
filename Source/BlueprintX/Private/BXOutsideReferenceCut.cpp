// Blueprint-X - Copyright (C) 2026 KBroneZ. Licensed under the GNU GPL v3 (see LICENSE).

#include "BXOutsideReferenceCut.h"

#include "BlueprintX.h"
#include "Buildables/FGBuildable.h"
#include "FGBlueprintSubsystem.h"
#include "FGSubsystem.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Pointers the save writes as object references. Soft and lazy pointers are paths, not
	 *  touched here (the game does not use them for connections). */
	const FObjectPropertyBase* AsReference(const FProperty* Property)
	{
		if (Property != nullptr && (Property->IsA<FObjectProperty>() || Property->IsA<FWeakObjectProperty>()))
		{
			return static_cast<const FObjectPropertyBase*>(Property);
		}
		return nullptr;
	}

	const AActor* GetOwningActor(const UObject* Object)
	{
		const AActor* Actor = Cast<AActor>(Object);
		return Actor != nullptr ? Actor : Object->GetTypedOuter<AActor>();
	}

	/** Same filter as the save archive: only SaveGame properties end up in a blueprint. */
	template <typename FMayHoldReferences, typename FOnProperty>
	void ForEachSavedProperty(const UStruct* Struct, void* Container, FMayHoldReferences&& MayHoldReferences,
		FOnProperty&& OnProperty)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_SaveGame) || !MayHoldReferences(*It))
			{
				continue;
			}
			for (int32 Index = 0; Index < It->ArrayDim; ++Index)
			{
				OnProperty(*It, It->ContainerPtrToValuePtr<void>(Container, Index));
			}
		}
	}
}

FBXOutsideReferenceCut::FBXOutsideReferenceCut(UWorld* World, const TArray<AFGBuildable*>& Buildables)
{
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
	if (BlueprintSubsystem == nullptr)
	{
		return;
	}

	// The exact objects WriteBlueprintToArchive serializes: it calls CollectObjects on its own copy
	// of the list too (checked like D-013).
	TArray<AFGBuildable*> Roots = Buildables;
	TArray<UObject*> Objects;
	BlueprintSubsystem->CollectObjects(Roots, Objects);

	WorldPackage = World->GetOutermost();
	SavedActors.Reserve(Objects.Num());
	SavedObjects.Reserve(Objects.Num());
	for (const AFGBuildable* Buildable : Buildables)
	{
		SavedActors.Add(Buildable);
	}
	for (const UObject* Object : Objects)
	{
		SavedObjects.Add(Object);
		if (const AActor* Actor = Cast<AActor>(Object))
		{
			SavedActors.Add(Actor);
		}
	}

	for (UObject* Object : Objects)
	{
		if (IsValid(Object))
		{
			ForEachSavedProperty(Object->GetClass(), Object,
				[this](const FProperty* Property) { return MayHoldReferences(Property); },
				[this](const FProperty* Property, void* Address) { CutValue(Property, Address); });
		}
	}
}

FBXOutsideReferenceCut::~FBXOutsideReferenceCut()
{
	// Back to front, the reverse of the cut.
	for (int32 Index = CutArrays.Num() - 1; Index >= 0; --Index)
	{
		const FCutArray& Cut = CutArrays[Index];
		const FObjectPropertyBase* Inner = AsReference(Cut.Property->Inner);
		FScriptArrayHelper Helper(Cut.Property, Cut.Address);
		Helper.Resize(Cut.Original.Num());
		for (int32 Element = 0; Element < Cut.Original.Num(); ++Element)
		{
			Inner->SetObjectPropertyValue(Helper.GetRawPtr(Element), Cut.Original[Element]);
		}
	}
	for (int32 Index = CutObjects.Num() - 1; Index >= 0; --Index)
	{
		const FCutObject& Cut = CutObjects[Index];
		Cut.Property->SetObjectPropertyValue(Cut.Address, Cut.Original);
	}
}

FString FBXOutsideReferenceCut::DescribeCut(int32 MaxClasses) const
{
	TArray<TPair<const UClass*, int32>> Sorted = CutByClass.Array();
	Sorted.Sort([](const TPair<const UClass*, int32>& A, const TPair<const UClass*, int32>& B) { return A.Value > B.Value; });
	TArray<FString> Parts;
	for (int32 Index = 0; Index < Sorted.Num() && Index < MaxClasses; ++Index)
	{
		Parts.Add(FString::Printf(TEXT("%dx %s"), Sorted[Index].Value, *GetNameSafe(Sorted[Index].Key)));
	}
	if (Sorted.Num() > MaxClasses)
	{
		Parts.Add(TEXT("..."));
	}
	return FString::Join(Parts, TEXT(", "));
}

void FBXOutsideReferenceCut::CutValue(const FProperty* Property, void* Address)
{
	if (const FObjectPropertyBase* Reference = AsReference(Property))
	{
		UObject* Target = Reference->GetObjectPropertyValue(Address);
		if (Target != nullptr && !IsSaved(Target))
		{
			CutObjects.Add({Reference, Address, Target});
			Reference->SetObjectPropertyValue(Address, nullptr);
			NoteCut(Target);
		}
	}
	else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		if (const FObjectPropertyBase* Inner = AsReference(ArrayProperty->Inner))
		{
			CutObjectArray(ArrayProperty, Inner, Address);
		}
		else if (const FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProperty->Inner))
		{
			// Only leaf arrays of pointers are resized, so element addresses here stay valid.
			FScriptArrayHelper Helper(ArrayProperty, Address);
			for (int32 Element = 0; Element < Helper.Num(); ++Element)
			{
				CutValue(InnerStruct, Helper.GetRawPtr(Element));
			}
		}
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		ForEachSavedProperty(StructProperty->Struct, Address,
			[this](const FProperty* Member) { return MayHoldReferences(Member); },
			[this](const FProperty* Member, void* MemberAddress) { CutValue(Member, MemberAddress); });
	}
	else if (!WarnedProperties.Contains(Property))
	{
		// Maps, sets, interfaces, soft pointers: no game connection uses them today. Warn so a game
		// update that adds one shows up in the log.
		WarnedProperties.Add(Property);
		UE_LOG(LogBlueprintX, Warning, TEXT("Blueprint save: references in %s (%s) are not checked for leaving the box."),
			*Property->GetPathName(), *Property->GetClass()->GetName());
	}
}

bool FBXOutsideReferenceCut::MayHoldReferences(const FProperty* Property)
{
	if (const bool* Cached = MayHoldReferencesCache.Find(Property))
	{
		return *Cached;
	}
	TArray<const FStructProperty*> EncounteredStructs;
	const bool bMayHold = Property->ContainsObjectReference(EncounteredStructs,
		EPropertyObjectReferenceType::Strong | EPropertyObjectReferenceType::Weak);
	MayHoldReferencesCache.Add(Property, bMayHold);
	return bMayHold;
}

void FBXOutsideReferenceCut::CutObjectArray(const FArrayProperty* ArrayProperty, const FObjectPropertyBase* Inner,
	void* Address)
{
	FScriptArrayHelper Helper(ArrayProperty, Address);
	TArray<UObject*> Original;
	TArray<UObject*> Kept;
	Original.Reserve(Helper.Num());
	Kept.Reserve(Helper.Num());
	for (int32 Element = 0; Element < Helper.Num(); ++Element)
	{
		UObject* Target = Inner->GetObjectPropertyValue(Helper.GetRawPtr(Element));
		Original.Add(Target);
		if (Target == nullptr || IsSaved(Target))
		{
			Kept.Add(Target);
		}
		else
		{
			NoteCut(Target);
		}
	}
	if (Kept.Num() == Original.Num())
	{
		return;
	}

	// Removed rather than nulled: the game warns about null entries in lists like mWires.
	Helper.Resize(Kept.Num());
	for (int32 Element = 0; Element < Kept.Num(); ++Element)
	{
		Inner->SetObjectPropertyValue(Helper.GetRawPtr(Element), Kept[Element]);
	}
	CutArrays.Add({ArrayProperty, Address, MoveTemp(Original)});
}

bool FBXOutsideReferenceCut::IsSaved(const UObject* Target) const
{
	// Classes, recipes, descriptors and other assets live in their own packages and are the same
	// in every world, like the map content of streamed levels.
	const UPackage* Package = Target->GetOutermost();
	if (Package != WorldPackage && Package != GetTransientPackage())
	{
		return true;
	}
	if (SavedObjects.Contains(Target))
	{
		return true;
	}
	const AActor* Owner = GetOwningActor(Target);
	if (Owner == nullptr)
	{
		return false;
	}
	if (SavedActors.Contains(Owner))
	{
		return true;
	}
	// World singletons such as the buildable subsystem have the same name in every world and
	// vanilla blueprints reference them too. What they own (power circuits...) is per world.
	if (Target == Owner && Owner->IsA<AFGSubsystem>())
	{
		return true;
	}
	// Actors placed in the map (water volumes, resource nodes) exist in every save of the map;
	// vanilla blueprints reference them too. Everything built while playing is spawned.
	return Owner->HasAnyFlags(RF_WasLoaded) || Owner->IsNetStartupActor();
}

void FBXOutsideReferenceCut::NoteCut(const UObject* Target)
{
	const AActor* Owner = GetOwningActor(Target);
	++CutByClass.FindOrAdd(Owner != nullptr ? Owner->GetClass() : Target->GetClass());
	++NumCut;
}
