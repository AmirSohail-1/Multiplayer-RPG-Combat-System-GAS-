// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_WeaponBase.generated.h"

class UWeaponDataAsset;
class ABaseWeapon;
 
UCLASS()
class MYY_API UGA_WeaponBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WeaponBase();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// CORRECT SIGNATURE - matches UGameplayAbility base class
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** If true ability will check stamina cost parsed from the GE before activation */
	UPROPERTY(EditDefaultsOnly, Category = "Costs")
	bool bCheckStaminaBeforeActivate = true;
 
	UPROPERTY(EditDefaultsOnly, Category = "Costs")
	float RequiredStaminaCost = 10.f;   // default, can be overridden per GA
 

protected:
	// Cached weapon data - no runtime lookups needed
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	UWeaponDataAsset* CachedWeaponData;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	UWeaponDataAsset* GetWeaponData() const { return CachedWeaponData; }

	UFUNCTION(BlueprintCallable, Category = "Ability")
	ABaseWeapon* GetCurrentWeapon() const;
 
 
	bool CheckStaminaCost() const;

	// ApplyCost can be left empty because the GE itself contains the stamina modifier (if you set it),
	// but we keep it for extensibility.
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
						   const FGameplayAbilityActorInfo* ActorInfo,
						   const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
};