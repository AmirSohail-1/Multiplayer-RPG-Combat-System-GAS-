// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "UnarmedCombatDataAsset.generated.h"

/**
 * UNARMED COMBAT DATA ASSET
 * Special case: Has combos but NO weapon actor
 * Used when NO weapons are equipped
 */

// This is used in unarmed combat for fist trace sockets 		-----------------			currently moved to AnimNotify_UnarmedCombatDataAsset.h
// USTRUCT(BlueprintType)
// struct FUnarmedTraceSocket
// {
// 	GENERATED_BODY()
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	FName SocketOrBone;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	float Radius = 12.f;
// };


UCLASS()
class MYY_API UUnarmedCombatDataAsset : public UWeaponDataAsset
{
	GENERATED_BODY()
	
public:
	UUnarmedCombatDataAsset()
	{
		WeaponType = EWeaponType::Unarmed;
		WeaponName = "Unarmed";
		WeaponActorClass = nullptr; // NO weapon actor - bare hands
				
		// NEW: Melee weapons use full body animations
		Animations.AnimLayerUsage = EAnimLayerUsage::FullBodyOnly;
		
	}

	// ========== UNARMED-SPECIFIC ==========
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unarmed")
	float UnarmedDamageMultiplier = 0.5f; // 50% of weapon damage

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unarmed")
	float UnarmedSpeedMultiplier = 1.5f; // 50% faster

	// Fist sockets for hit detection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unarmed|Combat")
	FName LeftFistSocket = "LeftHandSocket";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unarmed|Combat")
	FName RightFistSocket = "RightHandSocket";
};
