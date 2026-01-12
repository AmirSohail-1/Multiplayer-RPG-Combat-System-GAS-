// PlayerVsAIGameMode.cpp - FIXED VERSION
#include "PlayerVsAIGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "MYY/AbilitySystem/AI/AICharacter.h"
#include "MYY/AbilitySystem/AI/AIController/MinionAIController.h"
#include "MYY/AbilitySystem/Characters/PlayerCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "MYY/PlayerController/MYYPlayerController.h"

APlayerVsAIGameMode::APlayerVsAIGameMode()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APlayerVsAIGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Auto-start match after 3 seconds
    FTimerHandle StartTimer;
    GetWorld()->GetTimerManager().SetTimer(StartTimer, this, &APlayerVsAIGameMode::StartMatch, 3.0f, false);

    UE_LOG(LogTemp, Warning, TEXT("🔥 GameMode BeginPlay - Default Pawn: %s"), *GetNameSafe(DefaultPawnClass));
    UE_LOG(LogTemp, Warning, TEXT("🔥 AI Class: %s"), *GetNameSafe(AICharacterClass));
}

void APlayerVsAIGameMode::StartMatch()
{
    UE_LOG(LogTemp, Warning, TEXT("🎮 Match Started! First to %d kills wins!"), KillsToWin);
    
    CurrentMatchState = EMatchState::InProgress;
    PlayerKills = 0;
    PlayerDeaths = 0;

    // ✅ NEW: Spawn player first
    SpawnPlayer();
    
    // Spawn AI enemies
    SpawnAIEnemies();
}

void APlayerVsAIGameMode::SpawnPlayer()
{
    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || PC->GetPawn())
    {
        return; // Already has pawn
    }

    // Find spawn point with "PlayerSpawn" tag
    TArray<AActor*> PlayerSpawns;
    UGameplayStatics::GetAllActorsOfClassWithTag(
        GetWorld(), 
        APlayerStart::StaticClass(), 
        FName("PlayerSpawn"), 
        PlayerSpawns
    );

    if (PlayerSpawns.Num() == 0) return;

    // Get transform from spawn point (like BP's GetActorTransform)
    FTransform SpawnTransform = PlayerSpawns[0]->GetActorTransform();

    // Spawn parameters
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = 
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawn player pawn
    APawn* PlayerPawn = GetWorld()->SpawnActor<APawn>(
        DefaultPawnClass,
        SpawnTransform,
        SpawnParams
    );

    if (PlayerPawn)
    {
        // Possess immediately (or with small delay for replication)
        PC->Possess(PlayerPawn);
        
        UE_LOG(LogTemp, Log, TEXT("✅ Spawned and possessed player"));
    }
}


