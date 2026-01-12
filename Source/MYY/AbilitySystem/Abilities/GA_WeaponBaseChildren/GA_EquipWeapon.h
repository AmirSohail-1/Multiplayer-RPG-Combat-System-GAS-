// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_EquipWeapon.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UGA_EquipWeapon : public UGA_WeaponBase
{
	GENERATED_BODY()
public:
	UGA_EquipWeapon();

	// CORRECT - Remove ActivationInfo parameter
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayTagContainer* SourceTags = nullptr, 
		const FGameplayTagContainer* TargetTags = nullptr, 
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// CORRECT - Use FGameplayEventData* instead of FGameplayTagContainer*
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon();

	 
};