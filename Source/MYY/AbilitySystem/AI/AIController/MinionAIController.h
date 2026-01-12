#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "MinionAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;

UCLASS()
class MYY_API AMinionAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMinionAIController();
	
	static const FName BB_TargetActor;
	static const FName BB_PatrolLocation;
	static const FName BB_IsInCombat;
	static const FName BB_LastKnownLocation;
	static const FName BB_CanAttack;
	static const FName BB_ShouldBlock;
	static const FName BB_HomeLocation;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	AActor* GetTargetActor() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ClearTarget();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

	// Perception
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Damage* DamageConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
					   AController* InstigatedBy, AActor* DamageCauser);

	// Behavior Tree
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBlackboardComponent* BlackboardComponent;

	// Home location for patrol/reset
	FVector HomeLocation;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

};