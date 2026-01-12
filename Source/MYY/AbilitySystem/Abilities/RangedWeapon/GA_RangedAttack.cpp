// GA_RangedAttack.cpp
#include "GA_RangedAttack.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

UGA_RangedAttack::UGA_RangedAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
    

    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Attack.Ranged"));
    SetAssetTags(AssetTags);

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Attacking"));
    BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat"));

    // CurrentState = ERangedAttackState::Idle;
    bCheckStaminaBeforeActivate = false;
    RequiredStaminaCost = 15.f;
 
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.Fire");
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    bIsAimingActive = false;
}

bool UGA_RangedAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] Super::CanActivateAbility failed"));
        return false;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character || !Character->EquipmentComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] No character or EquipmentComponent"));
        return false;
    }
    
    ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
    if (!Weapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] No weapon equipped"));
        return false;
    }

    URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
    if (!RangedData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] Weapon '%s' is not ranged! WeaponData class: %s"),
            *Weapon->GetName(),
            *GetNameSafe(Weapon->WeaponData ? Weapon->WeaponData->GetClass() : nullptr));
        return false;
    }

    if (!RangedData->AmmoConfig.bInfiniteAmmo && !Weapon->HasAmmo())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] Out of ammo!"));
        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag("Ability.Failed.NoAmmo"));
        }
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] ✅ CanActivateAbility = TRUE (Weapon: %s, Ammo: %d)"),
        *RangedData->WeaponName.ToString(), Weapon->CurrentAmmo);

    return true;
}

void UGA_RangedAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // ✅ FIX: Check if this is a FIRE event
    if (TriggerEventData && TriggerEventData->EventTag.MatchesTag(
        FGameplayTag::RequestGameplayTag("GameplayEvent.Fire")))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] 🔥 FIRE EVENT"));
        
        if (bIsAimingActive && CurrentState == ERangedAttackState::Aiming)
        {
            FireArrow();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ⚠️ Not aiming (State: %d), cannot fire"), 
                (int32)CurrentState);
        }
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ✅ Ability Activated - Entering Aim Mode"));
    
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ Failed to commit ability"));
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
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ No weapon"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    RangedWeaponData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
    if (!RangedWeaponData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ Not ranged"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // ✅ Enable aiming state
    Character->SetAiming(true);
    bIsAimingActive = true;
    CurrentState = ERangedAttackState::Aiming;
    
    UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Character now aiming"));

    // ✅ FIX: Play aim montage and store task reference
    if (RangedWeaponData->AimMontage)
    {
        AimMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, 
            NAME_None, 
            RangedWeaponData->AimMontage, 
            1.0f,     // PlayRate
            NAME_None, // StartSection
            true,     // bStopWhenAbilityEnds
            1.0f,     // AnimRootMotionTranslationScale
            0.0f      // StartTimeSeconds
        );

        // ✅ CRITICAL: Bind to interrupted/cancelled to handle cleanup
        AimMontageTask->OnInterrupted.AddDynamic(this, &UGA_RangedAttack::OnAimMontageInterrupted);
        AimMontageTask->OnCancelled.AddDynamic(this, &UGA_RangedAttack::OnAimMontageCancelled);
        AimMontageTask->OnBlendOut.AddDynamic(this, &UGA_RangedAttack::OnAimMontageBlendOut);
        
        AimMontageTask->ReadyForActivation();
        
        UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Playing aim montage (looping)"));
    }
}


void UGA_RangedAttack::StartDrawing()
{
    CurrentState = ERangedAttackState::Drawing;
    DrawStartTime = GetWorld()->GetTimeSeconds();
    CurrentDrawPercent = 0.f;

    UE_LOG(LogTemp, Log, TEXT("Started drawing bow"));

    // Play draw montage
    if (RangedWeaponData->DrawMontage)
    {
        UAbilityTask_PlayMontageAndWait* DrawTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, RangedWeaponData->DrawMontage, 1.0f);

        DrawTask->OnCompleted.AddDynamic(this, &UGA_RangedAttack::OnDrawMontageCompleted);
        DrawTask->OnCancelled.AddDynamic(this, &UGA_RangedAttack::OnDrawMontageCompleted);
        DrawTask->OnInterrupted.AddDynamic(this, &UGA_RangedAttack::OnDrawMontageCompleted);
        DrawTask->OnBlendOut.AddDynamic(this, &UGA_RangedAttack::OnDrawMontageBlendOut);
        DrawTask->ReadyForActivation();
    }

    // Start tracking draw progress
    GetWorld()->GetTimerManager().SetTimer(
        DrawProgressTimer,
        this,
        &UGA_RangedAttack::UpdateDrawProgress,
        0.05f,
        true
    );
}

