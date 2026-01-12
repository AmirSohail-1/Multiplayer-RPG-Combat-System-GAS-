// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSetBase.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"

UAttributeSetBase::UAttributeSetBase()
{
    // Initialize default values
    /*  Explicit Method 
        Health.SetBaseValue(100.0f);
        Health.SetCurrentValue(100.0f);
    */
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitStamina(100.f);
    InitMaxStamina(100.f);
}

void UAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, MaxStamina, COND_None, REPNOTIFY_Always);
}
// attribute set manually clamp values before they are set
void UAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // Clamp values before they're set
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
    else if (Attribute == GetStaminaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
    }
}
  

void UAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    // Handle DAMAGE meta attribute
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        const float DamageDone = GetDamage();
        SetDamage(0.f);

        UE_LOG(LogTemp, Error, TEXT("🩸 DAMAGE DETECTED: %.1f"), DamageDone);

        if (DamageDone > 0.f)
        {
            const float OldHealth = GetHealth();
            const float NewHealth = FMath::Max(OldHealth - DamageDone, 0.f);
            SetHealth(NewHealth);

            UE_LOG(LogTemp, Warning, TEXT("Damage Applied: %.1f | Old Health: %.1f | New Health: %.1f"),
                DamageDone, OldHealth, NewHealth);

            AActor* TargetActor = nullptr;
            AActor* SourceActor = nullptr;

            // Target = damaged character
            if (Data.Target.AbilityActorInfo.IsValid() &&
                Data.Target.AbilityActorInfo->AvatarActor.IsValid())
            {
                TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
            }

            // Source = damage instigator
            if (Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
            {
                SourceActor =
                    Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent()->GetAvatarActor();
            }

            //---------------------------------------------------------
            //                      HIT REACT
            //---------------------------------------------------------
            if (NewHealth > 0.f) // Still alive
            {
                UE_LOG(LogTemp, Error, TEXT("🎯 Attempting to trigger HitReact..."));
                UE_LOG(LogTemp, Error, TEXT("   Target: %s"), *GetNameSafe(TargetActor));
                UE_LOG(LogTemp, Error, TEXT("   Source: %s"), *GetNameSafe(SourceActor));

                UAbilitySystemComponent* TargetASC = nullptr;
                if (TargetActor)
                {
                    // ✅ IMPORTANT: Check if actor implements the interface
                    if (TargetActor->Implements<UAbilitySystemInterface>())
                    {
                        TargetASC = Cast<IAbilitySystemInterface>(TargetActor)->GetAbilitySystemComponent();
                    }
                    
                    // Fallback: Try to find ASC component
                    if (!TargetASC)
                    {
                        TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
                    }
                }

                // Trigger HitReact gameplay event
                if (TargetASC)
                {
                    FGameplayEventData HitReactEventData;
                    HitReactEventData.Instigator = SourceActor;
                    HitReactEventData.Target = TargetActor;
                    HitReactEventData.EventMagnitude = DamageDone;

                    int32 TriggerCount = TargetASC->HandleGameplayEvent(
                                        FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact"),
                                        &HitReactEventData);

                    bool bTriggered = (TriggerCount > 0);

                    UE_LOG(LogTemp, Error,
                        TEXT("   HitReact Event Triggered: %s"),
                        bTriggered ? TEXT("YES") : TEXT("NO"));

                    // ✅ IMPORTANT: Add ability check debugging from old logic
                    if (!bTriggered)
                    {
                        UE_LOG(LogTemp, Error, TEXT("❌ HitReact event failed. Checking ability..."));
                        
                        // Check if ability exists
                        for (const FGameplayAbilitySpec& Spec : TargetASC->GetActivatableAbilities())
                        {
                            FString AbilityName = GetNameSafe(Spec.Ability);
                            if (AbilityName.Contains("HitReact"))
                            {
                                UE_LOG(LogTemp, Error, TEXT("   Found HitReact ability: %s"), *AbilityName);
                            }
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("✅ Triggered HitReact event on %s"),
                            *GetNameSafe(TargetActor));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ Failed to get TargetASC for HitReact! TargetActor: %s"),
                        *GetNameSafe(TargetActor));
                }
            }
            //---------------------------------------------------------
            //                      DEATH
            //---------------------------------------------------------
            else if (NewHealth <= 0.f && OldHealth > 0.f)
            {
                UE_LOG(LogTemp, Error, TEXT("💀 Character DIED: %s | Killer: %s"),
                    *GetNameSafe(TargetActor), *GetNameSafe(SourceActor));

                UAbilitySystemComponent* TargetASC = nullptr;
                if (TargetActor)
                {
                    // ✅ IMPORTANT: Check if actor implements the interface
                    if (TargetActor->Implements<UAbilitySystemInterface>())
                    {
                        TargetASC = Cast<IAbilitySystemInterface>(TargetActor)->GetAbilitySystemComponent();
                    }
                    
                    // Fallback: Try to find ASC component
                    if (!TargetASC)
                    {
                        TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
                    }
                }

                // Trigger death gameplay event
                if (TargetASC)
                {
                    FGameplayEventData DeathEventData;
                    DeathEventData.Instigator = SourceActor;
                    DeathEventData.Target = TargetActor;
                    DeathEventData.EventMagnitude = DamageDone;

 
                    int32 DeathTriggerCount  = TargetASC->HandleGameplayEvent(
                            FGameplayTag::RequestGameplayTag("GameplayEvent.Death"),
                            &DeathEventData);

                    bool bDeathTriggered  = (DeathTriggerCount  > 0);

                    // Player VS AI game mode logic used by BP

                    // 🔒 Prevent double death at AttributeSet level
                    if (bIsDead)
                    {
                        return;
                    }

                    bIsDead = true;

                    if (AMYYCharacterBase* TargetCharacter = Cast<AMYYCharacterBase>(TargetActor))
                    {
                        TargetCharacter->HandleDeath(SourceActor);
                    }
                    

                    UE_LOG(LogTemp, Error,
                        TEXT("   Death Event Triggered: %s"),
                        bDeathTriggered  ? TEXT("YES") : TEXT("NO"));

                    if (bDeathTriggered )
                    {
                        UE_LOG(LogTemp, Warning, TEXT("✅ Triggered Death event on %s"),
                            *GetNameSafe(TargetActor));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ Failed to get TargetASC for Death event! TargetActor: %s"),
                        *GetNameSafe(TargetActor));
                }

                // // Call Die() on character
                // if (AMYYCharacterBase* TargetCharacter = Cast<AMYYCharacterBase>(TargetActor))
                // {
                //     TargetCharacter->Die(SourceActor);
                // }
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        // Health is being directly modified - ensure it's clamped
        float CurrentHealth = GetHealth();
        float ClampedHealth = FMath::Clamp(CurrentHealth, 0.f, GetMaxHealth());
        if (CurrentHealth != ClampedHealth)
        {
            SetHealth(ClampedHealth);
            UE_LOG(LogTemp, Warning, TEXT("Health clamped: %.1f -> %.1f"), CurrentHealth, ClampedHealth);
        }
    }
    else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
    {
        // Stamina is being directly modified - ensure it's clamped
        float CurrentStamina = GetStamina();
        float ClampedStamina = FMath::Clamp(CurrentStamina, 0.f, GetMaxStamina());
        if (CurrentStamina != ClampedStamina)
        {
            SetStamina(ClampedStamina);
        }
    }
}

void UAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Health, OldHealth);
}

void UAttributeSetBase::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, MaxHealth, OldMaxHealth);
}

void UAttributeSetBase::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Stamina, OldStamina);
}

void UAttributeSetBase::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, MaxStamina, OldMaxStamina);
}
