// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_DirectionalDodge.generated.h"

UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Forward     UMETA(DisplayName = "Forward"),
	Backward    UMETA(DisplayName = "Backward"),
	Left        UMETA(DisplayName = "Left"),
	Right       UMETA(DisplayName = "Right"),
	None        UMETA(DisplayName = "None")
};


class AMYYCharacterBase;

UCLASS()
class MYY_API UGA_DirectionalDodge : public UGA_WeaponBase
{
	GENERATED_BODY()

public:
	UGA_DirectionalDodge();

	// CORRECT SIGNATURE - matches GA_WeaponBase signature
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// ActivateAbility signature - matches UGameplayAbility
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	bool bUseRootMotion = true;

	// Single roll montage - will be pulled from WeaponDataAsset
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	UAnimMontage* DefaultDodgeMontage;

private:
	void PerformDodge();
	EDodgeDirection CalculateDodgeDirection() const;
	UAnimMontage* GetDodgeMontage() const;
	void RotateCharacterToDodgeDirection(AMYYCharacterBase* Character, EDodgeDirection Direction);
	void ApplyDodgeMovement(AMYYCharacterBase* Character, EDodgeDirection Direction);
	void OnDodgeComplete();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();
};