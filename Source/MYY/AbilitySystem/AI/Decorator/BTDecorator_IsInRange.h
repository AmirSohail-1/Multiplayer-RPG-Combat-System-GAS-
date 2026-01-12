#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsInRange.generated.h"


UCLASS()
class MYY_API UBTDecorator_IsInRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_IsInRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float MaxRange = 500.f;
};
