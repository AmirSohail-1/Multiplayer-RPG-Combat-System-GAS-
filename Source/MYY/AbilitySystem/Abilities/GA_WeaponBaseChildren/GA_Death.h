// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Death.generated.h"


class UAbilityTask_PlayMontageAndWait;


/**
 * 
 */
UCLASS()
class MYY_API UGA_Death : public UGA_WeaponBase
{
	GENERATED_BODY()
public:

	UGA_Death();

protected:

	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* MontageTask;

	// Death ability always activates, regardless of stamina or restrictions
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
						   const FGameplayAbilityActorInfo* ActorInfo,
						   FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
									const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayTagContainer* SourceTags,
									const FGameplayTagContainer* TargetTags,
									FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnDeathMontageCompleted();
};
