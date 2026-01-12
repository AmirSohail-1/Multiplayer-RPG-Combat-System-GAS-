 
#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"  
#include "MYY/AbilitySystem/DataAsset/WeaponTypeDA/RangedWeaponDataAsset.h"


// Sets default values
APlayerCharacter::APlayerCharacter()
{
	
	PrimaryActorTick.bCanEverTick = true;

	// Camera setup
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

 
	// Character movement config
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// UI Crosshair.
	ActiveCrosshairWidget = nullptr;


}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

	// ✅ CRITICAL: Delay input setup to ensure controller is ready
	FTimerHandle InputSetupTimer;
	GetWorld()->GetTimerManager().SetTimer(InputSetupTimer, [this]()
	{
		SetupInputMappingContext();
	}, 0.2f, false);

	// Range weapon aim state

	// ✅ Initialize camera distance
	TargetArmLength = NormalCameraDistance;
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = TargetArmLength;
	}
	
}

void APlayerCharacter::SetupInputMappingContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// ✅ Clear any existing contexts first
			Subsystem->ClearAllMappings();
            
			// ✅ Add input mapping context
			// ✅ Add input mapping context
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("✅ Input Mapping Context added"));  
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ DefaultMappingContext is NULL!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ Failed to get EnhancedInputLocalPlayerSubsystem"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Controller is not APlayerController!"));
	}
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// ✅ Smooth camera zoom
	if (CameraBoom)
	{
		// Smooth boom length
		CameraBoom->TargetArmLength =
			FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaSeconds, CameraZoomSpeed);

		// Smooth camera shoulder offset
		CameraBoom->SocketOffset =
			FMath::VInterpTo(CameraBoom->SocketOffset,
							 bIsAiming ? AimCameraOffset : NormalCameraOffset,
							 DeltaSeconds, CameraZoomSpeed);
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Movement
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        
        // Jump
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerCharacter::JumpOrVault);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        
        // Combat
    	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::AttackPressed);
    	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &APlayerCharacter::AttackReleased);

    	//Block
        EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &APlayerCharacter::BlockPressed);
        EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &APlayerCharacter::BlockReleased);

    	// Aim
    	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APlayerCharacter::AimPressed);
    	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APlayerCharacter::AimReleased);

		// Dodge
    	EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Dodge);

    	// Interaction
    	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Interact);
    	
    	// Drop, Equip & Holster
    	EnhancedInputComponent->BindAction(PrimaryEquipUnEquipAction, ETriggerEvent::Started, this, &APlayerCharacter::HolsterOrEquipPrimaryWeapon);
    	EnhancedInputComponent->BindAction(SecondaryEquipUnEquipAction, ETriggerEvent::Completed, this, &APlayerCharacter::HolsterOrEquipSecondaryWeapon);

    	EnhancedInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Started, this, &APlayerCharacter::DropCurrentWeapon);
    	
    }
}
 

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// UE_LOG(LogTemp, Warning, TEXT("🎮 MOVE INPUT RECEIVED!"));
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
    else
    {
    	UE_LOG(LogTemp, Error, TEXT("❌ Controller is NULL in Move()!"));
    }
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}



void APlayerCharacter::AttackPressed()
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerCharCpp:🎮 AttackPressed called"));
	// UE_LOG(LogTemp, Warning, TEXT("   AbilitySystemComponent valid: %s"), 
	// 	AbilitySystemComponent ? TEXT("YES") : TEXT("NO"));
	// UE_LOG(LogTemp, Warning, TEXT("   Sending InputID: %d"), (int32)EAbilityInputID::PrimaryAttack);
	
 
		 // 🔥 RANGED ATTACK MODE
		if (bIsAiming   && EquipmentComponent)
		{
			ABaseWeapon* Weapon = EquipmentComponent->GetCurrentWeapon();
			if (Weapon && Cast<URangedWeaponDataAsset>(Weapon->WeaponData))
			{
				UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter] 🏹 Aiming with ranged weapon - Fire on RELEASE"));
				// ✅ Don't fire on press - wait for release

				//											----------------------------------------------									Mobile UMG Button Press not working while aiming so, add this in on Press or Start Enhance input
				SendAbilityLocalInput((int32)EAbilityInputID::Fire, true);  // Activate
				SendAbilityLocalInput((int32)EAbilityInputID::Fire, false); // Release ( Triggers InputReleased )
				return;
			}
		}

        // If aiming but weapon is not ranged, fallback to melee
 
	
	// Arrow Shoot End

	// ✅ Only reaches here if NOT aiming
	UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter]  Melee Acttack  Sending InputID: %d"), 
		(int32)EAbilityInputID::PrimaryAttack);
	SendAbilityLocalInput((int32)EAbilityInputID::PrimaryAttack, true);
	
}


