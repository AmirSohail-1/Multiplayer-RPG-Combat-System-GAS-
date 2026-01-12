// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/EquipmentComponent.h"
#include "DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/AttributeSet/AttributeSetBase.h"
#include "Subsystem/WeaponDataSubsystem.h"

ABaseWeapon::ABaseWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);
    SetReplicatingMovement(true);

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetIsReplicated(true);

    // Interaction sphere for pickup - CRASH FIX: Disable by default
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(WeaponMesh);
    InteractionSphere->SetSphereRadius(150.f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);  
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionSphere->SetGenerateOverlapEvents(false); // FIX: No overlap events by default

   
    // Scene components as trace socket placeholders
    TraceStartSocket = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStartSocket"));
    TraceStartSocket->SetupAttachment(WeaponMesh);

    TraceEndSocket = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEndSocket"));
    TraceEndSocket->SetupAttachment(WeaponMesh);

    // Initialize flags
    bIsTracing = false;

    bCanBePickedUp = true; // Changed from false
    
    // Also enable collision by default
    if (InteractionSphere)
    {
        InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionSphere->SetGenerateOverlapEvents(true);
    }
}

void ABaseWeapon::DebugWeapon()
{
    UE_LOG(LogTemp, Warning, TEXT("=== WEAPON DEBUG ==="));
    UE_LOG(LogTemp, Warning, TEXT("Weapon: %s"), *GetName());
    UE_LOG(LogTemp, Warning, TEXT("Owner: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("None"));
    UE_LOG(LogTemp, Warning, TEXT("bIsTracing: %s"), bIsTracing ? TEXT("True") : TEXT("False"));
    // UE_LOG(LogTemp, Warning, TEXT("bIsCurrentlyTracing: %s"), bIsCurrentlyTracing ? TEXT("True") : TEXT("False"));
    // UE_LOG(LogTemp, Warning, TEXT("bIsProcessingHit: %s"), bIsProcessingHit ? TEXT("True") : TEXT("False"));
    UE_LOG(LogTemp, Warning, TEXT("HitActors Count: %d"), HitActorsThisSwing.Num());
    
    bool bHasTimer = false;
    if (GetWorld())
    {
        bHasTimer = GetWorld()->GetTimerManager().IsTimerActive(TraceTimerHandle);
    }
    UE_LOG(LogTemp, Warning, TEXT("Has Timer: %s"), bHasTimer ? TEXT("True") : TEXT("False"));
    UE_LOG(LogTemp, Warning, TEXT("WeaponData: %s"), WeaponData ? *WeaponData->GetName() : TEXT("None"));
    UE_LOG(LogTemp, Warning, TEXT("==================="));
}

void ABaseWeapon::BeginPlay()
{
    Super::BeginPlay();
    
    // Debug Data on start
    DebugWeapon();

    // ✅ NEW: Initialize ammo for ranged weapons on server
    if (HasAuthority() && WeaponData)
    {
        UE_LOG(LogTemp, Error, TEXT("WeaponData Not Found"));
        if (URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(WeaponData))
        {
            UE_LOG(LogTemp, Error, TEXT("RangeData Not Found"));
            if (CurrentAmmo == 0) // Only initialize if not already set
            {
                CurrentAmmo = RangedData->AmmoConfig.StartingAmmo;
                UE_LOG(LogTemp, Log, TEXT("Initialized ranged weapon with %d ammo"), CurrentAmmo);
            }
        }
    }

    if (WeaponData && WeaponMesh->GetStaticMesh())
    {
        // Attach scene components to weapon mesh sockets if they exist
        if (WeaponMesh->DoesSocketExist(WeaponData->TraceStartSocketName))
        {
            TraceStartSocket->AttachToComponent(WeaponMesh, 
                FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
                WeaponData->TraceStartSocketName);
        }
        
        if (WeaponMesh->DoesSocketExist(WeaponData->TraceEndSocketName))
        {
            TraceEndSocket->AttachToComponent(WeaponMesh, 
                FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
                WeaponData->TraceEndSocketName);
        }
    }

    // Bind overlap events
    if (HasAuthority() && InteractionSphere )
    {
        // CRITICAL FIX: Bind overlap events ALWAYS (not just on authority)
        // Clients need this too for visual feedback
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(
            this, &ABaseWeapon::OnInteractionSphereBeginOverlap);
    
        // Also bind end overlap for better UX
        InteractionSphere->OnComponentEndOverlap.AddDynamic(
            this, &ABaseWeapon::OnInteractionSphereEndOverlap);
    }

    
    if (HasAuthority() && WeaponData)
    {
        WeaponDataID = WeaponData->WeaponID_ReplicateWeapon_DA;   // Send lightweight ID
    }

    
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaseWeapon, bIsTracing);
    DOREPLIFETIME(ABaseWeapon, bCanBePickedUp);
    DOREPLIFETIME(ABaseWeapon, WeaponData);   // PDA is should not replication support
    DOREPLIFETIME(ABaseWeapon, WeaponDataID);    // Replicate the WeaponDataID instead which is in pda
    DOREPLIFETIME(ABaseWeapon, CurrentAmmo);  // ✅ NEW LINE
    
}

void ABaseWeapon::OnRep_WeaponDataID()
{
    if (!GetGameInstance()) return; // Add this check
	
    if (UWeaponDataSubsystem* SDS = GetGameInstance()->GetSubsystem<UWeaponDataSubsystem>())
    {
        WeaponData = SDS->GetByID(WeaponDataID);
        if (!WeaponData)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find weapon data for ID: %d"), WeaponDataID);
        }
    }
} 
void ABaseWeapon::OnRep_CurrentAmmo()
{
    // Broadcast to UI that ammo changed
    UE_LOG(LogTemp, Log, TEXT("Ammo changed: %d"), CurrentAmmo);
    
    // You can add a delegate broadcast here if needed:
    // OnAmmoChanged.Broadcast(CurrentAmmo, GetMaxAmmo());
}

