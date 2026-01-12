// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Block.generated.h"

 

class UAbilityTask_PlayMontageAndWait;
/**
 * 
 */
UCLASS()
class MYY_API UGA_Block : public UGA_WeaponBase
{
	GENERATED_BODY()
public:
	UGA_Block();
	bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	                        FGameplayTagContainer* OptionalRelevantTags) const;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
	                             const FGameplayAbilityActorInfo* ActorInfo, 
	                             const FGameplayAbilityActivationInfo ActivationInfo, 
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		bool bReplicateCancelAbility) override; 

protected:
	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* BlockMontageTask;
 

	UPROPERTY()
	FActiveGameplayEffectHandle ActiveBlockEffectHandle;

	UFUNCTION()
	void OnBlockMontageCompleted();

	UFUNCTION()
	void OnBlockMontageBlendOut();
 
};
