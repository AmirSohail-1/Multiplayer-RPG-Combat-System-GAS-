// CustomPlayerStart.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "CustomPlayerStart.generated.h"

UENUM(BlueprintType)
enum class ESpawnPointType : uint8
{
	Player UMETA(DisplayName = "Player Spawn"),
	AI UMETA(DisplayName = "AI Spawn"),
	Both UMETA(DisplayName = "Both")
};

UCLASS()
class MYY_API ACustomPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	ACustomPlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
	ESpawnPointType SpawnType = ESpawnPointType::Player;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
	bool bShowDebugSphere = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Settings")
	float DebugSphereRadius = 50.0f;

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif
};