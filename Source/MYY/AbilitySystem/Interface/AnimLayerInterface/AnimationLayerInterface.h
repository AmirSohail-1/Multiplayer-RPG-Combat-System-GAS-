// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Animation/AnimInstance.h"
#include "AnimationLayerInterface.generated.h"


UINTERFACE(MinimalAPI, Blueprintable)
class UAnimationLayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Animation Layer Interface for dynamic weapon animation switching
 * Implement this in your weapon-specific Animation Blueprints (ABP_Sword, ABP_Bow, ABP_Unarmed)
 */

class MYY_API IAnimationLayerInterface
{
	GENERATED_BODY()

public:
	/**
	 * Full Body Layer - Used for melee weapons (Sword, Unarmed)
	 * Replaces all base animations including locomotion
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation Layers")
	void FullBodyLayer(const FPoseLink& InPose, FPoseLink& FullBodyLayer);

	/**
	 * Upper Body Layer - Used for ranged weapons (Bow, Gun)
	 * Only affects upper body, allows shooting while moving
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation Layers")
	void UpperBodyLayer(const FPoseLink& InPose, FPoseLink& UpperBodyLayer);

	/**
	 * Lower Body Layer - Used for ranged weapons (Bow, Gun)
	 * Maintains strafe/walk animations during combat
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation Layers")
	void LowerBodyLayer(const FPoseLink& InPose, FPoseLink& LowerBodyLayer);
	
};