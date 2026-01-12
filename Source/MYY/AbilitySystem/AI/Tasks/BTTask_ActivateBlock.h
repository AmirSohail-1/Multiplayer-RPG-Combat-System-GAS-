#pragma once
 
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ActivateBlock.generated.h"

UCLASS()
class MYY_API UBTTask_ActivateBlock : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ActivateBlock();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	float BlockDuration = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FBlackboardKeySelector ShouldBlockKey;
};