void APlayerCharacter::AttackReleased()
{
	UE_LOG(LogTemp, Warning, TEXT("🎮 [Player Character]  AttackReleased called"));
	// UE_LOG(LogTemp, Warning, TEXT(" [Player Character]   AbilitySystemComponent valid: %s"), 
	// 	AbilitySystemComponent ? TEXT("YES") : TEXT("NO"));
	// UE_LOG(LogTemp, Warning, TEXT("   [Player Character]  Sending InputID: %d"), (int32)EAbilityInputID::PrimaryAttack);

	if (bIsAiming  && EquipmentComponent)
	{
		ABaseWeapon* Weapon = EquipmentComponent->GetCurrentWeapon();
		if (Weapon && Cast<URangedWeaponDataAsset>(Weapon->WeaponData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter] 🏹 Firing arrow"));
			
			SendAbilityLocalInput((int32)EAbilityInputID::Fire, true);  // Activate
			SendAbilityLocalInput((int32)EAbilityInputID::Fire, false); // Release ( Triggers InputReleased )
			
			return;
		}
	}

	// MELEE MODE
	SendAbilityLocalInput((int32)EAbilityInputID::PrimaryAttack, true);
}

void APlayerCharacter::JumpOrVault()
{
	if (!AbilitySystemComponent)
	{
		Jump();
		return;
	}

	// Find vault ability
	FGameplayAbilitySpec* VaultSpec = nullptr;
	FGameplayTag VaultTag = FGameplayTag::RequestGameplayTag(FName("Ability.Movement.Vault"));
    
	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(VaultTag))
		{
			VaultSpec = &Spec;
			break;
		}
	}

	// Try vault first
	bool bVaultSucceeded = false;
	if (VaultSpec)
	{
		bVaultSucceeded = AbilitySystemComponent->TryActivateAbility(VaultSpec->Handle);
	}

	// If vault failed or doesn't exist, do normal jump
	if (!bVaultSucceeded)
	{
		if (!this->GetCharacterMovement()->IsFalling())
		{
			Jump();
		}
	}
}

