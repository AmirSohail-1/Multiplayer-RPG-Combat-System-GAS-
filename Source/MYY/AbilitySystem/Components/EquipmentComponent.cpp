#include "EquipmentComponent.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimLayerInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"
#include "MYY/AbilitySystem/Interface/AnimLayerInterface/AnimationLayerInterface.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerCharacter = Cast<AMYYCharacterBase>(GetOwner());

    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent owner is not AMYYCharacterBase!"));
        return;
    }

    if (OwnerCharacter->GetMesh())
    {
        DefaultAnimInstanceClass = OwnerCharacter->GetMesh()->GetAnimClass();
        UE_LOG(LogTemp, Log, TEXT("Cached default anim class: %s"), *GetNameSafe(DefaultAnimInstanceClass));
    }

    // Equip default weapon on server
    if (GetOwner()->HasAuthority() && DefaultWeaponData)
    {
        EquipDefaultWeapon();
    }
    // NEW: If no default weapon, start with unarmed combat
    else if (GetOwner()->HasAuthority() && !DefaultWeaponData && DefaultUnarmedData)
    {
        SwitchToUnarmedCombat();
    }
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UEquipmentComponent, PrimarySlot);
    DOREPLIFETIME(UEquipmentComponent, SecondarySlot);
}

// ========== NEW: GET ACTIVE WEAPON DATA ==========

UWeaponDataAsset* UEquipmentComponent::GetActiveWeaponData() const
{
    // Return current weapon data if any weapon is in hand
    ABaseWeapon* CurrentWeapon = GetCurrentWeapon();
    if (CurrentWeapon && CurrentWeapon->GetWeaponData())
    {
        return CurrentWeapon->GetWeaponData();
    }
    
    // Otherwise return unarmed data
    return DefaultUnarmedData;
}

// ========== UNARMED COMBAT SYSTEM ==========

void UEquipmentComponent::SwitchToUnarmedCombat()
{
    if (!DefaultUnarmedData || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("No DefaultUnarmedData set for unarmed combat!"));
        return;
    }

    if (GetCurrentWeapon())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("❌ Prevented accidental unarmed switch — weapon still in hand: %s"),
            *GetCurrentWeapon()->GetName());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Switching to unarmed combat"));
    
    // Grant unarmed abilities
    GrantUnarmedAbilities();
  
    ApplyWeaponAnimLayers(DefaultUnarmedData); 
}

void UEquipmentComponent::DropAllWeapons()
{
    if (!OwnerCharacter) return;
    
    // Call the server function
    Server_DropAllWeapons();
}


void UEquipmentComponent::Server_DropAllWeapons_Implementation()
{
    if (!OwnerCharacter) return;
    
    UE_LOG(LogTemp, Warning, TEXT("💀 Server_DropAllWeapons called for: %s"), *OwnerCharacter->GetName());

    StopAllWeaponTraces();

    TArray<ABaseWeapon*> WeaponsToDrop;
    TArray<UWeaponDataAsset*> WeaponDataToDrop;

    // Collect Primary weapon
    if (PrimarySlot.bIsOccupied && PrimarySlot.Weapon)
    {
        WeaponsToDrop.Add(PrimarySlot.Weapon);
        WeaponDataToDrop.Add(PrimarySlot.WeaponDataAsset);
    }

    // Collect Secondary weapon
    if (SecondarySlot.bIsOccupied && SecondarySlot.Weapon)
    {
        WeaponsToDrop.Add(SecondarySlot.Weapon);
        WeaponDataToDrop.Add(SecondarySlot.WeaponDataAsset);
    }

    if (WeaponsToDrop.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("  No weapons to drop"));
        return;
    }

    // Drop each weapon
    for (int32 i = 0; i < WeaponsToDrop.Num(); i++)
    {
        ABaseWeapon* Weapon = WeaponsToDrop[i];
        UWeaponDataAsset* WeaponData = WeaponDataToDrop[i];
        
        if (!Weapon) continue;

        UE_LOG(LogTemp, Warning, TEXT("  Dropping weapon %d/%d: %s"), 
            i + 1, WeaponsToDrop.Num(), *Weapon->GetName());

        RemoveWeaponAbilities(WeaponData);
        Weapon->SetPickupEnabled(true);
        Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        // ✅ Use helper function with randomness for multiple weapons
        FVector DropLocation;
        FRotator DropRotation;
        
        // Create list of other weapons to ignore in trace
        TArray<AActor*> OtherWeapons;
        for (ABaseWeapon* OtherWeapon : WeaponsToDrop)
        {
            if (OtherWeapon != Weapon)
            {
                OtherWeapons.Add(OtherWeapon);
            }
        }
        
        CalculateWeaponDropTransform(Weapon, DropLocation, DropRotation, OtherWeapons);
        
        // Add randomness for multiple weapons
        DropLocation += FVector(
            FMath::RandRange(-50.f, 50.f),
            FMath::RandRange(-50.f, 50.f),
            0.f
        );
        
        Weapon->SetActorLocation(DropLocation);
        Weapon->SetActorRotation(DropRotation);

        UE_LOG(LogTemp, Log, TEXT("    ✅ Dropped at: %s"), *DropLocation.ToString());
    }

    // Clear slots
    if (PrimarySlot.bIsOccupied)
    {
        PrimarySlot.Weapon = nullptr;
        PrimarySlot.WeaponDataAsset = nullptr;
        PrimarySlot.bIsInHand = false;
        PrimarySlot.bIsOccupied = false;
    }

    if (SecondarySlot.bIsOccupied)
    {
        SecondarySlot.Weapon = nullptr;
        SecondarySlot.WeaponDataAsset = nullptr;
        SecondarySlot.bIsInHand = false;
        SecondarySlot.bIsOccupied = false;
    }

    if (!GetCurrentWeapon())
    {
        SwitchToUnarmedCombat();
    }
    
    OnWeaponChanged.Broadcast(nullptr, nullptr);

    UE_LOG(LogTemp, Warning, TEXT("✅ Successfully dropped all weapons (%d total)"), 
        WeaponsToDrop.Num());
}


void UEquipmentComponent::GrantUnarmedAbilities()
{
    if (!DefaultUnarmedData || !OwnerCharacter || !OwnerCharacter->AbilitySystemComponent)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Granting unarmed abilities"));
    
    for (const FAbilityInputMapping& Mapping : DefaultUnarmedData->GrantedAbilities)
    {
        if (!Mapping.AbilityClass) continue;
        
        FGameplayAbilitySpec AbilitySpec(
            Mapping.AbilityClass, 
            1, 
            (int32)Mapping.InputID,
            DefaultUnarmedData // Pass unarmed data as source object
        );
        
        FGameplayAbilitySpecHandle Handle = 
            OwnerCharacter->AbilitySystemComponent->GiveAbility(AbilitySpec);
        
        OwnerCharacter->GrantedAbilityHandles.Add(Mapping.AbilityClass, Handle);
    }
}

