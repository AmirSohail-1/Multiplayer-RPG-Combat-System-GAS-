 
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "MYY/AbilitySystem/AttributeSet/AttributeSetBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "MYY/Enums/AbilityInputTypes.h"
#include "MYYCharacterBase.generated.h"


USTRUCT(BlueprintType)
struct FUnarmedTraceSocket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketOrBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 12.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, Health, float, MaxHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChanged, float, Stamina, float, MaxStamina, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDied, AMYYCharacterBase*, Victim, AActor*, Killer);

class UAbilitySystemComponent;
class UAttributeSetBase;
class UEquipmentComponent;
class UMotionWarpingComponent;

UCLASS()
class MYY_API AMYYCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMYYCharacterBase();
	
	// 0=Player, 1=Enemy, 2=Neutral
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Team")
	uint8 TeamID = 0;

	// IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	

	// Ability System Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem| Attributes")
	UAttributeSetBase* AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UMotionWarpingComponent* MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", Replicated)
	UEquipmentComponent* EquipmentComponent;
 
	// Add debug function
	UFUNCTION(Exec, Category = "Debug")
	void DebugAbilities();
	void DisableMovementDuringAbility(bool bCond);

	// Add   persistent abilities --------------------- other array add & clear ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> InteractAbility;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> EquipAbility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> UnequipAbility;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> HitReactAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

	// Default abilities granted at spawn	 // dON'T FORGET ADD INTERACT ABILITY
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities", meta = (DisplayPriority = 100))
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities", meta = (DisplayPriority = 100))
	TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	// Delegates for UI/gameplay events
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCharacterDied OnCharacterDied;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void InitializeAbilities();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void Die(AActor* Killer);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void PerformBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void StopBlocking();

	// For AI targeting
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetCurrentTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Combat")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// Range Weapon
	UPROPERTY(ReplicatedUsing=OnRep_IsAiming, BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;

	UFUNCTION()
	void OnRep_IsAiming();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetAiming(bool bInAiming);

	// Death event for Blueprint

	UFUNCTION(BlueprintImplementableEvent, Category="Death")
	void BP_OnCharacterDeath(AActor* Killer);

	void HandleDeath(AActor* KillerActor);

	UPROPERTY(BlueprintReadOnly, Category = "Death")
	bool bIsDead = false;

	//  Death event end.

 	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Store granted ability handles for removal
	UPROPERTY()
	TMap<TSubclassOf<UGameplayAbility>, FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	// CharacterBase.h - Add this property
	UPROPERTY(ReplicatedUsing = OnRep_IsInBlockWindow, BlueprintReadWrite, Category = "Combat")
	bool bIsInBlockWindow = false;
 

	UFUNCTION(Server, Reliable )
	void Server_SetBlockWindow(bool bInWindow);
 
	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* InteractableActor);

	//  Unarmed Trace system Start -----------------------------
	UFUNCTION()
	void PerformUnarmedTrace(const TArray<FUnarmedTraceSocket>& TraceSockets);

	UPROPERTY()
	TSet<AActor*> HitActorsThisSwing;

	// Unarmed trace state
	UPROPERTY()
	bool bIsUnarmedTracing = false;

	UPROPERTY()
	TArray<FUnarmedTraceSocket> ActiveUnarmedTraceSockets;

	void StartUnarmedTrace(	const TArray<FUnarmedTraceSocket>& TraceSockets);
	void EndUnarmedTrace();
 
	void ForceStopAllCombatTraces();

	//  Unarmed Trace system End  -----------------------------
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void BindAttributeCallbacks();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem", meta = (DisplayPriority = 100))
	EGameplayEffectReplicationMode ASCReplicationMode = EGameplayEffectReplicationMode::Mixed;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// AI
	virtual void PossessedBy(AController* NewController) override;
	// player start or replicated
	virtual void OnRep_PlayerState() override;

	void InitializeAbilitySystem();

	// Attribute callbacks
	virtual void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(Replicated)
	AActor* CurrentTarget;

	UFUNCTION()
	void OnRep_IsInBlockWindow();

	virtual void OnStaminaAttributeChanged(const FOnAttributeChangeData& Data);
	
	//  Initialize AttributeSet & update for UI, every new spawn.

	UFUNCTION(BlueprintCallable)
	void InitializeHealthStaminaUI();
	 
	

	 

};
