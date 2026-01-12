#include "BTService_UpdateCombatState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/AI/AICharacter.h"
#include "AbilitySystemComponent.h"
#include "MYY/AbilitySystem/AttributeSet/AttributeSetBase.h"

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
    NodeName = "Update Combat State";
    Interval = 0.5f;
    RandomDeviation = 0.1f;

    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, TargetActorKey), AActor::StaticClass());
    CanAttackKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, CanAttackKey));
    ShouldBlockKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, ShouldBlockKey));
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }

    AMYYCharacterBase* AICharacter = Cast<AMYYCharacterBase>(AIController->GetPawn());
    AAICharacter* AIChar = Cast<AAICharacter>(AIController->GetPawn());
    
    if (!AICharacter || !AIChar || !AICharacter->AbilitySystemComponent)
    {
        return;
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    // Check if we have a target
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    if (!TargetActor)
    {
        BlackboardComp->SetValueAsBool(CanAttackKey.SelectedKeyName, false);
        BlackboardComp->SetValueAsBool(ShouldBlockKey.SelectedKeyName, false);
        return;
    }
    
    UAttributeSetBase* AttributeSet = const_cast<UAttributeSetBase*>( Cast<UAttributeSetBase>( AICharacter->AbilitySystemComponent->GetAttributeSet( UAttributeSetBase::StaticClass() ) ) );
    float CurrentStamina = AttributeSet ? AttributeSet->GetStamina() : 0.f;

    float CurrentTime = AIController->GetWorld()->GetTimeSeconds();
    bool bCanAttack = (CurrentTime - LastAttackTime >= AttackCooldown) && (CurrentStamina >= MinStaminaForAttack);

    BlackboardComp->SetValueAsBool(CanAttackKey.SelectedKeyName, bCanAttack);

    if (bCanAttack)
    {
        LastAttackTime = CurrentTime;
    }

    // Determine if should block based on:
    // 1. Enemy is attacking (has State.Combat.Attacking tag)
    // 2. Random chance based on BlockChance
    // 3. Has enough stamina
    bool bShouldBlock = false;

    AMYYCharacterBase* TargetChar = Cast<AMYYCharacterBase>(TargetActor);
    if (TargetChar && TargetChar->AbilitySystemComponent)
    {
        bool bTargetAttacking = TargetChar->AbilitySystemComponent->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag("State.Combat.Attacking")
        );

        if (bTargetAttacking && CurrentStamina >= 10.f)
        {
            float RandomValue = FMath::FRand();
            bShouldBlock = (RandomValue <= AIChar->BlockChance);
        }
    }

    BlackboardComp->SetValueAsBool(ShouldBlockKey.SelectedKeyName, bShouldBlock);
}