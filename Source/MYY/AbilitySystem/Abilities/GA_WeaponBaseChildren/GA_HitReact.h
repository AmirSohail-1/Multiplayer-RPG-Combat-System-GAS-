// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_HitReact.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UGA_HitReact : public UGA_WeaponBase
{
	GENERATED_BODY()
public:
	UGA_HitReact();
	
	bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	                        FGameplayTagContainer* OptionalRelevantTags) const;
	void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                     FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	UFUNCTION(  Category = "Ability")
	void PlayRandomHitReactMontage();
	
	UFUNCTION(  Category = "Ability")
	void OnHitReactMontageCompleted();

	UFUNCTION()
	void OnBlockMontageBlendOut();
};
