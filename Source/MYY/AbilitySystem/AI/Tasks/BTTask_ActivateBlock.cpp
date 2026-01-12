#include "BTTask_ActivateBlock.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

UBTTask_ActivateBlock::UBTTask_ActivateBlock()
{
    NodeName = "Activate Block";
    bNotifyTick = false;
    
    ShouldBlockKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ActivateBlock, ShouldBlockKey) );
}

EBTNodeResult::Type UBTTask_ActivateBlock::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    // Activate block ability
    FGameplayTagContainer BlockTags;
    BlockTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat.Block"));

    bool bActivated = AICharacter->AbilitySystemComponent->TryActivateAbilitiesByTag(BlockTags);

    if (bActivated)
    {
        UE_LOG(LogTemp, Log, TEXT("BTTask_ActivateBlock: Blocking for %.2f seconds"), BlockDuration);

        // Set timer to stop blocking
        FTimerHandle BlockTimer;
        AIController->GetWorld()->GetTimerManager().SetTimer(BlockTimer, [AICharacter]()
        {
            if (AICharacter && AICharacter->AbilitySystemComponent)
            {
                // Cancel block ability
                FGameplayTagContainer CancelTags;
                CancelTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Blocking"));
                AICharacter->AbilitySystemComponent->CancelAbilities(&CancelTags);
            }
        }, BlockDuration, false);

        return EBTNodeResult::Succeeded;
    }

    UE_LOG(LogTemp, Warning, TEXT("BTTask_ActivateBlock: Failed to activate block"));
    return EBTNodeResult::Failed;
}