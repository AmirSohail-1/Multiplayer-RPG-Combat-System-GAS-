// Fill out your copyright notice in the Description page of Project Settings.


#include "MYYCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "BaseWeapon.h"
#include "MotionWarpingComponent.h"
#include "AttributeSet/AttributeSetBase.h"
#include "Characters/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "MYY/AbilitySystem/UI/HealthStaminaWidget.h"

// #include "MYY/AbilitySystem/AttributeSet/AttributeSetBase.h" 

// Sets default values
AMYYCharacterBase::AMYYCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create Ability System Component
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UAttributeSetBase>(TEXT("MYYAttributeSet"));
	
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(ASCReplicationMode);

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	
	// Create Equipment Component
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->SetIsReplicated(true);
	
	// Setup for network
	bReplicates = true;
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Orient rotation to movement
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Set the character's rotation rate (speed of rotation in each direction)
	GetCharacterMovement()->RotationRate = FRotator(500.0f, 500.0f, 0.0f);

	// Set jump velocity
	GetCharacterMovement()->JumpZVelocity = 500.0f;

	// Set air control (how much control the character has when in the air)
	GetCharacterMovement()->AirControl = 0.35f;

	// Set maximum walk speed
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;

	// Set minimum analog walk speed
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;

	// Set braking deceleration while walking
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// Set braking deceleration while falling
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	TeamID = 0; // Default to Player team
	

}


FGenericTeamId AMYYCharacterBase::GetGenericTeamId() const
{
	return FGenericTeamId(TeamID);
}

ETeamAttitude::Type AMYYCharacterBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const IGenericTeamAgentInterface* OtherTeam = Cast<IGenericTeamAgentInterface>(&Other))
	{
		FGenericTeamId OtherTeamID = OtherTeam->GetGenericTeamId();
        
		if (TeamID == OtherTeamID.GetId())
		{
			return ETeamAttitude::Friendly;
		}
		else
		{
			return ETeamAttitude::Hostile;
		}
	}
    
	return ETeamAttitude::Neutral;
}



void AMYYCharacterBase::DebugAbilities()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("No Ability System Component!"));
		return;
	}
    
	UE_LOG(LogTemp, Warning, TEXT("=== DEBUG ABILITIES ==="));
	UE_LOG(LogTemp, Warning, TEXT("Total Granted Abilities: %d"), AbilitySystemComponent->GetActivatableAbilities().Num());
    
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		UE_LOG(LogTemp, Warning, TEXT("  Ability: %s, InputID: %d, Source: %s"),
			*GetNameSafe(Spec.Ability),
			Spec.InputID,
			*GetNameSafe(Spec.SourceObject.Get()));
	}
	UE_LOG(LogTemp, Warning, TEXT("======================"));
}



void AMYYCharacterBase::HandleDeath(AActor* KillerActor)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	BP_OnCharacterDeath(KillerActor);
}





// Called when the game starts or when spawned
void AMYYCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Initialize ability system first
		if (AbilitySystemComponent && AttributeSet)
		{
			// ✅ CRITICAL: Delay callback binding to ensure everything is initialized
			FTimerHandle BindCallbacksTimer;
			GetWorld()->GetTimerManager().SetTimer(BindCallbacksTimer, [this]()
			{
				BindAttributeCallbacks();
			}, 0.05f, false); // Small delay to ensure initialization
		}
	}
 
}

// Separate function to bind callbacks
void AMYYCharacterBase::BindAttributeCallbacks()
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// Bind all your attribute callbacks here
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetHealthAttribute()).AddUObject(this, &AMYYCharacterBase::OnHealthAttributeChanged);
    
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetStaminaAttribute()).AddUObject(this, &AMYYCharacterBase::OnStaminaAttributeChanged);
 
	// Bind other attributes...

	UE_LOG(LogTemp, Log, TEXT("✅ Attribute callbacks bound for %s"), *GetName());
}

void AMYYCharacterBase::OnStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
	// ✅ CRITICAL: Guard against uninitialized state
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// ✅ Guard against invalid values during initialization
	if (!FMath::IsFinite(Data.NewValue))
	{
		return;
	}
	
	float Delta = Data.NewValue - Data.OldValue;
	OnStaminaChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxStamina(), Delta);
}

void AMYYCharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{

	// ✅ Guard checks
	if (!AbilitySystemComponent || !AttributeSet || !FMath::IsFinite(Data.NewValue))
	{
		return;
	}
	
	float Delta = Data.NewValue - Data.OldValue;
	OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth(), Delta);

	// // Check for death
	// if (Data.NewValue <= 0.f && Data.OldValue > 0.f)
	// {
	//     Die(nullptr);
	// }
}


