// AnimNotifyState_BlockWindow.cpp
#include "AnimNotifyState_BlockWindow.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UAnimNotifyState_BlockWindow::UAnimNotifyState_BlockWindow()
{
    bIsParryWindow = true;
    WindowDuration = 0.3f;
    BlockWindowTag = FGameplayTag::RequestGameplayTag("State.Combat.BlockWindow");
}

void UAnimNotifyState_BlockWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration,EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    // Set block window flag (replicated)
    if (Character->HasAuthority())
    {
        Character->bIsInBlockWindow = true;
    }
    else
    {
        // Call server RPC to set flag
        Character->Server_SetBlockWindow(true);
    }

    // Add gameplay tag for block window
    if (Character->AbilitySystemComponent)
    {
        FGameplayTagContainer TagContainer;
        TagContainer.AddTag(BlockWindowTag);
        
        if (bIsParryWindow)
        {
            TagContainer.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.ParryWindow"));
        }

        Character->AbilitySystemComponent->AddLooseGameplayTags(TagContainer);
    }

    // Visual/audio feedback
    if (bIsParryWindow)
    {
        // Play parry ready sound/VFX
        UE_LOG(LogTemp, Log, TEXT("Parry Window Started for %s"), *Character->GetName());
    }
}

void UAnimNotifyState_BlockWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation,  EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(MeshComp->GetOwner());
    if (!Character) return;

    // Clear block window flag (replicated)
    if (Character->HasAuthority())
    {
        Character->bIsInBlockWindow = false;
    }
    else
    {
        Character->Server_SetBlockWindow(false);
    }

    // Remove gameplay tags
    if (Character->AbilitySystemComponent)
    {
        FGameplayTagContainer TagContainer;
        TagContainer.AddTag(BlockWindowTag);
        
        if (bIsParryWindow)
        {
            TagContainer.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.ParryWindow"));
        }

        Character->AbilitySystemComponent->RemoveLooseGameplayTags(TagContainer);
    }

    UE_LOG(LogTemp, Log, TEXT("Block Window Ended for %s"), *Character->GetName());
}

FString UAnimNotifyState_BlockWindow::GetNotifyName_Implementation() const
{
    return bIsParryWindow ? TEXT("Parry Window") : TEXT("Block Window");
}