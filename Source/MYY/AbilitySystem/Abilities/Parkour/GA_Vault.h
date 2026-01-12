// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_Vault.generated.h"

class UMotionWarpingComponent;
class AMYYCharacterBase;   


USTRUCT(BlueprintType)
struct FVaultTraceResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bCanVault = false;

	UPROPERTY()
	FVector ObstacleTopLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector LandingLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector ObstacleNormal = FVector::ZeroVector;

	UPROPERTY()
	float ObstacleHeight = 0.f;

	UPROPERTY()
	float ObstacleDistance = 0.f;
};


UCLASS()
class MYY_API UGA_Vault : public UGA_WeaponBase
{
	GENERATED_BODY()
public:
	UGA_Vault();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	// ====================================================================
	// VAULT DETECTION PARAMETERS
	// ====================================================================
	
	/** Minimum height of obstacle to trigger vault (prevents vaulting tiny objects) */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection", meta = (ClampMin = "30", ClampMax = "200"))
	float MinVaultHeight = 50.f;

	/** Maximum height character can vault over */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection", meta = (ClampMin = "50", ClampMax = "300"))
	float MaxVaultHeight = 150.f;

	/** How far forward to check for obstacles */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection|Red Sphere ", meta = (ClampMin = "50", ClampMax = "300"))
	float ForwardTraceDistance = 150.f;

	/** How far to check for landing spot beyond obstacle */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection|Green Sphere", meta = (ClampMin = "50", ClampMax = "300"))
	float LandingCheckDistance = 100.f;

	/** Maximum depth of landing spot (prevents vaulting off cliffs) */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection", meta = (ClampMin = "100", ClampMax = "500"))
	float MaxLandingDepth = 200.f;

	/** Minimum obstacle thickness to prevent vaulting through thin walls */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection", meta = (ClampMin = "10", ClampMax = "100"))
	float MinObstacleThickness = 20.f;

	/** Capsule radius for forward trace (should match character capsule) */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection", meta = (ClampMin = "20", ClampMax = "100"))
	float TraceCapsuleRadius = 40.f;

	/** Capsule half-height for forward trace */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Detection", meta = (ClampMin = "20", ClampMax = "100"))
	float TraceCapsuleHalfHeight = 50.f;

	// ====================================================================
	// VAULT EXECUTION PARAMETERS
	// ====================================================================

	UFUNCTION()
	UAnimMontage* GetVaultMontage() const;

	// UFUNCTION()
	// UAnimMontage* GetClimbMontage() const;

	/** Motion Warping sync point name (must match montage notify) */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Animation")
	FName WarpTargetName = TEXT("VaultTarget");

	/** Speed multiplier for vault animation */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Animation", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float VaultPlayRate = 1.0f;

	// ====================================================================
	// WEAPON RESTRICTIONS
	// ====================================================================

	/** If true, can vault while unarmed */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Restrictions")
	bool bAllowVaultUnarmed = true;

	/** If true, can vault with any weapon equipped */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Restrictions")
	bool bAllowVaultWithWeapon = true;

	/** Specific weapon types that allow vaulting (if empty, all weapons allowed) */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Restrictions")
	TArray<FName> AllowedWeaponTypes;

	// ====================================================================
	// DEBUG
	// ====================================================================

	/** Show debug traces and vault detection info */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Debug")
	bool bDebugVault = false;

	/** How long to display debug shapes */
	UPROPERTY(EditDefaultsOnly, Category = "Vault|Debug", meta = (EditCondition = "bDebugVault"))
	float DebugDrawTime = 5.0f;

private:
	// Core vault detection logic
	FVaultTraceResult PerformVaultTraces(const AMYYCharacterBase* Character) const;
	
	// Individual trace functions
	bool TraceForObstacle(const AMYYCharacterBase* Character, FHitResult& OutHit) const;
	bool FindObstacleTop(const FVector& ObstacleHitLocation, const FVector& ForwardDir, FVector& OutTopLocation, float& OutHeight) const;
	bool ValidateLandingSpot(const FVector& ObstacleTop, const FVector& ForwardDir, FVector& OutLandingLocation) const;
	bool CheckObstacleThickness(const FVector& ObstacleTop, const FVector& ForwardDir) const;
	
	// Weapon validation
	bool IsWeaponAllowedForVault() const;
	
	// Execution
	void ExecuteVault(const FVaultTraceResult& VaultData);
	
	// Montage callbacks
	UFUNCTION()
	void OnVaultMontageCompleted();
	
	UFUNCTION()
	void OnVaultMontageCancelled();

	// Debug helpers
	void DrawVaultDebug(const FVaultTraceResult& VaultData, bool bSuccess) const;
};
