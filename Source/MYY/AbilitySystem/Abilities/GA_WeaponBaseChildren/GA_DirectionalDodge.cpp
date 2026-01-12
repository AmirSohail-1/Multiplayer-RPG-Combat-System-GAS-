#include "GA_DirectionalDodge.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_DirectionalDodge::UGA_DirectionalDodge()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    bCheckStaminaBeforeActivate = true;
    RequiredStaminaCost = 10.0f;
    
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Movement.Dodge")));
    SetAssetTags(AssetTags);
}

void UGA_DirectionalDodge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: ActivateAbility called"));

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: CommitAbility failed (likely stamina or cooldown)"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: CommitAbility succeeded, calling PerformDodge"));
    PerformDodge();
}

bool UGA_DirectionalDodge::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: CanActivateAbility called"));

    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Super::CanActivateAbility returned false"));
        return false;
    }

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor);
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: Character is NULL"));
        return false;
    }

    if (!Character->GetCharacterMovement())
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: CharacterMovement is NULL"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: CanActivateAbility passed all checks"));
    return true;
}

void UGA_DirectionalDodge::PerformDodge()
{
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: PerformDodge started"));

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: Character is NULL in PerformDodge"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // Calculate dodge direction based on input
    EDodgeDirection DodgeDir = CalculateDodgeDirection();
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Calculated dodge direction: %d"), static_cast<int32>(DodgeDir));
    
    // Rotate character to face dodge direction
    RotateCharacterToDodgeDirection(Character, DodgeDir);
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Rotated character to dodge direction"));
    
    // Get the single dodge/roll montage
    UAnimMontage* MontageToPlay = GetDodgeMontage();
    
    if (!MontageToPlay)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: No dodge montage found! Check WeaponDataAsset or DefaultDodgeMontage"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Found montage to play: %s"), *MontageToPlay->GetName());

    // Play montage using ability task for proper replication
    UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this,
        NAME_None,
        MontageToPlay,
        1.0f,
        NAME_None,
        true,
        1.0f
    );

    if (!PlayMontageTask)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: Failed to create PlayMontageTask"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: PlayMontageTask created, binding delegates"));

    PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_DirectionalDodge::OnMontageCompleted);
    PlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_DirectionalDodge::OnMontageCompleted);
    PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_DirectionalDodge::OnMontageCancelled);
    PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_DirectionalDodge::OnMontageCancelled);

    PlayMontageTask->ReadyForActivation();
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Montage task activated successfully"));

    // Apply movement if not using root motion
    if (!bUseRootMotion)
    {
        UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Applying velocity-based dodge movement"));
        ApplyDodgeMovement(Character, DodgeDir);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Using root motion from montage"));
    }
}

EDodgeDirection UGA_DirectionalDodge::CalculateDodgeDirection() const
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character || !Character->GetController())
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: CalculateDodgeDirection - Character or Controller is NULL"));
        return EDodgeDirection::Backward;
    }

    // Get the ACTUAL input velocity from character movement
    FVector InputVector = Character->GetLastMovementInputVector();
    
    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: LastMovementInputVector: %s"), *InputVector.ToString());

    // If no input, default to backward dodge
    if (InputVector.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: No input detected, defaulting to BACKWARD"));
        return EDodgeDirection::Backward;
    }

    // Get controller to determine forward/right relative to camera
    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: PlayerController is NULL"));
        return EDodgeDirection::Backward;
    }

    FRotator ControlRotation = PC->GetControlRotation();
    ControlRotation.Pitch = 0.0f;
    ControlRotation.Roll = 0.0f;

    FVector CameraForward = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
    FVector CameraRight = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);

    // Project input onto camera axes
    float ForwardDot = FVector::DotProduct(InputVector, CameraForward);
    float RightDot = FVector::DotProduct(InputVector, CameraRight);

    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: ForwardDot: %f, RightDot: %f"), ForwardDot, RightDot);

    // Determine primary direction based on stronger component
    float AbsForward = FMath::Abs(ForwardDot);
    float AbsRight = FMath::Abs(RightDot);

    EDodgeDirection ResultDirection;

    if (AbsForward > AbsRight)
    {
        ResultDirection = (ForwardDot > 0) ? EDodgeDirection::Forward : EDodgeDirection::Backward;
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Forward/Backward stronger - Direction: %s"), 
            ResultDirection == EDodgeDirection::Forward ? TEXT("FORWARD") : TEXT("BACKWARD"));
    }
    else
    {
        ResultDirection = (RightDot > 0) ? EDodgeDirection::Right : EDodgeDirection::Left;
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Left/Right stronger - Direction: %s"), 
            ResultDirection == EDodgeDirection::Right ? TEXT("RIGHT") : TEXT("LEFT"));
    }

    return ResultDirection;
}