void UEquipmentComponent::RemoveUnarmedAbilities()
{
    if (!DefaultUnarmedData || !OwnerCharacter)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Removing unarmed abilities"));
    
    for (const FAbilityInputMapping& Mapping : DefaultUnarmedData->GrantedAbilities)
    {
        OwnerCharacter->RemoveAbility(Mapping.AbilityClass);
    }
}

// ========== EXISTING FUNCTIONS WITH UNARMED SUPPORT ==========

void UEquipmentComponent::EquipDefaultWeapon()
{
    UE_LOG(LogTemp, Warning, TEXT("EquipDefaultWeapon called"));
    
    if (DefaultWeaponData && GetOwner()->HasAuthority())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = OwnerCharacter;
        SpawnParams.Instigator = OwnerCharacter;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ABaseWeapon* DefaultWeapon = GetWorld()->SpawnActor<ABaseWeapon>(
            DefaultWeaponData->WeaponActorClass, FTransform::Identity, SpawnParams);

        if (DefaultWeapon)
        {
            DefaultWeapon->WeaponData = DefaultWeaponData;
            
            // ✅ Double-check owner is set
            if (!DefaultWeapon->GetOwner())
            {
                DefaultWeapon->SetOwner(OwnerCharacter);
                DefaultWeapon->SetInstigator(OwnerCharacter);
            }

            // ✅ NEW: Initialize ammo for ranged weapons
            if (URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(DefaultWeaponData))
            {
                DefaultWeapon->CurrentAmmo = RangedData->AmmoConfig.StartingAmmo;
                UE_LOG(LogTemp, Log, TEXT("Initialized ranged weapon with %d ammo"), 
                    DefaultWeapon->CurrentAmmo);
            }
            
            Server_EquipWeapon(DefaultWeapon);
        }
    }
}

ABaseWeapon* UEquipmentComponent::GetCurrentWeapon() const
{
    if (PrimarySlot.bIsInHand && PrimarySlot.Weapon)
    {
        return PrimarySlot.Weapon;
    }
    
    if (SecondarySlot.bIsInHand && SecondarySlot.Weapon)
    {
        return SecondarySlot.Weapon;
    }
    
    return nullptr;
}

ABaseWeapon* UEquipmentComponent::GetWeaponInSlot(EWeaponSlot Slot) const
{
    switch (Slot)
    {
        case EWeaponSlot::Primary:
            return PrimarySlot.Weapon;
        case EWeaponSlot::Secondary:
            return SecondarySlot.Weapon;
        default:
            return nullptr;
    }
}

bool UEquipmentComponent::HasAvailableSlot() const
{
    return !PrimarySlot.bIsOccupied || !SecondarySlot.bIsOccupied;
}

FName UEquipmentComponent::GetHandSocketForWeapon(UWeaponDataAsset* WeaponData) const
{
    if (!WeaponData)
        return WeaponSocketName;
    
    FName HandSocket = WeaponData->HandSocketName;
    if (HandSocket.IsNone())
    {
        HandSocket = WeaponSocketName;
    }
    
    return HandSocket;
}

void UEquipmentComponent::Server_DropCurrentWeapon_Implementation()
{  
    ABaseWeapon* CurrentWeapon = GetCurrentWeapon();
    if (!CurrentWeapon || !OwnerCharacter) return;

    UE_LOG(LogTemp, Warning, TEXT("[Equip Test] Dropping current weapon"));

    EWeaponSlot CurrentSlot = GetSlotForWeapon(CurrentWeapon);
    FWeaponSlotData* SlotData = (CurrentSlot == EWeaponSlot::Primary) ? &PrimarySlot : &SecondarySlot;
    
    RemoveWeaponAbilities(SlotData->WeaponDataAsset);
    
    SlotData->Weapon = nullptr;
    SlotData->WeaponDataAsset = nullptr;
    SlotData->bIsInHand = false;
    SlotData->bIsOccupied = false;
    
    CurrentWeapon->SetPickupEnabled(true);
    CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // ✅ Use helper function
    FVector DropLocation;
    FRotator DropRotation;
    CalculateWeaponDropTransform(CurrentWeapon, DropLocation, DropRotation);
    
    CurrentWeapon->SetActorLocation(DropLocation);
    CurrentWeapon->SetActorRotation(DropRotation);
    
    if (!GetCurrentWeapon())
    {
        SwitchToUnarmedCombat();
    }
    
    OnWeaponChanged.Broadcast(nullptr, CurrentWeapon);
}


void UEquipmentComponent::Server_UnequipWeapon_Implementation()
{
    ABaseWeapon* ActiveWeapon = GetCurrentWeapon();
    if (!ActiveWeapon || !OwnerCharacter) return;

    // NEW: Stop weapon traces before unequipping
    StopAllWeaponTraces();

    EWeaponSlot CurrentSlot = GetSlotForWeapon(ActiveWeapon);
    FWeaponSlotData* CurrentSlotData = (CurrentSlot == EWeaponSlot::Primary) ? &PrimarySlot : &SecondarySlot;
 
    RemoveWeaponAbilities(CurrentSlotData->WeaponDataAsset);
    
    ActiveWeapon->SetPickupEnabled(true);
    
    CurrentSlotData->Weapon = nullptr;
    CurrentSlotData->WeaponDataAsset = nullptr;
    CurrentSlotData->bIsInHand = false;
    CurrentSlotData->bIsOccupied = false;

    // NEW: Switch to unarmed if no weapons remain
    if (!GetCurrentWeapon())
    {
        SwitchToUnarmedCombat();
    }

    OnWeaponChanged.Broadcast(nullptr, ActiveWeapon);
}

