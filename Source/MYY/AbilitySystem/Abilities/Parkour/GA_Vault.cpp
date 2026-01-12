#include "GA_Vault.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "DrawDebugHelpers.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"

UGA_Vault::UGA_Vault()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;

	// Stamina not recquired
	bCheckStaminaBeforeActivate = false;
	RequiredStaminaCost = 15.0f;
	
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Movement.Vault")));
	SetAssetTags(AssetTags);
}

bool UGA_Vault::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Super::CanActivateAbility failed"));
		return false;
	}

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor);
	if (!Character || !Character->GetCharacterMovement())
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Invalid character or movement component"));
		return false;
	}

	// ✅ FIX: More lenient ground check
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	
	// Allow vault if walking OR if very close to ground (within 10cm)
	bool bIsOnGround = MovementComp->IsMovingOnGround();
	bool bIsNearGround = !MovementComp->IsFalling() || 
						 (Character->GetActorLocation().Z - MovementComp->GetActorLocation().Z) < 10.f;
	
	if (!bIsOnGround && !bIsNearGround)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Character is not on ground (MovementMode: %d, IsFalling: %s)"), 
				(int32)MovementComp->MovementMode,
				MovementComp->IsFalling() ? TEXT("Yes") : TEXT("No"));
		}
		return false;
	}

	// Check weapon restrictions
	// if (!IsWeaponAllowedForVault())
	// {
	// 	if (bDebugVault)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Current weapon not allowed for vaulting"));
	// 	}
	// 	return false;
	// }

	// Perform vault detection traces
	FVaultTraceResult VaultData = PerformVaultTraces(Character);
	
	if (bDebugVault)
	{
		DrawVaultDebug(VaultData, VaultData.bCanVault);
	}

	if (!VaultData.bCanVault)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Verbose, TEXT("GA_Vault: No valid vault detected"));
		}
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("GA_Vault: ✅ Valid vault detected! Height: %.1f, Distance: %.1f"), 
		VaultData.ObstacleHeight, VaultData.ObstacleDistance);
	
	return true;
}

void UGA_Vault::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Log, TEXT("GA_Vault: ActivateAbility called"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: CommitAbility failed"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: Character is NULL"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Re-perform traces to get fresh vault data
	FVaultTraceResult VaultData = PerformVaultTraces(Character);
	
	if (!VaultData.bCanVault)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: Vault data invalid at activation time"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ExecuteVault(VaultData);
}

// ====================================================================
// VAULT DETECTION LOGIC
// ====================================================================

FVaultTraceResult UGA_Vault::PerformVaultTraces(const AMYYCharacterBase* Character) const
{
	FVaultTraceResult Result;
	Result.bCanVault = false;

	if (!Character)
	{
		return Result;
	}

	// Step 1: Trace forward from chest height to detect obstacle
	FHitResult ObstacleHit;
	if (!TraceForObstacle(Character, ObstacleHit))
	{
		return Result; // No obstacle found
	}

	// Step 2: Find the top surface of the obstacle
	FVector ForwardDir = Character->GetActorForwardVector();
	FVector ObstacleTop;
	float ObstacleHeight;
	
	if (!FindObstacleTop(ObstacleHit.ImpactPoint, ForwardDir, ObstacleTop, ObstacleHeight))
	{
		return Result; // Can't find top or invalid height
	}

	// Step 3: Check obstacle thickness (prevent vaulting through thin walls)
	if (!CheckObstacleThickness(ObstacleTop, ForwardDir))
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Obstacle too thin (< %.1f cm)"), MinObstacleThickness);
		}
		return Result;
	}

	// Step 4: Validate landing spot on the other side
	FVector LandingLocation;
	if (!ValidateLandingSpot(ObstacleTop, ForwardDir, LandingLocation))
	{
		return Result; // No valid landing spot
	}

	// All checks passed!
	Result.bCanVault = true;
	Result.ObstacleTopLocation = ObstacleTop;
	Result.LandingLocation = LandingLocation;
	Result.ObstacleNormal = ObstacleHit.ImpactNormal;
	Result.ObstacleHeight = ObstacleHeight;
	Result.ObstacleDistance = FVector::Dist2D(Character->GetActorLocation(), ObstacleHit.ImpactPoint);

	return Result;
}

