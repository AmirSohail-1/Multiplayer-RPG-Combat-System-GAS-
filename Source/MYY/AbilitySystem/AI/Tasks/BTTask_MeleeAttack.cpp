// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MeleeAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"


UBTTask_MeleeAttack::UBTTask_MeleeAttack()
{
	NodeName = "Melee Attack";
	bNotifyTick = false;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MeleeAttack, TargetActorKey), AActor::StaticClass());
}
 
EBTNodeResult::Type UBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
    if (Distance > AttackRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_MeleeAttack: Target out of range (%.2f > %.2f)"), Distance, AttackRange);
        return EBTNodeResult::Failed;
    }

    // Try to activate melee attack ability
    FGameplayTagContainer AttackTags;
    AttackTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Attack.Melee"));

    bool bActivated = AICharacter->AbilitySystemComponent->TryActivateAbilitiesByTag(AttackTags);

    UE_LOG(LogTemp, Log, TEXT("BTTask_MeleeAttack: Attack %s for %s"), 
        bActivated ? TEXT("SUCCESS") : TEXT("FAILED"), 
        *AICharacter->GetName());

    return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}