#include "GA_UnequipWeapon.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "MYY/AbilitySystem/DataAsset/WeaponDataAsset.h"

UGA_UnequipWeapon::UGA_UnequipWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	
	// Use SetAssetTags instead of AbilityTags (deprecated)
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.Unequip")));
	SetAssetTags(AssetTags);
}

bool UGA_UnequipWeapon::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Can only unequip if we have a weapon currently equipped
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(ActorInfo->AvatarActor);
	if (!Character || !Character->EquipmentComponent)
	{
		return false;
	}

	// Check if there's a weapon to unequip
	ABaseWeapon* CurrentWeapon = Character->EquipmentComponent->GetCurrentWeapon();
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unequip Ability: No weapon currently equipped"));
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGA_UnequipWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UnequipWeapon();
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_UnequipWeapon::UnequipWeapon()
{
	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->EquipmentComponent)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Unequipping weapon"));
	Character->EquipmentComponent->Server_UnequipWeapon();
}