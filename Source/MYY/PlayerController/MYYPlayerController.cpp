#include "MYYPlayerController.h"

#include "MYY/AbilitySystem/Characters/PlayerCharacter.h"
#include "MYY/AbilitySystem/UI/MatchHUDWidget.h"

void AMYYPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Only create UI for local player
	if (IsLocalController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UMatchHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("✅ HUD Widget created for local player"));
		}
	}
}

void AMYYPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (!IsLocalController())
		return;

	APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(InPawn);
	if (!PlayerChar)
		return;

	// 🔧 FIX: delay input setup by one frame
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(
		TempHandle,
		[PlayerChar]()
		{
			PlayerChar->SetupInputMappingContext();
			
		},
		0.09f,
		false
	);

	if (APlayerCharacter* PC = Cast<APlayerCharacter>(InPawn))
	{
		if (PC->EquipmentComponent)
		{
			PC->EquipmentComponent->EquipDefaultWeapon();
		}
	}

}

void AMYYPlayerController::Client_UpdateKillCount_Implementation(int32 Kills, int32 MaxKills)
{
	if (HUDWidget)
	{
		HUDWidget->UpdateKillCount(Kills, MaxKills);
	}
}

void AMYYPlayerController::Client_UpdateDeathCount_Implementation(int32 Deaths)
{
	if (HUDWidget)
	{
		HUDWidget->UpdateDeathCount(Deaths);
	}
}

void AMYYPlayerController::Client_ShowRespawnTimer_Implementation(float TimeRemaining)
{
	if (HUDWidget)
	{
		HUDWidget->ShowRespawnTimer(TimeRemaining);
	}
}