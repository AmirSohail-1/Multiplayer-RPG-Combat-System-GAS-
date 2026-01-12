// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MYYPlayerController.generated.h"

class UMatchHUDWidget;

UCLASS()
class MYY_API AMYYPlayerController : public APlayerController
{
	GENERATED_BODY()
 
protected:
	virtual void BeginPlay() override;

public:

	virtual void OnPossess(APawn* InPawn) override;
 
	// Widget class reference (set in Blueprint)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMatchHUDWidget> HUDWidgetClass;

	UPROPERTY()
	UMatchHUDWidget* HUDWidget;

	// UI Update functions
	UFUNCTION(Client, Reliable, Category = "UI")
	void Client_UpdateKillCount(int32 Kills, int32 MaxKills);

	UFUNCTION(Client, Reliable, Category = "UI")
	void Client_UpdateDeathCount(int32 Deaths);

	UFUNCTION(Client, Reliable, Category = "UI")
	void Client_ShowRespawnTimer(float TimeRemaining);
};