void UEquipmentComponent::Server_SwapWeaponHandToHolster_Implementation(EWeaponSlot Slot)
{
    if (!OwnerCharacter) return;

    FWeaponSlotData* TargetSlot = (Slot == EWeaponSlot::Primary) ? &PrimarySlot : &SecondarySlot;
    FString SlotName = (Slot == EWeaponSlot::Primary) ? TEXT("PRIMARY") : TEXT("SECONDARY");

    if (!TargetSlot->bIsOccupied || !TargetSlot->Weapon || !TargetSlot->WeaponDataAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] Swap failed: %s slot not occupied"), *SlotName);
        return;
    }

    // ✅ Ensure weapon has owner
    if (TargetSlot->Weapon && !TargetSlot->Weapon->GetOwner())
    {
        TargetSlot->Weapon->SetOwner(OwnerCharacter);
        TargetSlot->Weapon->SetInstigator(OwnerCharacter);
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] Fixed missing owner for weapon in %s slot"), *SlotName);
    }

    // NEW: Stop weapon traces before unequipping
    StopAllWeaponTraces();

    ABaseWeapon* CurrentInHandWeapon = GetCurrentWeapon();

    if (CurrentInHandWeapon)
    {
        CurrentInHandWeapon->SetPickupEnabled(false);
    }
    
    if (TargetSlot->bIsInHand)
    {
        // ============== HOLSTER WEAPON ==============
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] Holstering %s weapon"), *SlotName);
        
        TargetSlot->bIsInHand = false;
        RemoveWeaponAbilities(TargetSlot->WeaponDataAsset);
        
        FName HolsterSocket = (Slot == EWeaponSlot::Primary) ? PrimaryHolsterSocket : SecondaryHolsterSocket;
        
        TargetSlot->Weapon->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HolsterSocket
        );
        
        bool bAnyWeaponStillInHand = false;
        if (PrimarySlot.bIsOccupied && PrimarySlot.bIsInHand)
            bAnyWeaponStillInHand = true;
        if (SecondarySlot.bIsOccupied && SecondarySlot.bIsInHand)
            bAnyWeaponStillInHand = true;
            
        if (!bAnyWeaponStillInHand)
        {
            SwitchToUnarmedCombat();
        }

        TargetSlot->Weapon->SetPickupEnabled(false);
    }
    else
    {
        // ============== EQUIP FROM HOLSTER ==============
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] Equipping %s weapon from holster"), *SlotName);
        
        if (DefaultUnarmedData)
        {
            RemoveUnarmedAbilities();
        }
        
        if (CurrentInHandWeapon)
        {
            EWeaponSlot ActiveSlot = GetSlotForWeapon(CurrentInHandWeapon);
            FWeaponSlotData* ActiveSlotData = (ActiveSlot == EWeaponSlot::Primary) ? &PrimarySlot : &SecondarySlot;
            
            ActiveSlotData->bIsInHand = false;
            RemoveWeaponAbilities(ActiveSlotData->WeaponDataAsset);
            
            FName HolsterSocket = (ActiveSlot == EWeaponSlot::Primary) ? PrimaryHolsterSocket : SecondaryHolsterSocket;
            
            CurrentInHandWeapon->AttachToComponent(
                OwnerCharacter->GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                HolsterSocket
            );
        }
        
        TargetSlot->bIsInHand = true;
        GrantWeaponAbilities(TargetSlot->WeaponDataAsset);
        
        FName HandSocket = TargetSlot->WeaponDataAsset->HandSocketName;
        if (HandSocket.IsNone()) HandSocket = WeaponSocketName;
        
        TargetSlot->Weapon->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HandSocket
        );
        
        // MODIFIED: Use layer system
        ApplyWeaponAnimLayers(TargetSlot->WeaponDataAsset);
        TargetSlot->Weapon->SetPickupEnabled(false);
    }

    OnWeaponChanged.Broadcast(GetCurrentWeapon(), nullptr);
}

 

