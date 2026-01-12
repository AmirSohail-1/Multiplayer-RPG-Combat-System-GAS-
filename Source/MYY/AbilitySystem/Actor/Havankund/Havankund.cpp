#include "Havankund.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "Ghost.h"

AHavankund::AHavankund()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>("Root");
    SetRootComponent(Root);

    TargetPoint = CreateDefaultSubobject<USceneComponent>("TargetPoint");
    TargetPoint->SetupAttachment(Root);

    ActivationSphere = CreateDefaultSubobject<USphereComponent>("ActivationSphere");
    ActivationSphere->SetupAttachment(Root);
    
    // CRITICAL: Proper setup for overlaps
    ActivationSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ActivationSphere->SetCollisionObjectType(ECC_WorldStatic); // Changed from WorldDynamic
    ActivationSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    ActivationSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    
    // CRITICAL: Must generate overlap events
    ActivationSphere->SetGenerateOverlapEvents(true);
    ActivationSphere->bMultiBodyOverlap = true; // Enable multi-body overlaps

    ActivationSphere->SetSphereRadius(20.f);
}

void AHavankund::BeginPlay()
{
    Super::BeginPlay();

    // Set radius BEFORE binding events
    ActivationSphere->SetSphereRadius(ActivationRadius);

    // Bind events
    if (bAutoActivateOnOverlap)
    {
        ActivationSphere->OnComponentBeginOverlap.AddDynamic(this, &AHavankund::OnActivationBegin);
        ActivationSphere->OnComponentEndOverlap.AddDynamic(this, &AHavankund::OnActivationEnd);
        
        UE_LOG(LogTemp, Warning, TEXT("Havankund: Overlap events bound successfully"));
    }

    // Debug: Check collision settings
    UE_LOG(LogTemp, Warning, TEXT("Havankund: CollisionEnabled = %d, GenerateOverlapEvents = %d"), 
        (int32)ActivationSphere->GetCollisionEnabled(), 
        ActivationSphere->GetGenerateOverlapEvents());

    InitializePool();
    DeactivateAll();
}

void AHavankund::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActive) return;

    // Check for inactive ghosts and respawn them
    for (FHavankundPoolItem& Item : Pool)
    {
        if (!Item.GhostActor || !Item.bActive) continue;

        // Only count if ghost was hit by projectile AND is inactive
        if (!Item.GhostActor->IsActive())
        {
            Item.bActive = false;
            
            // Only increment count if hit by projectile
            if (Item.GhostActor->WasHitByProjectile())
            {
                DestroyedCount++;
                OnDestroyedCountChanged.Broadcast(DestroyedCount);
                UE_LOG(LogTemp, Warning, TEXT("✅ Ghost destroyed by projectile! Count: %d"), DestroyedCount);
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Ghost reached target (not counted)"));
            }
            
            RespawnGhost(Item.GhostActor);
        }
    }
}

