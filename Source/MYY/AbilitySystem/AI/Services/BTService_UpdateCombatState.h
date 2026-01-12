#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatState.generated.h"


UCLASS()
class MYY_API UBTService_UpdateCombatState : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_UpdateCombatState();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FBlackboardKeySelector CanAttackKey;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FBlackboardKeySelector ShouldBlockKey;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackCooldown = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MinStaminaForAttack = 15.0f;

private:
	float LastAttackTime = 0.f;
};