// // ✅ ADD THIS NEW FUNCTION
// void APlayerVsAIGameMode::SpawnPlayer()
// {
//     // Get the first player controller
//     APlayerController* PC = GetWorld()->GetFirstPlayerController();
//     if (!PC)
//     {
//         UE_LOG(LogTemp, Error, TEXT("❌ No PlayerController found!"));
//         return;
//     }
//
//     // Check if player already has a pawn
//     if (PC->GetPawn())
//     {
//         UE_LOG(LogTemp, Warning, TEXT("✅ Player already has a pawn: %s"), *PC->GetPawn()->GetName());
//         return;
//     }
//
//     // Find PlayerStart with "PlayerSpawn" tag
//     TArray<AActor*> PlayerSpawns;
//     UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName("PlayerSpawn"), PlayerSpawns);
//
//     if (PlayerSpawns.Num() == 0)
//     {
//         UE_LOG(LogTemp, Error, TEXT("❌ No PlayerStart with 'PlayerSpawn' tag found!"));
//         // Fallback: try any PlayerStart
//         UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerSpawns);
//         
//         if (PlayerSpawns.Num() == 0)
//         {
//             UE_LOG(LogTemp, Error, TEXT("❌ No PlayerStart actors in level at all!"));
//             return;
//         }
//     }
//
//     // ✅ Get spawn point (exact location, no randomization)
//     AActor* SpawnPoint = PlayerSpawns[0]; // Use first spawn point (or randomize if you want)
//     FVector SpawnLocation = SpawnPoint->GetActorLocation();
//     FRotator SpawnRotation = SpawnPoint->GetActorRotation();
//
//     UE_LOG(LogTemp, Warning, TEXT("🎯 Spawning player at: %s"), *SpawnLocation.ToString());
//
//     // ✅ Spawn the player pawn
//     FActorSpawnParameters SpawnParams;
//     SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
//
//     if (!DefaultPawnClass)
//     {
//         UE_LOG(LogTemp, Error, TEXT("❌ DefaultPawnClass is NULL in GameMode!"));
//         return;
//     }
//
//     APawn* PlayerPawn = GetWorld()->SpawnActor<APawn>(
//         DefaultPawnClass,
//         SpawnLocation,
//         SpawnRotation,
//         SpawnParams
//     );
//
//     if (PlayerPawn)
//     {
//         // ✅ Delay possession to ensure replication
//         FTimerHandle PossessTimer;
//         GetWorld()->GetTimerManager().SetTimer(PossessTimer, [this, PC, PlayerPawn]()
//         {
//             if (IsValid(PC) && IsValid(PlayerPawn))
//             {
//                 PC->Possess(PlayerPawn);
//                 UE_LOG(LogTemp, Log, TEXT("✅ Player controller possessed player pawn"));
//             }
//         }, 0.2f, false);
//
//         UE_LOG(LogTemp, Warning, TEXT("✅ Spawned player character: %s"), *PlayerPawn->GetName());
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("❌ Failed to spawn player character!"));
//     }
// }


