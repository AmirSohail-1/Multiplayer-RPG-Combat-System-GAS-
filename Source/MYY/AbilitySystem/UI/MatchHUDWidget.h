#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "MatchHUDWidget.generated.h"

UENUM(BlueprintType)
enum class EMatchWinner : uint8
{
	None        UMETA(DisplayName = "None"),
	Player      UMETA(DisplayName = "Player"),
	AI          UMETA(DisplayName = "AI"),
	Draw        UMETA(DisplayName = "Draw")
};

class UMatchHUDWidget;

UCLASS()
class MYY_API UMatchHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	// ========== TEXT BLOCKS (Bind these in Blueprint) ==========
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* KillCountText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DeathCountText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RespawnTimerText;

	// ========== UPDATE FUNCTIONS ==========
    
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateKillCount(int32 Kills, int32 MaxKills);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateDeathCount(int32 Deaths);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowRespawnTimer(float TimeRemaining);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideRespawnTimer();

	// ---- SCORE SETTERS (CALLED FROM GAMEMODE BP) ----

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetPlayerKills(int32 NewKills);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetAIKills(int32 NewKills);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowResult(EMatchWinner Winner);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void BP_OnShowResult(EMatchWinner Winner);


protected:
	virtual void NativeConstruct() override;

private:
	FTimerHandle RespawnTimerHandle;
	float RemainingRespawnTime = 0.f;

	UFUNCTION()
	void UpdateRespawnTimerDisplay();


protected:

	// ---- INTERNAL VALUES ----
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 PlayerKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 AIKills = 0;
	
};
