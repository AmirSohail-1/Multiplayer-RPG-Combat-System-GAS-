#include "MatchHUDWidget.h"
#include "TimerManager.h"

#include "MatchHUDWidget.h"


void UMatchHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();


    if (KillCountText)
    {
        KillCountText->SetText(FText::FromString("Kills: 0 / 10"));
    }

    if (DeathCountText)
    {
        DeathCountText->SetText(FText::FromString("Deaths: 0"));
    }

    if (RespawnTimerText)
    {
        RespawnTimerText->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UMatchHUDWidget::UpdateKillCount(int32 Kills, int32 MaxKills)
{
    if (!KillCountText) return;

    FString KillText = FString::Printf(TEXT("Kills: %d / %d"), Kills, MaxKills);
    KillCountText->SetText(FText::FromString(KillText));

    UE_LOG(LogTemp, Log, TEXT("UI Updated: %s"), *KillText);
}

void UMatchHUDWidget::SetPlayerKills(int32 NewKills)
{
    PlayerKills = NewKills;
}

void UMatchHUDWidget::SetAIKills(int32 NewKills)
{
    AIKills = NewKills;
}


void UMatchHUDWidget::ShowResult(EMatchWinner Winner)
{
    BP_OnShowResult(Winner);
}


void UMatchHUDWidget::UpdateDeathCount(int32 Deaths)
{
    if (!DeathCountText) return;

    FString DeathText = FString::Printf(TEXT("Deaths: %d"), Deaths);
    DeathCountText->SetText(FText::FromString(DeathText));
}

void UMatchHUDWidget::ShowRespawnTimer(float TimeRemaining)
{
    if (!RespawnTimerText) return;

    RemainingRespawnTime = TimeRemaining;
    RespawnTimerText->SetVisibility(ESlateVisibility::Visible);

    // Update every 0.1 seconds for smooth countdown
    GetWorld()->GetTimerManager().SetTimer(
        RespawnTimerHandle, 
        this, 
        &UMatchHUDWidget::UpdateRespawnTimerDisplay, 
        0.1f, 
        true
    );
}

void UMatchHUDWidget::HideRespawnTimer()
{
    if (!RespawnTimerText) return;

    RespawnTimerText->SetVisibility(ESlateVisibility::Hidden);

    if (RespawnTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
    }
}

void UMatchHUDWidget::UpdateRespawnTimerDisplay()
{
    if (!RespawnTimerText) return;

    RemainingRespawnTime -= 0.1f;

    if (RemainingRespawnTime <= 0.f)
    {
        HideRespawnTimer();
        return;
    }

    FString TimerText = FString::Printf(TEXT("Respawning in %.1f..."), RemainingRespawnTime);
    RespawnTimerText->SetText(FText::FromString(TimerText));
}