void APlayerCharacter::DebugVault()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] ========== VAULT DIAGNOSTIC =========="), *GetClass()->GetName());
	
	// Check character state
	UE_LOG(LogTemp, Warning, TEXT("[%s] Character Location: %s"), *GetClass()->GetName(), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("[%s] Character Forward: %s"), *GetClass()->GetName(), *GetActorForwardVector().ToString());
	
	// Check capsule
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		float FeetZ = GetActorLocation().Z - HalfHeight;
		
		UE_LOG(LogTemp, Warning, TEXT("[%s] Capsule Half Height: %.1f"), *GetClass()->GetName(), HalfHeight);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Feet Z Position: %.1f"), *GetClass()->GetName(), FeetZ);
	}
	
	// Check movement
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Movement Mode: %d (1=Walking, 3=Flying, 4=Falling)"), 
			*GetClass()->GetName(), (int32)MoveComp->MovementMode);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Is On Ground: %s"), 
			*GetClass()->GetName(), MoveComp->IsMovingOnGround() ? TEXT("YES") : TEXT("NO"));
		UE_LOG(LogTemp, Warning, TEXT("[%s] Is Falling: %s"), 
			*GetClass()->GetName(), MoveComp->IsFalling() ? TEXT("YES") : TEXT("NO"));
	}
	
	// Check forward trace manually
	FVector Start = GetActorLocation() + FVector(0, 0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.3f);
	FVector End = Start + (GetActorForwardVector() * 150.f);
	
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);
	
	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ✅ FORWARD HIT DETECTED:"), *GetClass()->GetName());
		UE_LOG(LogTemp, Warning, TEXT("[%s]   Hit Location: %s"), *GetClass()->GetName(), *Hit.ImpactPoint.ToString());
		UE_LOG(LogTemp, Warning, TEXT("[%s]   Hit Actor: %s"), *GetClass()->GetName(), *GetNameSafe(Hit.GetActor()));
		UE_LOG(LogTemp, Warning, TEXT("[%s]   Distance: %.1f cm"), *GetClass()->GetName(), FVector::Dist(Start, Hit.ImpactPoint));
		
		// Check height above feet
		float FeetZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		float HitHeight = Hit.ImpactPoint.Z - FeetZ;
		UE_LOG(LogTemp, Warning, TEXT("[%s]   Hit Height from Feet: %.1f cm"), *GetClass()->GetName(), HitHeight);
		
		// Draw debug
		DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Red, false, 5.f, 0, 3.f);
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 20.f, 12, FColor::Orange, false, 5.f);
		
		// Try to find top
		FVector TopStart = Hit.ImpactPoint + FVector(0, 0, 200.f) + (GetActorForwardVector() * 30.f);
		FVector TopEnd = TopStart - FVector(0, 0, 300.f);
		
		FHitResult TopHit;
		bool bFoundTop = GetWorld()->LineTraceSingleByChannel(
			TopHit,
			TopStart,
			TopEnd,
			ECC_Visibility,
			QueryParams
		);
		
		if (bFoundTop)
		{
			float TopHeight = TopHit.ImpactPoint.Z - FeetZ;
			UE_LOG(LogTemp, Warning, TEXT("[%s] ✅ TOP SURFACE FOUND:"), *GetClass()->GetName());
			UE_LOG(LogTemp, Warning, TEXT("[%s]   Top Location: %s"), *GetClass()->GetName(), *TopHit.ImpactPoint.ToString());
			UE_LOG(LogTemp, Warning, TEXT("[%s]   Top Height from Feet: %.1f cm"), *GetClass()->GetName(), TopHeight);
			
			// GREEN sphere - obstacle top (climb destination)
			DrawDebugSphere(GetWorld(), TopHit.ImpactPoint, 25.f, 12, FColor::Green, false, 5.f);
			DrawDebugLine(GetWorld(), TopStart, TopHit.ImpactPoint, FColor::Yellow, false, 5.f, 0, 3.f);
			
			// Now check for ground landing beyond obstacle
			FVector LandingStart = TopHit.ImpactPoint 
				+ (GetActorForwardVector() * 100.f) 
				+ FVector(0, 0, 50.f);
			FVector LandingEnd = LandingStart - FVector(0, 0, 300.f);
			
			FHitResult LandingHit;
			bool bFoundLanding = GetWorld()->LineTraceSingleByChannel(
				LandingHit,
				LandingStart,
				LandingEnd,
				ECC_Visibility,
				QueryParams
			);
			
			if (bFoundLanding)
			{
				float LandingHeight = LandingHit.ImpactPoint.Z - FeetZ;
				UE_LOG(LogTemp, Warning, TEXT("[%s] ✅ LANDING GROUND FOUND:"), *GetClass()->GetName());
				UE_LOG(LogTemp, Warning, TEXT("[%s]   Landing Location: %s"), *GetClass()->GetName(), *LandingHit.ImpactPoint.ToString());
				UE_LOG(LogTemp, Warning, TEXT("[%s]   Landing Height from Start Feet: %.1f cm"), *GetClass()->GetName(), LandingHeight);
				
				// BLUE sphere - ground landing (vault destination)
				DrawDebugSphere(GetWorld(), LandingHit.ImpactPoint, 25.f, 12, FColor::Blue, false, 5.f);
				DrawDebugLine(GetWorld(), LandingStart, LandingHit.ImpactPoint, FColor::White, false, 5.f, 0, 3.f);
				
				// Determine which action to use
				if (FMath::Abs(LandingHeight) < 20.f)
				{
					UE_LOG(LogTemp, Warning, TEXT("[%s] 🎯 USE VAULT MONTAGE -> Land on BLUE sphere (ground)"), *GetClass()->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[%s] 🎯 USE CLIMB MONTAGE -> Land on GREEN sphere (obstacle top)"), *GetClass()->GetName());
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[%s] ❌ NO LANDING GROUND FOUND"), *GetClass()->GetName());
				DrawDebugLine(GetWorld(), LandingStart, LandingEnd, FColor::Red, false, 5.f, 0, 3.f);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] ❌ NO TOP SURFACE FOUND"), *GetClass()->GetName());
			DrawDebugLine(GetWorld(), TopStart, TopEnd, FColor::Red, false, 5.f, 0, 3.f);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ❌ NO OBSTACLE DETECTED IN FORWARD TRACE"), *GetClass()->GetName());
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 5.f, 0, 3.f);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] ====================================="), *GetClass()->GetName());
}