void AMYYCharacterBase::InitializeHealthStaminaUI()
{
	// ✅ Guard checks
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// -----------------------------
	// Health initial push
	// -----------------------------
	const float CurrentHealth = AttributeSet->GetHealth();
	const float MaxHealth = AttributeSet->GetMaxHealth();

	OnHealthChanged.Broadcast(
		CurrentHealth,
		MaxHealth,
		0.f // Delta = 0 on initialization
	);

	// -----------------------------
	// Stamina initial push
	// -----------------------------
	const float CurrentStamina = AttributeSet->GetStamina();
	const float MaxStamina = AttributeSet->GetMaxStamina();

	OnStaminaChanged.Broadcast(
		CurrentStamina,
		MaxStamina,
		0.f // Delta = 0 on initialization
	);

	UE_LOG(LogTemp, Log, TEXT("✅ Initial Health/Stamina UI initialized for %s"), *GetName());
}




void AMYYCharacterBase::InitializeAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent) return;

	UE_LOG(LogTemp, Warning, TEXT("Initializing Abilities..."));

	// Grant default abilities
	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		GrantAbility(AbilityClass);
	}

	// Grant default abilities (these should have correct input IDs in their specs)
	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		GrantAbility(AbilityClass);
	}
    
	// CRITICAL FIX: Grant persistent abilities WITH PROPER INPUT IDs
	// Interact (Input ID = 7)
	if (InteractAbility)
	{
		FGameplayAbilitySpec InteractSpec(InteractAbility, 1, (int32)EAbilityInputID::Interact, this);
		FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(InteractSpec);
		GrantedAbilityHandles.Add(InteractAbility, Handle);
        
		UE_LOG(LogTemp, Warning, TEXT("Granted Interact ability with InputID: %d"), (int32)EAbilityInputID::Interact);
	}

	// Equip (Input ID = 11)
	if (EquipAbility)
	{
		FGameplayAbilitySpec EquipSpec(EquipAbility, 1, (int32)EAbilityInputID::Equip, this);
		FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(EquipSpec);
		GrantedAbilityHandles.Add(EquipAbility, Handle);
        
		UE_LOG(LogTemp, Warning, TEXT("Granted Equip ability with InputID: %d"), (int32)EAbilityInputID::Equip);
	}

	// Unequip (Input ID = 12)
	if (UnequipAbility)
	{
		FGameplayAbilitySpec UnequipSpec(UnequipAbility, 1, (int32)EAbilityInputID::Unequip, this);
		FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(UnequipSpec);
		GrantedAbilityHandles.Add(UnequipAbility, Handle);
        
		UE_LOG(LogTemp, Warning, TEXT("Granted Unequip ability with InputID: %d"), (int32)EAbilityInputID::Unequip);
	}

	// ✅ NEW: Grant Death Ability (event-triggered, no input ID needed)
	if (DeathAbilityClass)
	{
		FGameplayAbilitySpec DeathSpec(DeathAbilityClass, 1, -1, this);
		FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(DeathSpec);
		GrantedAbilityHandles.Add(DeathAbilityClass, Handle);
        
		UE_LOG(LogTemp, Warning, TEXT("✅ Granted Death ability (event-triggered)"));
	}

	 
	// ✅ FIXED: Grant HitReact Ability (event-triggered, no input ID needed)
	if (HitReactAbilityClass)
	{
		FGameplayAbilitySpec HitReactSpec(HitReactAbilityClass, 1, -1, this);
		FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(HitReactSpec);
		GrantedAbilityHandles.Add(HitReactAbilityClass, Handle);
        
		UE_LOG(LogTemp, Warning, TEXT("✅ Granted HitReact ability (event-triggered)"));
		
		// ✅ FIXED: Verify without accessing protected member
		UGameplayAbility* HitReactCDO = HitReactAbilityClass->GetDefaultObject<UGameplayAbility>();
		if (HitReactCDO)
		{
			UE_LOG(LogTemp, Error, TEXT("   HitReact Ability Class: %s"), *HitReactAbilityClass->GetName());
		}
		
	}
	
}


// Called every frame
void AMYYCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsUnarmedTracing)
	{
		PerformUnarmedTrace(ActiveUnarmedTraceSockets);
	}
}
 
// Called to bind functionality to input
void AMYYCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
 
UAbilitySystemComponent* AMYYCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
 
void AMYYCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	UE_LOG(LogTemp, Log, TEXT("🎮 %s possessed by %s"), 
		*GetName(), *GetNameSafe(NewController));

	// Initialize ASC on server when possessed
	if (HasAuthority())
	{
		// ✅ Small delay to ensure replication is ready
		FTimerHandle InitTimer;
		GetWorld()->GetTimerManager().SetTimer(InitTimer, [this]()
		{
			if (IsValid(this))
			{
				InitializeAbilitySystem();
			}
		}, 0.1f, false);
	}

	
}

void AMYYCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Initialize ASC on client when PlayerState replicates
	InitializeAbilitySystem();
	
 
}

void AMYYCharacterBase::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
    
	InitializeAbilities();

	// Bind to attribute changes
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetHealthAttribute()).AddUObject(this, &AMYYCharacterBase::OnHealthAttributeChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetStaminaAttribute()).AddUObject(this, &AMYYCharacterBase::OnStaminaAttributeChanged);

	// Apply default effects (only on server)
	if (HasAuthority())
	{
		for (TSubclassOf<UGameplayEffect>& EffectClass : DefaultEffects)
		{
			if (EffectClass)
			{
				FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
				EffectContext.AddSourceObject(this);

				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
					EffectClass, 1, EffectContext);

				if (SpecHandle.IsValid())
				{
					AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
		}
	}


	
}



void AMYYCharacterBase::GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
    if (!HasAuthority() || !AbilitySystemComponent || !AbilityClass) return;

    FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
    FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(AbilitySpec);
    
    GrantedAbilityHandles.Add(AbilityClass, Handle);
}

void AMYYCharacterBase::RemoveAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
    if (!HasAuthority() || !AbilitySystemComponent) return;

    FGameplayAbilitySpecHandle* HandlePtr = GrantedAbilityHandles.Find(AbilityClass);
    if (HandlePtr)
    {
        AbilitySystemComponent->ClearAbility(*HandlePtr);
        GrantedAbilityHandles.Remove(AbilityClass);
    }
}


void AMYYCharacterBase::Die(AActor* Killer)
{
	if (!IsAlive()) return;

	UE_LOG(LogTemp, Warning, TEXT("💀 %s died! Killer: %s"), 
		*GetName(), *GetNameSafe(Killer));

	// ========== REMOVED: Weapon dropping (now handled by GA_Death) ==========
	// The GA_Death ability will call EquipmentComponent->DropAllWeapons()
	// This prevents duplicate drops and ensures proper timing

	// ========== DISABLE MOVEMENT (may be redundant with GA_Death) ==========
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	// ========== DISABLE COLLISION ==========
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ========== CANCEL ABILITIES ==========
	AbilitySystemComponent->CancelAllAbilities();

	// ========== BROADCAST DEATH (for GameMode) ==========
	OnCharacterDied.Broadcast(this, Killer);

	// ========== OPTIONAL: RAGDOLL ==========
	// GetMesh()->SetSimulatePhysics(true);
	// GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// ========== DESTROY AFTER DELAY ==========
	// NOTE: GA_Death also sets lifespan, this may be redundant
	SetLifeSpan(5.0f);
}


bool AMYYCharacterBase::IsAlive() const
{
    return AttributeSet && AttributeSet->GetHealth() > 0.f;
}


void AMYYCharacterBase::SetCurrentTarget(AActor* NewTarget)
{
    CurrentTarget = NewTarget;
}

void AMYYCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMYYCharacterBase, EquipmentComponent);
    DOREPLIFETIME(AMYYCharacterBase, CurrentTarget);
	DOREPLIFETIME(AMYYCharacterBase, bIsInBlockWindow);
	DOREPLIFETIME(AMYYCharacterBase, bIsAiming);
	DOREPLIFETIME(AMYYCharacterBase, TeamID);          
}

