// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "RangedWeaponDataAsset.generated.h"

/**
 * Ammo configuration for ranged weapons
 */
USTRUCT(BlueprintType)
struct FAmmoConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 MaxAmmo = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 StartingAmmo = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	bool bInfiniteAmmo = false;
};

/**
 * RANGED WEAPON DATA ASSET
 * Use for: Bows, Guns, Crossbows
 * Extends base WeaponDataAsset with ranged-specific features
 */

UCLASS()
class MYY_API URangedWeaponDataAsset : public UWeaponDataAsset
{
	GENERATED_BODY()
public:
	URangedWeaponDataAsset()
	{
		WeaponType = EWeaponType::Ranged;

		// NEW: Ranged weapons use upper + lower body split
		Animations.AnimLayerUsage = EAnimLayerUsage::UpperLowerSplit;
		
	}

	// ========== RANGED-SPECIFIC DATA ==========
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Projectile")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Projectile")
	FName ProjectileSpawnSocket = "LeftHandSocket";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Combat")
	float DrawTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Combat")
	float ProjectileSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Ammo")
	FAmmoConfig AmmoConfig;

	// Ranged-specific animations (draw, aim, fire)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Animation")
	UAnimMontage* DrawMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Animation")
	UAnimMontage* AimMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|Animation")
	UAnimMontage* FireMontage = nullptr;

	// Add this inside URangedWeaponDataAsset class
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged|UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;


	// ========== OVERRIDE VIRTUAL FUNCTIONS ==========
    
	virtual bool SupportsComboSystem() const override { return false; } 
	virtual bool RequiresProjectiles() const override { return true; }
	virtual bool RequiresAmmo() const override { return !AmmoConfig.bInfiniteAmmo; }
	virtual bool HasTraceSystem() const override { return false; }
	
};