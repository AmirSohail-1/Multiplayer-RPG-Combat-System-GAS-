#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"

#include "AnimNotify_StartUnarmedTrace.generated.h"


/**
 * 
 */
UCLASS()
class MYY_API UAnimNotify_StartUnarmedTrace : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unarmed Trace")
	TArray<FUnarmedTraceSocket> TraceSockets;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation) override;
	
	FString GetNotifyName_Implementation() const;
};
