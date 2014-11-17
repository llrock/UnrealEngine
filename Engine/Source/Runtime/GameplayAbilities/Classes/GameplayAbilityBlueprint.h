// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/Blueprint.h"
#include "GameplayAbilityBlueprint.generated.h"

/**
 * A Gameplay Ability Blueprint is essentially a specialized Blueprint whose graphs control a gameplay ability.
 */

UCLASS(BlueprintType)
class GAMEPLAYABILITIES_API UGameplayAbilityBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR

	virtual UClass* GetBlueprintClass() const override;

	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const override
	{
		return false;
	}
	// End of UBlueprint interface

	/** Returns the most base gameplay ability blueprint for a given blueprint (if it is inherited from another ability blueprint, returning null if only native / non-ability BP classes are it's parent) */
	static UGameplayAbilityBlueprint* FindRootGameplayAbilityBlueprint(UGameplayAbilityBlueprint* DerivedBlueprint);

#endif
};
