#include "GA_Fire.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


/**
     *               -------Doesn't Recquired Aim ability to fire Arrows
    // ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("State.Aiming"));

 **/


UGA_Fire::UGA_Fire()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
    
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Ranged.Fire"));

    

    // ✅ ADD THIS: Block activation while montage is playing
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Firing"));
    
    bCheckStaminaBeforeActivate = false;
}

bool UGA_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] Super::CanActivateAbility failed"));
        return false;
    }

    // ✅ ADD THIS: Check if cooldown timer is still active
    if (IsOnCooldown())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ❌ Cannot fire - still on cooldown!"));
        return false;
    }
        
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] No character"));
        return false;
    }

    // ✅ CRITICAL: Check if character is aiming
    if (!Character->bIsAiming)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ❌ Cannot fire - not aiming!"));
        return false;
    }

    if (!Character->EquipmentComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] No EquipmentComponent"));
        return false;
    }

    ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
    if (!Weapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] No weapon equipped"));
        return false;
    }

    URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
    if (!RangedData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] Weapon is not ranged"));
        return false;
    }


    if (!RangedData->AmmoConfig.bInfiniteAmmo && !Weapon->HasAmmo())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] Out of ammo!"));
 
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ CanActivateAbility = TRUE (Ammo: %d)"), Weapon->CurrentAmmo);
 
    
    return true;
}


void UGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                               const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo,
                               const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🔥 FIRING ARROW"));

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ Failed to commit ability"));
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
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ No weapon"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    RangedWeaponData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
    if (!RangedWeaponData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ Not ranged"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    
    // ✅ Consume ammo
    // if (!RangedWeaponData->AmmoConfig.bInfiniteAmmo)
    // {
    //     Weapon->ConsumeAmmo(1);
    //     UE_LOG(LogTemp, Log, TEXT("[GA_Fire] Ammo consumed: %d remaining"), Weapon->CurrentAmmo);
    // }

    // ✅ Store data for InputReleased() but DON'T consume ammo yet
    // ✅ DON'T end ability here - wait for InputReleased()
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ Ready to fire (Ammo: %d)"), Weapon->CurrentAmmo);
 
}

// void UGA_Fire::InputReleased(const FGameplayAbilitySpecHandle Handle, 
//                              const FGameplayAbilityActorInfo* ActorInfo,
//                              const FGameplayAbilityActivationInfo ActivationInfo)
// {
//     UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🏹 INPUT RELEASED - FIRING NOW!"));
//
//     if (!RangedWeaponData)
//     {
//         UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ RangedWeaponData is NULL!"));
//         EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
//         return;
//     }
//
//     // // ✅ ADD THIS: Store handles for later use in callback
//     // CachedSpecHandle = Handle;
//     // CachedActorInfo = ActorInfo;
//     // CachedActivationInfo = ActivationInfo;
//
//     // ✅ Apply State.Firing tag to block rapid fire
//     FGameplayTagContainer FireTags;
//     FireTags.AddTag(FGameplayTag::RequestGameplayTag("State.Firing"));
//     UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
//     ASC->AddLooseGameplayTags(FireTags);
//     UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ State.Firing tag ADDED"));
//
//     // ✅ Consume ammo
//     if (!RangedWeaponData->AmmoConfig.bInfiniteAmmo)
//     {
//         ABaseWeapon* Weapon = GetCurrentWeapon();
//         if (Weapon)
//         {
//             Weapon->ConsumeAmmo(1);
//             UE_LOG(LogTemp, Log, TEXT("[GA_Fire] Ammo consumed: %d remaining"), Weapon->CurrentAmmo);
//         }
//     }
//
//     SpawnProjectile();
//
//     if (RangedWeaponData->FireMontage)
//     {
//         UAbilityTask_PlayMontageAndWait* FireTask = 
//             UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
//                 this, NAME_None, RangedWeaponData->FireMontage, 1.0f);
//
//         FireTask->OnCompleted.AddDynamic(this, &UGA_Fire::OnFireMontageCompleted);
//         FireTask->OnCancelled.AddDynamic(this, &UGA_Fire::OnFireMontageCompleted);
//         FireTask->OnInterrupted.AddDynamic(this, &UGA_Fire::OnFireMontageCompleted);
//         FireTask->ReadyForActivation();
//
//         UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ Playing fire montage"));
//     }
//     else
//     {
//         UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ⚠️ No fire montage - ending immediately"));
//         GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTags(FireTags);
//         EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
//     }
// }

