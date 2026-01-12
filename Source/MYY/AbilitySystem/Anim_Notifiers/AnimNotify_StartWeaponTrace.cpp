// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_StartWeaponTrace.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"

void UAnimNotify_StartWeaponTrace::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(MeshComp->GetOwner());
	if (!Character || !Character->EquipmentComponent) return;

	ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
	if (Weapon)
	{
		Weapon->StartTrace();
	}
}

FString UAnimNotify_StartWeaponTrace::GetNotifyName_Implementation() const
{
	return "Start Weapon Trace";
}
