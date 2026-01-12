#include "GA_Death.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Death::UGA_Death()
{
    // Triggered by GameplayEvent.Death
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.Death");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    // Death ability tag
    {
        const FGameplayTag DeathTag =
            FGameplayTag::RequestGameplayTag(TEXT("Ability.Combat.Death"), /*ErrorIfNotFound*/ false);
        if (DeathTag.IsValid())
        {
            FGameplayTagContainer AssetTags;
            AssetTags.AddTag(DeathTag);
            SetAssetTags(AssetTags);
        }
    }

    // Mark character as dead
    {
        const FGameplayTag DeadStateTag =
            FGameplayTag::RequestGameplayTag(TEXT("State.Dead"), /*ErrorIfNotFound*/ false);
        if (DeadStateTag.IsValid())
        {
            ActivationOwnedTags.AddTag(DeadStateTag);
        }
    }

    // Cancel other gameplay abilities
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Attacking"));
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Blocking"));
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Action"));

    // Prevent new abilities from activating
    BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Ability"));

    // Death should never check stamina
    bCheckStaminaBeforeActivate = false;

    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // Death doesn’t need client prediction; server-only is clean and avoids duplicate activations    -------------------
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
    
}

// =========================================================
// Death Ability ALWAYS activates (ignores stamina and cost)
// =========================================================

bool UGA_Death::CheckCost(const FGameplayAbilitySpecHandle Handle,
                          const FGameplayAbilityActorInfo* ActorInfo,
                          FGameplayTagContainer* OptionalRelevantTags) const
{
    return true; // Ignore stamina and all other costs completely
}

bool UGA_Death::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                   const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayTagContainer* SourceTags,
                                   const FGameplayTagContainer* TargetTags,
                                   FGameplayTagContainer* OptionalRelevantTags) const
{
    // Death can ALWAYS activate
    return true;
}

// =========================================================
// Activate Ability
// =========================================================

void UGA_Death::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UE_LOG(LogTemp, Error, TEXT("💀 GA_Death activated for %s"), *Character->GetName());

    if (Character->HasAuthority())
    {
        // ========== DROP ALL WEAPONS IMMEDIATELY ==========
        if (Character->EquipmentComponent)
        {
            Character->EquipmentComponent->DropAllWeapons();
            UE_LOG(LogTemp, Warning, TEXT("💀 Dropped all weapons for %s"), *Character->GetName());
        }
        
        Character->GetCharacterMovement()->DisableMovement();
        Character->GetCharacterMovement()->StopMovementImmediately();
        Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UAnimMontage* DeathMontage = nullptr;

    ABaseWeapon* Weapon = Character->EquipmentComponent
        ? Character->EquipmentComponent->GetCurrentWeapon()
        : nullptr;

    if (Weapon && Weapon->WeaponData && Weapon->WeaponData->Animations.DeathMontage)
    {
        DeathMontage = Weapon->WeaponData->Animations.DeathMontage;
        UE_LOG(LogTemp, Log, TEXT("GA_Death: Using WEAPON death montage (%s)"),
            *DeathMontage->GetName());
    }
    else if (Character->EquipmentComponent &&
             Character->EquipmentComponent->DefaultUnarmedData &&
             Character->EquipmentComponent->DefaultUnarmedData->Animations.DeathMontage)
    {
        DeathMontage = Character->EquipmentComponent->DefaultUnarmedData->Animations.DeathMontage;
        UE_LOG(LogTemp, Warning, TEXT("GA_Death: Using UNARMED (EquipmentComponent) death montage (%s)"),
            *DeathMontage->GetName());
    }

    if (!DeathMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_Death: ❌ No death montage found! Ending ability."));

        if (Character->HasAuthority())
        {
            Character->SetActorHiddenInGame(true);
            Character->SetLifeSpan(2.0f);
        }

        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // Server-only ability, but still guard anim playback
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, DeathMontage, 1.0f);

    if (!MontageTask)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_Death: ❌ Failed to create MontageTask, cleaning up."));

        if (Character->HasAuthority())
        {
            Character->SetActorHiddenInGame(true);
            Character->SetLifeSpan(2.0f);
        }

        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Death::OnDeathMontageCompleted);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Death::OnDeathMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Death::OnDeathMontageCompleted);
    MontageTask->ReadyForActivation();
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Playing death montage: %s"), *DeathMontage->GetName());
}


void UGA_Death::OnDeathMontageCompleted()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());

    UE_LOG(LogTemp, Warning, TEXT("💀 Death montage completed for %s"),
        Character ? *Character->GetName() : TEXT("Unknown"));

    if (Character && Character->HasAuthority())
    {
        Character->SetActorEnableCollision(false);
        Character->SetActorHiddenInGame(true);
        Character->SetLifeSpan(2.0f);   // or Destroy() if you really want immediate destroy
        
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

