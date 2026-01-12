// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "HealthStaminaWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UHealthStaminaWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UProgressBar* HealthBar;
    
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UProgressBar* StaminaBar;
    
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UTextBlock* HealthText;
    
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	class UTextBlock* StaminaText;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnHealthChanged(float Health, float MaxHealth, float Delta);
    
	UFUNCTION()
	void OnStaminaChanged(float Stamina, float MaxStamina, float Delta);

private:
	UPROPERTY()
	AMYYCharacterBase* OwningCharacter;
};