void APlayerCharacter::AimPressed()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter] 🎯 Aim Pressed"));

	SetAiming(true);  // Base class handles replication & movement
	SetCameraZoom(true);

	if (IsLocallyControlled() && EquipmentComponent)
	{
		ABaseWeapon* Weapon = EquipmentComponent->GetCurrentWeapon();
		if (Weapon)
		{
			URangedWeaponDataAsset* RangedWeaponData = Cast<URangedWeaponDataAsset>(Weapon->WeaponData);
			if (RangedWeaponData && RangedWeaponData->CrosshairWidgetClass)
			{
				if (APlayerController* PC = Cast<APlayerController>(GetController()))
				{
					if (!IsLocallyControlled())
					{
						return;
					}
					
					ActiveCrosshairWidget = CreateWidget<UUserWidget>(PC, RangedWeaponData->CrosshairWidgetClass);
					if (ActiveCrosshairWidget)
					{
						ActiveCrosshairWidget->AddToViewport();
						UE_LOG(LogTemp, Log, TEXT("[PlayerCharacter] 🎯 Crosshair Added To Viewport"));
					}
				}
			}
		}
	}
	
	// SendAbilityLocalInput((int32)EAbilityInputID::Aim, true);
}

void APlayerCharacter::AimReleased()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter] 🎯 Aim Released"));
 
    SetAiming(false);
    SetCameraZoom(false);

	if (!IsLocallyControlled())
	{
		return;
	}
	
	// ✅ Remove Crosshair UI
	if (ActiveCrosshairWidget)
	{
		ActiveCrosshairWidget->RemoveFromParent();
		ActiveCrosshairWidget = nullptr;
		UE_LOG(LogTemp, Log, TEXT("[PlayerCharacter] ❌ Crosshair Removed"));
	}

	if (!bIsAiming)
	{
		// SendAbilityLocalInput((int32)EAbilityInputID::Aim, false);
	} 
}
 
void APlayerCharacter::BlockPressed()
{
	SendAbilityLocalInput((int32)EAbilityInputID::Block, true);
}

void APlayerCharacter::BlockReleased()
{
	SendAbilityLocalInput((int32)EAbilityInputID::Block, false);
}

void APlayerCharacter::Dodge()
{
	SendAbilityLocalInput((int32)EAbilityInputID::Dodge, true);
}


 
void APlayerCharacter::HolsterOrEquipPrimaryWeapon()
{
	if (!EquipmentComponent) return;
    
	// Toggle primary weapon between hand and holster
	EquipmentComponent->Server_SwapWeaponHandToHolster(EWeaponSlot::Primary);
}

void APlayerCharacter::HolsterOrEquipSecondaryWeapon()
{
	if (!EquipmentComponent) return;
    
	// Toggle secondary weapon between hand and holster
	EquipmentComponent->Server_SwapWeaponHandToHolster(EWeaponSlot::Secondary);
}


void APlayerCharacter::DropCurrentWeapon()
{
	if (!EquipmentComponent) return;
    
	// Toggle secondary weapon between hand and holster
	EquipmentComponent->Server_DropCurrentWeapon();
}
 
void APlayerCharacter::Interact()
{ 
    
	SendAbilityLocalInput((int32)EAbilityInputID::Interact, true);
}


void APlayerCharacter::SendAbilityLocalInput(int32 InputID, bool bPressed)
{
	if (!AbilitySystemComponent) return;

	if (bPressed)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(InputID);
	}
	else
	{
		AbilitySystemComponent->AbilityLocalInputReleased(InputID);
	}
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
    
	UE_LOG(LogTemp, Warning, TEXT("🎮 PlayerCharacter possessed by: %s"), *GetNameSafe(NewController));

	// ✅ Setup input immediately after possession
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
            
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("✅ Input setup in PossessedBy"));
			}
		}
	}
}

void APlayerCharacter::Server_FireRangedWeapon_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter] 🔫 SERVER: Fire Ranged Weapon"));
    
	if (!AbilitySystemComponent) return;
    
	// Trigger fire event
	FGameplayEventData EventData;
	EventData.Instigator = this;
    
	AbilitySystemComponent->HandleGameplayEvent(
		FGameplayTag::RequestGameplayTag("GameplayEvent.Fire"),
		&EventData
	);
}

 

void APlayerCharacter::SetCameraZoom(bool bZoomIn)
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerCharacter] Camera Zoom: %s"), 
		bZoomIn ? TEXT("IN") : TEXT("OUT"));

	// Zoom boom length
	TargetArmLength = bZoomIn ? AimCameraDistance : NormalCameraDistance;

	// Shoulder camera offset
	FVector NewOffset = bZoomIn ? AimCameraOffset : NormalCameraOffset;
	CameraBoom->SocketOffset = NewOffset;
}



