// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "WeaponDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UWeaponDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	UWeaponDataAsset* GetByID(int32 ID) const;

private:

	// Local memory cache of ID → PDA
	UPROPERTY()
	TMap<int32, UWeaponDataAsset*> WeaponCache;
	
};