void UEquipmentComponent::Server_EquipWeapon_Implementation(ABaseWeapon* WeaponToEquip)
{
    if (!WeaponToEquip || !OwnerCharacter) return;

    if (!WeaponToEquip->GetWeaponData())
    {
        UE_LOG(LogTemp, Error, TEXT("[Equip Test] Weapon has no WeaponData!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Equip Test] 🔧 Server_EquipWeapon: %s"), 
        *WeaponToEquip->GetWeaponData()->WeaponName.ToString());

    // ✅ CRITICAL FIX: SET THE WEAPON OWNER
    WeaponToEquip->SetOwner(OwnerCharacter);
    WeaponToEquip->SetInstigator(OwnerCharacter);
    
    // Stop all weapon traces before equipping new weapon
    StopAllWeaponTraces();

    // Remove unarmed abilities before equipping weapon
    if (DefaultUnarmedData)
    {
        RemoveUnarmedAbilities();
    }

    ABaseWeapon* PreviousInHand = GetCurrentWeapon();
    EWeaponSlot TargetSlot = EWeaponSlot::None;

    bool bHasEmptyPrimary = !PrimarySlot.bIsOccupied;
    bool bHasEmptySecondary = !SecondarySlot.bIsOccupied;
    
    // 🔍 DEBUG: Log current state when entering Server_EquipWeapon
    UE_LOG(LogTemp, Warning, TEXT("[Equip Test] 📊 State at Server_EquipWeapon entry:"));
    UE_LOG(LogTemp, Warning, TEXT("  Primary: Empty=%s, InHand=%s"), 
        bHasEmptyPrimary ? TEXT("YES") : TEXT("NO"),
        PrimarySlot.bIsInHand ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Warning, TEXT("  Secondary: Empty=%s, InHand=%s"), 
        bHasEmptySecondary ? TEXT("YES") : TEXT("NO"),
        SecondarySlot.bIsInHand ? TEXT("YES") : TEXT("NO"));
    
    // ============== SCENARIO 1: Both slots empty ==============
    if (bHasEmptyPrimary && bHasEmptySecondary)
    {
        TargetSlot = EWeaponSlot::Primary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 1: Both slots empty, using PRIMARY"));
    }
    // ============== SCENARIO 2: Primary in hand, Secondary empty ==============
    else if (!bHasEmptyPrimary && PrimarySlot.bIsInHand && bHasEmptySecondary)
    {
        TargetSlot = EWeaponSlot::Secondary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 2: Primary in hand, Secondary empty - holster Primary, use Secondary"));
        
        PrimarySlot.bIsInHand = false;
        RemoveWeaponAbilities(PrimarySlot.WeaponDataAsset);
        
        FName HolsterSocket = PrimarySlot.WeaponDataAsset->HolsterSocketName;
        if (HolsterSocket.IsNone()) HolsterSocket = PrimaryHolsterSocket;
        
        if (PrimarySlot.Weapon)
        {
            PrimarySlot.Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            AttachWeaponToSocket(PrimarySlot.Weapon, HolsterSocket);
        }
    }
    // ============== SCENARIO 3: Primary holstered, Secondary empty ==============
    else if (!bHasEmptyPrimary && !PrimarySlot.bIsInHand && bHasEmptySecondary)
    {
        TargetSlot = EWeaponSlot::Secondary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 3: Primary holstered, Secondary empty - use Secondary"));
    }
    // ============== SCENARIO 4: Primary empty, Secondary holstered ==============
    else if (bHasEmptyPrimary && !bHasEmptySecondary && !SecondarySlot.bIsInHand)
    {
        TargetSlot = EWeaponSlot::Primary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 4: Primary empty, Secondary holstered - use Primary"));
    }
    // ============== SCENARIO 5: Primary empty, Secondary in hand ==============
    else if (bHasEmptyPrimary && !bHasEmptySecondary && SecondarySlot.bIsInHand)
    {
        TargetSlot = EWeaponSlot::Primary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 5: Primary empty, Secondary in hand - use Primary"));
    }
    // ============== SCENARIO 6: Secondary holstered, Primary in hand ==============
    else if (!bHasEmptyPrimary && PrimarySlot.bIsInHand && !bHasEmptySecondary && !SecondarySlot.bIsInHand)
    {
        TargetSlot = EWeaponSlot::Primary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 6: Primary in hand, Secondary holstered - Holster Primary, use Primary"));
        
        // Holster the primary weapon
        PrimarySlot.bIsInHand = false;
        RemoveWeaponAbilities(PrimarySlot.WeaponDataAsset);
        
        FName HolsterSocket = PrimarySlot.WeaponDataAsset->HolsterSocketName;
        if (HolsterSocket.IsNone()) HolsterSocket = PrimaryHolsterSocket;
        
        if (PrimarySlot.Weapon)
        {
            PrimarySlot.Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            AttachWeaponToSocket(PrimarySlot.Weapon, HolsterSocket);
        }
        
        // Clear the primary slot for new weapon
        PrimarySlot.Weapon = nullptr;
        PrimarySlot.WeaponDataAsset = nullptr;
        PrimarySlot.bIsOccupied = false;
    }
    // ============== SCENARIO 7: Primary holstered, Secondary in hand ==============
    else if (!bHasEmptyPrimary && !PrimarySlot.bIsInHand && !bHasEmptySecondary && SecondarySlot.bIsInHand)
    {
        TargetSlot = EWeaponSlot::Secondary;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ SCENARIO 7: Secondary in hand, Primary holstered - Holster Secondary, use Secondary"));
        
        // Holster the secondary weapon
        SecondarySlot.bIsInHand = false;
        RemoveWeaponAbilities(SecondarySlot.WeaponDataAsset);
        
        FName HolsterSocket = SecondarySlot.WeaponDataAsset->HolsterSocketName;
        if (HolsterSocket.IsNone()) HolsterSocket = SecondaryHolsterSocket;
        
        if (SecondarySlot.Weapon)
        {
            SecondarySlot.Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            AttachWeaponToSocket(SecondarySlot.Weapon, HolsterSocket);
        }
        
        // Clear the secondary slot for new weapon
        SecondarySlot.Weapon = nullptr;
        SecondarySlot.WeaponDataAsset = nullptr;
        SecondarySlot.bIsOccupied = false;
    }
    // ============== SCENARIO 8: Both holstered ==============
    else if (!bHasEmptyPrimary && !PrimarySlot.bIsInHand && !bHasEmptySecondary && !SecondarySlot.bIsInHand)
    {
        UE_LOG(LogTemp, Error, TEXT("[Equip Test] ❌ SCENARIO 8: Both slots occupied with holstered weapons - Should not reach here!"));
        UE_LOG(LogTemp, Error, TEXT("  This should have been handled by Server_PickupWeaponWithDrop"));
        return;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Equip Test] ❌ Unexpected slot state!"));
        return;
    }

    // Disable pickup for weapon being equipped
    WeaponToEquip->SetPickupEnabled(false);

    UWeaponDataAsset* WeaponData = WeaponToEquip->GetWeaponData();
    FName HandSocket = GetHandSocketForWeapon(WeaponData);

    if (OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->DoesSocketExist(HandSocket))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] Socket %s exists on mesh"), *HandSocket.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Equip Test] Socket %s does NOT exist on mesh!"), *HandSocket.ToString());
        HandSocket = WeaponSocketName;
    }

    WeaponToEquip->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    if (WeaponToEquip->WeaponMesh)
    {
        WeaponToEquip->WeaponMesh->SetSimulatePhysics(false);
        WeaponToEquip->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    AttachWeaponToSocket(WeaponToEquip, HandSocket);

    // Assign to target slot
    if (TargetSlot == EWeaponSlot::Primary)
    {
        PrimarySlot.Weapon = WeaponToEquip;
        PrimarySlot.WeaponDataAsset = WeaponData;
        PrimarySlot.bIsInHand = true;
        PrimarySlot.bIsOccupied = true;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ Equipped to PRIMARY slot"));
    }
    else if (TargetSlot == EWeaponSlot::Secondary)
    {
        SecondarySlot.Weapon = WeaponToEquip;
        SecondarySlot.WeaponDataAsset = WeaponData;
        SecondarySlot.bIsInHand = true;
        SecondarySlot.bIsOccupied = true;
        UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ Equipped to SECONDARY slot"));
    }

    GrantWeaponAbilities(WeaponData);
    ApplyWeaponAnimLayers(WeaponData);

    OnWeaponChanged.Broadcast(WeaponToEquip, PreviousInHand);
    
    UE_LOG(LogTemp, Warning, TEXT("[Equip Test] ✅ Equip complete!"));
}

 

