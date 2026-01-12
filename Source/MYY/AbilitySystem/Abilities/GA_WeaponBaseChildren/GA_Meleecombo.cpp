#include "GA_Meleecombo.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

UGA_Meleecombo::UGA_Meleecombo()
{
	CurrentComboIndex = 0;
	bComboWindowOpen = false;
	bHasBufferedInput = false;
	bUseRandomCombo = false;

	// Set stamina cost for melee combo
	bCheckStaminaBeforeActivate = true;
	RequiredStaminaCost = 10.0f; // Default cost, can be overridden per weapon
 
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Attack")));
	BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Defense.Blocking")));
	BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Action.Swap")));
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Attack")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Action.Dodge")));

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UGA_Meleecombo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UGA_Meleecombo, CurrentComboIndex);
	DOREPLIFETIME(UGA_Meleecombo, bComboWindowOpen);
}

void UGA_Meleecombo::ServerHandleComboInput_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[SERVER RPC] Handling combo input"));
    
	if (bComboWindowOpen)
	{
		TriggerNextCombo();
	}
	else
	{
		bHasBufferedInput = true;
	}
}

 

 

void UGA_Meleecombo::OnRep_CurrentComboIndex()
{
}

void UGA_Meleecombo::OnRep_bComboWindowOpen()
{
}


void UGA_Meleecombo::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
									 const FGameplayAbilityActorInfo* ActorInfo,
									 const FGameplayAbilityActivationInfo ActivationInfo,
									 const FGameplayEventData* TriggerEventData)
{
	// ✅ ADD THIS FIRST
	UE_LOG(LogTemp, Error, TEXT("🔥🔥🔥 GA_Meleecombo::ActivateAbility CALLED!"));
	UE_LOG(LogTemp, Error, TEXT("   Handle Valid: %s"), Handle.IsValid() ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Error, TEXT("   ActorInfo: %s"), ActorInfo ? TEXT("Valid") : TEXT("NULL"));
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

 
	// ✅ CRITICAL FIX: Get weapon data if not cached
	if (!CachedWeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ GA_MeleeCombo: CachedWeaponData is NULL, attempting to get from equipment"));
        
		AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor);
		if (Character && Character->EquipmentComponent)
		{
			CachedWeaponData = Character->EquipmentComponent->GetActiveWeaponData();
            
			if (CachedWeaponData)
			{
				UE_LOG(LogTemp, Log, TEXT("✅ Retrieved weapon data: %s"), *CachedWeaponData->WeaponName.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ Failed to get weapon data from equipment"));
				EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
				return;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ No character or equipment component"));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	float CurrentStamina = ASC->GetNumericAttribute(UAttributeSetBase::GetStaminaAttribute());

	const float StaminaCost = 10.f; // or get from DataAsset

	if (CurrentStamina < StaminaCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Not enough stamina for MeleeCombo"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedActorInfo = ActorInfo;
	CachedHandle = Handle;
	CachedActivationInfo = ActivationInfo;

	// NOW weapon type check is safe
	if (CachedWeaponData->WeaponType == EWeaponType::Ranged)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_MeleeCombo: Ranged weapons cannot MeleeCombo"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// NOW check weapon type (after verifying CachedWeaponData exists)
	if (CachedWeaponData->WeaponType == EWeaponType::Ranged)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_MeleeCombo: Ranged weapons cannot MeleeCombo"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);  // FIX: Added EndAbility call
		return;
	}	

 
	const auto& ComboArray = CachedWeaponData->Animations.ComboMontages;
	if (ComboArray.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_MeleeCombo: No combo montages in weapon data!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Select montage based on mode
	int32 MontageIndex = bUseRandomCombo ? 
		FMath::RandRange(0, ComboArray.Num() - 1) : 
		CurrentComboIndex % ComboArray.Num();
	
	MontageIndex = FMath::Clamp(MontageIndex, 0, ComboArray.Num() - 1);

	UAnimMontage* AttackMontage = ComboArray[MontageIndex];
	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

 
	
	// Use GAS montage task for automatic replication
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, 1.0f, NAME_None, false);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Meleecombo::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Meleecombo::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Meleecombo::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}


void UGA_Meleecombo::OnMontageCompleted()
{
	FGameplayAbilityActorInfo ActorInfo = GetActorInfo();
	EndAbility(CachedHandle, &ActorInfo, CachedActivationInfo, true, false);
}

void UGA_Meleecombo::OnMontageCancelled()
{
	FGameplayAbilityActorInfo ActorInfo = GetActorInfo();
	EndAbility(CachedHandle, &ActorInfo, CachedActivationInfo, true, true);
}

void UGA_Meleecombo::OnMontageInterrupted()
{
	FGameplayAbilityActorInfo ActorInfo = GetActorInfo();
	EndAbility(CachedHandle, &ActorInfo, CachedActivationInfo, true, true);
}

void UGA_Meleecombo::OpenComboWindow()
{
	if (GetOwningActorFromActorInfo()->HasAuthority())
	{
		bComboWindowOpen = true;

		if (bHasBufferedInput)
		{
			bHasBufferedInput = false;
			TriggerNextCombo();
		}
	}
}

void UGA_Meleecombo::CloseComboWindow()
{
	if (GetOwningActorFromActorInfo()->HasAuthority())
	{
		bComboWindowOpen = false;
		bHasBufferedInput = false;
	}
}

void UGA_Meleecombo::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] InputPressed - ComboWindowOpen: %s, HasBufferedInput: %s, CurrentCombo: %d"), 
		GetOwningActorFromActorInfo()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		bComboWindowOpen ? TEXT("true") : TEXT("false"),
		bHasBufferedInput ? TEXT("true") : TEXT("false"),
		CurrentComboIndex);

	// Call server RPC to handle the input on server
	if (!GetOwningActorFromActorInfo()->HasAuthority())
	{
		ServerHandleComboInput();
	}

	// Also handle locally for client prediction
	if (bComboWindowOpen)
	{
		TriggerNextCombo();
	}
	else
	{
		bHasBufferedInput = true;
	}
}

