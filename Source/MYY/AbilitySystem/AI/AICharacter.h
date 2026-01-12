// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AICharacter.generated.h"


class UBehaviorTree;

UCLASS()
class MYY_API AAICharacter : public AMYYCharacterBase
{
	GENERATED_BODY()

public:
	AAICharacter();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	// Combat parameters for AI decision making
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Combat")
	float MeleeAttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Combat")
	float OptimalCombatDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockChance = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AggressionLevel = 0.7f;

protected:
	virtual void BeginPlay() override;
	void PossessedBy(AController* NewController);
	
};