#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckStamina.generated.h"
 
UCLASS()
class MYY_API UBTDecorator_CheckStamina : public UBTDecorator
{
	GENERATED_BODY()
public:
	UBTDecorator_CheckStamina();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float MinimumStamina = 20.f;
	
};