//  Range WEapon Logic start
// 4. NEW: ConsumeAmmo - Reduce ammo count (server only)
void ABaseWeapon::ConsumeAmmo(int32 Amount)
{
    if (!HasAuthority()) return;

    CurrentAmmo = FMath::Max(0, CurrentAmmo - Amount);
    UE_LOG(LogTemp, Log, TEXT("Consumed %d ammo. Remaining: %d"), Amount, CurrentAmmo);
}

// 5. NEW: GetMaxAmmo - Get max ammo from weapon data
int32 ABaseWeapon::GetMaxAmmo() const
{
    if (URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(WeaponData))
    {
        return RangedData->AmmoConfig.MaxAmmo;
    }
    return 0;
}

//  Range WEapon Logic End

void ABaseWeapon::StartTrace()
{
    if (!HasAuthority()) return;
 
    bIsTracing = true;
    ClearHitActors();
    bHasLastPositions = false;
    
    // Perform trace every 0.01 seconds during active swing
    GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, 
        &ABaseWeapon::PerformTrace, 0.01f, true);
}

void ABaseWeapon::EndTrace()
{
    if (!HasAuthority()) return;

    bIsTracing = false;
    bHasLastPositions = false;
    GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
}

void ABaseWeapon::ClearHitActors()
{
    HitActorsThisSwing.Empty();
}

void ABaseWeapon::OnRep_IsTracing()
{
    // Client-side visual feedback can go here
}

float ABaseWeapon::CalculateDamage(bool& bOutIsCritical)
{
    if (!WeaponData)
    {
        bOutIsCritical = false;
        return 0.f;
    }

    float Damage = WeaponData->Stats.BaseDamage;

    // Calculate critical hit
    float RandomValue = FMath::FRand();
    if (RandomValue <= WeaponData->Stats.CriticalHitChance)
    {
        bOutIsCritical = true;
        Damage *= WeaponData->Stats.CriticalHitMultiplier;
    }
    else
    {
        bOutIsCritical = false;
    }

    return Damage;
}

 

