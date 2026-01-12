// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_UnequipWeapon.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UGA_UnequipWeapon : public UGA_WeaponBase
{
	GENERATED_BODY()
public:
	UGA_UnequipWeapon();

	// CORRECT SIGNATURE - Match base class
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// ActivateAbility signature from UGameplayAbility
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();
};