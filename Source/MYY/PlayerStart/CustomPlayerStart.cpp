#include "CustomPlayerStart.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"

ACustomPlayerStart::ACustomPlayerStart(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
    
	// Visual feedback in editor
	if (GetRootComponent())
	{
		GetRootComponent()->SetMobility(EComponentMobility::Static);
	}
}

void ACustomPlayerStart::BeginPlay()
{
	Super::BeginPlay();

	// Auto-set tag based on spawn type
	if (SpawnType == ESpawnPointType::Player)
	{
		Tags.AddUnique(FName("PlayerSpawn"));
	}
	else if (SpawnType == ESpawnPointType::AI)
	{
		Tags.AddUnique(FName("AISpawn"));
	}
}

#if WITH_EDITOR
void ACustomPlayerStart::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Visual feedback in editor
	if (GetRootComponent())
	{
		if (SpawnType == ESpawnPointType::Player)
		{
			GetRootComponent()->SetVisibility(true);
		}
		else if (SpawnType == ESpawnPointType::AI)
		{
			GetRootComponent()->SetVisibility(true);
		}
	}
}
#endif