bool UGA_Vault::TraceForObstacle(const AMYYCharacterBase* Character, FHitResult& OutHit) const
{
	if (!Character)
	{
		return false;
	}

	// ✅ FIX: Start from WAIST height, not chest (more reliable)
	FVector CharLocation = Character->GetActorLocation();
	float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	// Start at waist level (slightly below center)
	FVector StartLocation = CharLocation + FVector(0, 0, -CapsuleHalfHeight * 0.3f);
	FVector EndLocation = StartLocation + (Character->GetActorForwardVector() * ForwardTraceDistance);

	// Use capsule trace to match character collision
	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(TraceCapsuleRadius, TraceCapsuleHalfHeight);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.bTraceComplex = false;

	bool bHit = Character->GetWorld()->SweepSingleByChannel(
		OutHit,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_Visibility,
		CapsuleShape,
		QueryParams
	);

	// Debug visualization
	if (bDebugVault)
	{
		FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugCapsule(
			Character->GetWorld(),
			bHit ? OutHit.ImpactPoint : EndLocation,
			TraceCapsuleHalfHeight,
			TraceCapsuleRadius,
			FQuat::Identity,
			DebugColor,
			false,
			DebugDrawTime,
			0,
			2.0f
		);
		
		DrawDebugLine(
			Character->GetWorld(),
			StartLocation,
			bHit ? OutHit.ImpactPoint : EndLocation,
			DebugColor,
			false,
			DebugDrawTime,
			0,
			3.0f
		);

		if (bHit)
		{
			// Draw hit normal
			DrawDebugDirectionalArrow(
				Character->GetWorld(),
				OutHit.ImpactPoint,
				OutHit.ImpactPoint + (OutHit.ImpactNormal * 50.f),
				20.f,
				FColor::Orange,
				false,
				DebugDrawTime,
				0,
				2.0f
			);
		}
	}

	return bHit && OutHit.bBlockingHit;
}

