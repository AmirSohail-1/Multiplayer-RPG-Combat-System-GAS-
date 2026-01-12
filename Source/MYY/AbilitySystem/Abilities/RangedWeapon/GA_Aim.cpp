// GA_Aim.cpp
#include "GA_Aim.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"

#include "Blueprint/UserWidget.h"

UGA_Aim::UGA_Aim()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;

    // Ability tags
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Ranged.Aim"));
    
    // While aiming, we're in "aiming state"
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Aiming"));
    
    
    // Block other combat abilities while aiming (except Fire)
    BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Ability.Attack.Melee"));
    
    bCheckStaminaBeforeActivate = false;
}

bool UGA_Aim::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] Super::CanActivateAbility failed"));
        return false;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character || !Character->EquipmentComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] No character or EquipmentComponent"));
        return false;
    }

    ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
    if (!Weapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] No weapon equipped"));
        return false;
    }

    URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
    if (!RangedData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Aim] Weapon '%s' is not ranged!"), *Weapon->GetName());
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[GA_Aim] ✅ CanActivateAbility = TRUE"));
    return true;

    
}

void UGA_Aim::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ✅ Ability Activated - Entering Aim Mode"));

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Aim] ❌ Failed to commit ability"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character || !Character->EquipmentComponent)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
    if (!Weapon)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Aim] ❌ No weapon"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    RangedWeaponData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
    if (!RangedWeaponData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Aim] ❌ Not ranged"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // ✅ Enable aiming state on CHARACTER
    Character->SetAiming(true);
    
    UE_LOG(LogTemp, Log, TEXT("[GA_Aim] Character now aiming"));

    // ======================================================
    // 📌 Create Crosshair Widget (Local Player Only)
    // ======================================================
    if (Character->IsLocallyControlled() && RangedWeaponData->CrosshairWidgetClass)
    {
        if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
        {
            ActiveCrosshairWidget = CreateWidget<UUserWidget>(PC, RangedWeaponData->CrosshairWidgetClass);
            if (ActiveCrosshairWidget)
            {
                ActiveCrosshairWidget->AddToViewport();
                UE_LOG(LogTemp, Log, TEXT("[GA_Aim] 🎯 Crosshair Added To Viewport"));
            }
        }
    }
    
    // ✅ Play aim montage (looping)
    if (RangedWeaponData->AimMontage)
    {
        AimMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, 
            NAME_None, 
            RangedWeaponData->AimMontage, 
            1.0f,           // PlayRate
            NAME_None,      // StartSection
            true            // bStopWhenAbilityEnds
        );

        AimMontageTask->OnInterrupted.AddDynamic(this, &UGA_Aim::OnAimMontageInterrupted);
        AimMontageTask->OnCancelled.AddDynamic(this, &UGA_Aim::OnAimMontageCancelled);

        AimMontageTask->OnCompleted.AddDynamic(this, &UGA_Aim::OnAimMontageCompleted);
        
        AimMontageTask->ReadyForActivation();
        
        UE_LOG(LogTemp, Log, TEXT("[GA_Aim] Playing aim montage (looping)"));
    }
}

void UGA_Aim::InputReleased(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] 🎯 Aim Released - Exiting Aim Mode"));

    // Stop Montage immediately
    if (AimMontageTask)
    {
        if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
        {
            if (RangedWeaponData && RangedWeaponData->AimMontage)
            {
                UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ⏹️ Force stopping montage with 0.0f blend"));
                AnimInstance->Montage_Stop(0.0f, RangedWeaponData->AimMontage); // ✅ 0.0f = instant stop
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ⏹️ Stopping aim montage IMMEDIATELY"));
        
        if (AimMontageTask && IsValid(AimMontageTask))
        {
            AimMontageTask->EndTask();
            AimMontageTask = nullptr;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ✅ Aim montage STOPPED"));
    }
    
    CleanupAimState();
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Aim::CancelAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateCancelAbility)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] Ability Cancelled"));

  
    // Stop Montage immediately
    if (AimMontageTask)
    {
        if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
        {
            if (RangedWeaponData && RangedWeaponData->AimMontage)
            {
                UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ⏹️ Force stopping montage with 0.0f blend"));
                AnimInstance->Montage_Stop(0.0f, RangedWeaponData->AimMontage);         // ✅ 0.0f = instant stop
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ⏹️ Stopping aim montage due to cancel"));
        
        if (AimMontageTask && IsValid(AimMontageTask))
        {
            AimMontageTask->EndTask();
            AimMontageTask = nullptr;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ✅ Aim montage STOPPED"));
    }
    
    CleanupAimState();
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_Aim::OnAimMontageInterrupted()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ⚠️ Aim montage INTERRUPTED"));
    AimMontageTask = nullptr;
}

void UGA_Aim::OnAimMontageCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ⚠️ Aim montage CANCELLED"));
    AimMontageTask = nullptr;
}


void UGA_Aim::OnAimMontageCompleted()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Aim] ✅ Aim montage COMPLETED"));

    CleanupAimState();

    // 🔥 THIS IS THE KEY LINE YOU WERE ASKING ABOUT
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}


void UGA_Aim::CleanupAimState()
{
    UE_LOG(LogTemp, Log, TEXT("[GA_Aim] 🧹 Cleaning up aim state"));
    
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->SetAiming(false);
        // Character->DisableMovementDuringAbility(false);
        UE_LOG(LogTemp, Log, TEXT("[GA_Aim] ✅ Cleanup complete"));
    }

    // ======================================================
    // REMOVE CROSSHAIR UI
    // ======================================================
    if (ActiveCrosshairWidget)
    {
        ActiveCrosshairWidget->RemoveFromParent();
        ActiveCrosshairWidget = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[GA_Aim] ❌ Crosshair Removed"));
    }
    
}

ABaseWeapon* UGA_Aim::GetCurrentWeapon() const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->EquipmentComponent)
    {
        return nullptr;
    }
    return Character->EquipmentComponent->GetCurrentWeapon();
}