void ABaseWeapon::PerformTrace()
{
    if (!bIsTracing || !WeaponData || !HasAuthority()) return;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) 
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Weapon has no owner!"));
        return;
    }

    UAbilitySystemComponent* InstigatorASC = 
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
    
    if (!InstigatorASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ InstigatorASC is null for owner %s. Damage won't be applied, but parry/block may still trigger."), *OwnerActor->GetName());
        // Don't return - continue for parry/block events
    }

    FVector Start = TraceStartSocket->GetComponentLocation();
    FVector End = TraceEndSocket->GetComponentLocation();

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(OwnerActor);
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = false;

    TArray<FHitResult> HitResults;

    
    // Line base Trace.
    // if (bHasLastPositions)
    // {
    //     GetWorld()->SweepMultiByChannel(
    //         HitResults, LastStartPos, Start, FQuat::Identity,
    //         ECC_Pawn, FCollisionShape::MakeSphere(10.f), QueryParams);
    //
    //     GetWorld()->SweepMultiByChannel(
    //         HitResults, LastEndPos, End, FQuat::Identity,
    //         ECC_Pawn, FCollisionShape::MakeSphere(10.f), QueryParams);
    // }
    //
    // GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Pawn, QueryParams);

    // ✅ Get collision shape from weapon data
    float TraceRadius = WeaponData->Stats.TraceRadius;
    float TraceHalfHeight = WeaponData->Stats.TraceHalfHeight;
    bool bUseCapsule = WeaponData->Stats.bUseCapsuleTrace;

    FCollisionShape CollisionShape = bUseCapsule 
        ? FCollisionShape::MakeCapsule(TraceRadius, TraceHalfHeight)
        : FCollisionShape::MakeSphere(TraceRadius);

    if (bHasLastPositions)
    {
        // Sweep from last positions to current positions
        GetWorld()->SweepMultiByChannel( 
            HitResults, LastStartPos, Start, FQuat::Identity,
            ECC_Pawn, CollisionShape, QueryParams);

        GetWorld()->SweepMultiByChannel( 
            HitResults, LastEndPos, End, FQuat::Identity,
            ECC_Pawn, CollisionShape, QueryParams);
    }

    // Current frame trace between start and end
    GetWorld()->SweepMultiByChannel( 
        HitResults, Start, End, FQuat::Identity,
        ECC_Pawn, CollisionShape, QueryParams);

    //-------------------------------------------------------

    LastStartPos = Start;
    LastEndPos = End;
    bHasLastPositions = true;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        
        if (!HitActor || HitActorsThisSwing.Contains(HitActor) || HitActor == OwnerActor) 
            continue;

        UAbilitySystemComponent* TargetASC = 
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

        if (!TargetASC)
        {
            // UE_LOG(LogTemp, Warning, TEXT("Skipping hit actor %s - no ASC found"), *HitActor->GetName());
            continue;
        }

        HitActorsThisSwing.Add(HitActor);

        bool bIsCritical = false;
        float Damage = CalculateDamage(bIsCritical);

        // Check if target is blocking/parrying
        AMYYCharacterBase* TargetCharacter = Cast<AMYYCharacterBase>(HitActor);
        bool bWasParried = false;
        
        if (TargetCharacter && TargetCharacter->bIsInBlockWindow)
        {
            // Perfect parry - no damage, trigger parry event
            bWasParried = true;
            Damage = 0.f;

            FGameplayEventData ParryEventData;
            ParryEventData.Instigator = OwnerActor;
            ParryEventData.Target = HitActor;
            TargetASC->HandleGameplayEvent(
                FGameplayTag::RequestGameplayTag("GameplayEvent.Parry"), &ParryEventData);

            // Stagger attacker
            if (InstigatorASC)
            {
                FGameplayEventData StaggerEventData;
                StaggerEventData.Instigator = HitActor;
                StaggerEventData.Target = OwnerActor;
                InstigatorASC->HandleGameplayEvent(
                    FGameplayTag::RequestGameplayTag("GameplayEvent.Stagger"), &StaggerEventData);
            }
        }
        else if (TargetASC->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag("State.Combat.Blocking")))
        {
            // Regular block - reduced damage
            Damage *= (1.0f - WeaponData->Stats.BlockDamageReduction);
        }

        // ✅ APPLY DAMAGE USING GAMEPLAY EFFECT
        if (!bWasParried && Damage > 0.f)
        {
            // ✅ IMPORTANT: Check if InstigatorASC exists before applying damage
            if (InstigatorASC && WeaponData && WeaponData->DamageEffect)
            {
                // Build effect context
                FGameplayEffectContextHandle EffectContext = InstigatorASC->MakeEffectContext();
                EffectContext.AddSourceObject(this);
                EffectContext.AddHitResult(Hit);

                // ✅ IMPORTANT: Controller fallback logic from old version
                AController* InstigatorController = OwnerActor->GetInstigatorController();
                if (!InstigatorController)
                {
                    if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
                    {
                        InstigatorController = OwnerPawn->GetController();
                    }
                }

                EffectContext.AddInstigator(OwnerActor, InstigatorController);

                // Create spec
                FGameplayEffectSpecHandle SpecHandle =
                    InstigatorASC->MakeOutgoingSpec(WeaponData->DamageEffect, 1.f, EffectContext);

                if (SpecHandle.IsValid())
                {
                    // Damage tag
                    FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag("Data.Damage");
                    SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, Damage);

                    // ✅ Add gameplay tags to the effect spec
                    SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(
                        FGameplayTag::RequestGameplayTag("Ability.Attack.Melee"));
                    
                    // Apply
                    FActiveGameplayEffectHandle GEHandle =
                        InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

                    if (GEHandle.IsValid())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("✅ Applied %.1f damage to %s"),
                            Damage, *HitActor->GetName());
                    }
                    else
                    {
                        // ✅ IMPORTANT: Better error logging from old logic
                        UE_LOG(LogTemp, Error, TEXT("❌ Failed to apply GameplayEffect to %s. Checking effect..."),
                            *HitActor->GetName());
                        
                        UE_LOG(LogTemp, Error, TEXT("   Effect Class: %s"), *GetNameSafe(WeaponData->DamageEffect));
                        UE_LOG(LogTemp, Error, TEXT("   Target ASC: %s"), *TargetASC->GetName());
                        UE_LOG(LogTemp, Error, TEXT("   Instigator ASC: %s"), *InstigatorASC->GetName());
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ Invalid effect spec for damage effect"));
                }
            }
            else if (!InstigatorASC)
            {
                UE_LOG(LogTemp, Error, TEXT("❌ Cannot apply damage: InstigatorASC is null"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ WeaponData or DamageEffect is null!"));
            }
        }

        // Spawn VFX/SFX
        if (bWasParried && WeaponData->ParrySFX)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), WeaponData->ParrySFX, Hit.ImpactPoint);
        }
        else if (WeaponData->HitVFX)
        {
            UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(), WeaponData->HitVFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
        }

        if (WeaponData->HitSFX && !bWasParried)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), WeaponData->HitSFX, Hit.ImpactPoint);
        }
    }

    #if !UE_BUILD_SHIPPING
        if (WeaponData->bDebugWeaponTrace)
        {
            // Draw the trace line between sockets
            DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.1f, 0, 2.f);
        
            // ✅ Visualize collision shape SWEEPING along the trace line
            if (bUseCapsule)
            {
                // Calculate the center point and half-length of the sweep
                FVector CenterPoint = (Start + End) * 0.5f;
                float Distance = FVector::Dist(Start, End);
                float CapsuleHalfLength = (Distance * 0.5f) + TraceHalfHeight; // Total half-height includes sweep distance
                
                // Calculate rotation to align capsule along the sweep direction
                FVector Direction = (End - Start).GetSafeNormal();
                FQuat CapsuleRotation = Direction.ToOrientationQuat();
                
                // Draw ONE capsule spanning the entire sweep
                DrawDebugCapsule(GetWorld(), CenterPoint, CapsuleHalfLength, TraceRadius, 
                    CapsuleRotation, FColor::Yellow, false, 0.1f, 0, 1.f);
            }
            else
            {
                // For sphere, draw at start, middle, and end to show the sweep volume
                FVector MidPoint = (Start + End) * 0.5f;
                DrawDebugSphere(GetWorld(), Start, TraceRadius, 12, FColor::Yellow, false, 0.1f, 0, 1.f);
                DrawDebugSphere(GetWorld(), MidPoint, TraceRadius, 12, FColor::Yellow, false, 0.1f, 0, 1.f);
                DrawDebugSphere(GetWorld(), End, TraceRadius, 12, FColor::Yellow, false, 0.1f, 0, 1.f);
            }
            
            // ✅ Optional: Visualize the previous frame sweep (if exists)
            if (bHasLastPositions)
            {
                DrawDebugLine(GetWorld(), LastStartPos, Start, FColor::Orange, false, 0.1f, 0, 1.f);
                DrawDebugLine(GetWorld(), LastEndPos, End, FColor::Orange, false, 0.1f, 0, 1.f);
            }
        }
    #endif
    
}

