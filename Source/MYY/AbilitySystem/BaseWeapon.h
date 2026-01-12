// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "Interface/Interactable.h"
#include "BaseWeapon.generated.h"

class USphereComponent;

UCLASS()
class MYY_API ABaseWeapon : public AActor, public IInteractable
{
	GENERATED_BODY()

public:    
	ABaseWeapon();

	/****/
	// Debug command
	UFUNCTION(Exec, Category = "Weapon|Debug")
	void DebugWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default Weapon")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	USphereComponent* InteractionSphere;

	// Scene components as placeholders for trace sockets
	// These will attach to weapon mesh sockets if they exist
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	USceneComponent* TraceStartSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	USceneComponent* TraceEndSocket;

	UPROPERTY(Replicated,EditDefaultsOnly, BlueprintReadOnly, Category = "Default Weapon")
	UWeaponDataAsset* WeaponData;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponDataID)
	int32 WeaponDataID;

	UFUNCTION()
	void  OnRep_WeaponDataID();

	// Track hit actors during single attack to prevent double-hits
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<AActor*> HitActorsThisSwing;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearHitActors();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UWeaponDataAsset* GetWeaponData() const { return WeaponData; }

	// ========== AMMO SYSTEM FOR RANGED WEAPONS ==========
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentAmmo, BlueprintReadOnly, Category = "Ammo")
	int32 CurrentAmmo = 0;

	UFUNCTION()
	void OnRep_CurrentAmmo();

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void ConsumeAmmo(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = "Ammo")
	bool HasAmmo() const { return CurrentAmmo > 0; }

	UFUNCTION(BlueprintPure, Category = "Ammo")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Ammo")
	int32 GetMaxAmmo() const;
	// ====================================================		

	// Damage calculation with critical hit logic
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float CalculateDamage(bool& bOutIsCritical);
	
	// IInteractable interface
	virtual void OnInteract_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	// Enable/disable pickup
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetPickupEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsPickupEnabled() const { return bCanBePickedUp; }

	// CRASH FIX: Safe collision control
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EnableWeaponCollision(bool bForAttack = false);
    
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DisableWeaponCollision();
	void UpdatePickupCollision(bool bEnabled);

	// In BaseWeapon.h, add:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetPickupEnabled(bool bEnabled);

	// Temp for EquipComp & move to protected later------------------
	UPROPERTY()
	FTimerHandle TraceTimerHandle;

	UPROPERTY(Replicated)
	bool bIsTracing = false;

	// Temp for EquipComp & move to protected later---------------------

	

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	

	UPROPERTY(Replicated)
	bool bCanBePickedUp = true;

	
 
	// Main trace function - called repeatedly during attack
	void PerformTrace();

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Last frame positions for swept trace (prevents missing fast hits)
	FVector LastStartPos;
	FVector LastEndPos;
	bool bHasLastPositions = false;

	UFUNCTION()
	void OnRep_IsTracing();

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
};