void APlayerVsAIGameMode::SpawnAIEnemies()
{
    if (!AICharacterClass)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No AICharacterClass set in GameMode!"));
        return;
    }

    // ✅ Get AI spawn points only
    TArray<AActor*> AISpawns;
    UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName("AISpawn"), AISpawns);

    if (AISpawns.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No PlayerStart actors with 'AISpawn' tag!"));
        return;
    }

    for (int32 i = 0; i < NumberOfAIEnemies; i++)
    {
        // ✅ Select random spawn point (but spawn exactly at its location)
        AActor* SpawnPoint = AISpawns[FMath::RandRange(0, AISpawns.Num() - 1)];

        // ✅ FIXED: Spawn at EXACT PlayerStart location (no random offset)
        FVector SpawnLocation = SpawnPoint->GetActorLocation();
        FRotator SpawnRotation = SpawnPoint->GetActorRotation();

        UE_LOG(LogTemp, Log, TEXT("🎯 Spawning AI %d at exact location: %s"), i + 1, *SpawnLocation.ToString());

        // ✅ CRITICAL: Proper spawn parameters
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        SpawnParams.bDeferConstruction = false;

        AAICharacter* AIChar = GetWorld()->SpawnActor<AAICharacter>(
            AICharacterClass, 
            SpawnLocation, 
            SpawnRotation, 
            SpawnParams
        );

        if (AIChar)
        {
            // 🔍 DEBUG: Log component status
            UE_LOG(LogTemp, Warning, TEXT("🔍 AI %d Component Check:"), i + 1);
            UE_LOG(LogTemp, Warning, TEXT("   - AbilitySystemComponent: %s"), 
                AIChar->AbilitySystemComponent ? TEXT("✅ EXISTS") : TEXT("❌ NULL"));
            UE_LOG(LogTemp, Warning, TEXT("   - AttributeSet: %s"), 
                AIChar->AttributeSet ? TEXT("✅ EXISTS") : TEXT("❌ NULL"));

            // 🛡️ MODIFIED: Only check ASC (AttributeSet might initialize later)
            if (!AIChar->AbilitySystemComponent)
            {
                UE_LOG(LogTemp, Error, TEXT("❌ AI %d has no AbilitySystemComponent!"), i + 1);
                AIChar->Destroy();
                continue;
            }

            AIChar->TeamID = 1; // Enemy team
            
            // 🛡️ CRASH FIX: Delay controller spawn to ensure AI initialization completes
            FTimerHandle ControllerSpawnTimer;
            GetWorld()->GetTimerManager().SetTimer(ControllerSpawnTimer, [this, AIChar, SpawnLocation, SpawnRotation, i]()
            {
                if (!IsValid(AIChar))
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ AI %d was destroyed before controller could possess"), i + 1);
                    return;
                }

                // ✅ CRITICAL: Spawn AI Controller AFTER character spawns
                FActorSpawnParameters ControllerParams;
                ControllerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                
                AMinionAIController* AIController = GetWorld()->SpawnActor<AMinionAIController>(
                    AMinionAIController::StaticClass(),
                    SpawnLocation,
                    SpawnRotation,
                    ControllerParams
                );

                if (AIController)
                {
                    AIController->Possess(AIChar);
                    UE_LOG(LogTemp, Log, TEXT("✅ AI Controller possessed AI character %d"), i + 1);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ Failed to spawn controller for AI %d"), i + 1);
                }
                
            }, 0.2f, false);

            SpawnedAI.Add(AIChar);

            // Bind death event
            AIChar->OnCharacterDied.AddDynamic(this, &APlayerVsAIGameMode::OnCharacterKilled);

            UE_LOG(LogTemp, Log, TEXT("✅ Spawned AI %d at %s"), i + 1, *SpawnLocation.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to spawn AI %d"), i + 1);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("🤖 Spawned %d AI enemies"), SpawnedAI.Num());
}


void APlayerVsAIGameMode::OnCharacterKilled(AMYYCharacterBase* Victim, AActor* Killer)
{
    if (CurrentMatchState != EMatchState::InProgress) return;

    UE_LOG(LogTemp, Warning, TEXT("💀 %s killed %s"), 
        *GetNameSafe(Killer), *GetNameSafe(Victim));

    APlayerCharacter* PlayerKiller = Cast<APlayerCharacter>(Killer);
    AAICharacter* AIVictim = Cast<AAICharacter>(Victim);
    
    APlayerCharacter* PlayerVictim = Cast<APlayerCharacter>(Victim);
    AAICharacter* AIKiller = Cast<AAICharacter>(Killer);

    // ========== PLAYER KILLED AI ==========
    if (PlayerKiller && AIVictim)
    {
        PlayerKills++;
        UE_LOG(LogTemp, Warning, TEXT("🎯 Player Kills: %d / %d"), PlayerKills, KillsToWin);

        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (AMYYPlayerController* PC = Cast<AMYYPlayerController>(It->Get()))
            {
                PC->Client_UpdateKillCount(PlayerKills, KillsToWin);
            }
        }
    }
    // ========== AI KILLED PLAYER ==========
    else if (AIKiller && PlayerVictim)
    {
        PlayerDeaths++;
        UE_LOG(LogTemp, Warning, TEXT("☠️ Player Deaths: %d"), PlayerDeaths);

        if (AMYYPlayerController* PC = Cast<AMYYPlayerController>(PlayerVictim->GetController()))
        {
            PC->Client_UpdateDeathCount(PlayerDeaths);
            PC->Client_ShowRespawnTimer(RespawnDelay);
        }

        // Respawn player after delay
        AController* PlayerController = PlayerVictim->GetController();
        if (PlayerController)
        {
            FTimerHandle RespawnTimer;
            GetWorld()->GetTimerManager().SetTimer(RespawnTimer, [this, PlayerController]()
            {
                RespawnCharacter(PlayerController);
            }, RespawnDelay, false);
        }
    }

    // Respawn AI after delay
    if (AIVictim)
    {
        AController* AIController = AIVictim->GetController();
        if (AIController)
        {
            FTimerHandle RespawnTimer;
            GetWorld()->GetTimerManager().SetTimer(RespawnTimer, [this, AIController]()
            {
                RespawnCharacter(AIController);
            }, RespawnDelay, false);
        }
    }

    CheckWinCondition();
}

