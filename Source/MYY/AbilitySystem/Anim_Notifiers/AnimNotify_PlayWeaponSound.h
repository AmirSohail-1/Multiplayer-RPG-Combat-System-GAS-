// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h" 
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayWeaponSound.generated.h"



UENUM(BlueprintType)
enum class EWeaponSoundType : uint8
{
	Swing, Impact, Draw, Sheathe
};


/**
 * 
 */
UCLASS()
class MYY_API UAnimNotify_PlayWeaponSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(  USkeletalMeshComponent* MeshComp,	UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Sound")
	EWeaponSoundType SoundType = EWeaponSoundType::Swing;

	UPROPERTY(EditAnywhere, Category = "Sound")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Sound")
	float PitchMultiplier = 1.0f;
};
