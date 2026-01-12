#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GA_Meleecombo.generated.h"

UCLASS()
class MYY_API UGA_Meleecombo : public UGA_WeaponBase
{
	GENERATED_BODY()
	
public:
	UGA_Meleecombo();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
	   const FGameplayAbilitySpecHandle Handle,
	   const FGameplayAbilityActorInfo* ActorInfo,
	   const FGameplayAbilityActivationInfo ActivationInfo,
	   bool bReplicateEndAbility,
	   bool bWasCancelled) override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// Called by AnimNotifies
	UFUNCTION()
	void OpenComboWindow();
	
	UFUNCTION()
	void CloseComboWindow();

protected:
	UPROPERTY()
	FGameplayAbilitySpecHandle CachedHandle;

	UPROPERTY()
	FGameplayAbilityActivationInfo CachedActivationInfo;

	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* MontageTask;

	UPROPERTY()
	FActiveGameplayEffectHandle ActiveComboEffectHandle;

 


protected:
	// Replicated variables
	UPROPERTY(ReplicatedUsing = OnRep_CurrentComboIndex)
	int32 CurrentComboIndex;

	UPROPERTY(ReplicatedUsing = OnRep_bComboWindowOpen)
	bool bComboWindowOpen;
    
	// Replication notification functions
	UFUNCTION()
	void OnRep_CurrentComboIndex();
    
	UFUNCTION()
	void OnRep_bComboWindowOpen();

	UPROPERTY()
	bool bHasBufferedInput;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	bool bUseRandomCombo;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	int32 MaxComboChain = 4;

	UFUNCTION()
	void OnMontageCompleted();
	
	UFUNCTION()
	void OnMontageCancelled();
	
	UFUNCTION()
	void OnMontageInterrupted();

	void TriggerNextCombo();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

 
	const FGameplayAbilityActorInfo* CachedActorInfo;
protected:
	UFUNCTION(Server, Reliable)
	void ServerHandleComboInput();
};