#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RangedAttack.generated.h"

UCLASS()
class MYY_API UBTTask_RangedAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RangedAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MaxAttackRange = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MinAttackRange = 800.f;
};