bool UGA_Vault::FindObstacleTop(const FVector& ObstacleHitLocation, const FVector& ForwardDir, 
	FVector& OutTopLocation, float& OutHeight) const
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	// ✅ IMPROVED APPROACH: Multi-step trace to find top edge reliably
	
	// Step 1: Find how high the obstacle goes (trace upward from hit point)
	FVector UpwardStart = ObstacleHitLocation + FVector(0, 0, 10.f);
	FVector UpwardEnd = UpwardStart + FVector(0, 0, MaxVaultHeight * 1.5f);
	
	FHitResult UpwardHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.bTraceComplex = true;

	// Trace upward along the wall to find where it ends
	bool bFoundEdge = Character->GetWorld()->LineTraceSingleByChannel(
		UpwardHit,
		UpwardStart,
		UpwardEnd,
		ECC_Visibility,
		QueryParams
	);

	// Step 2: Position above the obstacle and trace down to find top surface
	FVector TopSearchStart;
	
	if (bFoundEdge && UpwardHit.bBlockingHit)
	{
		// Found the edge going up - start from there
		TopSearchStart = UpwardHit.ImpactPoint + FVector(0, 0, 20.f) + (ForwardDir * 30.f);
	}
	else
	{
		// Didn't hit edge - start from max height
		TopSearchStart = UpwardStart + FVector(0, 0, MaxVaultHeight) + (ForwardDir * 30.f);
	}

	// Trace down to find the actual top walkable surface
	FVector TopSearchEnd = TopSearchStart - FVector(0, 0, MaxVaultHeight + 100.f);
	
	FHitResult TopHit;
	bool bFoundTop = Character->GetWorld()->LineTraceSingleByChannel(
		TopHit,
		TopSearchStart,
		TopSearchEnd,
		ECC_Visibility,
		QueryParams
	);

	// ✅ DEBUG: Enhanced visualization
	if (bDebugVault)
	{
		// Draw upward trace (magenta)
		DrawDebugLine(
			Character->GetWorld(),
			UpwardStart,
			bFoundEdge ? UpwardHit.ImpactPoint : UpwardEnd,
			FColor::Magenta,
			false,
			DebugDrawTime,
			0,
			3.0f
		);

		// Draw downward trace (yellow/orange)
		DrawDebugLine(
			Character->GetWorld(),
			TopSearchStart,
			bFoundTop ? TopHit.ImpactPoint : TopSearchEnd,
			bFoundTop ? FColor::Yellow : FColor::Orange,
			false,
			DebugDrawTime,
			0,
			3.0f
		);

		if (bFoundTop)
		{
			// Draw top surface point (cyan sphere)
			DrawDebugSphere(
				Character->GetWorld(),
				TopHit.ImpactPoint,
				15.0f,
				12,
				FColor::Cyan,
				false,
				DebugDrawTime,
				0,
				3.0f
			);

			// Draw height measurement
			float CharacterFeetZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector FeetLocation = FVector(TopHit.ImpactPoint.X, TopHit.ImpactPoint.Y, CharacterFeetZ);
			
			DrawDebugLine(
				Character->GetWorld(),
				FeetLocation,
				TopHit.ImpactPoint,
				FColor::White,
				false,
				DebugDrawTime,
				0,
				4.0f
			);

			float MeasuredHeight = TopHit.ImpactPoint.Z - CharacterFeetZ;
			DrawDebugString(
				Character->GetWorld(),
				(FeetLocation + TopHit.ImpactPoint) * 0.5f,
				FString::Printf(TEXT("%.1f cm"), MeasuredHeight),
				nullptr,
				FColor::Yellow,
				DebugDrawTime,
				true,
				1.2f
			);
		}
	}

	if (!bFoundTop)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Could not find obstacle top surface"));
		}
		return false;
	}

	// Calculate height from character's feet
	float CharacterFeetZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	OutHeight = TopHit.ImpactPoint.Z - CharacterFeetZ;

	// ✅ RELAXED VALIDATION: More lenient height check
	// Allow negative heights (step down) up to a small amount
	if (OutHeight < -20.f) // Allow small step-downs
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Obstacle too low (%.1f cm below feet)"), OutHeight);
		}
		return false;
	}

	if (OutHeight < MinVaultHeight)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Obstacle height %.1f below minimum %.1f"), 
				OutHeight, MinVaultHeight);
		}
		return false;
	}

	if (OutHeight > MaxVaultHeight)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Obstacle height %.1f above maximum %.1f"), 
				OutHeight, MaxVaultHeight);
		}
		return false;
	}

	OutTopLocation = TopHit.ImpactPoint;
	
	if (bDebugVault)
	{
		UE_LOG(LogTemp, Log, TEXT("GA_Vault: ✅ Valid obstacle - Height: %.1f cm, Top Z: %.1f, Feet Z: %.1f"), 
			OutHeight, TopHit.ImpactPoint.Z, CharacterFeetZ);
	}
	
	return true;
}

bool UGA_Vault::CheckObstacleThickness(const FVector& ObstacleTop, const FVector& ForwardDir) const
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	// Trace through the obstacle to measure its thickness
	FVector TraceStart = ObstacleTop + (ForwardDir * 10.f); // Start slightly into obstacle
	FVector TraceEnd = TraceStart + (ForwardDir * 200.f);

	FHitResult ThicknessHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	bool bHit = Character->GetWorld()->LineTraceSingleByChannel(
		ThicknessHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (!bHit)
	{
		// No back face found - obstacle might be too thick or infinite
		return false;
	}

	float Thickness = FVector::Dist(ObstacleTop, ThicknessHit.ImpactPoint);

	if (bDebugVault)
	{
		DrawDebugLine(
			Character->GetWorld(),
			TraceStart,
			ThicknessHit.ImpactPoint,
			Thickness >= MinObstacleThickness ? FColor::Purple : FColor::Red,
			false,
			DebugDrawTime,
			0,
			2.0f
		);
	}

	return Thickness >= MinObstacleThickness;
}


