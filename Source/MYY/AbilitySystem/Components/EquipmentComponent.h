// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/UnarmedCombatDataAsset.h"
#include "EquipmentComponent.generated.h"

 

class ABaseWeapon;
class AMYYCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, ABaseWeapon*, NewWeapon, ABaseWeapon*, OldWeapon);

UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Primary = 0,
	Secondary = 1,
	None = 255
};

USTRUCT(BlueprintType)
struct FWeaponSlotData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ABaseWeapon* Weapon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UWeaponDataAsset* WeaponDataAsset = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsInHand = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsOccupied = false;
};

/**
 * Manages weapon equipping, unequipping, and switching
 * Handles ability granting/removal and animation blueprint switching
 */ 
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYY_API UEquipmentComponent : public UActorComponent 
{
	GENERATED_BODY()

public:    
	UEquipmentComponent();

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FName WeaponSocketName = TEXT("Right_HandSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FName PrimaryHolsterSocket = TEXT("HolsterSocket_Primary");

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FName SecondaryHolsterSocket = TEXT("HolsterSocket_Secondary");

	UPROPERTY(ReplicatedUsing = OnRep_PrimarySlot, BlueprintReadOnly, Category = "Equipment")
	FWeaponSlotData PrimarySlot;

	UPROPERTY(ReplicatedUsing = OnRep_SecondarySlot, BlueprintReadOnly, Category = "Equipment")
	FWeaponSlotData SecondarySlot;

	UFUNCTION()
	FName GetHandSocketForWeapon(UWeaponDataAsset* WeaponData) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment", Server, Reliable)
	void Server_DropCurrentWeapon();

	/**
	* Calculates a safe drop location for a weapon with ground checking
	* @param WeaponToDrop - The weapon being dropped
	* @param OutLocation - The calculated drop location
	* @param OutRotation - The calculated drop rotation
	* @param AdditionalIgnoredActors - Extra actors to ignore in the trace (optional)
	*/
	void CalculateWeaponDropTransform(ABaseWeapon* WeaponToDrop, FVector& OutLocation, FRotator& OutRotation, const TArray<AActor*>& AdditionalIgnoredActors = TArray<AActor*>());
	 
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> DefaultAnimInstanceClass;

	UPROPERTY()
	TSubclassOf<UAnimInstance> PreviousAnimInstance;
 
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnWeaponChanged OnWeaponChanged;
 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	UWeaponDataAsset* DefaultWeaponData;

	// ========== NEW: UNARMED COMBAT SYSTEM ==========
	
	/**
	 * Default unarmed combat data - used when NO weapons are equipped
	 * Set this in BP to your unarmed combat PDA
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Unarmed")
	UUnarmedCombatDataAsset* DefaultUnarmedData;

	/**
	 * Gets the currently active weapon data OR unarmed data if no weapons equipped
	 * Use this in abilities to check weapon stats/animations
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UWeaponDataAsset* GetActiveWeaponData() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment", Server, Reliable)
	void Server_EquipWeapon(ABaseWeapon* WeaponToEquip);

	UFUNCTION(BlueprintCallable, Category = "Equipment", Server, Reliable)
	void Server_UnequipWeapon();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void Server_SwapWeaponHandToHolster(EWeaponSlot Slot);
 
	UFUNCTION(Server, Reliable)
	void Server_PickupWeaponWithDrop(ABaseWeapon* PickupWeapon);	

	UFUNCTION(BlueprintPure, Category = "Equipment")
	ABaseWeapon* GetWeaponInSlot(EWeaponSlot Slot) const;
	
	bool HasAvailableSlot() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	ABaseWeapon* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipDefaultWeapon();

	UFUNCTION()
	void ApplyWeaponAnimInstance(UWeaponDataAsset* WeaponData);
	
	UFUNCTION()
	void ApplyWeaponAnimLayers(UWeaponDataAsset* WeaponData);

	/**
	 * Gets the current weapon type (Unarmed, Melee, Ranged)
	 * Returns Unarmed if no weapon equipped
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EWeaponType GetCurrentWeaponType() const;
 
	/**
	 * NEW: Stops all active weapon traces
	 * Called when unequipping, holstering, or switching weapons
	 */
	UFUNCTION()
	void StopAllWeaponTraces();

	/**
	 * Switches to unarmed combat when no weapons equipped
	 */ 
	void SwitchToUnarmedCombat();


public:
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void DropAllWeapons();
    
	UFUNCTION(Server, Reliable)
	void Server_DropAllWeapons();
	
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	void GrantWeaponAbilities(UWeaponDataAsset* WeaponData) const;
	void RemoveWeaponAbilities(UWeaponDataAsset* WeaponData); 
	
	UPROPERTY()
	AMYYCharacterBase* OwnerCharacter;

	void ApplyDropPhysics(ABaseWeapon* WeaponToDrop, FVector ImpulseDirection);
	void DisableDropPhysics(ABaseWeapon* Weapon);
	
	UFUNCTION()
	void OnRep_PrimarySlot();

	UFUNCTION()
	void OnRep_SecondarySlot();
	
	void RestoreDefaultAnimInstance();

	// ========== NEW: UNARMED SYSTEM HELPERS ==========
	
	
	/**
	 * Grants unarmed combat abilities
	 */
	void GrantUnarmedAbilities();
	
	/**
	 * Removes unarmed combat abilities
	 */
	void RemoveUnarmedAbilities();
	// MODIFIED: Changed function name
	void UnlinkAllAnimLayers();
	
	ABaseWeapon* SpawnWeapon(UWeaponDataAsset* WeaponData);
	void AttachWeaponToSocket(ABaseWeapon* Weapon, FName SocketName);
	EWeaponSlot FindAvailableSlot() const;
	EWeaponSlot GetSlotForWeapon(ABaseWeapon* Weapon) const;
 
	// NEW: Track if layers are currently linked
	UPROPERTY()
	bool bHasLinkedLayers = false;
	
};