// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "MYY/Enums/AbilityInputTypes.h"
#include "WeaponDataAsset.generated.h"


// Forward declarations
class UGameplayAbility;
class UAnimInstance;
class ABaseWeapon;

USTRUCT(BlueprintType)
struct FAbilityInputMapping
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EAbilityInputID InputID = EAbilityInputID::None;
};

// ADD THIS NEW ENUM
UENUM(BlueprintType)
enum class EAnimLayerUsage : uint8
{
	FullBodyOnly    UMETA(DisplayName = "Full Body Only"),      // Sword, Unarmed
	UpperLowerSplit UMETA(DisplayName = "Upper + Lower Body")   // Bow, Gun
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Melee       UMETA(DisplayName = "Melee"),
    Ranged      UMETA(DisplayName = "Ranged"),
    Unarmed     UMETA(DisplayName = "Unarmed")
};

USTRUCT(BlueprintType)
struct FWeaponStats
{
    GENERATED_BODY() 

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float BaseDamage = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float AttackSpeed = 1.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float Range = 200.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float BlockDamageReduction = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float CriticalHitChance = 0.1f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float CriticalHitMultiplier = 2.0f;

	// ✅ NEW: Trace collision configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Trace")
	float TraceRadius = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Trace")
	float TraceHalfHeight = 50.f;  // Only used for capsule

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Trace")
	bool bUseCapsuleTrace = false;  // false = sphere, true = capsule
};

USTRUCT(BlueprintType)
struct FWeaponAnimationSet
{
    GENERATED_BODY()

    // Combo system - array of attack animations
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
    TArray<UAnimMontage*> ComboMontages;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
    UAnimMontage* BlockMontage=nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
	UAnimMontage* ParryMontage=nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
	UAnimMontage* DodgeMontage=nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Reactions")
	UAnimMontage* ParryReactMontage=nullptr; // Attacker gets staggered
	
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
    UAnimMontage* EquipMontage=nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
    UAnimMontage* UnequipMontage=nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Reactions")
    TArray<UAnimMontage*> HitReactMontages;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Reactions")
    UAnimMontage* KnockdownMontage=nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Reactions")
    UAnimMontage* DeathMontage=nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Vault|Animation")
	UAnimMontage* VaultMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Vault|Animation")
	UAnimMontage* ClimbMontage;

	// Linked Animation Layer - THIS IS THE KEY FOR DYNAMIC ANIM SWITCHING
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TSubclassOf<UAnimInstance> WeaponAnimInstanceClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	EAnimLayerUsage AnimLayerUsage = EAnimLayerUsage::FullBodyOnly;
	
 
};

/**
 * Primary Data Asset for weapon configuration
 * Create variants: PDA_Sword, PDA_Bow, PDA_Unarmed
 */
UCLASS(BlueprintType)
class MYY_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int32 WeaponID_ReplicateWeapon_DA;								// Add this - set manually in each PDA & WILL used for Replication
 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AActor> WeaponActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FWeaponStats Stats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FWeaponAnimationSet Animations;

	// Old Method - Remove later if not used
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	// TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GamePlayEffects") 
	TArray<TSubclassOf<UGameplayEffect>> ComboDamageEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GamePlayEffects")
	TSubclassOf<UGameplayEffect> BlockEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GamePlayEffects")
	TSubclassOf<UGameplayEffect> DamageEffect;
	

	// NEW: Abilities with explicit input mapping
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<FAbilityInputMapping> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	FGameplayTag WeaponTag;

	/// Socket configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attachment")
	FName HandSocketName = "RightHandSocket";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attachment")
	FName HolsterSocketName = "HolsterSocket";

	// Trace sockets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName TraceStartSocketName = "TraceStart";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName TraceEndSocketName = "TraceEnd";

	// VFX and SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* HitVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* HitSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* SwingSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* BlockSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* ParrySFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug| Projectile Arrow")
	bool bDebugProjectile = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|WeaponTrace")
	bool bDebugWeaponTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|UnArmedTrace")
	bool bDebugUnArmedTrace = false;

	// ========== VIRTUAL FUNCTIONS ==========
    
	/**
	 * Override in child classes to provide weapon-type-specific behavior
	 */
	virtual bool SupportsComboSystem() const { return false; }
	virtual bool RequiresProjectiles() const { return false; }
	virtual bool RequiresAmmo() const { return false; }
	virtual bool HasTraceSystem() const { return false; }

	/**
	 * Get the animation layer class for this weapon
	 * Used by EquipmentComponent to switch animation layers
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual TSubclassOf<UAnimInstance> GetAnimationLayerClass() const
	{
		return Animations.WeaponAnimInstanceClass;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("Weapon", GetFName());
	}
	
};