void UGA_RangedAttack::UpdateDrawProgress()
{
    if (!RangedWeaponData || CurrentState != ERangedAttackState::Drawing)
    {
        GetWorld()->GetTimerManager().ClearTimer(DrawProgressTimer);
        return;
    }

    float ElapsedTime = GetWorld()->GetTimeSeconds() - DrawStartTime;
    CurrentDrawPercent = FMath::Clamp(ElapsedTime / RangedWeaponData->DrawTime, 0.f, 1.f);

    if (CurrentDrawPercent >= 1.f)
    {
        CurrentState = ERangedAttackState::Aiming;
        GetWorld()->GetTimerManager().ClearTimer(DrawProgressTimer);
        UE_LOG(LogTemp, Log, TEXT("Bow fully drawn - now aiming"));
    }
}

// ✅ MODIFY: InputReleased - Now exits aim mode OR fires if PrimaryAttack
void UGA_RangedAttack::InputReleased(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] 🎯 Aim Released - Exiting Aim Mode"));
    
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (Character)
    {
        Character->SetAiming(false);
        // Critical
        Character->DisableMovementDuringAbility(false);
    }
    
    bIsAimingActive = false;
    // CurrentState = ERangedAttackState::Idle;
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_RangedAttack::InputPressed(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] 🎯 Aim Input Pressed"));
    
    // InputPressed is called AFTER ActivateAbility
    // So the ability is already active at this point
    // You can use this to track that aim button is held
}


void UGA_RangedAttack::FireArrow()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] 🏹 FireArrow called"));
    
    if (CurrentState == ERangedAttackState::Firing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ⚠️ Already firing - ignoring"));
        return;
    }

    if (CurrentState != ERangedAttackState::Aiming)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ Not aiming (State: %d)"), 
            (int32)CurrentState);
        return;
    }

    CurrentState = ERangedAttackState::Firing;

    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] Firing with draw: %.2f"), CurrentDrawPercent);

    // ✅ Play fire montage
    if (RangedWeaponData && RangedWeaponData->FireMontage)
    {
        UAbilityTask_PlayMontageAndWait* FireTask = 
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                this, NAME_None, RangedWeaponData->FireMontage, 1.0f);

        FireTask->OnCompleted.AddDynamic(this, &UGA_RangedAttack::OnFireMontageCompleted);
        FireTask->OnCancelled.AddDynamic(this, &UGA_RangedAttack::OnFireMontageCompleted);
        FireTask->OnInterrupted.AddDynamic(this, &UGA_RangedAttack::OnFireMontageCompleted);
        FireTask->ReadyForActivation();

        UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Playing fire montage"));
    }

    // ✅ SPAWN PROJECTILE IMMEDIATELY
    SpawnProjectile();

    // ✅ Consume ammo
    ABaseWeapon* Weapon = GetCurrentWeapon();
    if (Weapon && RangedWeaponData && !RangedWeaponData->AmmoConfig.bInfiniteAmmo)
    {
        Weapon->ConsumeAmmo(1);
        UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Ammo consumed: %d remaining"), 
            Weapon->CurrentAmmo);
    }

    // ✅ Return to aiming state
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        if (bIsAimingActive)
        {
            CurrentState = ERangedAttackState::Aiming;
            UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Returned to aiming state"));
        }
    });
}


