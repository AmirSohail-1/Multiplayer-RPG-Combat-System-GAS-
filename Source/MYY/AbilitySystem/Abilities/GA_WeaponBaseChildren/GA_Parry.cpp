// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Parry.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Kismet/GameplayStatics.h"

UGA_Parry::UGA_Parry()
{
    
    //
    // 1. Ability Tags (REQUIRED → replace AbilityTags)
    //
    
        FGameplayTagContainer Tags;
        Tags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat.Parry"));
        SetAssetTags(Tags); 
    //
    // 2. Activation Owned Tags (USE MUTABLE ACCESSOR)
    // 
        ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.ParrySuccess"));
     
    
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;

    // Trigger on parry event
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.Parry");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UGA_Parry::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
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
    UAnimMontage* ParryMontage = nullptr;
    
    if (Weapon && Weapon->WeaponData)
    {
        ParryMontage = Weapon->WeaponData->Animations.ParryMontage;
    }

    if (ParryMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, ParryMontage, 1.0f);

        Task->OnCompleted.AddDynamic(this, &UGA_Parry::OnParryMontageCompleted);
        Task->OnCancelled.AddDynamic(this, &UGA_Parry::OnParryMontageCompleted);
        Task->OnInterrupted.AddDynamic(this, &UGA_Parry::OnParryMontageCompleted);
        Task->ReadyForActivation();
    }
    else
    {
        OnParryMontageCompleted();
    }

    // Play parry success sound
    if (Weapon && Weapon->WeaponData && Weapon->WeaponData->ParrySFX)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Weapon->WeaponData->ParrySFX, 
            Character->GetActorLocation());
    }

    // Visual feedback
    UE_LOG(LogTemp, Warning, TEXT("%s successfully parried an attack!"), *Character->GetName());
}

void UGA_Parry::OnParryMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}