void APlayerVsAIGameMode::RespawnCharacter(AController* Controller)
{
    if (!Controller) return;

    AActor* SpawnPoint = nullptr;
    
    if (Controller->IsPlayerController())
    {
        TArray<AActor*> PlayerSpawns;
        UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName("PlayerSpawn"), PlayerSpawns);
        
        if (PlayerSpawns.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ No PlayerStart with 'PlayerSpawn' tag!"));
            return;
        }
        
        SpawnPoint = PlayerSpawns[FMath::RandRange(0, PlayerSpawns.Num() - 1)];
    }
    else
    {
        TArray<AActor*> AISpawns;
        UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName("AISpawn"), AISpawns);
        
        if (AISpawns.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ No PlayerStart with 'AISpawn' tag!"));
            return;
        }
        
        SpawnPoint = AISpawns[FMath::RandRange(0, AISpawns.Num() - 1)];
    }

    // ✅ CRITICAL: Proper spawn with initialization delay
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.bDeferConstruction = false;

    APawn* NewPawn = nullptr;
    
    if (Controller->IsPlayerController())
    {
        NewPawn = GetWorld()->SpawnActor<APlayerCharacter>(
            DefaultPawnClass, 
            SpawnPoint->GetActorLocation(), 
            SpawnPoint->GetActorRotation(),
            SpawnParams
        );

        if (NewPawn)
        {
            // ✅ Delay possession to ensure replication
            FTimerHandle PossessTimer;
            GetWorld()->GetTimerManager().SetTimer(PossessTimer, [this, Controller, NewPawn]()
            {
                Controller->Possess(NewPawn);
                UE_LOG(LogTemp, Log, TEXT("✅ Player possessed after delay"));
            }, 0.1f, false);
        }
    }
    else
    {
        NewPawn = GetWorld()->SpawnActor<AAICharacter>(
            AICharacterClass, 
            SpawnPoint->GetActorLocation(), 
            SpawnPoint->GetActorRotation(),
            SpawnParams
        );

        if (AAICharacter* AIChar = Cast<AAICharacter>(NewPawn))
        {
            AIChar->TeamID = 1;
            AIChar->OnCharacterDied.AddDynamic(this, &APlayerVsAIGameMode::OnCharacterKilled);

            // ✅ Delay possession for AI to ensure BT initialization
            FTimerHandle PossessTimer;
            GetWorld()->GetTimerManager().SetTimer(PossessTimer, [this, Controller, NewPawn]()
            {
                Controller->Possess(NewPawn);
                UE_LOG(LogTemp, Log, TEXT("✅ AI possessed after delay"));
            }, 0.1f, false);
        }
    }

    if (NewPawn)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ Spawned %s"), *GetNameSafe(NewPawn));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to spawn pawn for respawn"));
    }
}

void APlayerVsAIGameMode::CheckWinCondition()
{
    if (PlayerKills >= KillsToWin)
    {
        EndMatch();
    }
}

void APlayerVsAIGameMode::EndMatch()
{
    EndMatchWithResult(true);
}

void APlayerVsAIGameMode::EndMatchWithResult(bool bPlayerWon)
{
    if (CurrentMatchState == EMatchState::MatchEnded) return;

    CurrentMatchState = EMatchState::MatchEnded;

    if (bPlayerWon)
    {
        UE_LOG(LogTemp, Warning, TEXT("🏆 PLAYER WINS! (%d kills)"), PlayerKills);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("💀 PLAYER LOST!"));
    }
}