void UGA_RangedAttack::SpawnProjectile()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] 🏹 SpawnProjectile called"));
    
    if (!RangedWeaponData || !RangedWeaponData->ProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ No RangedWeaponData or ProjectileClass!"));
        return;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ No character"));
        return;
    }

    // ✅ CRITICAL: Only server spawns projectiles
    if (!Character->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ⚠️ CLIENT - skipping spawn"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ✅ SERVER - Spawning projectile"));

    ABaseWeapon* Weapon = GetCurrentWeapon();
    if (!Weapon || !Weapon->WeaponMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ No weapon or mesh"));
        return;
    }

    // ✅ Get spawn location
    FVector SpawnLocation;
    FName SpawnSocket = RangedWeaponData->ProjectileSpawnSocket;
    
    if (Weapon->WeaponMesh->DoesSocketExist(SpawnSocket))
    {
        SpawnLocation = Weapon->WeaponMesh->GetSocketLocation(SpawnSocket);
        UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Using socket '%s': %s"), 
            *SpawnSocket.ToString(), *SpawnLocation.ToString());
    }
    else
    {
        SpawnLocation = Character->GetActorLocation() + 
                       (Character->GetActorForwardVector() * 100.f) + 
                       (Character->GetActorUpVector() * 50.f);
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ⚠️ Socket '%s' not found, using fallback: %s"),
            *SpawnSocket.ToString(), *SpawnLocation.ToString());
    }

    // ✅ Calculate aim direction
    FVector AimTarget = GetCenterScreenAimLocation();
    FVector Direction = (AimTarget - SpawnLocation).GetSafeNormal();
    FRotator SpawnRotation = Direction.Rotation();

    UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] 🎯 Spawn: %s → Target: %s → Dir: %s"),
        *SpawnLocation.ToCompactString(),
        *AimTarget.ToCompactString(),
        *Direction.ToCompactString());

    // ✅ Spawn projectile
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;
    SpawnParams.Instigator = Character;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Projectile = GetWorld()->SpawnActor<AActor>(
        RangedWeaponData->ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (Projectile)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_RangedAttack] ✅ Spawned projectile: %s"), 
            *Projectile->GetName());

        // ✅ Set projectile velocity
        float SpeedMultiplier = FMath::Lerp(0.5f, 1.0f, CurrentDrawPercent);
        float FinalSpeed = RangedWeaponData->ProjectileSpeed * SpeedMultiplier;

        if (UProjectileMovementComponent* ProjectileMovement = 
            Projectile->FindComponentByClass<UProjectileMovementComponent>())
        {
            ProjectileMovement->InitialSpeed = FinalSpeed;
            ProjectileMovement->MaxSpeed = FinalSpeed;
            ProjectileMovement->Velocity = Direction * FinalSpeed;
            
            UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Set velocity: %s (Speed: %.0f)"),
                *ProjectileMovement->Velocity.ToCompactString(), FinalSpeed);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ Projectile has no ProjectileMovementComponent!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_RangedAttack] ❌ FAILED TO SPAWN PROJECTILE!"));
        UE_LOG(LogTemp, Error, TEXT("   Class: %s"), *GetNameSafe(RangedWeaponData->ProjectileClass));
        UE_LOG(LogTemp, Error, TEXT("   Location: %s"), *SpawnLocation.ToString());
    }
}

FVector UGA_RangedAttack::GetCenterScreenAimLocation() const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character) return FVector::ZeroVector;

    UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
    if (!Camera) return Character->GetActorForwardVector() * 5000.f;

    FVector CameraLocation = Camera->GetComponentLocation();
    FVector CameraForward = Camera->GetForwardVector();
    
    // Trace from center of screen
    FVector TraceStart = CameraLocation;
    FVector TraceEnd = CameraLocation + (CameraForward * 10000.f);
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Character);
    QueryParams.AddIgnoredActor(GetCurrentWeapon());
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, 
        ECC_Visibility, QueryParams))
    {
        UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] 🎯 Aim trace hit: %s"), 
            *HitResult.GetActor()->GetName());
        return HitResult.Location;
    }
    
    return TraceEnd;
}

// ✅ ADD: Helper to get weapon safely
ABaseWeapon* UGA_RangedAttack::GetCurrentWeapon() const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->EquipmentComponent)
    {
        return nullptr;
    }
    return Character->EquipmentComponent->GetCurrentWeapon();
}

void UGA_RangedAttack::OnDrawMontageCompleted()
{
    // Don't end ability - wait for fire
}

void UGA_RangedAttack::OnDrawMontageBlendOut()
{
    // Don't end ability - wait for fire
}

void UGA_RangedAttack::OnFireMontageCompleted()
{
    UE_LOG(LogTemp, Log, TEXT("[GA_RangedAttack] Fire montage completed - returning to aim"));
    
    // ✅ Don't end ability - return to aiming state
    CurrentState = ERangedAttackState::Aiming;
}


void UGA_RangedAttack::OnFireMontageBlendOut()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    }
}

void UGA_RangedAttack::OnAimMontageInterrupted()
{
    
}

void UGA_RangedAttack::OnAimMontageCancelled()
{
    
}

void UGA_RangedAttack::OnAimMontageBlendOut()
{
    
}

void UGA_RangedAttack::CleanupAimState()
{
    
}

void UGA_RangedAttack::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo,
                                     bool bReplicateCancelAbility)
{
    GetWorld()->GetTimerManager().ClearTimer(DrawProgressTimer);

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (Character)
    {
        Character->DisableMovementDuringAbility(false);
    }

    // CurrentState = ERangedAttackState::Idle;
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}
