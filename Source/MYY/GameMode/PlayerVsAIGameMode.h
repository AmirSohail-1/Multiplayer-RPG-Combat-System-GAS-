// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PlayerVsAIGameMode.generated.h"

UENUM(BlueprintType)
enum class EMatchState : uint8
{
	WaitingToStart,
	InProgress,
	MatchEnded
};

class AAICharacter;

UCLASS(Blueprintable, BlueprintType)
class MYY_API APlayerVsAIGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	APlayerVsAIGameMode();

	// ========== MATCH SETTINGS ==========
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Settings")
	int32 KillsToWin = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Settings")
	int32 NumberOfAIEnemies = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Settings")
	float RespawnDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Settings")
	TSubclassOf<AAICharacter> AICharacterClass;

	// ========== MATCH STATE ==========
    
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	EMatchState CurrentMatchState = EMatchState::WaitingToStart;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 PlayerKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 PlayerDeaths = 0;

	// ========== FUNCTIONS ==========
    
	virtual void StartMatch() override;

	UFUNCTION(BlueprintCallable, Category = "Match")
	void SpawnPlayer();

	UFUNCTION(BlueprintCallable, Category = "Match")
	void SpawnAIEnemies();

	virtual void EndMatch() override;
	
	UFUNCTION(BlueprintCallable, Category = "Match")
	void EndMatchWithResult(bool bPlayerWon);
	
	UFUNCTION(BlueprintCallable, Category = "Match")
	void OnCharacterKilled(AMYYCharacterBase* Victim, AActor* Killer);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void RespawnCharacter(AController* Controller);


protected:
	virtual void BeginPlay() override;
 

	UFUNCTION()
	void CheckWinCondition();

	UPROPERTY()
	TArray<AAICharacter*> SpawnedAI;
};

