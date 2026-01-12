#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ghost.h"
#include "Havankund.generated.h"


USTRUCT()
struct FHavankundPoolItem
{
    GENERATED_BODY()

    UPROPERTY()
    AGhost* GhostActor = nullptr;
    bool bActive = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FHavankundDestroyedCountChanged,  int32, TotalDestroyed );
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnGhostReachedTarget, int32, CurrentGhostCount );

UCLASS()
class MYY_API AHavankund : public AActor
{
    GENERATED_BODY()

public:
    AHavankund();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="Havankund")
    void ManualActivate() { ActivateAll(); }

    UFUNCTION(BlueprintCallable, Category="Havankund")
    void ManualDeactivate() { DeactivateAll(); }

    UFUNCTION(BlueprintPure, Category="Havankund")
    int32 GetHitCount() const { return DestroyedCount; }

    UFUNCTION(BlueprintCallable, Category="Havankund")
    void ResetHitCount() { DestroyedCount = 0; OnDestroyedCountChanged.Broadcast(0); }


    // ===== OVERRIDES =====
    // 🔢 Current number of ghosts reached
    // UPROPERTY(BlueprintReadOnly, Category = "Ghost")
    // int32 GhostReachedCount = 0;
    //
    // // 🎯 Required number (X)
    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
    // int32 RequiredGhostCount = 5;
    //
    // // 📢 Blueprint can bind to this
    // UPROPERTY(BlueprintAssignable, Category = "Ghost")
    // FOnGhostReachedTarget OnGhostReachedTarget;
    //
    // // 🚨 Called by Ghost when it reaches target
    // UFUNCTION()
    // void RegisterGhostReached();
    // =====================

protected:
    virtual void BeginPlay() override;

    /* Root */
    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

    /* Target Point - Where ghosts move toward */
    UPROPERTY(VisibleAnywhere)
    USceneComponent* TargetPoint;

    /* Activation Sphere - Auto-activate when pawn enters */
    UPROPERTY(VisibleAnywhere)
    class USphereComponent* ActivationSphere;

    /* ===== CONFIG ===== */

    UPROPERTY(EditAnywhere, Category="Havankund|Ghost")
    TSubclassOf<AGhost> GhostActorClass;

    UPROPERTY(EditAnywhere, Category="Havankund|Ghost")
    EGhostMeshType MeshType = EGhostMeshType::StaticMesh;

    UPROPERTY(EditAnywhere, Category="Havankund|Ghost")
    int32 SpawnCount = 5;

    UPROPERTY(EditAnywhere, Category="Havankund|Ghost")
    float SpawnRadius = 600.f;

    UPROPERTY(EditAnywhere, Category="Havankund|Ghost")
    float MoveSpeed = 250.f;

    UPROPERTY(EditAnywhere, Category="Havankund|Ghost")
    float RespawnDelay = 2.f;

    UPROPERTY(EditAnywhere, Category="Havankund|Activation")
    float ActivationRadius = 20.f;

    UPROPERTY(EditAnywhere, Category="Havankund|Activation")
    bool bAutoActivateOnOverlap = true;

    UPROPERTY(EditAnywhere, Category="Havankund|Static")
    UStaticMesh* StaticMesh;

    UPROPERTY(EditAnywhere, Category="Havankund|Skeletal")
    USkeletalMesh* SkeletalMesh;

    UPROPERTY(EditAnywhere, Category="Havankund|Skeletal")
    UAnimationAsset* LoopingAnimation;

    /* ===== STATE ===== */
    bool bIsActive = false;
    int32 DestroyedCount = 0;

    /* ===== POOL ===== */
    TArray<FHavankundPoolItem> Pool;

public:
    /* ===== DELEGATE ===== */
    UPROPERTY(BlueprintAssignable)
    FHavankundDestroyedCountChanged OnDestroyedCountChanged;

private:
    /* ===== INTERNAL ===== */
    void InitializePool();
    void ActivateAll();
    void DeactivateAll();
    void RespawnGhost(AGhost* Ghost);
    FVector GetRandomSpherePoint() const;

    UFUNCTION()
    void OnActivationBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UFUNCTION()
    void OnActivationEnd(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex
    );

    // Timer handle for respawning
    TMap<AGhost*, FTimerHandle> RespawnTimers;
};