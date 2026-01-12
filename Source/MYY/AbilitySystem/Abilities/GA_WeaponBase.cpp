// GA_WeaponBase.cpp
#include "GA_WeaponBase.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "AbilitySystemComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"

UGA_WeaponBase::UGA_WeaponBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_WeaponBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// // Cache weapon data from SourceObject										/********************************************
	CachedWeaponData = Cast<UWeaponDataAsset>(Spec.SourceObject.Get());
	//
	// DEBUG: Verify PDA is set
	// if (CachedWeaponData)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("[%s] Cached Weapon Data FROM GA_WEAPON: %s (InputID: %d)"),
	// 		*GetClass()->GetName(),
	// 		*CachedWeaponData->GetName(),
	// 		Spec.InputID);
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("[%s] FAILED TO CACHE WEAPON DATA FROM GA_WEAPON!"), 
	// 		*GetClass()->GetName());
	// }
	
}

bool UGA_WeaponBase::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{

	// First check stamina
	if (!CheckStaminaCost())
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(FGameplayTag::RequestGameplayTag("Ability.Failed.NoStamina"));
		}
        
		UE_LOG(LogTemp, Warning, TEXT("Cannot activate ability %s: Not enough stamina"), 
			*GetClass()->GetName());
		return false;
	}
	
	// Don't return false by default - check weapon data
	if (!CachedWeaponData)
	{
		UE_LOG(LogTemp, Log, TEXT("GA_WeaponBase: No weapon data cached"));
		return false;
	}
	
	// Default check - weapon exists and character can use it
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor);
	if (!Character || !Character->GetAbilitySystemComponent())
	{
		return false;
	}
	
	// Base implementation returns true if weapon data exists
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

  
bool UGA_WeaponBase::CheckStaminaCost() const
{
	if (!bCheckStaminaBeforeActivate || RequiredStaminaCost <= 0.f)
	{
		return true;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		float CurrentStamina = ASC->GetNumericAttribute(UAttributeSetBase::GetStaminaAttribute());
		return CurrentStamina >= RequiredStaminaCost;
	}
    
	return false;
}

void UGA_WeaponBase::ApplyCost(const FGameplayAbilitySpecHandle Handle,
							   const FGameplayAbilityActorInfo* ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!bCheckStaminaBeforeActivate || RequiredStaminaCost <= 0.f)
		return;

	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		// Apply simple stamina reduction (instant)
		ASC->ApplyModToAttribute(UAttributeSetBase::GetStaminaAttribute(), 
								 EGameplayModOp::Additive, 
								 -RequiredStaminaCost);
	}
}
 
ABaseWeapon* UGA_WeaponBase::GetCurrentWeapon() const
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->EquipmentComponent) return nullptr;

	return Character->EquipmentComponent->GetCurrentWeapon();
}


