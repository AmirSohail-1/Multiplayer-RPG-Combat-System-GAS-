// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Stagger.generated.h"

/**
 * Triggered when attacker is parried
 * Plays stagger animation and prevents actions
 */


UCLASS()
class MYY_API UGA_Stagger : public UGA_WeaponBase
{
	GENERATED_BODY()

public:
	UGA_Stagger();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnStaggerMontageCompleted();

	UPROPERTY(EditDefaultsOnly, Category = "Stagger")
	float StaggerDuration = 1.5f;
	
};
