// ArrowPickup.cpp
#include "ArrowPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"

AArrowPickup::AArrowPickup()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
    RootComponent = ArrowMesh;
    ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrowMesh->SetIsReplicated(true);

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(ArrowMesh);
    InteractionSphere->SetSphereRadius(150.f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionSphere->SetGenerateOverlapEvents(true);
}

void AArrowPickup::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && InteractionSphere)
    {
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(
            this, &AArrowPickup::OnInteractionSphereBeginOverlap);
        InteractionSphere->OnComponentEndOverlap.AddDynamic(
            this, &AArrowPickup::OnInteractionSphereEndOverlap);
        
        UE_LOG(LogTemp, Log, TEXT("[ArrowPickup] %s: Overlap events bound"), *GetName());
    }
}

void AArrowPickup::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(OtherActor);
    if (Character)
    {
        UE_LOG(LogTemp, Log, TEXT("Arrow pickup in range: %s"), *GetName());
    }
}

void AArrowPickup::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority()) return;
    
    UE_LOG(LogTemp, Log, TEXT("Arrow pickup out of range"));
}

void AArrowPickup::OnInteract_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] %s: OnInteract called by %s"), 
        *GetName(), *GetNameSafe(Interactor));

    // ✅ Server-only logic
    if (!HasAuthority()) 
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] Client calling OnInteract - ignored"));
        return;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(Interactor);
    if (!Character || !Character->EquipmentComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] Invalid character or no EquipmentComponent"));
        return;
    }

    ABaseWeapon* CurrentWeapon = Character->EquipmentComponent->GetCurrentWeapon();
    if (!CurrentWeapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] No weapon equipped"));
        return;
    }

    URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(CurrentWeapon->WeaponData);
    if (!RangedData)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] Equipped weapon is not ranged"));
        return;
    }

    if (CurrentWeapon->CurrentAmmo < RangedData->AmmoConfig.MaxAmmo)
    {
        int32 AmmoToAdd = FMath::Min(ArrowCount, RangedData->AmmoConfig.MaxAmmo - CurrentWeapon->CurrentAmmo);
        CurrentWeapon->CurrentAmmo += AmmoToAdd;

        UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] ✅ Added %d arrows. Total: %d/%d"),
            AmmoToAdd, CurrentWeapon->CurrentAmmo, RangedData->AmmoConfig.MaxAmmo);

        Destroy();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArrowPickup] Ammo already full (%d/%d)"),
            CurrentWeapon->CurrentAmmo, RangedData->AmmoConfig.MaxAmmo);
    }
}

bool AArrowPickup::CanInteract_Implementation(AActor* Interactor) const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(Interactor);
    if (!Character || !Character->EquipmentComponent) return false;

    ABaseWeapon* CurrentWeapon = Character->EquipmentComponent->GetCurrentWeapon();
    if (!CurrentWeapon) return false;

    URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(CurrentWeapon->WeaponData);
    if (!RangedData) return false;

    bool bCanPickup = CurrentWeapon->CurrentAmmo < RangedData->AmmoConfig.MaxAmmo;
    
    UE_LOG(LogTemp, Log, TEXT("[ArrowPickup] CanInteract: %s (Ammo: %d/%d)"),
        bCanPickup ? TEXT("YES") : TEXT("NO"),
        CurrentWeapon->CurrentAmmo, RangedData->AmmoConfig.MaxAmmo);

    return bCanPickup;
}

FText AArrowPickup::GetInteractionPrompt_Implementation() const
{
    return FText::Format(FText::FromString("Pick up {0} Arrows"), 
        FText::AsNumber(ArrowCount));
}