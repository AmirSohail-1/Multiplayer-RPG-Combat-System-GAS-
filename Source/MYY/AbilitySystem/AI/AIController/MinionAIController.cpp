#include "MinionAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "MYY/AbilitySystem/AI//AICharacter.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

const FName AMinionAIController::BB_TargetActor     =       TEXT("TargetActor");
const FName AMinionAIController::BB_PatrolLocation  =    TEXT("PatrolLocation");
const FName AMinionAIController::BB_IsInCombat      =        TEXT("IsInCombat");
const FName AMinionAIController::BB_LastKnownLocation = TEXT("LastKnownLocation");
const FName AMinionAIController::BB_CanAttack =         TEXT("CanAttack");
const FName AMinionAIController::BB_ShouldBlock =       TEXT("ShouldBlock");
const FName AMinionAIController::BB_HomeLocation =      TEXT("HomeLocation");

AMinionAIController::AMinionAIController()
{
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	// Configure Sight
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2000.f;
	SightConfig->LoseSightRadius = 2500.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;      // ✅ Detect hostile teams
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;     // ✅ Detect neutral actors
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;  // ✅ Ignore teammates
	SightConfig->SetMaxAge(5.f);

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	// Configure Damage sense
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	// Bind perception events
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AMinionAIController::OnTargetPerceptionUpdated);

    // ✅ CRITICAL: Run on server only
    bWantsPlayerState = false;
}

void AMinionAIController::BeginPlay()
{
	Super::BeginPlay();
	
	if (!HasAuthority())
	{
		return;
	}

    if (GetPawn())
    {
        HomeLocation = GetPawn()->GetActorLocation();
        UE_LOG(LogTemp, Log, TEXT("🤖 AI Controller BeginPlay - Home: %s"), *HomeLocation.ToString());
    }
}

void AMinionAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // Only run behavior tree on server
    if (!HasAuthority())
    {
        return;
    }

    AAICharacter* AIChar = Cast<AAICharacter>(InPawn);

    if (AAICharacter* PC = Cast<AAICharacter>(InPawn))
    {
        if (PC->EquipmentComponent)
        {
            PC->EquipmentComponent->EquipDefaultWeapon();
        }
    }
    
    if (AIChar && AIChar->BehaviorTree)
    {
        // Initialize blackboard
        if (BlackboardComponent && AIChar->BehaviorTree->BlackboardAsset)
        {
            BlackboardComponent->InitializeBlackboard(*AIChar->BehaviorTree->BlackboardAsset);
            
            // Set home location in blackboard
            BlackboardComponent->SetValueAsVector(BB_HomeLocation, HomeLocation);
            
            UE_LOG(LogTemp, Log, TEXT("AI Controller: Blackboard initialized for %s"), *InPawn->GetName());
        }

        // Run behavior tree
        if (BehaviorTreeComponent)
        {
            BehaviorTreeComponent->StartTree(*AIChar->BehaviorTree);
            UE_LOG(LogTemp, Log, TEXT("AI Controller: Behavior tree started for %s"), *InPawn->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI Controller: No behavior tree assigned to %s"),  InPawn ? *InPawn->GetName() : TEXT("NULL"));
    }
}

void AMinionAIController::OnUnPossess()
{
    if (HasAuthority() && BehaviorTreeComponent)
    {
        BehaviorTreeComponent->StopTree();
    }

    Super::OnUnPossess();
    
}

void AMinionAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{

    // FIX: perception can fire before possession or after destroy
    if (!IsValid(Actor))
        return;

    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
        return;

    if (!Blackboard)
        return;

    //----------------------------------------------------------
    
    if (!HasAuthority() || !Actor || !BlackboardComponent)
    {
        return;
    }

    // ✅ FIRST: Check if it's a valid character target
    AMYYCharacterBase* TargetChar = Cast<AMYYCharacterBase>(Actor);
    if (!TargetChar)
    {
        return; // Ignore non-characters
    }

    // ✅ SECOND: Check team affiliation
    AMYYCharacterBase* AIPawn = Cast<AMYYCharacterBase>(GetPawn());
    if (AIPawn)
    {
        ETeamAttitude::Type Attitude = AIPawn->GetTeamAttitudeTowards(*Actor);
        if (Attitude != ETeamAttitude::Hostile)
        {
            return; // Ignore friendly/neutral
        }
    }

    // ✅ NOW: Process hostile targets
    if (Stimulus.WasSuccessfullySensed())
    {
        UE_LOG(LogTemp, Log, TEXT("AI %s detected HOSTILE target: %s"), 
            *GetPawn()->GetName(), *Actor->GetName());

        SetTargetActor(Actor);
        BlackboardComponent->SetValueAsVector(BB_LastKnownLocation, Actor->GetActorLocation() );
        BlackboardComponent->SetValueAsBool(BB_IsInCombat, true);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("AI %s lost sight of: %s"), *GetPawn()->GetName(), *Actor->GetName());

        BlackboardComponent->SetValueAsVector(BB_LastKnownLocation, Actor->GetActorLocation());
        
        FTimerHandle ClearTargetTimer;
        GetWorld()->GetTimerManager().SetTimer(ClearTargetTimer, [this]()
        {
            if (!IsValid(this) || !BlackboardComponent || !AIPerceptionComponent)
            {
                return;
            }
    
            TArray<AActor*> PerceivedActors;
            AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
    
            if (PerceivedActors.Num() == 0)
            {
                ClearTarget();
            }
        }, 5.0f, false);
    }
}

void AMinionAIController::OnDamageTaken(AActor* DamagedActor, float Damage, 
    const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (!HasAuthority() || !InstigatedBy || !InstigatedBy->GetPawn())
    {
        return;
    }

    // React to damage by targeting the attacker
    if (!GetTargetActor())
    {
        SetTargetActor(InstigatedBy->GetPawn());
        BlackboardComponent->SetValueAsBool(BB_IsInCombat, true);
        
        UE_LOG(LogTemp, Warning, TEXT("AI %s taking damage from %s - entering combat!"), 
            *GetPawn()->GetName(), *InstigatedBy->GetPawn()->GetName());
    }
}

void AMinionAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UAIPerceptionComponent* Perception = FindComponentByClass<UAIPerceptionComponent>())
    {
        // 🔧 FIX: Prevent perception callbacks after destruction
        Perception->OnTargetPerceptionUpdated.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}


void AMinionAIController::SetTargetActor(AActor* NewTarget)
{
    if (!BlackboardComponent)
    {
        return;
    }

    BlackboardComponent->SetValueAsObject(BB_TargetActor, NewTarget);

    // Update character's target for GAS targeting
    AMYYCharacterBase* AIChar = Cast<AMYYCharacterBase>(GetPawn());
    if (AIChar)
    {
        AIChar->SetCurrentTarget(NewTarget);
    }
}

AActor* AMinionAIController::GetTargetActor() const
{
    if (!BlackboardComponent)
    {
        return nullptr;
    }
 
    return Cast<AActor>(BlackboardComponent->GetValueAsObject(BB_TargetActor));
}

void AMinionAIController::ClearTarget()
{
    if (!BlackboardComponent)
    {
        return;
    }

    BlackboardComponent->ClearValue(BB_TargetActor);
    BlackboardComponent->SetValueAsBool(BB_IsInCombat, false);

    AMYYCharacterBase* AIChar = Cast<AMYYCharacterBase>(GetPawn());
    if (AIChar)
    {
        AIChar->SetCurrentTarget(nullptr);
    }

    UE_LOG(LogTemp, Log, TEXT("AI %s cleared target"), *GetPawn()->GetName());
}


 