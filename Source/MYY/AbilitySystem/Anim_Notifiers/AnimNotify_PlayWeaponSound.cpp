// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_PlayWeaponSound.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem/Components/EquipmentComponent.h"
#include "MYY/AbilitySystem/BaseWeapon.h"
#include "Kismet/GameplayStatics.h"


void UAnimNotify_PlayWeaponSound::Notify(  USkeletalMeshComponent* MeshComp,	UAnimSequenceBase* Animation,	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation,EventReference);
	if (!MeshComp) return;

	AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(MeshComp->GetOwner());
	if (!Character || !Character->EquipmentComponent) return;

	ABaseWeapon* Weapon = Character->EquipmentComponent->GetCurrentWeapon();
	if (!Weapon || !Weapon->WeaponData) return;

	USoundBase* SoundToPlay = nullptr;
    
	switch (SoundType)
	{
	case EWeaponSoundType::Swing: SoundToPlay = Weapon->WeaponData->SwingSFX; break;
	case EWeaponSoundType::Impact: SoundToPlay = Weapon->WeaponData->HitSFX; break;
	default: break;
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(
			Character->GetWorld(), SoundToPlay, Character->GetActorLocation(),
			VolumeMultiplier, PitchMultiplier);
	}
}

FString UAnimNotify_PlayWeaponSound::GetNotifyName_Implementation() const
{
	FString TypeName;
	switch (SoundType)
	{
	case EWeaponSoundType::Swing: TypeName = "Swing"; break;
	case EWeaponSoundType::Impact: TypeName = "Impact"; break;
	case EWeaponSoundType::Draw: TypeName = "Draw"; break;
	case EWeaponSoundType::Sheathe: TypeName = "Sheathe"; break;
	}
	return FString::Printf(TEXT("Play Weapon Sound: %s"), *TypeName);
}