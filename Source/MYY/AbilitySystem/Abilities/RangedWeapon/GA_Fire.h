// GA_Fire.h
#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Fire.generated.h"

class URangedWeaponDataAsset;
class UAbilityTask_PlayMontageAndWait;

UCLASS()
class MYY_API UGA_Fire : public UGA_WeaponBase
{
	GENERATED_BODY()

public:
	UGA_Fire();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
					   const FGameplayAbilityActorInfo* ActorInfo,
					   const FGameplayAbilityActivationInfo ActivationInfo,
					   bool bReplicateEndAbility,
					   bool bWasCancelled) override;

protected:
	void SpawnProjectile();
	FVector GetCenterScreenAimLocation() const;
	ABaseWeapon* GetCurrentWeapon() const;

	UFUNCTION()
	void OnFireMontageCompleted();

	UPROPERTY()
	URangedWeaponDataAsset* RangedWeaponData;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float CurrentDrawPercent = 1.0f;

	// ✅ ADD THIS: Timer to track montage duration
	FTimerHandle MontageTimerHandle;

	bool IsOnCooldown() const;

	// // ✅ ADD THESE: Cache ability handles for use in montage callback
	FGameplayAbilitySpecHandle CachedSpecHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};