void UEquipmentComponent::Server_PickupWeaponWithDrop_Implementation(ABaseWeapon* PickupWeapon)
{
    if (!PickupWeapon || !OwnerCharacter) return;

    UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════════════"));
    UE_LOG(LogTemp, Warning, TEXT("[Pickup] 🎯 Attempting to pickup weapon: %s"), 
        *PickupWeapon->GetName());

    // ✅ Set owner before any logic
    PickupWeapon->SetOwner(OwnerCharacter);
    PickupWeapon->SetInstigator(OwnerCharacter);

    UWeaponDataAsset* WeaponDataToEquip = PickupWeapon->GetWeaponData();
    if (!WeaponDataToEquip) 
    {
        UE_LOG(LogTemp, Error, TEXT("[Pickup] ❌ Weapon has no WeaponData!"));
        return;
    }

    // Check slot occupancy
    bool bPrimaryOccupied = PrimarySlot.bIsOccupied;
    bool bSecondaryOccupied = SecondarySlot.bIsOccupied;
    bool bPrimaryInHand = PrimarySlot.bIsInHand;
    bool bSecondaryInHand = SecondarySlot.bIsInHand;

    // 🔍 DEBUG: Log current state
    UE_LOG(LogTemp, Warning, TEXT("[Pickup] 📊 Current Weapon State:"));
    UE_LOG(LogTemp, Warning, TEXT("  Primary: Occupied=%s, InHand=%s, Weapon=%s"), 
        bPrimaryOccupied ? TEXT("YES") : TEXT("NO"),
        bPrimaryInHand ? TEXT("YES") : TEXT("NO"),
        PrimarySlot.Weapon ? *PrimarySlot.Weapon->GetName() : TEXT("NONE"));
    UE_LOG(LogTemp, Warning, TEXT("  Secondary: Occupied=%s, InHand=%s, Weapon=%s"), 
        bSecondaryOccupied ? TEXT("YES") : TEXT("NO"),
        bSecondaryInHand ? TEXT("YES") : TEXT("NO"),
        SecondarySlot.Weapon ? *SecondarySlot.Weapon->GetName() : TEXT("NONE"));

    // ============== SCENARIO 1: Both slots empty ==============
    if (!bPrimaryOccupied && !bSecondaryOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] ✅ SCENARIO 1: Both slots empty - Normal equip"));
        Server_EquipWeapon(PickupWeapon);
        return;
    }

    // ============== SCENARIO 2: Primary occupied (in hand), Secondary empty ==============
    if (bPrimaryOccupied && bPrimaryInHand && !bSecondaryOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] ✅ SCENARIO 2: Primary in hand, Secondary empty - Holster Primary, equip to Secondary"));
        Server_EquipWeapon(PickupWeapon);
        return;
    }

    // ============== SCENARIO 3: Primary occupied (holstered), Secondary empty ==============
    if (bPrimaryOccupied && !bPrimaryInHand && !bSecondaryOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] ✅ SCENARIO 3: Primary holstered, Secondary empty - Equip to Secondary"));
        Server_EquipWeapon(PickupWeapon);
        return;
    }

    // ============== SCENARIO 4: Primary empty, Secondary occupied (in hand) ==============
    if (!bPrimaryOccupied && bSecondaryOccupied && bSecondaryInHand)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] ✅ SCENARIO 4: Secondary in hand, Primary empty - Equip to Primary"));
        Server_EquipWeapon(PickupWeapon);
        return;
    }

    // ============== SCENARIO 5: Primary empty, Secondary occupied (holstered) ==============
    if (!bPrimaryOccupied && bSecondaryOccupied && !bSecondaryInHand)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] ✅ SCENARIO 5: Secondary holstered, Primary empty - Equip to Primary"));
        Server_EquipWeapon(PickupWeapon);
        return;
    }

    // ============== SCENARIO 6: Primary in hand, Secondary holstered ==============
    if (bPrimaryOccupied && bPrimaryInHand && bSecondaryOccupied && !bSecondaryInHand)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] 💥 SCENARIO 6: Primary in hand, Secondary holstered - DROP Primary, equip to Primary slot"));
        
        ABaseWeapon* WeaponToDrop = PrimarySlot.Weapon;
        UE_LOG(LogTemp, Warning, TEXT("  📦 Weapon to drop: %s"), *WeaponToDrop->GetName());
        
        // Stop traces before dropping
        StopAllWeaponTraces();
        
        // Remove abilities and clear slot
        RemoveWeaponAbilities(PrimarySlot.WeaponDataAsset);
        PrimarySlot.Weapon = nullptr;
        PrimarySlot.WeaponDataAsset = nullptr;
        PrimarySlot.bIsOccupied = false;
        PrimarySlot.bIsInHand = false;
        
        UE_LOG(LogTemp, Warning, TEXT("  🗑️ Primary slot cleared"));
        
        // Drop the weapon
        if (WeaponToDrop)
        {
            WeaponToDrop->SetPickupEnabled(true);
            WeaponToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            
            FVector DropLocation;
            FRotator DropRotation;
            TArray<AActor*> IgnoreActors = { PickupWeapon };
            CalculateWeaponDropTransform(WeaponToDrop, DropLocation, DropRotation, IgnoreActors);
            
            WeaponToDrop->SetActorLocation(DropLocation);
            WeaponToDrop->SetActorRotation(DropRotation);
            
            UE_LOG(LogTemp, Warning, TEXT("  ✅ Dropped at: %s"), *DropLocation.ToString());
        }
        
        // Equip new weapon
        UE_LOG(LogTemp, Warning, TEXT("  🔄 Now calling Server_EquipWeapon for new weapon..."));
        Server_EquipWeapon(PickupWeapon);
        UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════════════"));
        return;
    }

    // ============== SCENARIO 7: Primary holstered, Secondary in hand ==============
    if (bPrimaryOccupied && !bPrimaryInHand && bSecondaryOccupied && bSecondaryInHand)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] 💥 SCENARIO 7: Secondary in hand, Primary holstered - DROP Secondary, equip to Secondary slot"));
        
        ABaseWeapon* WeaponToDrop = SecondarySlot.Weapon;
        UE_LOG(LogTemp, Warning, TEXT("  📦 Weapon to drop: %s"), *WeaponToDrop->GetName());
        
        // Stop traces before dropping
        StopAllWeaponTraces();
        
        // Remove abilities and clear slot
        RemoveWeaponAbilities(SecondarySlot.WeaponDataAsset);
        SecondarySlot.Weapon = nullptr;
        SecondarySlot.WeaponDataAsset = nullptr;
        SecondarySlot.bIsOccupied = false;
        SecondarySlot.bIsInHand = false;
        
        UE_LOG(LogTemp, Warning, TEXT("  🗑️ Secondary slot cleared"));
        
        // Drop the weapon
        if (WeaponToDrop)
        {
            WeaponToDrop->SetPickupEnabled(true);
            WeaponToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            
            FVector DropLocation;
            FRotator DropRotation;
            TArray<AActor*> IgnoreActors = { PickupWeapon };
            CalculateWeaponDropTransform(WeaponToDrop, DropLocation, DropRotation, IgnoreActors);
            
            WeaponToDrop->SetActorLocation(DropLocation);
            WeaponToDrop->SetActorRotation(DropRotation);
            
            UE_LOG(LogTemp, Warning, TEXT("  ✅ Dropped at: %s"), *DropLocation.ToString());
        }
        
        // Equip new weapon
        UE_LOG(LogTemp, Warning, TEXT("  🔄 Now calling Server_EquipWeapon for new weapon..."));
        Server_EquipWeapon(PickupWeapon);
        UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════════════"));
        return;
    }

    // ============== SCENARIO 8: Both weapons holstered ==============
    if (bPrimaryOccupied && !bPrimaryInHand && bSecondaryOccupied && !bSecondaryInHand)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] 💥 SCENARIO 8: Both holstered - DROP Primary, equip to Primary slot"));
        
        ABaseWeapon* WeaponToDrop = PrimarySlot.Weapon;
        UE_LOG(LogTemp, Warning, TEXT("  📦 Weapon to drop: %s"), *WeaponToDrop->GetName());
        
        // Remove abilities and clear slot
        RemoveWeaponAbilities(PrimarySlot.WeaponDataAsset);
        PrimarySlot.Weapon = nullptr;
        PrimarySlot.WeaponDataAsset = nullptr;
        PrimarySlot.bIsOccupied = false;
        PrimarySlot.bIsInHand = false;
        
        UE_LOG(LogTemp, Warning, TEXT("  🗑️ Primary slot cleared"));
        
        // Drop the weapon
        if (WeaponToDrop)
        {
            WeaponToDrop->SetPickupEnabled(true);
            WeaponToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            
            FVector DropLocation;
            FRotator DropRotation;
            TArray<AActor*> IgnoreActors = { PickupWeapon };
            CalculateWeaponDropTransform(WeaponToDrop, DropLocation, DropRotation, IgnoreActors);
            
            WeaponToDrop->SetActorLocation(DropLocation);
            WeaponToDrop->SetActorRotation(DropRotation);
            
            UE_LOG(LogTemp, Warning, TEXT("  ✅ Dropped at: %s"), *DropLocation.ToString());
        }
        
        // Equip new weapon
        UE_LOG(LogTemp, Warning, TEXT("  🔄 Now calling Server_EquipWeapon for new weapon..."));
        Server_EquipWeapon(PickupWeapon);
        UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════════════"));
        return;
    }

    // ============== FALLBACK: Unexpected state ==============
    UE_LOG(LogTemp, Error, TEXT("[Pickup] ❌ UNEXPECTED STATE!"));
    UE_LOG(LogTemp, Error, TEXT("  Primary: Occupied=%s, InHand=%s"), 
        bPrimaryOccupied ? TEXT("YES") : TEXT("NO"),
        bPrimaryInHand ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Error, TEXT("  Secondary: Occupied=%s, InHand=%s"), 
        bSecondaryOccupied ? TEXT("YES") : TEXT("NO"),
        bSecondaryInHand ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════════════"));
}



