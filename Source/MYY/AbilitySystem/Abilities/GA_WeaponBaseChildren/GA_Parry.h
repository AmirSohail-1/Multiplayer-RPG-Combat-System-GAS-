// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Parry.generated.h"

/**
 * Triggered when a perfect parry occurs
 * Plays parry success animation and staggers attacker
 */
UCLASS()
class MYY_API UGA_Parry : public UGA_WeaponBase
{
	GENERATED_BODY()
public:
	UGA_Parry();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnParryMontageCompleted();

	UPROPERTY(EditDefaultsOnly, Category = "Parry")
	float ParryStunDuration = 2.0f;
	
};
