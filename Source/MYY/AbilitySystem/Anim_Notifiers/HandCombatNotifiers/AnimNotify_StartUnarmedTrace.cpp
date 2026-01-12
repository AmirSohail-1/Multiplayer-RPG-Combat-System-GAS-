#include "AnimNotify_StartUnarmedTrace.h"


void UAnimNotify_StartUnarmedTrace::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	if (AMYYCharacterBase* Character =
		Cast<AMYYCharacterBase>(MeshComp->GetOwner()))
	{
		Character->StartUnarmedTrace(TraceSockets);
	}
}

FString UAnimNotify_StartUnarmedTrace::GetNotifyName_Implementation() const
{
	return "Start Unarmed Combat Trace";
}