void UGA_Fire::InputReleased(const FGameplayAbilitySpecHandle Handle, 
                             const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🏹 INPUT RELEASED - FIRING NOW!"));

    if (!RangedWeaponData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ RangedWeaponData is NULL!"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // ✅ Apply State.Firing tag to block rapid fire
    FGameplayTagContainer FireTags;
    FireTags.AddTag(FGameplayTag::RequestGameplayTag("State.Firing"));
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    ASC->AddLooseGameplayTags(FireTags);
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ State.Firing tag ADDED"));

    // ✅ Consume ammo
    if (!RangedWeaponData->AmmoConfig.bInfiniteAmmo)
    {
        ABaseWeapon* Weapon = GetCurrentWeapon();
        if (Weapon)
        {
            Weapon->ConsumeAmmo(1);
            UE_LOG(LogTemp, Log, TEXT("[GA_Fire] Ammo consumed: %d remaining"), Weapon->CurrentAmmo);
        }
    }

    // ✅ SPAWN PROJECTILE
    SpawnProjectile();

    // ✅ Get character and play montage directly
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    float MontageDuration = 1.0f; // Default fallback
    
    if (RangedWeaponData->FireMontage && Character)
    {
        // ✅ Get actual montage duration
        MontageDuration = RangedWeaponData->FireMontage->GetPlayLength();
        
        // ✅ Play montage on character (MONTAGE PLAYS HERE)
        Character->PlayAnimMontage(RangedWeaponData->FireMontage, 1.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ Playing fire montage (Duration: %.2f seconds)"), MontageDuration);
        
        // ✅ Setup timer to remove blocking tag after montage completes
        GetWorld()->GetTimerManager().SetTimer(MontageTimerHandle, [ASC, FireTags]()
        {
            if (ASC)
            {
                ASC->RemoveLooseGameplayTags(FireTags);
                UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ⏱️ Timer REMOVED State.Firing tag"));
            }
        }, MontageDuration, false);
    }
    else
    {
        // No montage, remove tag immediately
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ⚠️ No fire montage - ending immediately"));
        ASC->RemoveLooseGameplayTags(FireTags);
    }

    // ✅ END ABILITY IMMEDIATELY (ability ends but montage keeps playing)
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ Ability ending immediately"));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Fire::SpawnProjectile()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🏹 SpawnProjectile called"));

    if (!RangedWeaponData || !RangedWeaponData->ProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ No ProjectileClass!"));
        return;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ No character"));
        return;
    }

    // ✅ Only server spawns projectiles
    if (!Character->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ⚠️ CLIENT - skipping spawn"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ SERVER - Spawning projectile"));

    ABaseWeapon* Weapon = GetCurrentWeapon();
    if (!Weapon || !Weapon->WeaponMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ No weapon or mesh"));
        return;
    }

    // Get spawn location
    FVector SpawnLocation;
    FName SpawnSocket = RangedWeaponData->ProjectileSpawnSocket;
    
    if (Weapon->WeaponMesh->DoesSocketExist(SpawnSocket))
    {
        SpawnLocation = Weapon->WeaponMesh->GetSocketLocation(SpawnSocket);
    }
    else
    {
        SpawnLocation = Character->GetActorLocation() + 
                       (Character->GetActorForwardVector() * 100.f) + 
                       (Character->GetActorUpVector() * 50.f);
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ⚠️ Socket not found, using fallback"));
    }

    // Calculate aim direction
    FVector AimTarget = GetCenterScreenAimLocation();
    FVector Direction = (AimTarget - SpawnLocation).GetSafeNormal();
    FRotator SpawnRotation = Direction.Rotation();

    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🎯 Direction: %s"), *Direction.ToCompactString());

    // Spawn projectile
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
        float FinalSpeed = RangedWeaponData->ProjectileSpeed * CurrentDrawPercent;

        if (UProjectileMovementComponent* ProjectileMovement = 
            Projectile->FindComponentByClass<UProjectileMovementComponent>())
        {
            ProjectileMovement->InitialSpeed = FinalSpeed;
            ProjectileMovement->MaxSpeed = FinalSpeed;
            ProjectileMovement->Velocity = Direction * FinalSpeed;
            
            UE_LOG(LogTemp, Log, TEXT("[GA_Fire] Velocity: %s (Speed: %.0f)"),
                *ProjectileMovement->Velocity.ToCompactString(), FinalSpeed);
        }

        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ✅ Spawned: %s"), *Projectile->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Fire] ❌ FAILED TO SPAWN PROJECTILE!"));
    }
}

