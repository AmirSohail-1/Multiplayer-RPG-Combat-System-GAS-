// Ghost.cpp
#include "Ghost.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MYY/AbilitySystem/Actor/Projectile/ArrowProjectile.h"

AGhost::AGhost()
{
    PrimaryActorTick.bCanEverTick = true;

    // Root component
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // Collision sphere
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetupAttachment(Root);
    CollisionSphere->SetSphereRadius(50.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionSphere->SetNotifyRigidBodyCollision(true);

    // Static mesh (optional, created on demand)
    StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMeshComp->SetupAttachment(Root);
    StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Skeletal mesh (optional, created on demand)
    SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
    SkeletalMeshComp->SetupAttachment(Root);
    SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Start deactivated
    bIsActive = false;
    SetActorHiddenInGame(true);
}

void AGhost::BeginPlay()
{
    Super::BeginPlay();

    // Bind collision events
    CollisionSphere->OnComponentHit.AddDynamic(this, &AGhost::OnHit);
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGhost::OnOverlapBegin);
}

void AGhost::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActive) return;

    // Move toward target
    FVector CurrentLocation = GetActorLocation();
    FVector NewLocation = FMath::VInterpConstantTo(
        CurrentLocation,
        TargetLocation,
        DeltaTime,
        MoveSpeed
    );

    SetActorLocation(NewLocation);

    // Check if reached target (hide but DON'T deactivate - no count increment)
    float DistanceToTarget = FVector::Dist(NewLocation, TargetLocation);
    if (DistanceToTarget < 50.f)
    {
        // Just hide, don't call Deactivate() - this way Havankund won't count it
        bIsActive = false;
        SetActorHiddenInGame(true);
        CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        
        UE_LOG(LogTemp, Log, TEXT("Ghost reached target (not counted as hit)"));
    }
}

void AGhost::Initialize(EGhostMeshType MeshType, UStaticMesh* StaticMesh, 
                       USkeletalMesh* SkeletalMesh, UAnimationAsset* Animation)
{
    if (MeshType == EGhostMeshType::StaticMesh && StaticMesh)
    {
        StaticMeshComp->SetStaticMesh(StaticMesh);
        StaticMeshComp->SetVisibility(true);
        SkeletalMeshComp->SetVisibility(false);
    }
    else if (MeshType == EGhostMeshType::SkeletalMesh && SkeletalMesh)
    {
        SkeletalMeshComp->SetSkeletalMesh(SkeletalMesh);
        SkeletalMeshComp->SetVisibility(true);
        StaticMeshComp->SetVisibility(false);

        if (Animation)
        {
            SkeletalMeshComp->PlayAnimation(Animation, true);
        }
    }
}

void AGhost::Activate(const FVector& SpawnLocation)
{
    bIsActive = true;
    bWasHitByProjectile = false; // Reset flag on activate
    SetActorLocation(SpawnLocation);
    SetActorHiddenInGame(false);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    UE_LOG(LogTemp, Log, TEXT("Ghost activated at: %s"), *SpawnLocation.ToString());
}

void AGhost::Deactivate()
{
    bIsActive = false;
    SetActorHiddenInGame(true);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    UE_LOG(LogTemp, Log, TEXT("Ghost deactivated"));
}

void AGhost::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, 
                  UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!bIsActive) return;
    if (!OtherActor) return;

    // Only deactivate if hit by a projectile (not static meshes/walls)
    if (OtherActor->IsA(AArrowProjectile::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ghost hit by projectile: %s"), *GetNameSafe(OtherActor));
        bWasHitByProjectile = true; // Mark as hit by projectile
        Deactivate();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Ghost hit by non-projectile (ignored): %s"), *GetNameSafe(OtherActor));
    }
}

void AGhost::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                           bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bIsActive) return;
    if (!OtherActor) return;

    // Ignore pawns (only hit projectiles/weapons)
    if (Cast<APawn>(OtherActor))
    {
        return;
    }

    // Only deactivate if overlapped by a projectile
    if (OtherActor->IsA(AArrowProjectile::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ghost overlapped by projectile: %s"), *GetNameSafe(OtherActor));
        bWasHitByProjectile = true; // Mark as hit by projectile
        Deactivate();
    }
}