// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_EndUnarmedTrace.h"




void UAnimNotify_EndUnarmedTrace::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	if (AMYYCharacterBase* Character =
		Cast<AMYYCharacterBase>(MeshComp->GetOwner()))
	{
		Character->EndUnarmedTrace();
	}
}


FString UAnimNotify_EndUnarmedTrace::GetNotifyName_Implementation() const
{
	return "End Unarmed Combat Trace";
}