void UEquipmentComponent::CalculateWeaponDropTransform(ABaseWeapon* WeaponToDrop, FVector& OutLocation, FRotator& OutRotation, const TArray<AActor*>& AdditionalIgnoredActors)
{
    if (!OwnerCharacter || !WeaponToDrop)
    {
        OutLocation = FVector::ZeroVector;
        OutRotation = FRotator::ZeroRotator;
        return;
    }

    // Calculate trace positions
    FVector StartLocation = OwnerCharacter->GetActorLocation() + 
                          (OwnerCharacter->GetActorForwardVector() * 100.0f) +
                          FVector(0, 0, 100.0f); // Start 100 units above
    
    FVector EndLocation = StartLocation - FVector(0, 0, 500.0f); // Trace 500 units down
    
    // Setup collision parameters
    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(OwnerCharacter);
    CollisionParams.AddIgnoredActor(WeaponToDrop);
    CollisionParams.bTraceComplex = true;
    
    // Ignore all pawns
    TArray<AActor*> AllPawns;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), AllPawns);
    for (AActor* Pawn : AllPawns)
    {
        CollisionParams.AddIgnoredActor(Pawn);
    }
    
    // Add any additional actors to ignore
    for (AActor* Actor : AdditionalIgnoredActors)
    {
        if (Actor)
        {
            CollisionParams.AddIgnoredActor(Actor);
        }
    }
    
    // Perform ground trace
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_WorldStatic, // Only static world geometry
        CollisionParams
    );
    
    if (bHit)
    {
        // Place weapon on the ground surface
        OutLocation = HitResult.Location + FVector(0, 0, 5.0f); // Slightly above ground
        OutRotation = FRotator(0, OwnerCharacter->GetActorRotation().Yaw, 0);
        
        UE_LOG(LogTemp, Log, TEXT("    Calculated drop on ground: %s"), 
            *HitResult.GetComponent()->GetName());
    }
    else
    {
        // Fallback: drop at character feet level
        OutLocation = OwnerCharacter->GetActorLocation() + 
                     (OwnerCharacter->GetActorForwardVector() * 100.0f);
        OutLocation.Z = OwnerCharacter->GetActorLocation().Z;
        OutRotation = FRotator::ZeroRotator;
        
        UE_LOG(LogTemp, Warning, TEXT("    No ground found, using fallback location"));
    }
}

void UEquipmentComponent::ApplyDropPhysics(ABaseWeapon* WeaponToDrop, FVector ImpulseDirection)
{
    if (!WeaponToDrop || !WeaponToDrop->WeaponMesh) return;

    WeaponToDrop->WeaponMesh->SetSimulatePhysics(true);
    WeaponToDrop->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WeaponToDrop->WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
    WeaponToDrop->WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    
    FVector Impulse = ImpulseDirection * 500.0f;
    WeaponToDrop->WeaponMesh->AddImpulse(Impulse);
    
    FTimerHandle PhysicsTimerHandle;
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindUObject(this, &UEquipmentComponent::DisableDropPhysics, WeaponToDrop);
    
    GetWorld()->GetTimerManager().SetTimer(
        PhysicsTimerHandle, 
        TimerDelegate, 
        3.0f,
        false
    );
    
    UE_LOG(LogTemp, Warning, TEXT("[Drop Physics] Applied physics to dropped weapon"));
}

void UEquipmentComponent::DisableDropPhysics(ABaseWeapon* Weapon)
{
    if (!Weapon || !Weapon->WeaponMesh) return;
    
    Weapon->WeaponMesh->SetSimulatePhysics(false);
    Weapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Weapon->WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Weapon->WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Weapon->WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    
    FVector CurrentLocation = Weapon->GetActorLocation();
    FVector GroundLocation = CurrentLocation;
    GroundLocation.Z -= 100.0f;
    
    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(
        HitResult, 
        CurrentLocation, 
        GroundLocation, 
        ECC_WorldStatic))
    {
        Weapon->SetActorLocation(HitResult.Location + FVector(0, 0, 10.0f));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Drop Physics] Disabled physics on dropped weapon, enabled pickup collision"));
}