bool UGA_Vault::ValidateLandingSpot(const FVector& ObstacleTop, const FVector& ForwardDir, 
	FVector& OutLandingLocation) const
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	// ✅ FIX: Start trace from HIGHER up and go further forward
	// This ensures we clear the obstacle completely and find ground on the other side
	
	// Calculate character's ground level for reference
	float CharacterGroundZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	// Start trace well beyond and above the obstacle
	FVector LandingCheckStart = ObstacleTop 
		+ (ForwardDir * (LandingCheckDistance + 50.f))  // Go further forward (150cm total)
		+ FVector(0, 0, 100.f);  // Start higher to ensure we clear obstacle
	
	// Trace down to find ground - go deep enough to find ground level
	FVector LandingCheckEnd = LandingCheckStart - FVector(0, 0, MaxLandingDepth + 100.f);

	FHitResult LandingHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.bTraceComplex = true;

	bool bFoundLanding = Character->GetWorld()->LineTraceSingleByChannel(
		LandingHit,
		LandingCheckStart,
		LandingCheckEnd,
		ECC_Visibility,
		QueryParams
	);

	if (bDebugVault)
	{
		DrawDebugLine(
			Character->GetWorld(),
			LandingCheckStart,
			bFoundLanding ? LandingHit.ImpactPoint : LandingCheckEnd,
			bFoundLanding ? FColor::Green : FColor::Red,
			false,
			DebugDrawTime,
			0,
			3.0f
		);

		if (bFoundLanding)
		{
			DrawDebugSphere(
				Character->GetWorld(),
				LandingHit.ImpactPoint,
				15.0f,
				12,
				FColor::Emerald,
				false,
				DebugDrawTime
			);
			
			// Draw comparison line to character's ground level
			FVector CharGroundAtLanding = FVector(LandingHit.ImpactPoint.X, LandingHit.ImpactPoint.Y, CharacterGroundZ);
			DrawDebugLine(
				Character->GetWorld(),
				CharGroundAtLanding,
				LandingHit.ImpactPoint,
				FColor::White,
				false,
				DebugDrawTime,
				0,
				2.0f
			);
		}
	}

	if (!bFoundLanding)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: No landing surface found"));
		}
		return false;
	}

	// ✅ CRITICAL: Ensure landing is at ground level (similar to starting position)
	// Landing should be within 30cm of character's starting ground height
	float LandingGroundDiff = FMath::Abs(LandingHit.ImpactPoint.Z - CharacterGroundZ);
	
	if (LandingGroundDiff > 30.f)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Landing not at ground level (%.1f cm difference, Start Z: %.1f, Landing Z: %.1f)"), 
				LandingGroundDiff, CharacterGroundZ, LandingHit.ImpactPoint.Z);
		}
		// Still allow but log warning - might be ramp/stairs
	}

	// ✅ NEW CHECK: Make sure we're not landing ON TOP of the obstacle
	// Landing should be significantly different from obstacle top
	float HeightAboveObstacle = LandingHit.ImpactPoint.Z - ObstacleTop.Z;
	
	if (FMath::Abs(HeightAboveObstacle) < 20.f)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Landing is on obstacle top (%.1f cm diff), not ground!"), 
				HeightAboveObstacle);
		}
		return false; // Reject - landing on obstacle itself
	}

	// Check landing isn't too far below obstacle (cliff detection)
	float DropHeight = ObstacleTop.Z - LandingHit.ImpactPoint.Z;
	if (DropHeight > 150.f)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Landing too far below obstacle (%.1f cm drop)"), DropHeight);
		}
		return false;
	}

	// Check landing surface is walkable
	if (LandingHit.ImpactNormal.Z < 0.7f)
	{
		if (bDebugVault)
		{
			UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Landing surface too steep (Normal.Z = %.2f)"), LandingHit.ImpactNormal.Z);
		}
		return false;
	}

	OutLandingLocation = LandingHit.ImpactPoint;
	
	if (bDebugVault)
	{
		UE_LOG(LogTemp, Log, TEXT("GA_Vault: ✅ Valid landing - Z: %.1f (Ground: %.1f, Diff: %.1f cm, Above Obstacle: %.1f cm)"),
			LandingHit.ImpactPoint.Z, CharacterGroundZ, LandingGroundDiff, HeightAboveObstacle);
	}
	
	return true;
}

// ====================================================================
// MODIFY GetVaultMontage() function in GA_Vault.cpp
// ====================================================================

UAnimMontage* UGA_Vault::GetVaultMontage() const
{
	UE_LOG(LogTemp, Log, TEXT("GA_Vault: GetVaultMontage called"));

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: Character is NULL in GetVaultMontage"));
		return nullptr;
	}

	UEquipmentComponent* EquipComp = Character->FindComponentByClass<UEquipmentComponent>();
	
	UWeaponDataAsset* WeaponData = EquipComp->GetActiveWeaponData();
	if (!WeaponData)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: No active WeaponData"));
		return nullptr;
	}

	if (!WeaponData->Animations.VaultMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: WeaponData has no VaultMontage set!"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("GA_Vault: Using weapon vault montage: %s"), 
		*WeaponData->Animations.VaultMontage->GetName());
    
	return WeaponData->Animations.VaultMontage;
}