FVector UGA_Fire::GetCenterScreenAimLocation() const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character) return FVector::ZeroVector;

    UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
    if (!Camera) return Character->GetActorForwardVector() * 5000.f;

    FVector CameraLoc = Camera->GetComponentLocation();
    FVector CameraFwd = Camera->GetForwardVector();

    FVector TraceStart = CameraLoc;
    FVector TraceEnd   = CameraLoc + (CameraFwd * 90000.f);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Character);
    Params.AddIgnoredActor(GetCurrentWeapon());

    // 🔵 DEBUG: Camera Trace
    if (RangedWeaponData && RangedWeaponData->bDebugProjectile)
    {
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Blue, false, 2.f, 0, 1.f);
    }

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        FVector FinalHit = Hit.Location + (CameraFwd * 5.f);

        //  DEBUG: Hit location sphere
        if (RangedWeaponData->bDebugProjectile)
        {
            DrawDebugSphere(GetWorld(), FinalHit, 10.f, 12, FColor::Red, false, 2.f);
        }

        return FinalHit;
    }

    return TraceEnd;
}

ABaseWeapon* UGA_Fire::GetCurrentWeapon() const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->EquipmentComponent)
    {
        return nullptr;
    }
    return Character->EquipmentComponent->GetCurrentWeapon();
}

bool UGA_Fire::IsOnCooldown() const
{
    if (GetWorld() && MontageTimerHandle.IsValid())
    {
        return GetWorld()->GetTimerManager().IsTimerActive(MontageTimerHandle);
    }
    return false;
}

void UGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle,
                          const FGameplayAbilityActorInfo* ActorInfo,
                          const FGameplayAbilityActivationInfo ActivationInfo,
                          bool bReplicateEndAbility,
                          bool bWasCancelled)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🛑 EndAbility called (Cancelled: %s)"), 
        bWasCancelled ? TEXT("YES") : TEXT("NO"));
    
    // ✅ ONLY clear timer if ability was CANCELLED (not normal completion)
    if (bWasCancelled && GetWorld() && MontageTimerHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] ⏱️ Clearing timer - ability was cancelled"));
        GetWorld()->GetTimerManager().ClearTimer(MontageTimerHandle);
        
        // Remove tag if cancelled
        FGameplayTagContainer FireTags;
        FireTags.AddTag(FGameplayTag::RequestGameplayTag("State.Firing"));
        if (GetAbilitySystemComponentFromActorInfo())
        {
            GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTags(FireTags);
            UE_LOG(LogTemp, Warning, TEXT("[GA_Fire] 🧹 Cleaned up State.Firing tag"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[GA_Fire] ⏱️ Timer left running - will expire naturally"));
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Fire::OnFireMontageCompleted()
{
    UE_LOG(LogTemp, Log, TEXT("[GA_Fire] 🎬 Fire montage completed - cleaning up"));

    // ✅ Remove blocking tag so we can fire again
    FGameplayTagContainer FireTags;
    FireTags.AddTag(FGameplayTag::RequestGameplayTag("State.Firing"));
    GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTags(FireTags);

    // // ✅ CHANGE THIS: Use cached handles instead of CurrentSpecHandle
    // EndAbility(CachedSpecHandle, CachedActorInfo, CachedActivationInfo, true, false);
}