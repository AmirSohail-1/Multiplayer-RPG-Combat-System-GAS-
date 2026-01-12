#include "BTTask_RangedAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UBTTask_RangedAttack::UBTTask_RangedAttack()
{
    NodeName = "Ranged Attack";
    bNotifyTick = false;

    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_RangedAttack, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_RangedAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }

    AMYYCharacterBase* AICharacter = Cast<AMYYCharacterBase>(AIController->GetPawn());
    if (!AICharacter || !AICharacter->AbilitySystemComponent)
    {
        return EBTNodeResult::Failed;
    }

    // Get target
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    
    if (!TargetActor)
    {
        return EBTNodeResult::Failed;
    }

    // Check if in range
    float Distance = FVector::Dist(AICharacter->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > MaxAttackRange || Distance < MinAttackRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_RangedAttack: Target not in optimal range (%.2f)"), Distance);
        return EBTNodeResult::Failed;
    }

    // Try to activate ranged attack ability
    FGameplayTagContainer AttackTags;
    AttackTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Attack.Ranged"));

    bool bActivated = AICharacter->AbilitySystemComponent->TryActivateAbilitiesByTag(AttackTags);

    UE_LOG(LogTemp, Log, TEXT("BTTask_RangedAttack: Attack %s for %s"), 
        bActivated ? TEXT("SUCCESS") : TEXT("FAILED"), 
        *AICharacter->GetName());

    return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}