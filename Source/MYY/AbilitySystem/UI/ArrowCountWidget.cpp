// ArrowCountWidget.cpp
#include "ArrowCountWidget.h"
#include "Components/TextBlock.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"

void UArrowCountWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Log, TEXT("[ArrowCountWidget] Widget constructed"));
    
    OwningCharacter = Cast<AMYYCharacterBase>(GetOwningPlayerPawn());
    
    if (OwningCharacter && OwningCharacter->EquipmentComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("[ArrowCountWidget] Binding to EquipmentComponent"));
        
        // Bind to weapon changed event
        OwningCharacter->EquipmentComponent->OnWeaponChanged.AddDynamic(
            this, &UArrowCountWidget::OnWeaponChanged);
        
        // Initial update
        ABaseWeapon* CurrentWeapon = OwningCharacter->EquipmentComponent->GetCurrentWeapon();
        if (CurrentWeapon)
        {
            UE_LOG(LogTemp, Log, TEXT("[ArrowCountWidget] Initial weapon: %s"), 
                *CurrentWeapon->GetName());
            BindToWeapon(CurrentWeapon);
        }
        
        UpdateArrowCount();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[ArrowCountWidget] ❌ Failed to get character or equipment component"));
    }
}


void UArrowCountWidget::NativeDestruct()
{
    if (OwningCharacter && OwningCharacter->EquipmentComponent)
    {
        OwningCharacter->EquipmentComponent->OnWeaponChanged.RemoveDynamic(
            this, &UArrowCountWidget::OnWeaponChanged);
    }
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
    
    Super::NativeDestruct();
}

void UArrowCountWidget::OnWeaponChanged(ABaseWeapon* NewWeapon, ABaseWeapon* OldWeapon)
{
    if (OldWeapon)
    {
        UnbindFromWeapon(OldWeapon);
    }
    
    if (NewWeapon)
    {
        BindToWeapon(NewWeapon);
    }
    
    UpdateArrowCount();
}

void UArrowCountWidget::BindToWeapon(ABaseWeapon* Weapon)
{
    if (!Weapon) return;
    
    // Start timer to update ammo count (since OnRep_CurrentAmmo might not always trigger)
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            UpdateTimerHandle,
            this,
            &UArrowCountWidget::UpdateArrowCount,
            0.1f,
            true
        );
    }
}

void UArrowCountWidget::UnbindFromWeapon(ABaseWeapon* Weapon)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
}

void UArrowCountWidget::UpdateArrowCount()
{
    if (!ArrowCountText || !OwningCharacter || !OwningCharacter->EquipmentComponent)
    {
        return;
    }

    ABaseWeapon* CurrentWeapon = OwningCharacter->EquipmentComponent->GetCurrentWeapon();
    
    if (!CurrentWeapon)
    {
        ArrowCountText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    URangedWeaponDataAsset* RangedData = Cast<URangedWeaponDataAsset>(CurrentWeapon->WeaponData);
    
    if (!RangedData)
    {
        ArrowCountText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    ArrowCountText->SetVisibility(ESlateVisibility::Visible);
    
    // UE_LOG(LogTemp, Log, TEXT("[ArrowCountWidget] Current Ammo: %d / %d"), CurrentWeapon->CurrentAmmo, RangedData->AmmoConfig.MaxAmmo);
    
    if (RangedData->AmmoConfig.bInfiniteAmmo)
    {
        ArrowCountText->SetText(FText::FromString("∞"));
    }
    else
    {
        ArrowCountText->SetText(FText::Format(
            FText::FromString("{0} / {1}"),
            FText::AsNumber(CurrentWeapon->CurrentAmmo),
            FText::AsNumber(RangedData->AmmoConfig.MaxAmmo)
        ));
    }
}