void UEquipmentComponent::OnRep_PrimarySlot()
{
    UE_LOG(LogTemp, Log, TEXT("[Client OnRep] PrimarySlot - bIsInHand: %s, bIsOccupied: %s"), 
        PrimarySlot.bIsInHand ? TEXT("True") : TEXT("False"),
        PrimarySlot.bIsOccupied ? TEXT("True") : TEXT("False"));

    if (!OwnerCharacter) return;

    // ✅ FIX: Handle weapon removal
    if (!PrimarySlot.Weapon || !PrimarySlot.WeaponDataAsset)
    {
        // Weapon was removed - switch to unarmed if no other weapon in hand
        if (!GetCurrentWeapon() && DefaultUnarmedData)
        {
            ApplyWeaponAnimLayers(DefaultUnarmedData);
        }
        OnWeaponChanged.Broadcast(GetCurrentWeapon(), nullptr);
        return;
    }

    if (PrimarySlot.bIsInHand)
    {
        FName HandSocket = PrimarySlot.WeaponDataAsset->HandSocketName;
        if (HandSocket.IsNone()) HandSocket = WeaponSocketName;
        
        PrimarySlot.Weapon->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HandSocket
        );
        
        // MODIFIED: Use layer system
        ApplyWeaponAnimLayers(PrimarySlot.WeaponDataAsset);
        UE_LOG(LogTemp, Log, TEXT("[Client] Attached PRIMARY to hand: %s"), *HandSocket.ToString());
    }
    else if (PrimarySlot.bIsOccupied)
    {
        FName HolsterSocket = PrimarySlot.WeaponDataAsset->HolsterSocketName;
        
        PrimarySlot.Weapon->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HolsterSocket
        );
        
        // ✅ FIX: Switch to unarmed/other weapon anims if this weapon was holstered
        if (!GetCurrentWeapon() && DefaultUnarmedData)
        {
            ApplyWeaponAnimLayers(DefaultUnarmedData);
        }
        else if (GetCurrentWeapon() && GetCurrentWeapon()->GetWeaponData())
        {
            ApplyWeaponAnimLayers(GetCurrentWeapon()->GetWeaponData());
        }
        UE_LOG(LogTemp, Log, TEXT("[Client] Attached PRIMARY to holster: %s"), *HolsterSocket.ToString());
    }
    
    OnWeaponChanged.Broadcast(GetCurrentWeapon(), nullptr);
}

void UEquipmentComponent::OnRep_SecondarySlot()
{
    UE_LOG(LogTemp, Log, TEXT("[Client OnRep] SecondarySlot - bIsInHand: %s, bIsOccupied: %s"), 
        SecondarySlot.bIsInHand ? TEXT("True") : TEXT("False"),
        SecondarySlot.bIsOccupied ? TEXT("True") : TEXT("False"));

    // ✅ FIX: Handle weapon removal
    if (!SecondarySlot.Weapon || !SecondarySlot.WeaponDataAsset)
    {
        // Weapon was removed - switch to unarmed if no other weapon in hand
        if (!GetCurrentWeapon() && DefaultUnarmedData)
        {
            ApplyWeaponAnimLayers(DefaultUnarmedData);
        }
        OnWeaponChanged.Broadcast(GetCurrentWeapon(), nullptr);
        return;
    }

    if (SecondarySlot.bIsInHand)
    {
        FName HandSocket = SecondarySlot.WeaponDataAsset->HandSocketName;
        if (HandSocket.IsNone()) HandSocket = WeaponSocketName;
        
        SecondarySlot.Weapon->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HandSocket
        );
        
        // MODIFIED: Use layer system
        ApplyWeaponAnimLayers(SecondarySlot.WeaponDataAsset);
        UE_LOG(LogTemp, Log, TEXT("[Client] Attached SECONDARY to hand: %s"), *HandSocket.ToString());
    }
    else if (SecondarySlot.bIsOccupied)
    {
        FName HolsterSocket = SecondarySlot.WeaponDataAsset->HolsterSocketName;
        
        SecondarySlot.Weapon->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HolsterSocket
        );
        
        // ✅ FIX: Switch to unarmed/other weapon anims if this weapon was holstered
        if (!GetCurrentWeapon() && DefaultUnarmedData)
        {
            ApplyWeaponAnimLayers(DefaultUnarmedData);
        }
        else if (GetCurrentWeapon() && GetCurrentWeapon()->GetWeaponData())
        {
            ApplyWeaponAnimLayers(GetCurrentWeapon()->GetWeaponData());
        }
        UE_LOG(LogTemp, Log, TEXT("[Client] Attached SECONDARY to holster: %s"), *HolsterSocket.ToString());
    }
    
    OnWeaponChanged.Broadcast(GetCurrentWeapon(), nullptr);
}


void UEquipmentComponent::GrantWeaponAbilities(UWeaponDataAsset* WeaponData) const
{
    if (!OwnerCharacter || !WeaponData || !OwnerCharacter->AbilitySystemComponent) return;

    UE_LOG(LogTemp, Warning, TEXT("🔧 GrantWeaponAbilities for: %s"), *WeaponData->WeaponName.ToString());
    
    for (const FAbilityInputMapping& Mapping : WeaponData->GrantedAbilities)
    {
        if (!Mapping.AbilityClass) continue;
        
        FGameplayAbilitySpec AbilitySpec(
            Mapping.AbilityClass, 
            1, 
            (int32)Mapping.InputID,
            WeaponData
        );
        
        FGameplayAbilitySpecHandle Handle = 
            OwnerCharacter->AbilitySystemComponent->GiveAbility(AbilitySpec);
        
        OwnerCharacter->GrantedAbilityHandles.Add(Mapping.AbilityClass, Handle);

        // ✅ ADD THIS DEBUG
        UE_LOG(LogTemp, Warning, TEXT("   ✅ Granted: %s (InputID: %d, Handle Valid: %s)"),
            *Mapping.AbilityClass->GetName(),
            (int32)Mapping.InputID,
            Handle.IsValid() ? TEXT("YES") : TEXT("NO"));
    }
}

void UEquipmentComponent::RemoveWeaponAbilities(UWeaponDataAsset* WeaponData)
{
    if (!OwnerCharacter || !WeaponData) return;

    for (const FAbilityInputMapping& Mapping : WeaponData->GrantedAbilities)
    {
        OwnerCharacter->RemoveAbility(Mapping.AbilityClass);
    }
}

void UEquipmentComponent::ApplyWeaponAnimInstance(UWeaponDataAsset* WeaponData)
{
    if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return;
    
    if (!PreviousAnimInstance)
    {
        PreviousAnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance()->GetClass();
        UE_LOG(LogTemp, Log, TEXT("Stored previous anim instance: %s"), *GetNameSafe(PreviousAnimInstance));
    }
    
    if (WeaponData && WeaponData->Animations.WeaponAnimInstanceClass)
    {
        if (OwnerCharacter->GetMesh()->GetAnimInstance()->GetClass() != WeaponData->Animations.WeaponAnimInstanceClass)
        {
            OwnerCharacter->GetMesh()->SetAnimInstanceClass(WeaponData->Animations.WeaponAnimInstanceClass);
            UE_LOG(LogTemp, Log, TEXT("Applied weapon anim: %s"), *GetNameSafe(WeaponData->Animations.WeaponAnimInstanceClass));
        }
    }
}