UAnimMontage* UGA_DirectionalDodge::GetDodgeMontage() const
{
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: GetDodgeMontage called"));

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: Character is NULL in GetDodgeMontage"));
        return nullptr;
    }

    // Priority 1: Try to get weapon-specific dodge montage
    UEquipmentComponent* EquipComp = Character->FindComponentByClass<UEquipmentComponent>();
    if (!EquipComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: No EquipmentComponent found, using fallback"));
        return DefaultDodgeMontage;
    }

    UWeaponDataAsset* WeaponData = EquipComp->GetActiveWeaponData();
    if (!WeaponData)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: No active WeaponData, using fallback"));
        return DefaultDodgeMontage;
    }

    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Found WeaponData: %s"), *WeaponData->WeaponName.ToString());

    if (WeaponData->Animations.DodgeMontage)
    {
        UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Using weapon dodge montage: %s"), 
            *WeaponData->Animations.DodgeMontage->GetName());
        return WeaponData->Animations.DodgeMontage;
    }

    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: WeaponData has no DodgeMontage set, using fallback"));
    
    // Priority 2: Use ability-defined fallback montage
    if (DefaultDodgeMontage)
    {
        UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Using DefaultDodgeMontage: %s"), 
            *DefaultDodgeMontage->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GA_DirectionalDodge.cpp: DefaultDodgeMontage is also NULL!"));
    }

    return DefaultDodgeMontage;
}

void UGA_DirectionalDodge::RotateCharacterToDodgeDirection(AMYYCharacterBase* Character, EDodgeDirection Direction)
{
    if (!Character)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (!PC)
    {
        return;
    }

    // Get camera/control rotation to determine world-space directions
    FRotator ControlRotation = PC->GetControlRotation();
    ControlRotation.Pitch = 0.0f;
    ControlRotation.Roll = 0.0f;

    FRotator NewRotation = ControlRotation;
    
    switch (Direction)
    {
        case EDodgeDirection::Forward:
            // Roll in camera forward direction
            break;
        case EDodgeDirection::Backward:
            // Roll opposite to camera direction
            NewRotation.Yaw += 180.0f;
            break;
        case EDodgeDirection::Left:
            // Roll to camera left
            NewRotation.Yaw -= 90.0f;
            break;
        case EDodgeDirection::Right:
            // Roll to camera right
            NewRotation.Yaw += 90.0f;
            break;
    }

    Character->SetActorRotation(NewRotation);
}

void UGA_DirectionalDodge::ApplyDodgeMovement(AMYYCharacterBase* Character, EDodgeDirection Direction)
{
    if (!Character)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (!PC)
    {
        return;
    }

    // Get camera rotation for direction calculation
    FRotator ControlRotation = PC->GetControlRotation();
    ControlRotation.Pitch = 0.0f;
    ControlRotation.Roll = 0.0f;

    FVector DodgeDirectionVec = FVector::ZeroVector;
    
    // Calculate dodge direction based on camera orientation
    FVector CameraForward = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
    FVector CameraRight = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
    
    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Camera Forward: %s, Camera Right: %s"), 
        *CameraForward.ToString(), *CameraRight.ToString());
    
    switch (Direction)
    {
        case EDodgeDirection::Forward:
            DodgeDirectionVec = CameraForward;
            UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Applying FORWARD dodge velocity"));
            break;
        case EDodgeDirection::Backward:
            DodgeDirectionVec = -CameraForward;
            UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Applying BACKWARD dodge velocity"));
            break;
        case EDodgeDirection::Left:
            DodgeDirectionVec = -CameraRight;
            UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Applying LEFT dodge velocity"));
            break;
        case EDodgeDirection::Right:
            DodgeDirectionVec = CameraRight;
            UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Applying RIGHT dodge velocity"));
            break;
        default:
            DodgeDirectionVec = -CameraForward;
            UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Applying DEFAULT (backward) dodge velocity"));
            break;
    }

    DodgeDirectionVec.Normalize();
    FVector DodgeVelocity = DodgeDirectionVec * DodgeDistance / DodgeDuration;

    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Final Dodge Direction Vec: %s"), *DodgeDirectionVec.ToString());
    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Final Dodge Velocity: %s (Distance: %f, Duration: %f)"), 
        *DodgeVelocity.ToString(), DodgeDistance, DodgeDuration);

    Character->GetCharacterMovement()->Velocity = DodgeVelocity;
    Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);

    FTimerHandle DodgeTimerHandle;
    FTimerDelegate DodgeEndDelegate;
    DodgeEndDelegate.BindUObject(this, &UGA_DirectionalDodge::OnDodgeComplete);
    
    Character->GetWorld()->GetTimerManager().SetTimer(
        DodgeTimerHandle, 
        DodgeEndDelegate, 
        DodgeDuration, 
        false
    );
}

void UGA_DirectionalDodge::OnDodgeComplete()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    }
}

void UGA_DirectionalDodge::OnMontageCompleted()
{
     
    UE_LOG(LogTemp, Log, TEXT("GA_DirectionalDodge.cpp: Montage completed successfully"));
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_DirectionalDodge::OnMontageCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("GA_DirectionalDodge.cpp: Montage was cancelled/interrupted"));
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}