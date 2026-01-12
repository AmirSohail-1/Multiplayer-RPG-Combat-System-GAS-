// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_HitReact.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_HitReact::UGA_HitReact()
{
    // Use SetAssetTags instead of AbilityTags
    // FGameplayTagContainer AssetTags;
    // AssetTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat.HitReact"));
    // SetAssetTags(AssetTags);
    
    // ---- Event that activates this ability ----
    // // This tells GAS: “When GameplayEvent.HitReact happens, this ability should activate.”
    // AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact"));
    //
    // // Extra protection: ensure the ability can also activate ONLY when this tag event happens
    // ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact"));

    // ✅ CORRECT: Trigger setup (BUT it's fighting with above mistakes)
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
    
    // ✅ CORRECT: Use SetAssetTags for ability identification
    {
        const FGameplayTag HitReactTag = 
            FGameplayTag::RequestGameplayTag(TEXT("Ability.Combat.HitReact"), /*ErrorIfNotFound*/ false);
        if (HitReactTag.IsValid())
        {
            FGameplayTagContainer AssetTags;
            AssetTags.AddTag(HitReactTag);
            SetAssetTags(AssetTags);
        }
    }
    
    // ---- Stamina / Costs ----
    bCheckStaminaBeforeActivate = false;

    // ---- Tags granted while running ----
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.HitStunned"));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.HitReact"));

    // ---- Tags that cancel this ability ----
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Attacking"));

    // ✅ CRITICAL FIX: Use ServerInitiated instead of ServerOnly
    // This allows:
    // - Server activates the ability
    // - Client-owned characters execute locally after server confirms
    // - Server-owned characters execute on server and replicate
    
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;  // ← CHANGED    
    // Policy	        Server hits Client	    Client hits Server	        Client sees own reaction
    // ServerOnly	    ✅ Works	            ✅ Works	                ❌ Delayed/Missing
    // LocalPredicted	✅ Works	            ❌ Fails	                ✅ Instant
    // ServerInitiated	✅ Works	            ✅ Works	                ✅ Works
    UE_LOG(LogTemp, Warning, TEXT("✅ GA_HitReact constructor: ServerInitiated execution policy"))
 
    
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.HitReact"));
    
    UE_LOG(LogTemp, Warning, TEXT("GA_HitReact registered for tag: GameplayEvent.HitReact"))
    
}

bool UGA_HitReact::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayTagContainer* SourceTags, 
    const FGameplayTagContainer* TargetTags, 
    FGameplayTagContainer* OptionalRelevantTags) const
{
    // if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    // {
    //     return false;
    // }
    //
    // // Don't interrupt blocking or parrying
    // const AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    // if (Character && Character->AbilitySystemComponent)
    // {
    //     if (Character->AbilitySystemComponent->HasMatchingGameplayTag(
    //         FGameplayTag::RequestGameplayTag("State.Combat.Blocking")) ||
    //         Character->bIsInBlockWindow)
    //     {
    //         return false;
    //     }
    // }

    // ✅ Don't call Super - it might check stamina/weapon requirements we don't need
    // Just check blocking state
    const AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (Character && Character->AbilitySystemComponent)
    {
        if (Character->AbilitySystemComponent->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag("State.Combat.Blocking")) ||
            Character->bIsInBlockWindow)
        {
            UE_LOG(LogTemp, Warning, TEXT("GA_HitReact: Blocked - character is blocking/parrying"));
            return false;
        }

        // ✅ Also check if already in hit react (prevent spam)
        if (Character->AbilitySystemComponent->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag("State.Combat.HitReact")))
        {
            UE_LOG(LogTemp, Warning, TEXT("GA_HitReact: Already in hit react animation"));
            return false;
        }
    }
    
    return true;
}

void UGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    // ✅ Log which machine is executing
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    FString NetRole = Character ? 
        (Character->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT")) : TEXT("UNKNOWN");
    
    UE_LOG(LogTemp, Warning, TEXT("✅ [%s] GA_HitReact ACTIVATED on %s"), 
        *NetRole, *GetNameSafe(ActorInfo->AvatarActor.Get()));

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("GA_HitReact: CommitAbility failed"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    PlayRandomHitReactMontage();
}

void UGA_HitReact::PlayRandomHitReactMontage()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->EquipmentComponent) 
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // ✅ TRY TO GET MONTAGES FROM EQUIPPED WEAPON
    TArray<UAnimMontage*> HitReactMontages;
    ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
    
    if (Weapon && Weapon->WeaponData && Weapon->WeaponData->Animations.HitReactMontages.Num() > 0)
    {
        HitReactMontages = Weapon->WeaponData->Animations.HitReactMontages;
        UE_LOG(LogTemp, Log, TEXT("GA_HitReact: Using weapon montages (%d found)"), HitReactMontages.Num());
    }
    else
    {
        // ✅ FALLBACK TO UNARMED DATA IF NO WEAPON OR NO WEAPON MONTAGES
        if (Character->EquipmentComponent->DefaultUnarmedData)
        {
            HitReactMontages = Character->EquipmentComponent->DefaultUnarmedData->Animations.HitReactMontages;
            UE_LOG(LogTemp, Warning, TEXT("GA_HitReact: Using unarmed fallback montages (%d found)"), HitReactMontages.Num());
        }
    }

    // ✅ FAIL IF NO MONTAGES FOUND
    if (HitReactMontages.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_HitReact: No hit react montages available!"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // Select random hit react montage
    int32 RandomIndex = FMath::RandRange(0, HitReactMontages.Num() - 1);
    UAnimMontage* HitReactMontage = HitReactMontages[RandomIndex];

    if (!HitReactMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_HitReact: ❌ No HitReact montage found!"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    if (Character)
    {
        Character->DisableMovementDuringAbility(true);
    }

    // Play montage
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, HitReactMontage, 1.0f);

    if (!Task)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_HitReact: Failed to create montage task!"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }
    
    Task->OnCompleted.AddDynamic(this, &UGA_HitReact::OnHitReactMontageCompleted);
    Task->OnCancelled.AddDynamic(this, &UGA_HitReact::OnHitReactMontageCompleted);
    Task->OnInterrupted.AddDynamic(this, &UGA_HitReact::OnHitReactMontageCompleted);
    Task->OnBlendOut.AddDynamic(this, &UGA_HitReact::OnBlockMontageBlendOut);
    Task->ReadyForActivation();
    
    UE_LOG(LogTemp, Warning, TEXT("✅ GA_HitReact: Playing montage %s"), *HitReactMontage->GetName());
}

void UGA_HitReact::OnHitReactMontageCompleted()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    }
    
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_HitReact::OnBlockMontageBlendOut()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    } 
}
