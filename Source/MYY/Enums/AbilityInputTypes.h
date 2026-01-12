// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilityInputTypes.generated.h"


UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None = 0,
	Confirm = 1,
	Cancel = 2,
	PrimaryAttack = 3,
	SecondaryAttack = 4,
	Block = 5,
	Dodge = 6,
	Interact = 7,
	Skill1 = 8,
	Skill2 = 9,
	Ultimate = 10,
	Equip = 11,
	Unequip = 12,
	Aim = 13,
	Fire = 14,
	Vault = 15  UMETA(DisplayName = "Vault"),
};

/**
 * 
 */
UCLASS()
class MYY_API UAbilityInputTypes : public UObject
{
	GENERATED_BODY()
};