void ABaseWeapon::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Client-side: Update UI to show interactable is out of range
    if (!HasAuthority())
    {
        // You can broadcast an event here for UI
        UE_LOG(LogTemp, Log, TEXT("Weapon out of range: %s"), *GetName());
    }
}

void ABaseWeapon::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bCanBePickedUp) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Weapon %s: Pickup disabled"), *GetName());
        return;
    }
    
    // Don't process if overlapping with self or owner
    if (OtherActor == this || OtherActor == GetOwner()) return;

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(OtherActor);
    if (Character && Character->EquipmentComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Weapon %s: Overlap with character %s, bCanBePickedUp: %s"), 
            *GetName(), *Character->GetName(), bCanBePickedUp ? TEXT("True") : TEXT("False"));
        
        if (!HasAuthority())
        {
            // Client-side: Show UI prompt
            UE_LOG(LogTemp, Warning, TEXT("Weapon in range (Client): %s"), *GetName());
        }
    }
}


// IInteractable interface implementation
void ABaseWeapon::OnInteract_Implementation(AActor* Interactor)
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(Interactor);
    
    // CONDITION 2: Check if weapon can be picked up
    if (!Character || !Character->EquipmentComponent || !bCanBePickedUp) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot interact with weapon %s: bCanBePickedUp=%s"), 
            *GetName(), bCanBePickedUp ? TEXT("true") : TEXT("false"));
        return;
    }

    if (HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("Server: Picking up weapon %s for character %s"), 
            *GetName(), *Character->GetName());
            
        // FIX: Store the weapon data BEFORE destroying the actor
        UWeaponDataAsset* WeaponDataCopy = WeaponData;
        
        // FIX: Don't destroy yet - let EquipmentComponent handle the pickup
        // The EquipmentComponent will handle dropping the current weapon
        // and then this weapon should be picked up
        
        // Call EquipmentComponent to handle the pickup with drop logic
        Character->EquipmentComponent->Server_PickupWeaponWithDrop(this);
    }
    else
    {
        // Client-side: Show feedback that interaction was attempted
        UE_LOG(LogTemp, Warning, TEXT("Client: Attempting to pickup weapon %s"), *GetName());
        
        // You might want to add RPC call here to request pickup
    }
}


