// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Stagger.h"
 
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"


UGA_Stagger::UGA_Stagger()
{
    //
    // 1. Asset Tags (REPLACES AbilityTags)   ✔ REQUIRED
    //
    {
        FGameplayTagContainer Tags;
        Tags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat.Stagger"));   // <-- moved here
        SetAssetTags(Tags);
    }

    //
    // 2. Activation Owned Tags (STILL VALID TO MODIFY DIRECTLY)
    //
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Stunned"));

    //
    // 3. Cancel Abilities (STILL VALID)
    //
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Attacking"));

    //
    // 4. Block Abilities (STILL VALID)
    //
    BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat"));
    
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;

    // Trigger on stagger event
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.Stagger");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UGA_Stagger::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->EquipmentComponent)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
    UAnimMontage* StaggerMontage = nullptr;
    
    // Use parry react montage as stagger animation
    if (Weapon && Weapon->WeaponData)
    {
        StaggerMontage = Weapon->WeaponData->Animations.ParryReactMontage;
    }

    if (StaggerMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, StaggerMontage, 1.0f);

        Task->OnCompleted.AddDynamic(this, &UGA_Stagger::OnStaggerMontageCompleted);
        Task->OnCancelled.AddDynamic(this, &UGA_Stagger::OnStaggerMontageCompleted);
        Task->OnInterrupted.AddDynamic(this, &UGA_Stagger::OnStaggerMontageCompleted);
        Task->ReadyForActivation();
    }
    else
    {
        // No montage, just wait for duration
        FTimerHandle StaggerTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(StaggerTimerHandle, [this]()
        {
            OnStaggerMontageCompleted();
        }, StaggerDuration, false);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s was staggered by parry!"), *Character->GetName());
}

void UGA_Stagger::OnStaggerMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}