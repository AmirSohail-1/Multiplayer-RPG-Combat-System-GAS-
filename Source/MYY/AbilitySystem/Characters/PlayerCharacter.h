// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "PlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class MYY_API APlayerCharacter : public AMYYCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// Enhanced Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* AttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* BlockAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* DodgeAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* PrimaryEquipUnEquipAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* SecondaryEquipUnEquipAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* DropWeaponAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (DisplayPriority = 99))
	UInputAction* AimAction;

	// Ranged weapon firing
	UFUNCTION(Server, Reliable)
	void Server_FireRangedWeapon();

	// Add in public section
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraZoom(bool bZoomIn);

	// Default distance of camera boom (TPS view)
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float NormalCameraDistance = 400.f;

	// Distance when aiming (zoom in)
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimCameraDistance = 200.f;

	// Speed of smooth zoom interpolation
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraZoomSpeed = 10.f;

	// Internal (used for smooth zoom)
	float TargetArmLength = 400.f;

	/* --- Optional: Shoulder offset for aiming --- */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector NormalCameraOffset = FVector(0.f, 50.f, 0.f);      // slight right side

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector AimCameraOffset = FVector(0.f, 120.f, 20.f);       // right shoulder aim view

	// Range Weapon firing End

	UFUNCTION()
	void SetupInputMappingContext();

	// ✅ Simple getter to access parent's bIsAiming
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool GetIsAiming() const { return bIsAiming; }


	// Vault start	
	/** Attempts vault first, then falls back to jump if vault not possible */
	UFUNCTION(BlueprintCallable, Category = "Input") 
	void JumpOrVault();

	UFUNCTION(Exec, Category = "Debug")
	void DebugVault();
	// Vault end

protected:
	virtual void BeginPlay() override;

	
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION(BlueprintCallable, Category = "Input") 
	void AimPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Input") 
	void AimReleased();

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input") 
	void AttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Input") 
	void AttackReleased();
	
	UFUNCTION(BlueprintCallable, Category = "Input") 
	void BlockPressed();
 
	UFUNCTION(BlueprintCallable, Category = "Input") 
	void BlockReleased();

	UFUNCTION(BlueprintCallable, Category = "Input") 
	void Dodge(); 

	UFUNCTION(BlueprintCallable, Category = "Input") 
	void HolsterOrEquipPrimaryWeapon();
	
	UFUNCTION(BlueprintCallable, Category = "Input") 
	void HolsterOrEquipSecondaryWeapon();

	UFUNCTION(BlueprintCallable, Category = "Input") 
	void DropCurrentWeapon();

	UFUNCTION(BlueprintCallable, Category = "Input") 
	void Interact();
 
	void SendAbilityLocalInput(int32 InputID, bool bPressed);
	virtual void PossessedBy(AController* NewController) override;

	// Crosshair widget reference
	UPROPERTY()
	UUserWidget* ActiveCrosshairWidget;

	
};