bool ABaseWeapon::CanInteract_Implementation(AActor* Interactor) const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(Interactor);
    return bCanBePickedUp && Character && Character->EquipmentComponent;
}

FText ABaseWeapon::GetInteractionPrompt_Implementation() const
{
    if (WeaponData)
    {
        return FText::Format(FText::FromString("Pick up {0}"), 
            FText::FromName(WeaponData->WeaponName));
    }
    return FText::FromString("Pick up Weapon");
}

void ABaseWeapon::SetPickupEnabled(bool bEnabled)
{
    bCanBePickedUp = bEnabled;
    InteractionSphere->SetCollisionEnabled(bEnabled ? 
        ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    bCanBePickedUp = bEnabled;

    // CRITICAL: Use multicast function to ensure all clients see the change
    if (HasAuthority())
    {
        Multicast_SetPickupEnabled(bEnabled);
    }

    
    
    // CRASH FIX: Also control overlap events, not just collision
    if (InteractionSphere)
    {
        if (bEnabled)
        {
            InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            InteractionSphere->SetGenerateOverlapEvents(true);
        }
        else
        {
            InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            InteractionSphere->SetGenerateOverlapEvents(false);
        }
    }
}



 
void ABaseWeapon::EnableWeaponCollision(bool bForAttack)
{
    // CRASH FIX: Validate world before setting timers
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("EnableWeaponCollision: World is null!"));
        return;
    }
    
    if (bForAttack)
    {
        // For attack tracing - use trace system, NOT overlap events
        bIsTracing = true;
        ClearHitActors();
        bHasLastPositions = false;
        
        // CRASH FIX: Don't enable overlap events for attacks
        // Use timer-based tracing instead
        if (HasAuthority())
        {
            GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, 
                &ABaseWeapon::PerformTrace, 0.01f, true);
        }
    }
    else
    {
        // For pickup - enable overlap but only for interaction
        bCanBePickedUp = true;
        if (InteractionSphere)
        {
            InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            InteractionSphere->SetGenerateOverlapEvents(true);
        }
    }
}

void ABaseWeapon::DisableWeaponCollision()
{
    // CRASH FIX: Completely disable all collision and overlap events
    bIsTracing = false;
    bCanBePickedUp = false;
    
    if (InteractionSphere)
    {
        InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        InteractionSphere->SetGenerateOverlapEvents(false);
    }
    
    // Clear any active timers
    if (HasAuthority())
    {
        GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
    }
    
    ClearHitActors();
}

void ABaseWeapon::Multicast_SetPickupEnabled_Implementation(bool bEnabled)
{
    // Skip if we're the server (already updated)
    if (HasAuthority()) return;
    
    UpdatePickupCollision(bEnabled);
}

void ABaseWeapon::UpdatePickupCollision(bool bEnabled)
{
    if (InteractionSphere)
    {
        if (bEnabled)
        {
            InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            InteractionSphere->SetGenerateOverlapEvents(true);
            
            // Visual feedback: Make weapon glow or pulse
            WeaponMesh->SetRenderCustomDepth(true);
            WeaponMesh->SetCustomDepthStencilValue(252); // Gold color
        }
        else
        {
            InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            InteractionSphere->SetGenerateOverlapEvents(false);
            
            // Remove visual feedback
            WeaponMesh->SetRenderCustomDepth(false);
        }
    }
}