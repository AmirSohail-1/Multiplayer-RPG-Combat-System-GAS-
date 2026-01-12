// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Block.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"


UGA_Block::UGA_Block()
{
    // Set stamina cost for melee combo
    bCheckStaminaBeforeActivate = true;
    RequiredStaminaCost = 5.0f;         // Default cost, can be overridden per weapon
    
    // Use SetAssetTags instead of AbilityTags
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat.Block"));
    SetAssetTags(AssetTags);

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Blocking"));
    BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat"));                                                 

    // CRITICAL: Set activation policy for event-triggered ability
    AbilityTriggers.AddDefaulted();
    AbilityTriggers[0].TriggerTag = FGameplayTag::RequestGameplayTag("Ability.Combat.Block");
    AbilityTriggers[0].TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;

    
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

bool UGA_Block::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags
) const
{
    if (!Super::CanActivateAbility(
            Handle,
            ActorInfo,
            SourceTags,
            TargetTags,
            OptionalRelevantTags))
    {
        return false;
    }

    // Get character
    AMYYCharacterBase* Character =
        Cast<AMYYCharacterBase>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    
    if (!Character)
    {
        return false;
    }
    
    // ❌ Cannot block while in air
    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp || MoveComp->IsFalling())
    {
        UE_LOG(LogTemp, Verbose, TEXT("GA_Block: Block denied - character is airborne"));
        return false;
    }
    
    // ❌ Ranged weapons cannot block
    if (CachedWeaponData && CachedWeaponData->WeaponType == EWeaponType::Ranged)
    {
        return false;
    }

    return true;
}



void UGA_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
        // Get Character
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());

    // ❌ Cannot block while in air
    // UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    // if (!MoveComp || MoveComp->IsFalling())
    // {
    //     UE_LOG(LogTemp, Verbose, TEXT("GA_Block: Block denied - character is airborne"));
    //     return ;
    // }

    

    // NEW: Ranged weapons cannot block (no shield/parry)
    if (CachedWeaponData && CachedWeaponData->WeaponType == EWeaponType::Ranged)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_Block: Ranged weapons cannot block"));
        return ;
    }
    else if (CachedWeaponData->WeaponType == EWeaponType::Melee)
    {
        // Optionally: Handle special logic for melee weapon blocking
        UE_LOG(LogTemp, Warning, TEXT("GA_Block: Melee weapons can block"));
    }

    
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UWeaponDataAsset* WeaponData = GetWeaponData();
    if (!WeaponData)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // Choose montage: Parry or Block
    
    UAnimMontage* MontageToPlay = nullptr;

    // Check if player wants to parry (press block at right time vs hold block)
    // For this implementation, we'll use the parry montage if available
    if (WeaponData->Animations.ParryMontage && Character && Character->bIsInBlockWindow)
    {
        MontageToPlay = WeaponData->Animations.ParryMontage;
    }
    else if (WeaponData->Animations.BlockMontage)
    {
        MontageToPlay = WeaponData->Animations.BlockMontage;
    }

    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // ❌ Stop movement while hit react montage plays
    if (Character)
    {
        Character->DisableMovementDuringAbility(true);
    }

    // Play block/parry montage
    BlockMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, MontageToPlay, 1.0f, NAME_None, false);

    if (BlockMontageTask)
    {
        BlockMontageTask->OnCompleted.AddDynamic(this, &UGA_Block::OnBlockMontageCompleted);
        BlockMontageTask->OnCancelled.AddDynamic(this, &UGA_Block::OnBlockMontageCompleted);
        BlockMontageTask->OnInterrupted.AddDynamic(this, &UGA_Block::OnBlockMontageCompleted);
        BlockMontageTask->OnBlendOut.AddDynamic(this, &UGA_Block::OnBlockMontageBlendOut);
        BlockMontageTask->ReadyForActivation();
    }
    else
    {
        if (Character)
        {
            Character->DisableMovementDuringAbility(true);
        }
        
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    
 

    // Apply block damage reduction effect
    if (CachedWeaponData->BlockEffect)
    {
        auto BlockEffect = CachedWeaponData->BlockEffect;
        FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
        EffectContext.AddSourceObject(this);

        FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
            BlockEffect, 1, EffectContext);

        if (SpecHandle.IsValid())
        {
            ActiveBlockEffectHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToSelf(
                *SpecHandle.Data.Get());
        }
    }

    // Play block sound
    if (WeaponData->BlockSFX && Character)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), WeaponData->BlockSFX, 
            Character->GetActorLocation());
    }
}



void UGA_Block::CancelAbility(const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    bool bReplicateCancelAbility)
{
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);

 
    
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    }


    if (ActiveBlockEffectHandle.IsValid())
    {
        GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ActiveBlockEffectHandle);
        ActiveBlockEffectHandle = FActiveGameplayEffectHandle();
    }
    
}

void UGA_Block::InputReleased(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{ 
    CancelAbility(Handle, ActorInfo, ActivationInfo, true);
}


void UGA_Block::OnBlockMontageCompleted()
{ 
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Block::OnBlockMontageBlendOut()
{
     
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    } 
}