// remove this function                                     ----------------------------------------------------

void UEquipmentComponent::RestoreDefaultAnimInstance()
{
    if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return;
    
    if (PreviousAnimInstance)
    {
        if (OwnerCharacter->GetMesh()->GetAnimInstance()->GetClass() != PreviousAnimInstance)
        {
            OwnerCharacter->GetMesh()->SetAnimInstanceClass(PreviousAnimInstance);
            UE_LOG(LogTemp, Log, TEXT("Restored default anim instance: %s"), *GetNameSafe(PreviousAnimInstance));
        }
        
        PreviousAnimInstance = nullptr;
    }
    else if (DefaultAnimInstanceClass)
    {
        if (OwnerCharacter->GetMesh()->GetAnimInstance()->GetClass() != DefaultAnimInstanceClass)
        {
            OwnerCharacter->GetMesh()->SetAnimInstanceClass(DefaultAnimInstanceClass);
            UE_LOG(LogTemp, Log, TEXT("Restored to default anim instance: %s"), *GetNameSafe(DefaultAnimInstanceClass));
        }
    }
}


//
void UEquipmentComponent::AttachWeaponToSocket(ABaseWeapon* Weapon, FName SocketName)
{
    if (!Weapon || !OwnerCharacter || !OwnerCharacter->GetMesh()) 
    {
        UE_LOG(LogTemp, Error, TEXT("AttachWeaponToSocket: Missing references"));
        return;
    }
    
    if (SocketName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("AttachWeaponToSocket: SocketName is None! Using default"));
        SocketName = WeaponSocketName;
    }
    
    if (Weapon->WeaponMesh)
    {
        Weapon->WeaponMesh->SetSimulatePhysics(false);
        Weapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Attaching weapon to socket: %s"), *SocketName.ToString());
    
    Weapon->AttachToComponent(OwnerCharacter->GetMesh(), 
        FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
        SocketName);
}

EWeaponSlot UEquipmentComponent::FindAvailableSlot() const
{
    if (!PrimarySlot.bIsOccupied)
        return EWeaponSlot::Primary;
    
    if (!SecondarySlot.bIsOccupied)
        return EWeaponSlot::Secondary;
    
    return EWeaponSlot::None;
}

EWeaponSlot UEquipmentComponent::GetSlotForWeapon(ABaseWeapon* Weapon) const
{
    if (PrimarySlot.Weapon == Weapon)
        return EWeaponSlot::Primary;
    
    if (SecondarySlot.Weapon == Weapon)
        return EWeaponSlot::Secondary;
    
    return EWeaponSlot::None;
}

 

void UEquipmentComponent::ApplyWeaponAnimLayers(UWeaponDataAsset* WeaponData)
{
 
    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
 

    // DEBUG: Log which weapon data we're applying
    FString WeaponName = WeaponData ? WeaponData->WeaponName.ToString() : TEXT("NULL");
    UE_LOG(LogTemp, Log, TEXT("🔧 ApplyWeaponAnimLayers called for weapon: %s"), *WeaponName);

    // Determine which AnimInstanceClass we should apply
    TSubclassOf<UAnimInstance> DesiredAnimClass = nullptr;

    if (WeaponData && WeaponData->Animations.WeaponAnimInstanceClass)
    {
        DesiredAnimClass = WeaponData->Animations.WeaponAnimInstanceClass;
        UE_LOG(LogTemp, Log, TEXT("  Using weapon anim class: %s"), *GetNameSafe(DesiredAnimClass));
    }
    else if (DefaultUnarmedData && DefaultUnarmedData->Animations.WeaponAnimInstanceClass)
    {
        DesiredAnimClass = DefaultUnarmedData->Animations.WeaponAnimInstanceClass;
        UE_LOG(LogTemp, Log, TEXT("  Using unarmed anim class: %s"), *GetNameSafe(DesiredAnimClass));
    }
    else
    {
        DesiredAnimClass = DefaultAnimInstanceClass;
        UE_LOG(LogTemp, Log, TEXT("  Using default anim class: %s"), *GetNameSafe(DesiredAnimClass));
    }
 

    // Get current anim class
    TSubclassOf<UAnimInstance> CurrentAnimClass = Mesh->GetAnimInstance() ? 
        Mesh->GetAnimInstance()->GetClass() : nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("  Current anim class: %s"), *GetNameSafe(CurrentAnimClass));
    UE_LOG(LogTemp, Log, TEXT("  Desired anim class: %s"), *GetNameSafe(DesiredAnimClass));

    // Do not swap if already using the correct AnimClass
    if (CurrentAnimClass == DesiredAnimClass)
    {
        UE_LOG(LogTemp, Log, TEXT("  Already using correct anim class → no change"));
        return;
    }

    // ACTUAL AnimBP switch
    UE_LOG(LogTemp, Warning, TEXT("🔁 Swapping AnimInstance to: %s"), *GetNameSafe(DesiredAnimClass));

    Mesh->SetAnimInstanceClass(DesiredAnimClass);
}

EWeaponType UEquipmentComponent::GetCurrentWeaponType() const
{
    // Check if any weapon is equipped
    ABaseWeapon* CurrentWeapon = GetCurrentWeapon();
    if (CurrentWeapon && CurrentWeapon->GetWeaponData())
    {
        return CurrentWeapon->GetWeaponData()->WeaponType;
    }
    
    // No weapon equipped = Unarmed
    return EWeaponType::Unarmed;
}

void UEquipmentComponent::StopAllWeaponTraces()
{
    ABaseWeapon* CurrentWeapon = GetCurrentWeapon();
    if (!CurrentWeapon) return;

    UE_LOG(LogTemp, Warning, TEXT("🛑 Stopping all weapon traces for: %s"),
        *CurrentWeapon->GetName());

    // Server-only trace timers
    if (CurrentWeapon->HasAuthority())
    {
        CurrentWeapon->bIsTracing = false;

        if (CurrentWeapon->GetWorld())
        {
            CurrentWeapon->GetWorld()->GetTimerManager()
                .ClearTimer(CurrentWeapon->TraceTimerHandle);
        }
    }

    // Clear all hit actors from previous swings
    CurrentWeapon->ClearHitActors();
}
