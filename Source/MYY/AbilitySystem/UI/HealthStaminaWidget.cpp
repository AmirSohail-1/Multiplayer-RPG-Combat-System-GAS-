// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthStaminaWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void UHealthStaminaWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    OwningCharacter = Cast<AMYYCharacterBase>(GetOwningPlayerPawn());
    
    if (OwningCharacter)
    {
        // Bind to delegates
        OwningCharacter->OnHealthChanged.AddDynamic(this, &UHealthStaminaWidget::OnHealthChanged);
        OwningCharacter->OnStaminaChanged.AddDynamic(this, &UHealthStaminaWidget::OnStaminaChanged);
        
        // Initial update
        if (OwningCharacter->AttributeSet)
        {
            OnHealthChanged(
                OwningCharacter->AttributeSet->GetHealth(),
                OwningCharacter->AttributeSet->GetMaxHealth(),
                0.f
            );
            OnStaminaChanged(
                OwningCharacter->AttributeSet->GetStamina(),
                OwningCharacter->AttributeSet->GetMaxStamina(),
                0.f
            );
        }
    }
}

void UHealthStaminaWidget::NativeDestruct()
{
    if (OwningCharacter)
    {
        OwningCharacter->OnHealthChanged.RemoveDynamic(this, &UHealthStaminaWidget::OnHealthChanged);
        OwningCharacter->OnStaminaChanged.RemoveDynamic(this, &UHealthStaminaWidget::OnStaminaChanged);
    }
    
    Super::NativeDestruct();
}

void UHealthStaminaWidget::OnHealthChanged(float Health, float MaxHealth, float Delta)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(MaxHealth > 0 ? Health / MaxHealth : 0.f);
    }
    
    if (HealthText)
    {
        // Ceil when increasing, Floor when decreasing
        int32 DisplayHealth = Delta > 0 ? FMath::CeilToInt(Health) : FMath::FloorToInt(Health);
        int32 DisplayMaxHealth = FMath::CeilToInt(MaxHealth);
        
        HealthText->SetText(FText::Format(
            FText::FromString("{0}/{1}"),
            FText::AsNumber(DisplayHealth),
            FText::AsNumber(DisplayMaxHealth)
        ));
    }
}

void UHealthStaminaWidget::OnStaminaChanged(float Stamina, float MaxStamina, float Delta)
{
    if (StaminaBar)
    {
        StaminaBar->SetPercent(MaxStamina > 0 ? Stamina / MaxStamina : 0.f);
    }
    
    if (StaminaText)
    {
        // Ceil when increasing, Floor when decreasing
        int32 DisplayStamina = Delta > 0 ? FMath::CeilToInt(Stamina) : FMath::FloorToInt(Stamina);
        int32 DisplayMaxStamina = FMath::CeilToInt(MaxStamina);
        
        StaminaText->SetText(FText::Format(
            FText::FromString("{0}/{1}"),
            FText::AsNumber(DisplayStamina),
            FText::AsNumber(DisplayMaxStamina)
        ));
    }
}