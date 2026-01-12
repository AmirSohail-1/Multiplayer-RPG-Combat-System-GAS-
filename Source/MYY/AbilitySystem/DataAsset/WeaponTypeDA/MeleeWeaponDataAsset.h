// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "MeleeWeaponDataAsset.generated.h"



/**
 * MELEE WEAPON DATA ASSET
 * Use for: Swords, Axes, Hammers
 * Extends base WeaponDataAsset with melee-specific features
 */
UCLASS(BlueprintType)
class MYY_API UMeleeWeaponDataAsset : public UWeaponDataAsset
{
	GENERATED_BODY()

public:
	UMeleeWeaponDataAsset()
	{
		WeaponType = EWeaponType::Melee;
		
		// NEW: Melee weapons use full body animations
		Animations.AnimLayerUsage = EAnimLayerUsage::FullBodyOnly;
	}

 
	// ========== MELEE-SPECIFIC COMBO DATA ==========
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo")
	float ComboWindowDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo")
	float ComboDamageMultiplier = 0.2f; // +20% per combo hit

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo")
	float ComboResetTime = 2.0f;

	// ========== OVERRIDE VIRTUAL FUNCTIONS ==========
    
	virtual bool SupportsComboSystem() const override 
	{ 
		return Animations.ComboMontages.Num() > 0; 
	}
    
	virtual bool HasTraceSystem() const override { return true; }
};