void AMYYCharacterBase::DisableMovementDuringAbility(bool bDisable)
{
	if (!GetCharacterMovement()) return;

	if (bDisable)
	{
		GetCharacterMovement()->DisableMovement();  
		GetCharacterMovement()->StopMovementImmediately();
		bPressedJump = false;  
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AMYYCharacterBase::OnRep_IsInBlockWindow()
{
	// Visual feedback for block window can go here
}




void AMYYCharacterBase::Server_Interact_Implementation(AActor* InteractableActor)
{
	if (InteractableActor && InteractableActor->Implements<UInteractable>())
	{
		IInteractable::Execute_OnInteract(InteractableActor, this);
	}
}

void AMYYCharacterBase::Server_SetBlockWindow_Implementation(bool bInWindow)
{
	bIsInBlockWindow = bInWindow;
}

void AMYYCharacterBase::PerformBlock()
{
	if (!AbilitySystemComponent) return;

	// Try to activate block ability
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Combat.Block"));
    
	AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
}

void AMYYCharacterBase::StopBlocking()
{
	if (!AbilitySystemComponent) return;

	// Cancel block ability
	FGameplayTagContainer BlockTags;
	BlockTags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Blocking"));
    
	AbilitySystemComponent->CancelAbilities(&BlockTags);
}


void AMYYCharacterBase::SetAiming(bool bInAiming)
{
	if (!HasAuthority()) return;
    
	bIsAiming = bInAiming;
    
	UE_LOG(LogTemp, Log, TEXT("[MYYCharacterBase] %s: Aiming = %s"), 
		*GetName(), bIsAiming ? TEXT("TRUE") : TEXT("FALSE"));

	// Apply movement changes on SERVER immediately
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = !bIsAiming;
		UE_LOG(LogTemp, Log, TEXT("[MYYCharacterBase] SERVER - bOrientRotationToMovement = %s"), 
			!bIsAiming ? TEXT("TRUE") : TEXT("FALSE"));
	}
    
	bUseControllerRotationYaw = bIsAiming;
	
	UE_LOG(LogTemp, Log, TEXT("[MYYCharacterBase] SERVER - bUseControllerRotationYaw = %s"), 
		bIsAiming ? TEXT("TRUE") : TEXT("FALSE"));

	// Camera zoom (local only for player characters)
	if (IsLocallyControlled())
	{
		APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(this);
		if (PlayerChar)
		{
			PlayerChar->SetCameraZoom(bIsAiming);
		}
	}
	
}

void AMYYCharacterBase::OnRep_IsAiming()
{
	UE_LOG(LogTemp, Log, TEXT("[MYYCharacterBase] %s: OnRep_IsAiming = %s"), 
		*GetName(), bIsAiming ? TEXT("TRUE") : TEXT("FALSE"));
    
	// ✅ Apply movement changes on clients
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = !bIsAiming;
		UE_LOG(LogTemp, Log, TEXT("[MYYCharacterBase] bOrientRotationToMovement = %s"), 
			!bIsAiming ? TEXT("TRUE") : TEXT("FALSE"));
	}
    
	bUseControllerRotationYaw = bIsAiming;
	UE_LOG(LogTemp, Log, TEXT("[MYYCharacterBase] bUseControllerRotationYaw = %s"), 
		bIsAiming ? TEXT("TRUE") : TEXT("FALSE"));
    
	// ✅ Camera zoom (local player only)
	if (IsLocallyControlled())
	{
		APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(this);
		if (PlayerChar)
		{
			PlayerChar->SetCameraZoom(bIsAiming);
		}
	}
}

void AMYYCharacterBase::StartUnarmedTrace(
	const TArray<FUnarmedTraceSocket>& TraceSockets)
{
	if (!HasAuthority()) return;

	bIsUnarmedTracing = true;
	ActiveUnarmedTraceSockets = TraceSockets;
	HitActorsThisSwing.Reset();
}

void AMYYCharacterBase::EndUnarmedTrace()
{
	if (!HasAuthority()) return;

	bIsUnarmedTracing = false;
	ActiveUnarmedTraceSockets.Reset();
	HitActorsThisSwing.Reset();
}


void AMYYCharacterBase::ForceStopAllCombatTraces()
{
	if (!HasAuthority())
	{
		return;
	}

	/* ===============================
	 * WEAPON TRACE CLEANUP (CURRENT WEAPON)
	 * =============================== */

	if (EquipmentComponent)
	{
		if (ABaseWeapon* CurrentWeapon = EquipmentComponent->GetCurrentWeapon() )
		{
			// Stop weapon tracing safely
			CurrentWeapon->EndTrace();

			// Clear weapon hit cache
			CurrentWeapon->ClearHitActors();
		}
	}

	// Future improvement: Stop traces for all equipped weapons
	
	// if (EquipmentComponent)
	// {
	// 	if (ABaseWeapon* Primary = EquipmentComponent->PrimarySlot.Weapon)
	// 	{
	// 		Primary->EndTrace();
	// 		Primary->ClearHitActors();
	// 	}
	//
	// 	if (ABaseWeapon* Secondary = EquipmentComponent->SecondarySlot.Weapon)
	// 	{
	// 		Secondary->EndTrace();
	// 		Secondary->ClearHitActors();
	// 	}
	// }

	/* ===============================
	 * UNARMED TRACE CLEANUP
	 * =============================== */

	bIsUnarmedTracing = false;
	ActiveUnarmedTraceSockets.Reset();

	/* ===============================
	 * SHARED STATE
	 * =============================== */

	HitActorsThisSwing.Reset();

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("🛑 ForceStopAllCombatTraces called on %s"),
		*GetName()
	);
