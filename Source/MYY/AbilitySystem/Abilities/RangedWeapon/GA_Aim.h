// GA_Aim.h
#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Aim.generated.h"

class URangedWeaponDataAsset;
class UAbilityTask_PlayMontageAndWait;
class UUserWidget;

UCLASS()
class MYY_API UGA_Aim : public UGA_WeaponBase
{
	GENERATED_BODY()

public:
	UGA_Aim();

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

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	UPROPERTY()
	UUserWidget* ActiveCrosshairWidget = nullptr;

protected:
	UPROPERTY()
	URangedWeaponDataAsset* RangedWeaponData; 

	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* AimMontageTask;

	UFUNCTION()
	void OnAimMontageInterrupted();

	UFUNCTION()
	void OnAimMontageCancelled();
	
	void OnAimMontageCompleted();

	void CleanupAimState();
	ABaseWeapon* GetCurrentWeapon() const;
};