// ====================================================================	
// VAULT EXECUTION	
// ====================================================================	

void UGA_Vault::ExecuteVault(const FVaultTraceResult& VaultData)
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: Character is NULL in ExecuteVault"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
 
	UAnimMontage* VaultMontage = GetVaultMontage();

	if (!VaultMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: VaultMontage is NULL! Assign a vault animation."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	UMotionWarpingComponent* MotionWarpComp = Character->FindComponentByClass<UMotionWarpingComponent>();
	if (MotionWarpComp)
	{
		FMotionWarpingTarget WarpTarget;
		WarpTarget.Name = WarpTargetName;
		WarpTarget.Location = VaultData.LandingLocation;
		WarpTarget.Rotation = Character->GetActorRotation();
	
		MotionWarpComp->AddOrUpdateWarpTarget(WarpTarget);
		
		UE_LOG(LogTemp, Log, TEXT("GA_Vault: Motion Warp target set to %s"), *VaultData.LandingLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Vault: No MotionWarpingComponent found! Vault will not use motion warping"));
	}

	// Rotate character to face obstacle
	FVector ToObstacle = (VaultData.ObstacleTopLocation - Character->GetActorLocation()).GetSafeNormal2D();
	FRotator NewRotation = ToObstacle.Rotation();
	Character->SetActorRotation(NewRotation);

	// Play vault montage
	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		VaultMontage,
		VaultPlayRate,
		NAME_None,
		true,
		1.0f
	);

	if (!PlayMontageTask)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Vault: Failed to create PlayMontageTask"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_Vault::OnVaultMontageCompleted);
	PlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_Vault::OnVaultMontageCompleted);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_Vault::OnVaultMontageCancelled);
	PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_Vault::OnVaultMontageCancelled);

	PlayMontageTask->ReadyForActivation();
	
	UE_LOG(LogTemp, Log, TEXT("GA_Vault: Vault montage playing successfully"));
}

void UGA_Vault::OnVaultMontageCompleted()
{
	UE_LOG(LogTemp, Log, TEXT("GA_Vault: Vault completed successfully"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Vault::OnVaultMontageCancelled()
{
	UE_LOG(LogTemp, Warning, TEXT("GA_Vault: Vault was cancelled/interrupted"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ====================================================================
// DEBUG VISUALIZATION
// ====================================================================

void UGA_Vault::DrawVaultDebug(const FVaultTraceResult& VaultData, bool bSuccess) const
{
	if (!bDebugVault)
	{
		return;
	}

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->GetWorld())
	{
		return;
	}

	// Draw overall status at character location
	FVector CharLocation = Character->GetActorLocation() + FVector(0, 0, 100);		
	FString StatusText = bSuccess ? TEXT("VAULT: READY") : TEXT("VAULT: BLOCKED");		
	FColor StatusColor = bSuccess ? FColor::Green : FColor::Red;		
	
	DrawDebugString(
		Character->GetWorld(),
		CharLocation,
		StatusText,
		nullptr,
		StatusColor,
		DebugDrawTime,
		true,
		1.5f
	);

	if (bSuccess)
	{
		// Draw path visualization
		DrawDebugLine(
			Character->GetWorld(),
			Character->GetActorLocation(),
			VaultData.ObstacleTopLocation,
			FColor::Cyan,
			false,
			DebugDrawTime,
			0,
			4.0f
		);

		DrawDebugLine(
			Character->GetWorld(),
			VaultData.ObstacleTopLocation,
			VaultData.LandingLocation,
			FColor::Green,
			false,
			DebugDrawTime,
			0,
			4.0f
		);

		// Draw info text
		FString InfoText = FString::Printf(TEXT("Height: %.0f | Dist: %.0f"), 
			VaultData.ObstacleHeight, VaultData.ObstacleDistance);
		
		DrawDebugString(
			Character->GetWorld(),
			VaultData.ObstacleTopLocation + FVector(0, 0, 30),
			InfoText,
			nullptr,
			FColor::White,
			DebugDrawTime,
			true
		);
	}
}