#endif
}


void AMYYCharacterBase::PerformUnarmedTrace(
    const TArray<FUnarmedTraceSocket>& TraceSockets)
{
    if (!HasAuthority()) return;
    if (!EquipmentComponent || !EquipmentComponent->DefaultUnarmedData) return;

    AActor* OwnerActor = this;

    UAbilitySystemComponent* InstigatorASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);

    if (!InstigatorASC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("⚠️ Unarmed trace: InstigatorASC missing on %s"),
            *GetName());
        return;
    }

    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp) return;

    const float BaseDamage =
        EquipmentComponent->DefaultUnarmedData->Stats.BaseDamage;

    TSubclassOf<UGameplayEffect> DamageEffect =
        EquipmentComponent->DefaultUnarmedData->DamageEffect;

    if (!DamageEffect)
    {
        UE_LOG(LogTemp, Error,
            TEXT("❌ UnarmedDamageEffect missing on %s"),
            *GetName());
        return;
    }

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false;

    for (const FUnarmedTraceSocket& TraceData : TraceSockets)
    {
        const FVector Center =
            MeshComp->GetSocketLocation(TraceData.SocketOrBone);

        TArray<FHitResult> HitResults;

        GetWorld()->SweepMultiByChannel(
            HitResults,
            Center,
            Center,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(TraceData.Radius),
            QueryParams
        );

#if !UE_BUILD_SHIPPING
    	if (EquipmentComponent->DefaultUnarmedData->bDebugUnArmedTrace)
    	{
    		DrawDebugSphere(
			GetWorld(),
			Center,
			TraceData.Radius,
			12,
			FColor::Green,
			false,
			0.1f
		);
    		
    	}
    	    
#endif

        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (!HitActor || HitActor == OwnerActor) continue;
            if (HitActorsThisSwing.Contains(HitActor)) continue;

            UAbilitySystemComponent* TargetASC =
                UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

            if (!TargetASC) continue;

            HitActorsThisSwing.Add(HitActor);

            float Damage = BaseDamage;
            bool bWasParried = false;

            // ======================
            // BLOCK / PARRY
            // ======================
            if (AMYYCharacterBase* TargetChar = Cast<AMYYCharacterBase>(HitActor))
            {
                if (TargetChar->bIsInBlockWindow)
                {
                    bWasParried = true;
                    Damage = 0.f;

                    FGameplayEventData ParryEvent;
                    ParryEvent.Instigator = OwnerActor;
                    ParryEvent.Target = HitActor;

                    TargetASC->HandleGameplayEvent(
                        FGameplayTag::RequestGameplayTag("GameplayEvent.Parry"),
                        &ParryEvent);

                    FGameplayEventData StaggerEvent;
                    StaggerEvent.Instigator = HitActor;
                    StaggerEvent.Target = OwnerActor;

                    InstigatorASC->HandleGameplayEvent(
                        FGameplayTag::RequestGameplayTag("GameplayEvent.Stagger"),
                        &StaggerEvent);
                }
            }
            else if (TargetASC->HasMatchingGameplayTag(
                FGameplayTag::RequestGameplayTag("State.Combat.Blocking")))
            {
                Damage *= 0.3f;
            }

            // ======================
            // APPLY DAMAGE (GAS)
            // ======================
            if (!bWasParried && Damage > 0.f)
            {
                FGameplayEffectContextHandle Context =
                    InstigatorASC->MakeEffectContext();

                Context.AddSourceObject(this);
                Context.AddHitResult(Hit);
                Context.AddInstigator(OwnerActor, GetController());

                FGameplayEffectSpecHandle Spec =
                    InstigatorASC->MakeOutgoingSpec(
                        DamageEffect, 1.f, Context);

                if (Spec.IsValid())
                {
                    Spec.Data->SetSetByCallerMagnitude(
                        FGameplayTag::RequestGameplayTag("Data.Damage"),
                        Damage);

                    Spec.Data->CapturedSourceTags
                        .GetSpecTags()
                        .AddTag(
                            FGameplayTag::RequestGameplayTag(
                                "Ability.Attack.Unarmed"));

                    InstigatorASC->ApplyGameplayEffectSpecToTarget(
                        *Spec.Data.Get(), TargetASC);
                }
            }
        }
    }
}
