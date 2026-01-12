#include "GA_Interact.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "MYY/AbilitySystem//Interface/Interactable.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"

UGA_Interact::UGA_Interact()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
    // Set asset tags
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Interact")));
    SetAssetTags(AssetTags);
}

void UGA_Interact::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("GA_Interact: Activated!"));
    FindAndInteract();
    
    EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_Interact::FindAndInteract()
{
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character) return;
    
    UE_LOG(LogTemp, Warning, TEXT("GA_Interact: Looking for interactables around character"));

    
    TArray<AActor*> OverlappingActors;
    Character->GetOverlappingActors(OverlappingActors);
    
    for (AActor* Actor : OverlappingActors)
    {
        if (Actor && Actor->Implements<UInteractable>())
        {
            UE_LOG(LogTemp, Warning, TEXT("Found overlapping interactable: %s"), *Actor->GetName());
            InteractWithActor(Actor);
            return;
        }
    }
    
    // 2. Fallback: If no overlapping actors, do camera trace
    UE_LOG(LogTemp, Warning, TEXT("No overlapping interactables, doing camera trace"));
    
    UCameraComponent* Camera = nullptr;
    TArray<UCameraComponent*> CameraComponents;
    Character->GetComponents<UCameraComponent>(CameraComponents);
    if (CameraComponents.Num() > 0)
    {
        Camera = CameraComponents[0];
    }
    
    if (!Camera) return;
    
    FVector Start = Camera->GetComponentLocation();
    FVector End = Start + (Camera->GetForwardVector() * 300.f); // Shorter range
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Character);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && HitActor->Implements<UInteractable>())
        {
            InteractWithActor(HitActor);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("No interactable found"));
}

// Update InteractWithActor for multiplayer:
void UGA_Interact::InteractWithActor(AActor* InteractableActor)
{
    if (!InteractableActor) return;
    
    AMYYCharacterBase* Character = Cast<AMYYCharacterBase>(GetAvatarActorFromActorInfo());
    if (!Character) return;
    
    UE_LOG(LogTemp, Warning, TEXT("Attempting to interact with: %s"), *InteractableActor->GetName());
    
    // Call CanInteract first
    if (IInteractable::Execute_CanInteract(InteractableActor, Character))
    {
        // Server: Direct call, Client: Send RPC
        if (Character->HasAuthority())
        {
            IInteractable::Execute_OnInteract(InteractableActor, Character);
        }
        else
        {
            // Create a server RPC function in character
            Character->Server_Interact(InteractableActor);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot interact with: %s"), *InteractableActor->GetName());
    }
}

 