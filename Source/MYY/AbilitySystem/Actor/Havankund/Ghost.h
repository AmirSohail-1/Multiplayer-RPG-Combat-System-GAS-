#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ghost.generated.h"

UENUM(BlueprintType)
enum class EGhostMeshType : uint8
{
	StaticMesh,
	SkeletalMesh
};

UCLASS()
class MYY_API AGhost : public AActor
{
	GENERATED_BODY()
    
public:    
	AGhost();

	virtual void Tick(float DeltaTime) override;

	// Initialize the ghost with mesh type
	void Initialize(EGhostMeshType MeshType, UStaticMesh* StaticMesh, USkeletalMesh* SkeletalMesh, UAnimationAsset* Animation);

	// Activate/Deactivate from pool
	void Activate(const FVector& SpawnLocation);
	void Deactivate();

	// Set target location
	void SetTargetLocation(const FVector& Target) { TargetLocation = Target; }
	void SetMoveSpeed(float Speed) { MoveSpeed = Speed; }

	bool IsActive() const { return bIsActive; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* SkeletalMeshComp;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* CollisionSphere;

public:

	bool WasHitByProjectile() const { return bWasHitByProjectile; }

private:
	bool bIsActive = false;
	FVector TargetLocation;
	float MoveSpeed = 100.f;
	bool bWasHitByProjectile = false;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
			   FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
					   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
					   bool bFromSweep, const FHitResult& SweepResult);
};