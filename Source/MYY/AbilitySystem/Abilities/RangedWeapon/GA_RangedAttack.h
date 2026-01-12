// GA_RangedAttack.h - Handles Draw → Aim → Shoot sequence
#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBase.h"
#include "GA_RangedAttack.generated.h"

class URangedWeaponDataAsset;
class UAbilityTask_PlayMontageAndWait;

UENUM(BlueprintType)
enum class ERangedAttackState : uint8
{

    Drawing,
    Aiming,
    Firing,
};

UCLASS()
class MYY_API UGA_RangedAttack : public UGA_WeaponBase
{
    GENERATED_BODY()

public:
    UGA_RangedAttack();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void InputPressed(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) override;

    // Add helper function
    FVector GetCenterScreenAimLocation() const;
    ABaseWeapon* GetCurrentWeapon() const;

    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
                               const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo) override;

    virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateCancelAbility) override;

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    UPROPERTY()
    bool bIsAimingActive = false;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Ranged")
    ERangedAttackState CurrentState;

    UPROPERTY(BlueprintReadOnly, Category = "Ranged")
    float DrawStartTime;

    UPROPERTY(BlueprintReadOnly, Category = "Ranged")
    float CurrentDrawPercent;

    void StartDrawing();
    void UpdateDrawProgress();
    void FireArrow();
    void SpawnProjectile();

    UFUNCTION()
    void OnDrawMontageCompleted();

    UFUNCTION()
    void OnDrawMontageBlendOut();

    UFUNCTION()
    void OnFireMontageCompleted();

    UFUNCTION()
    void OnFireMontageBlendOut();

    FTimerHandle DrawProgressTimer;

    UPROPERTY()
    URangedWeaponDataAsset* RangedWeaponData;
protected:
    // ✅ NEW: Store aim montage task reference
    UPROPERTY()
    UAbilityTask_PlayMontageAndWait* AimMontageTask;

    // ✅ NEW: Aim montage handlers
    UFUNCTION()
    void OnAimMontageInterrupted();

    UFUNCTION()
    void OnAimMontageCancelled();

    UFUNCTION()
    void OnAimMontageBlendOut();

    // ✅ NEW: Centralized cleanup
    void CleanupAimState();
    
};