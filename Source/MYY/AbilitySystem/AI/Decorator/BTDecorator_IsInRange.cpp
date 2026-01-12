#include "BTDecorator_IsInRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsInRange::UBTDecorator_IsInRange()
{
	NodeName = "Is In Range";
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInRange, TargetActorKey), AActor::StaticClass());
}

bool UBTDecorator_IsInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		return false;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return false;
	}

	float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
	return (Distance >= MinRange && Distance <= MaxRange);
}