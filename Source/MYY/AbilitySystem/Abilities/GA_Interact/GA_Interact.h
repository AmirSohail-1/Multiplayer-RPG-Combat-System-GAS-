// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Interact.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UGA_Interact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Interact();
    
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void FindAndInteract();
    
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void InteractWithActor(AActor* InteractableActor);
    
private:
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentInteractable;
};