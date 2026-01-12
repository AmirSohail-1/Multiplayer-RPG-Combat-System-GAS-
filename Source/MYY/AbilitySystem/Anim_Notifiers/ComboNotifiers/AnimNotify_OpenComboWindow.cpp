// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_OpenComboWindow.h"
#include "AnimNotify_OpenComboWindow.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Abilities/GA_WeaponBaseChildren/GA_Meleecombo.h"
#include "AbilitySystemComponent.h"

void UAnimNotify_OpenComboWindow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(MeshComp->GetOwner());
	if (!Character || !Character->GetAbilitySystemComponent()) return;

	// Only execute on server
	if (!Character->HasAuthority()) return;

	FGameplayAbilitySpec* Spec = Character->GetAbilitySystemComponent()->FindAbilitySpecFromClass(UGA_Meleecombo::StaticClass());
	if (Spec && Spec->IsActive())
	{
		UGA_Meleecombo* MeleeAbility = Cast<UGA_Meleecombo>(Spec->GetPrimaryInstance());
		if (MeleeAbility)
		{
			MeleeAbility->OpenComboWindow();
		}
	}
}

FString UAnimNotify_OpenComboWindow::GetNotifyName_Implementation() const
{
	return "Open Combo Window";
}