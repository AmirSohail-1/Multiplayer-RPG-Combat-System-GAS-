// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_BlockWindow.generated.h"
/**
 * Manages block/parry timing windows in animations
 * Networked and works for both AI and Players
 */
UCLASS()
class MYY_API UAnimNotifyState_BlockWindow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	UAnimNotifyState_BlockWindow();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Window")
	bool bIsParryWindow = true; // If true, this is a parry window; if false, regular block

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Window")
	float WindowDuration = 0.3f; // Duration of the window

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Window")
	FGameplayTag BlockWindowTag;
  
};
