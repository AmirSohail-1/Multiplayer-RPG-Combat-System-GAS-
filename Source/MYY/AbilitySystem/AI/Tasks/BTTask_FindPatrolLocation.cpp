#include "BTTask_FindPatrolLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

UBTTask_FindPatrolLocation::UBTTask_FindPatrolLocation()
{
	NodeName = "Find Patrol Location";
    
	PatrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolLocation, PatrolLocationKey));
	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolLocation, HomeLocationKey));
}




EBTNodeResult::Type UBTTask_FindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    // Get home location from blackboard
    FVector HomeLocation = BlackboardComp->GetValueAsVector(HomeLocationKey.SelectedKeyName);
    if (HomeLocation.IsZero())
    {
        HomeLocation = AIController->GetPawn()->GetActorLocation();
    }

    // Find random navigable point around home
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Error, TEXT("BTTask_FindPatrolLocation: No Navigation System!"));
        return EBTNodeResult::Failed;
    }

    FNavLocation ResultLocation;
    bool bSuccess = NavSys->GetRandomPointInNavigableRadius(HomeLocation, PatrolRadius, ResultLocation);

    if (bSuccess)
    {
        BlackboardComp->SetValueAsVector(PatrolLocationKey.SelectedKeyName, ResultLocation.Location);
        UE_LOG(LogTemp, Log, TEXT("BTTask_FindPatrolLocation: Found patrol point at %s"), 
            *ResultLocation.Location.ToString());
        return EBTNodeResult::Succeeded;
    }

    // ✅ FALLBACK: If navigation fails, just move in random direction
    // UE_LOG(LogTemp, Warning, TEXT("BTTask_FindPatrolLocation: NavMesh failed, using fallback"));
    
    FVector RandomOffset = FVector(
        FMath::RandRange(-PatrolRadius, PatrolRadius),
        FMath::RandRange(-PatrolRadius, PatrolRadius),
        0.f
    );
    
    FVector FallbackLocation = HomeLocation + RandomOffset;
    BlackboardComp->SetValueAsVector(PatrolLocationKey.SelectedKeyName, FallbackLocation);
    
    return EBTNodeResult::Succeeded;
}