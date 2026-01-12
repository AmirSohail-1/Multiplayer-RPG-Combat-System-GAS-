#include "GA_EquipWeapon.h"

#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"

UGA_EquipWeapon::UGA_EquipWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	
	// Use SetAssetTags instead of AbilityTags
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.Equip")));
	SetAssetTags(AssetTags);
}

bool UGA_EquipWeapon::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle, 
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayTagContainer* SourceTags, 
	const FGameplayTagContainer* TargetTags, 
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Can equip if we have weapon data and no weapon is currently equipped
	if (!CachedWeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Equip Ability: No weapon data cached"));
		return false;
	}

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor);
	if (!Character || !Character->EquipmentComponent)
	{
		return false;
	}

	// Check if we're already holding this weapon
	if (Character->EquipmentComponent->GetCurrentWeapon() && 
		Character->EquipmentComponent->GetCurrentWeapon()->WeaponData == CachedWeaponData)
	{
		return false;
	}

	// IMPORTANT: Add the return statement!
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGA_EquipWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle, 
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo, 
	const FGameplayEventData* TriggerEventData) // Changed from FGameplayTagContainer*
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EquipWeapon();
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_EquipWeapon::EquipWeapon()
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->EquipmentComponent || !CachedWeaponData)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Equipping weapon: %s"), *CachedWeaponData->GetName());
	// Character->EquipmentComponent->Server_EquipWeapon(CachedWeaponData);												**************************```
}