void AHavankund::InitializePool()
{
    if (!GhostActorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Havankund: GhostActorClass not set!"));
        return;
    }

    Pool.SetNum(SpawnCount);

    for (int32 i = 0; i < Pool.Num(); i++)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AGhost* Ghost = GetWorld()->SpawnActor<AGhost>(
            GhostActorClass,
            GetActorLocation(),
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (Ghost)
        {
            Ghost->Initialize(MeshType, StaticMesh, SkeletalMesh, LoopingAnimation);
            Ghost->SetTargetLocation(TargetPoint->GetComponentLocation());
            Ghost->SetMoveSpeed(MoveSpeed);
            Ghost->Deactivate();

            Pool[i].GhostActor = Ghost;
            Pool[i].bActive = false;

            UE_LOG(LogTemp, Log, TEXT("Havankund: Spawned ghost %d"), i);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Havankund: Initialized pool with %d ghosts"), Pool.Num());
}

void AHavankund::ActivateAll()
{
    bIsActive = true;

    UE_LOG(LogTemp, Warning, TEXT("Havankund: Activating all ghosts"));

    for (FHavankundPoolItem& Item : Pool)
    {
        if (Item.GhostActor)
        {
            Item.bActive = true;
            Item.GhostActor->SetTargetLocation(TargetPoint->GetComponentLocation());
            Item.GhostActor->Activate(GetRandomSpherePoint());
        }
    }
}

void AHavankund::DeactivateAll()
{
    bIsActive = false;

    UE_LOG(LogTemp, Warning, TEXT("Havankund: Deactivating all ghosts"));

    for (FHavankundPoolItem& Item : Pool)
    {
        if (Item.GhostActor)
        {
            Item.bActive = false;
            Item.GhostActor->Deactivate();
        }
    }

    // Clear all respawn timers
    for (auto& Pair : RespawnTimers)
    {
        GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
    }
    RespawnTimers.Empty();
}

void AHavankund::RespawnGhost(AGhost* Ghost)
{
    if (!Ghost || !bIsActive) return;

    // Clear existing timer if any
    if (RespawnTimers.Contains(Ghost))
    {
        GetWorld()->GetTimerManager().ClearTimer(RespawnTimers[Ghost]);
    }

    // Schedule respawn after delay
    FTimerHandle TimerHandle;
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([this, Ghost]()
    {
        if (Ghost && bIsActive)
        {
            // Find the pool item
            for (FHavankundPoolItem& Item : Pool)
            {
                if (Item.GhostActor == Ghost)
                {
                    Item.bActive = true;
                    Ghost->SetTargetLocation(TargetPoint->GetComponentLocation());
                    Ghost->Activate(GetRandomSpherePoint());
                    
                    UE_LOG(LogTemp, Log, TEXT("Havankund: Respawned ghost"));
                    break;
                }
            }
        }

        // Remove from timer map
        RespawnTimers.Remove(Ghost);
    });

    GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, RespawnDelay, false);
    RespawnTimers.Add(Ghost, TimerHandle);
}

void AHavankund::OnActivationBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Error, TEXT("========================================"));
    UE_LOG(LogTemp, Error, TEXT("[Havankund] ✅ BeginOverlap FIRED!"));
    UE_LOG(LogTemp, Error, TEXT("OtherActor: %s"), *GetNameSafe(OtherActor));
    UE_LOG(LogTemp, Error, TEXT("OtherComp: %s"), *GetNameSafe(OtherComp));
    UE_LOG(LogTemp, Error, TEXT("========================================"));
    
    if (OtherActor)
    {
        APawn* Pawn = Cast<APawn>(OtherActor);
        if (Pawn)
        {
            UE_LOG(LogTemp, Error, TEXT("Havankund: ✅ PAWN DETECTED - ACTIVATING!"));
            ActivateAll();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Havankund: Actor is NOT a Pawn: %s"), *OtherActor->GetClass()->GetName());
        }
    }
}

void AHavankund::OnActivationEnd(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    UE_LOG(LogTemp, Error, TEXT("========================================"));
    UE_LOG(LogTemp, Error, TEXT("[Havankund] ❌ EndOverlap FIRED!"));
    UE_LOG(LogTemp, Error, TEXT("OtherActor: %s"), *GetNameSafe(OtherActor));
    UE_LOG(LogTemp, Error, TEXT("========================================"));

    if (OtherActor)
    {
        APawn* Pawn = Cast<APawn>(OtherActor);
        if (Pawn)
        {
            UE_LOG(LogTemp, Error, TEXT("Havankund: ❌ PAWN LEFT - DEACTIVATING!"));
            DeactivateAll();
        }
    }
}

FVector AHavankund::GetRandomSpherePoint() const
{
    // Generate random point in upper hemisphere only
    FVector RandomDir = FMath::VRand();
    
    // Force Z to be positive (upper hemisphere)
    RandomDir.Z = FMath::Abs(RandomDir.Z);
    
    // Normalize and scale by random radius
    RandomDir.Normalize();
    float RandomRadius = FMath::RandRange(0.f, SpawnRadius);
    
    return GetActorLocation() + (RandomDir * RandomRadius);
}