void UGA_Meleecombo::TriggerNextCombo()
{
	if (!GetOwningActorFromActorInfo())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[%s] TriggerNextCombo - CurrentCombo: %d"), 
		GetOwningActorFromActorInfo()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		CurrentComboIndex);

	// Predict combo index on both client and server
	if (!bUseRandomCombo)
	{
		int32 PredictedComboIndex = CurrentComboIndex + 1;
        
		if (CachedWeaponData && PredictedComboIndex >= CachedWeaponData->Animations.ComboMontages.Num())
		{
			PredictedComboIndex = 0;
		}
        
		if (PredictedComboIndex >= MaxComboChain)
		{
			PredictedComboIndex = 0;
		}
        
		// Server updates the replicated variable
		if (GetOwningActorFromActorInfo()->HasAuthority())
		{
			CurrentComboIndex = PredictedComboIndex;
			UE_LOG(LogTemp, Warning, TEXT("[SERVER] New CurrentComboIndex: %d"), CurrentComboIndex);
		}
		// Client predicts locally (will be corrected by replication if wrong)
		else
		{
			CurrentComboIndex = PredictedComboIndex;
			UE_LOG(LogTemp, Warning, TEXT("[CLIENT PREDICT] New CurrentComboIndex: %d"), CurrentComboIndex);
		}
	}
    
	// Try to activate ability
	if (GetAbilitySystemComponentFromActorInfo())
	{
		GetAbilitySystemComponentFromActorInfo()->TryActivateAbility(CachedHandle, true);
	}
}

void UGA_Meleecombo::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear cached pointer
	CachedActorInfo = nullptr;
	
	if (bWasCancelled && GetOwningActorFromActorInfo()->HasAuthority())
	{
		CurrentComboIndex = 0;
		bComboWindowOpen = false;
		bHasBufferedInput = false;
	}

	if (MontageTask)
	{
		MontageTask = nullptr;
	}

	if (AMYYCharacterBase* Character =
	   Cast<AMYYCharacterBase>(ActorInfo->AvatarActor.Get()))
	{
		Character->ForceStopAllCombatTraces();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}