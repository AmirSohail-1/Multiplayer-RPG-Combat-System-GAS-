// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"

#include "AnimNotify_EndUnarmedTrace.generated.h"

/**
 * 
 */
UCLASS()
class MYY_API UAnimNotify_EndUnarmedTrace : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(
	   USkeletalMeshComponent* MeshComp,
	   UAnimSequenceBase* Animation) override;
	
	FString GetNotifyName_Implementation() const;
};
