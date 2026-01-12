// Fill out your copyright notice in the Description page of Project Settings.

 
#include "AICharacter.h"
#include "AIController/MinionAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

AAICharacter::AAICharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    TeamID = 1; // Enemy team
    
    // ✅ Ensure AI controller spawns
    AIControllerClass = AMinionAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    
    // ✅ Ensure animations work
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    
    // ✅ Network setup
    bReplicates = true;
    SetReplicateMovement(true);
}

void AAICharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // ✅ Only initialize on server
    if (!HasAuthority()) return;
    
    UE_LOG(LogTemp, Log, TEXT("🤖 AI Character BeginPlay: %s"), *GetName());


    // ✅ Verify critical components
    if (!AbilitySystemComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ AI Character %s missing AbilitySystemComponent!"), *GetName());
        return;
    }

    if (!AttributeSet)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ AI Character %s missing AttributeSet!"), *GetName());
        return;
    }
    
    // ✅ Verify controller
    if (GetController())
    {
        UE_LOG(LogTemp, Log, TEXT("   ✅ Controller: %s"), *GetController()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("   ❌ NO CONTROLLER!"));
    }
    
    // ✅ Verify ability system
    if (AbilitySystemComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("   ✅ Ability System Component ready"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("   ❌ NO ABILITY SYSTEM COMPONENT!"));
    }
}

void AAICharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    UE_LOG(LogTemp, Log, TEXT("🤖 AI Character possessed by: %s"), *GetNameSafe(NewController));
    
    // ✅ Additional initialization after possession
    if (HasAuthority() && EquipmentComponent)
    {
        // Small delay to ensure everything is replicated
        FTimerHandle EquipTimer;
        GetWorld()->GetTimerManager().SetTimer(EquipTimer, [this]()
        {
            // Auto-equip weapons if available
            if (EquipmentComponent->PrimarySlot.Weapon)
            {
                EquipmentComponent->Server_SwapWeaponHandToHolster(EWeaponSlot::Primary);
                UE_LOG(LogTemp, Log, TEXT("   ✅ AI auto-equipped primary weapon"));
            }
        }, 0.5f, false);
    }
}