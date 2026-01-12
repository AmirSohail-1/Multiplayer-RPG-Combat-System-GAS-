Comprehensive Gameplay Ability System (GAS) Reference Guide
Table of Contents

Core Architecture
Attribute System
Gameplay Abilities
Gameplay Effects
Gameplay Tags
Event System
Network Replication
Ability Tasks
Execution Flow Chains
Advanced Concepts
Debugging & Troubleshooting


Core Architecture
Ability System Component (ASC)
The central hub of GAS. Every actor using GAS needs one.
cpp// Granting abilities to ASC
FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, (int32)EAbilityInputID::PrimaryAttack, SourceObject);
AbilitySystemComponent->GiveAbility(AbilitySpec);

// Removing abilities
AbilitySystemComponent->ClearAbility(AbilitySpecHandle);

// Getting attribute values
float Health = ASC->GetNumericAttribute(UAttributeSetBase::GetHealthAttribute());

// Checking for tags
bool HasTag = ASC->HasMatchingGameplayTag(TagToCheck);
ASC Ownership Models:

Player Character owns ASC: Most common, ASC lives on character
PlayerState owns ASC: Survives character respawn, better for multiplayer
AI Controller owns ASC: For AI-controlled characters


Attribute System
Attribute Lifecycle
cpp// 1. INITIALIZATION (Constructor)
InitHealth(100.f);        // Sets BaseValue = 100
InitMaxHealth(100.f);

// 2. CLAMPING (PreAttributeChange)
// Called BEFORE value changes, allows you to clamp/validate
void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
}

// 3. APPLICATION (PostGameplayEffectExecute)
// Called AFTER GameplayEffect executes
void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    // Extract who did what to whom
    AActor* SourceActor = Data.EffectSpec.GetEffectContext().GetInstigator();
    AActor* TargetActor = GetOwningActor();
    
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        const float DamageDone = GetDamage();
        SetDamage(0.f);  // Reset meta-attribute
        
        const float NewHealth = FMath::Max(GetHealth() - DamageDone, 0.f);
        SetHealth(NewHealth);
    }
}

// 4. REPLICATION (OnRep Functions)
void OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Health, OldHealth);
    // This macro handles: attribute clamping, broadcast to listeners, UI updates
}
Meta Attributes vs Persistent Attributes
Meta Attributes (Temporary):

Damage, Healing - exist only during calculation
Reset to 0 after use
NOT replicated
Used as intermediate values

Persistent Attributes (Permanent):

Health, Stamina, MaxHealth - stay across frames
Replicated to clients
Drive gameplay state

cpp// Meta-attribute pattern
if (Data.EvaluatedData.Attribute == GetDamageAttribute())
{
    const float DamageDone = GetDamage();
    SetDamage(0.f);  // ← ALWAYS RESET META-ATTRIBUTES
    
    // Apply to persistent attribute
    SetHealth(GetHealth() - DamageeDone);
}
Attribute Modifier Types
cpp// Additive: Health = Health + 50
Modifier.ModifierOp = EGameplayModOp::Additive;

// Multiplicative: Health = Health * 1.5
Modifier.ModifierOp = EGameplayModOp::Multiplicative;

// Override: Health = 100 (ignore current value)
Modifier.ModifierOp = EGameplayModOp::Override;

// Division: Health = Health / 2
Modifier.ModifierOp = EGameplayModOp::Division;

Gameplay Abilities
Ability Spec vs Ability Instance vs Ability CDO
Ability Spec (FGameplayAbilitySpec):

Container holding ability data for ONE actor
Stores: ability class, level, input ID, source object
Lives in ASC's ActivatableAbilities array

Ability Instance (UGameplayAbility):

The running copy of an ability
Created when ability activates (depending on instancing policy)
Stores runtime state: combo index, montage tasks, timers

Ability CDO (Class Default Object):

The template ability
Never runs, just holds default values
Shared by all actors

cpp// Instancing policies control instance creation:

// InstancedPerActor: One instance shared across activations
// - Combo system (CurrentComboIndex persists)
// - Block ability (ActiveBlockEffectHandle persists)
InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

// InstancedPerExecution: New instance every activation
// - Simple abilities without persistent state
// - Jump, instant damage
InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

// NonInstanced: Uses CDO directly
// - Extremely rare, avoid unless you know why
InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;
Ability Activation Flow
cpp// STEP 1: Check if can activate
bool CanActivateAbility(...)
{
    // Check tags
    if (ASC->HasMatchingGameplayTag(BlockedTag)) return false;
    
    // Check custom conditions
    if (!CheckStaminaCost()) return false;
    
    return Super::CanActivateAbility(...);
}

// STEP 2: Activate ability
void ActivateAbility(...)
{
    Super::ActivateAbility(...);  // ALWAYS call this first
    
    // STEP 3: Commit costs/cooldowns
    if (!CommitAbility(...))
    {
        EndAbility(...);
        return;
    }
    
    // STEP 4: Do ability logic
    PlayMontage();
    ApplyGameplayEffect();
    SpawnProjectile();
}

// STEP 5: End ability
void EndAbility(...)
{
    // Cleanup logic here
    ClearTimers();
    RemoveEffects();
    
    Super::EndAbility(...);  // ALWAYS call this last
}
Commit vs CommitExecute vs CommitCheck
cpp// CommitCheck: Test if ability can commit (doesn't apply costs)
if (CommitCheck(...))
{
    // Can activate, but hasn't yet
}

// CommitAbility: Checks AND applies costs/cooldowns
if (!CommitAbility(...))
{
    // Failed: not enough resources or on cooldown
    EndAbility(...);
    return;
}

// CommitExecute: Applies costs without checking (use carefully)
CommitExecute(...);  // Forces cost application
Input Handling
cpp// Three input hooks:

// 1. InputPressed: Button just pressed
void InputPressed(...)
{
    if (bComboWindowOpen)
    {
        TriggerNextCombo();
    }
    else
    {
        bHasBufferedInput = true;  // Save for later
    }
}

// 2. InputReleased: Button just released
void InputReleased(...)
{
    if (bIsCharging)
    {
        ReleaseCharge();
    }
}

// 3. InputHeld: Button held down (called every tick while held)
void InputHeld(...)
{
    ChargeAmount += DeltaTime;
}
Ability Triggers
cpp// INPUT TRIGGER (Player presses button)
FGameplayAbilitySpec Spec(AbilityClass);
Spec.InputID = (int32)EAbilityInputID::PrimaryAttack;
ASC->GiveAbility(Spec);

// Later when player presses button:
ASC->AbilityLocalInputPressed((int32)EAbilityInputID::PrimaryAttack);

// EVENT TRIGGER (Gameplay event happens)
FAbilityTriggerData TriggerData;
TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact");
TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
AbilityTriggers.Add(TriggerData);

// Later when damage happens:
ASC->HandleGameplayEvent(HitReactTag, &EventData);  // Auto-activates ability

// OWNED TAG TRIGGER (Tag gets added to actor)
FAbilityTriggerData TriggerData;
TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Status.OnFire");
TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;

Gameplay Effects
Duration Types
cpp// INSTANT: Apply once and done
GE->DurationPolicy = EGameplayEffectDurationType::Instant;
// Example: Immediate damage, instant heal

// HAS_DURATION: Lasts X seconds, then expires
GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
GE->DurationMagnitude = FScalableFloat(5.0f);  // 5 seconds
// Example: Speed boost for 5 seconds, temporary shield

// INFINITE: Lasts forever until manually removed
GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
// Example: Passive stats from equipment, permanent buffs
Modifier Operations Order
GAS applies modifiers in this order:

Additive (+50 damage)
Multiplicative (×1.5 damage)
Division (÷2 damage)
Override (= 100 damage, ignores all previous)

cpp// Example calculation:
// Base Damage = 100
// +20 Additive → 120
// ×1.5 Multiplicative → 180
// ÷2 Division → 90
// Final Damage = 90
Gameplay Effect Context
Context carries data about WHO did WHAT to WHOM:
cppFGameplayEffectContextHandle Context = ASC->MakeEffectContext();
Context.AddSourceObject(this);           // Who caused effect
Context.AddActors({TargetActor});        // Who is affected
Context.AddHitResult(HitResult);         // Where hit happened
Context.AddInstigator(Instigator, EffectCauser);  // Who initiated

// Later retrieve in AttributeSet:
AActor* SourceActor = Data.EffectSpec.GetEffectContext().GetInstigator();
FHitResult HitResult = Data.EffectSpec.GetEffectContext().GetHitResult();
Stacking
cpp// AGGREGATE_BY_SOURCE: Each source gets own stack
// Player 1 applies Poison → Stack 1
// Player 2 applies Poison → Stack 2 (separate)
StackingType = EGameplayEffectStackingType::AggregateBySource;

// AGGREGATE_BY_TARGET: All sources share one stack
// Player 1 applies Poison → Stack 1
// Player 2 applies Poison → Adds to Stack 1
StackingType = EGameplayEffectStackingType::AggregateByTarget;

// Stack limit and behavior:
StackLimitCount = 5;  // Max 5 stacks
StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
// When new stack applied: refresh duration to full
Periodic Effects
cpp// Apply damage every second
GE->Period = 1.0f;  // Tick every 1 second
GE->bExecutePeriodicEffectOnApplication = true;  // Tick immediately when applied

// Example: Poison deals 10 damage per second for 5 seconds
// Total ticks: 5 (at 0s, 1s, 2s, 3s, 4s)
// Total damage: 50
Conditional Effects
cpp// Only apply if target has low health
ConditionalGameplayEffect.RequiredSourceTags.AddTag(
    FGameplayTag::RequestGameplayTag("Status.LowHealth")
);

// Apply different effect based on tag
if (TargetASC->HasMatchingGameplayTag(ArmoredTag))
{
    ApplyArmorPiercingEffect();
}
else
{
    ApplyNormalDamageEffect();
}

Gameplay Tags
Tag Containers
cpp// Tag Container: Holds multiple tags
FGameplayTagContainer Tags;
Tags.AddTag(FGameplayTag::RequestGameplayTag("State.Combat.Attacking"));
Tags.AddTag(FGameplayTag::RequestGameplayTag("State.Moving"));

// Checking tags:
bool HasExact = Tags.HasTagExact(AttackingTag);           // Exact match only
bool HasAny = Tags.HasAny(RequiredTags);                  // Has at least one
bool HasAll = Tags.HasAll(RequiredTags);                  // Has all tags
bool MatchesQuery = Tags.MatchesQuery(TagQuery);          // Complex logic
Tag Queries (Advanced Logic)
cppFGameplayTagQuery Query;

// Require ("State.Combat.Attacking" AND "Weapon.Melee") 
// AND NOT "Status.Stunned"
Query = FGameplayTagQuery::MakeQuery_MatchAllTags(RequiredTags)
    .IgnoreTags(IgnoredTags);

// Custom query:
// (Has "State.Combat") AND (Has "Weapon.Melee" OR "Weapon.Magic")
Tag Relationships in Abilities
cpp// ABILITY TAGS: What IS this ability
AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Attack.Melee"));

// ACTIVATION OWNED TAGS: Tags ADDED when ability activates
ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Attacking"));
// While ability runs, actor has "State.Attacking"
// When ability ends, tag auto-removed

// ACTIVATION REQUIRED TAGS: Must HAVE these to activate
ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("State.Grounded"));
// Can only activate if on ground

// ACTIVATION BLOCKED TAGS: Must NOT HAVE these to activate
ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Stunned"));
// Can't activate if stunned

// BLOCK ABILITIES WITH TAG: Prevents OTHER abilities from activating
BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Combat"));
// Blocks all combat abilities while this runs

// CANCEL ABILITIES WITH TAG: Force-ends OTHER abilities
CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("State.Attack"));
// Cancels all attacks when this activates
```

### Tag Hierarchy

Tags use parent-child relationships:
```
State                         ← Parent
├── State.Combat              ← Child of State
│   ├── State.Combat.Attacking
│   └── State.Combat.Blocking
└── State.Movement
    ├── State.Movement.Walking
    └── State.Movement.Sprinting
cpp// HasMatchingGameplayTag checks EXACT match
ASC->HasMatchingGameplayTag("State.Combat.Attacking");  // Only this tag

// HasTag checks tag HIERARCHY
ASC->HasTag("State.Combat");  // Matches "State.Combat.Attacking" too

Event System
Gameplay Events vs Ability Triggers
Gameplay Event: Message sent through ASC with data
cpp// 1. Define event data
FGameplayEventData EventData;
EventData.Instigator = DamageDealer;
EventData.Target = DamageReceiver;
EventData.EventMagnitude = DamageAmount;

// 2. Send event
ASC->HandleGameplayEvent(
    FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact"),
    &EventData
);

// 3. Ability receives event via trigger
FAbilityTriggerData Trigger;
Trigger.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.HitReact");
Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
AbilityTriggers.Add(Trigger);

// 4. Access event data in ActivateAbility
void ActivateAbility(..., const FGameplayEventData* TriggerEventData)
{
    if (TriggerEventData)
    {
        AActor* Attacker = TriggerEventData->Instigator;
        float Damage = TriggerEventData->EventMagnitude;
    }
}
Event Data Payload
cppstruct FGameplayEventData
{
    FGameplayTag EventTag;              // What event is this
    AActor* Instigator;                 // Who caused it
    AActor* Target;                     // Who received it
    UObject* OptionalObject;            // Extra object reference
    UObject* OptionalObject2;           // Another extra reference
    
    FGameplayTagContainer InstigatorTags;
    FGameplayTagContainer TargetTags;
    
    float EventMagnitude;               // Numeric value (damage, heal, etc)
    
    FHitResult ContextHandle;           // Where/how hit happened
};
Broadcasting Custom Events
cpp// In your game code, define custom event delegate:
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboChanged, int32, NewComboIndex);

class UGA_Meleecombo : public UGameplayAbility
{
    UPROPERTY(BlueprintAssignable)
    FOnComboChanged OnComboChanged;
    
    void TriggerNextCombo()
    {
        CurrentComboIndex++;
        OnComboChanged.Broadcast(CurrentComboIndex);  // Notify listeners
    }
};

// In Blueprint or C++, bind to event:
MeleeAbility->OnComboChanged.AddDynamic(this, &AMyCharacter::HandleComboChange);

Network Replication
Replication Modes
cpp// COND_None: Always replicate to all clients
DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Health, COND_None, REPNOTIFY_Always);

// COND_OwnerOnly: Only replicate to owning client
DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Stamina, COND_OwnerOnly, REPNOTIFY_Always);

// COND_SkipOwner: Replicate to everyone EXCEPT owner
DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Shield, COND_SkipOwner, REPNOTIFY_Always);

// COND_SimulatedOnly: Only to simulated proxies (not autonomous)
DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Mana, COND_SimulatedOnly, REPNOTIFY_Always);

// REPNOTIFY_Always: Call OnRep even if value unchanged
// REPNOTIFY_OnChanged: Only call OnRep if value actually changed
Prediction
Local Prediction means client runs ability immediately without waiting for server:
cpp// LocalPredicted: Client predicts, server validates
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

// Flow:
// 1. Client: Start ability instantly (feels responsive)
// 2. Client: Send RPC to server
// 3. Server: Validate and run ability
// 4. Server: Send result back to client
// 5. Client: If mismatch, rollback and apply server state
Prediction Keys track predicted actions:
cpp// Client generates prediction key when activating
FPredictionKey PredictionKey = ASC->ScopedPredictionKey;

// Server receives same key, validates action
// If valid: Action confirmed
// If invalid: Client rolls back
Misprediction Handling:
cpp// Client predicted Health = 80 (after taking 20 damage)
// Server calculates Health = 75 (armor absorbed 5 damage)
// → Client receives correction
// → Health snaps to 75
// → Prediction key invalidated
RPCs in GAS
cpp// Server RPC: Client → Server
UFUNCTION(Server, Reliable)
void ServerHandleComboInput();

void UGA_Meleecombo::ServerHandleComboInput_Implementation()
{
    // Runs on server only
    if (bComboWindowOpen)
    {
        TriggerNextCombo();
    }
}

// Client RPC: Server → Client
UFUNCTION(Client, Reliable)
void ClientNotifyDeath();

void UAttributeSetBase::ClientNotifyDeath_Implementation()
{
    // Runs on owning client only
    PlayDeathVFX();
}

// Multicast RPC: Server → All Clients
UFUNCTION(NetMulticast, Reliable)
void MulticastPlayHitEffect(FVector Location);

void UAttributeSetBase::MulticastPlayHitEffect_Implementation(FVector Location)
{
    // Runs on server + all clients
    SpawnHitParticles(Location);
}
Authority Checks
cpp// Check if running on server
if (GetOwningActorFromActorInfo()->HasAuthority())
{
    // Server-only logic: apply damage, modify attributes
    SetHealth(NewHealth);
}

// Check if locally controlled
if (IsLocallyControlled())
{
    // Local player logic: play camera shake, UI updates
    UpdateHealthBar();
}

// Check if autonomous proxy (locally controlled client character)
if (IsAutonomousProxy())
{
    // Predicted client logic
}

// Check if simulated proxy (other players' characters on your client)
if (IsSimulatedProxy())
{
    // Non-predicted smoothing logic
}
```

### Replication Flow Example: Taking Damage
```
1. CLIENT (Attacker):
   - Plays attack animation (prediction)
   - Sends input to server via AbilityLocalInputPressed RPC

2. SERVER:
   - Receives input
   - Validates ability activation (CanActivateAbility)
   - Applies Gameplay Effect (damage)
   - PostGameplayEffectExecute runs
   - Health attribute modified
   - Replication system triggers

3. ALL CLIENTS (including attacker):
   - Receive replicated Health value
   - OnRep_Health() called
   - UI updates (health bar)
   - VFX/SFX play (blood splatter, hit sound)

4. TARGET CLIENT:
   - HitReact event triggered
   - GA_HitReact activates
   - Plays hit react animation

Ability Tasks
Common Ability Tasks
cpp// 1. PLAY MONTAGE AND WAIT
UAbilityTask_PlayMontageAndWait* Task = 
    UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this,                    // Owning ability
        NAME_None,               // Task name
        AttackMontage,           // Montage to play
        1.0f,                    // Play rate
        NAME_None,               // Start section
        false                    // Stop when ability ends
    );

Task->OnCompleted.AddDynamic(this, &UGA_Attack::OnMontageComplete);
Task->OnInterrupted.AddDynamic(this, &UGA_Attack::OnMontageInterrupted);
Task->OnCancelled.AddDynamic(this, &UGA_Attack::OnMontageCancelled);
Task->ReadyForActivation();  // Start task

// 2. WAIT FOR EVENT
UAbilityTask_WaitGameplayEvent* EventTask = 
    UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        FGameplayTag::RequestGameplayTag("GameplayEvent.Montage.Hit"),
        nullptr,  // Optional source
        true,     // Only trigger once
        false     // Only match exact tag
    );

EventTask->EventReceived.AddDynamic(this, &UGA_Attack::OnHitEventReceived);
EventTask->ReadyForActivation();

// 3. WAIT DELAY
UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, 2.0f);
DelayTask->OnFinish.AddDynamic(this, &UGA_Spell::OnCastComplete);
DelayTask->ReadyForActivation();

// 4. WAIT FOR ATTRIBUTE CHANGE
UAbilityTask_WaitAttributeChange* AttrTask = 
    UAbilityTask_WaitAttributeChange::WaitForAttributeChange(
        this,
        UAttributeSetBase::GetHealthAttribute()
    );

AttrTask->OnChange.AddDynamic(this, &UGA_HealthMonitor::OnHealthChanged);
AttrTask->ReadyForActivation();

// 5. WAIT FOR INPUT RELEASE
UAbilityTask_WaitInputRelease* InputTask = 
    UAbilityTask_WaitInputRelease::WaitInputRelease(this);

InputTask->OnRelease.AddDynamic(this, &UGA_ChargedAttack::OnReleaseCharge);
InputTask->ReadyForActivation();
Task Lifecycle
cpp// 1. CREATE: Task instantiated
UAbilityTask* Task = UAbilityTask::NewTask(...);

// 2. BIND DELEGATES: Connect callbacks
Task->OnComplete.AddDynamic(this, &UMyAbility::OnTaskComplete);

// 3. ACTIVATE: Task starts running
Task->ReadyForActivation();

// 4. EXECUTE: Task runs (montage plays, timer ticks, etc)

// 5. END: Task completes/cancelled
// - Delegates fire
// - Task auto-destroys
Custom Ability Task Example
cppUCLASS()
class UAbilityTask_ComboWindow : public UAbilityTask
{
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComboWindowDelegate);
    
    UPROPERTY(BlueprintAssignable)
    FComboWindowDelegate OnWindowOpened;
    
    UPROPERTY(BlueprintAssignable)
    FComboWindowDelegate OnWindowClosed;
    
    UFUNCTION(BlueprintCallable)
    static UAbilityTask_ComboWindow* CreateComboWindowTask(
        UGameplayAbility* OwningAbility,
        float WindowDuration);
    
    virtual void Activate() override;
    
private:
    FTimerHandle WindowTimerHandle;
    void CloseWindow();
};
```

---

## Execution Flow Chains

### Complete Attack Flow (Client → Server → All Clients)
```
┌─────────────────────────────────────────────────────────────┐
│ CLIENT (Attacker)                                            │
├─────────────────────────────────────────────────────────────┤
│ 1. Player presses attack button                             │
│ 2. PlayerController::AttackPressed()                         │
│ 3. ASC->AbilityLocalInputPressed(PrimaryAttack)             │
│ 4. GA_Meleecombo::ActivateAbility() [PREDICTED]             │
│    - Check stamina locally                                   │
│    - Play attack montage locally                             │
│ 5. Send ServerTryActivateAbility RPC →                      │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ SERVER                                                        │
├─────────────────────────────────────────────────────────────┤
│ 6. Receive RPC                                               │
│ 7. GA_Meleecombo::CanActivateAbility()                      │
│    - Validate tags (not stunned, not blocking)               │
│    - Validate stamina                                        │
│ 8. GA_Meleecombo::ActivateAbility() [AUTHORITATIVE]         │
│    - CommitAbility() deducts stamina                         │
│    - Play montage on server                                  │
│ 9. AnimNotify "Hit" fires                                    │
│ 10. ApplyDamageGameplayEffect()                              │
│    - Create GE context                                       │
│    - Apply to target's ASC                                   │
│ 11. UAttributeSetBase::PostGameplayEffectExecute()          │
│    - Extract damage from meta-attribute                      │
│    - Calculate new health                                    │
│    - SetHealth(NewHealth) → Marks for replication           │
│ 12. Health replicates to all clients →                      │
│ 13. HandleGameplayEvent("GameplayEvent.HitReact")           │
│    - Triggers GA_HitReact on victim                          │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ ALL CLIENTS (including attacker & victim)                    │
├─────────────────────────────────────────────────────────────┤
│ 14. OnRep_Health() called on victim's attribute set         │
│    - GAMEPLAYATTRIBUTE_REPNOTIFY macro                       │
│    - Update UI (health bar drops)                            │
│ 15. (Victim Client Only) GA_HitReact::ActivateAbility()    │
│    - Play hit react montage                                  │
│    - Play hit VFX/SFX                                        │
│ 16. (All Clients) See victim's montage replicate            │
└─────────────────────────────────────────────────────────────┘
```

### Gameplay Effect Application Chain
```
1. SOURCE: Create GE Spec
   ↓
   FGameplayEffectSpec Created
   - Magnitude calculated (base + modifiers)
   - Context added (who, where, how)
   - Tags evaluated

2. TARGET: ASC receives GE
   ↓
   CheckGameplayEffectStackRules()
   - Can this effect stack?
   - Stack limit reached?

3. TARGET: PreAttributeChange()
   ↓
   - Clamp incoming value
   - Validate ranges

4. TARGET: Apply Modifiers
   ↓
   - Additive → Multiplicative → Division
   - CurrentValue updated

5. TARGET: PostGameplayEffectExecute()
   ↓
   - Extract meta-attributes (Damage, Healing)
   - Apply to persistent attributes (Health)
   - Trigger gameplay events

6. TARGET: OnRep_[Attribute]()
   ↓
   - Replicate to clients
   - Update UI
   - Broadcast delegates

7. LISTENERS: Respond to Change
   ↓
   - UI updates health bar
   - Sound cue plays
   - VFX spawns
Tag Query Evaluation Flow
Actor requests ability activation
   ↓
1. Check ActivationRequiredTags
   - Actor MUST have these tags
   - If missing any: FAIL
   ↓
2. Check ActivationBlockedTags
   - Actor MUST NOT have these tags
   - If has any: FAILContinue with correct block & text  formatDec 13, 20252 / 2   ↓
3. Check BlockAbilitiesWithTag
   - Do any ACTIVE abilities have tags we block?
   - If yes: FAIL
   ↓
4. Check CanActivateAbility() override
   - Custom logic (stamina, state checks)
   - If returns false: FAIL
   ↓
5. ALL CHECKS PASSED → Activate Ability
   ↓
6. Apply ActivationOwnedTags to actor
   - These tags now on actor while ability runs
   ↓
7. Cancel abilities with CancelAbilitiesWithTag
   - Force-end conflicting abilities
   ↓
8. Ability runs...
   ↓
9. EndAbility() called
   ↓
10. Remove ActivationOwnedTags from actor
Event-Driven Ability Activation Flow
┌─────────────────────────────────────────────────────────────┐
│ TRIGGER SOURCE (e.g., AttributeSet taking damage)           │
├─────────────────────────────────────────────────────────────┤
│ 1. Damage applied to Health attribute                       │
│ 2. PostGameplayEffectExecute() detects Health > 0           │
│ 3. Create FGameplayEventData                                │
│    EventData.EventTag = "GameplayEvent.HitReact"            │
│    EventData.Instigator = Attacker                          │
│    EventData.Target = Victim                                │
│    EventData.EventMagnitude = DamageAmount                  │
│ 4. ASC->HandleGameplayEvent(EventTag, &EventData)          │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ ABILITY SYSTEM COMPONENT                                     │
├─────────────────────────────────────────────────────────────┤
│ 5. Iterate through all granted abilities                    │
│ 6. For each ability, check AbilityTriggers array            │
│ 7. Match TriggerTag with EventTag                           │
│    - GA_HitReact has trigger for "GameplayEvent.HitReact"   │
│ 8. Found match → TryActivateAbility()                       │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ TRIGGERED ABILITY (GA_HitReact)                             │
├─────────────────────────────────────────────────────────────┤
│ 9. CanActivateAbility() check                               │
│    - Verify not already in hitstun                          │
│    - Verify not dead                                        │
│ 10. ActivateAbility(Handle, ActorInfo, ActivationInfo,     │
│                      TriggerEventData)  ← Event data passed │
│ 11. Extract data from TriggerEventData                      │
│     AActor* Attacker = TriggerEventData->Instigator;        │
│     float Damage = TriggerEventData->EventMagnitude;        │
│ 12. Calculate hit direction                                 │
│     FVector HitDirection = (Target - Attacker).GetSafeNormal()│
│ 13. Select appropriate hit react montage                    │
│     - Front hit: FrontHitReact                              │
│     - Back hit: BackHitReact                                │
│     - Left/Right hit: SideHitReact                          │
│ 14. Play montage                                            │
│ 15. Apply temporary hitstun effect                          │
│ 16. EndAbility() when montage completes                     │
└─────────────────────────────────────────────────────────────┘

Advanced Concepts
Gameplay Cues
Gameplay Cues = Visual/audio feedback system separate from logic
cpp// THREE TYPES:

// 1. INSTANT: Fire-and-forget (one-shot effects)
FGameplayCueParameters CueParams;
CueParams.Location = HitLocation;
CueParams.Normal = HitNormal;
ASC->ExecuteGameplayCue(
    FGameplayTag::RequestGameplayTag("GameplayCue.Impact.Sword"),
    CueParams
);
// Spawns hit particles, plays hit sound, then destroys

// 2. ADDED: Persistent effect start
ASC->AddGameplayCue(
    FGameplayTag::RequestGameplayTag("GameplayCue.Status.OnFire"),
    CueParams
);
// Character catches fire, flames persist

// 3. REMOVED: Persistent effect end
ASC->RemoveGameplayCue(
    FGameplayTag::RequestGameplayTag("GameplayCue.Status.OnFire")
);
// Flames extinguish
Implementing Gameplay Cue Notifies:
cpp// In GameplayCueNotify_Actor or GameplayCueNotify_Static

// Called once when cue executes (instant)
bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), 
        HitParticles, 
        Parameters.Location
    );
    return true;
}

// Called when cue added (persistent start)
bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
    FireParticleComponent = UGameplayStatics::SpawnEmitterAttached(
        FireParticles,
        Target->GetRootComponent()
    );
    return true;
}

// Called while cue active (every tick if WhileActive implemented)
bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
    // Update fire intensity based on damage
    return true;
}

// Called when cue removed (persistent end)
bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
    if (FireParticleComponent)
    {
        FireParticleComponent->DestroyComponent();
    }
    return true;
}
Reliable vs Unreliable Cues:
cpp// RELIABLE: Guaranteed delivery, use for important feedback
ASC->ExecuteGameplayCue(...);  // Uses reliable multicast RPC

// UNRELIABLE: Fast but can drop packets, use for frequent effects
ASC->ExecuteGameplayCueLocal(...);  // Uses unreliable multicast RPC
// Example: Footstep sounds, frequent particle bursts
Ability Batching
Batching = Combine multiple RPCs into one network packet
cpp// Enable batching in ability
bServerRespectsRemoteAbilityCancellation = true;

// Multiple ability calls in same frame get batched:
ASC->TryActivateAbility(Ability1);  ┐
ASC->TryActivateAbility(Ability2);  ├─ Sent in ONE RPC
ASC->TryActivateAbility(Ability3);  ┘

// Without batching: 3 separate RPCs (more network overhead)
Gameplay Effect Execution Calculations
Execution Calculations = Custom damage formulas
cpp// Create custom execution class
UCLASS()
class UGE_DamageExecution : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()
    
public:
    UGE_DamageExecution()
    {
        // Define which attributes we read
        RelevantAttributesToCapture.Add(DamageStatics().DamageDef);
        RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
    }
    
    virtual void Execute_Implementation(
        const FGameplayEffectCustomExecutionParameters& ExecutionParams,
        FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override
    {
        // Get source and target ASCs
        UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
        UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
        
        // Capture attributes
        float Damage = 0.f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
            DamageStatics().DamageDef, 
            FAggregatorEvaluateParameters(), 
            Damage
        );
        
        float Armor = 0.f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
            DamageStatics().ArmorDef,
            FAggregatorEvaluateParameters(),
            Armor
        );
        
        // CUSTOM FORMULA: Damage reduced by armor percentage
        float DamageReduction = Armor / (Armor + 100.f);  // Diminishing returns
        float FinalDamage = Damage * (1.f - DamageReduction);
        
        // Apply final damage
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(
                DamageStatics().DamageProperty,
                EGameplayModOp::Additive,
                FinalDamage
            )
        );
    }
};
In Gameplay Effect Blueprint:

Set Calculation Type to Custom Calculation Class
Select UGE_DamageExecution
Now this GE uses your custom damage formula

Gameplay Effect Containers
Containers = Bundle GE + Target data + Context together
cppUSTRUCT(BlueprintType)
struct FGameplayEffectContainer
{
    GENERATED_BODY()
    
    // Which effect to apply
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UGameplayEffect> GameplayEffect;
    
    // How to find targets
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETargetingType TargetingType;
    
    // How many targets max
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxTargets = 1;
};

// Usage in ability:
FGameplayEffectContainer& Container = GetEffectContainer(AttackTag);
TArray<AActor*> Targets = FindTargets(Container.TargetingType);

for (AActor* Target : Targets)
{
    ApplyEffectContainer(Container, Target);
}
Attribute-Based Modifiers
MMC (Magnitude Modifier Calculation) = Calculate GE magnitude from attributes
cppUCLASS()
class UMMC_MaxHealth : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()
    
public:
    UMMC_MaxHealth()
    {
        // Capture Level attribute
        LevelDef.AttributeToCapture = UAttributeSetBase::GetLevelAttribute();
        LevelDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
        LevelDef.bSnapshot = false;  // Use current value, not snapshot
        
        RelevantAttributesToCapture.Add(LevelDef);
    }
    
    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override
    {
        float Level = 0.f;
        GetCapturedAttributeMagnitude(LevelDef, Spec, FAggregatorEvaluateParameters(), Level);
        
        // Formula: MaxHealth = 100 + (Level * 10)
        // Level 1: 110 health
        // Level 10: 200 health
        return 100.f + (Level * 10.f);
    }
    
private:
    FGameplayEffectAttributeCaptureDefinition LevelDef;
};
In Gameplay Effect:

Modifier → Magnitude Calculation Type: Custom Calculation Class
Select UMMC_MaxHealth
Now MaxHealth scales with Level automatically

Target Data
Target Data = Information about what was targeted and how
cpp// Create target data
FGameplayAbilityTargetDataHandle TargetDataHandle;

// Single actor target
FGameplayAbilityTargetData_SingleTargetHit* SingleTarget = 
    new FGameplayAbilityTargetData_SingleTargetHit();
SingleTarget->HitResult = HitResult;
TargetDataHandle.Add(SingleTarget);

// Actor array target
FGameplayAbilityTargetData_ActorArray* ActorArray = 
    new FGameplayAbilityTargetData_ActorArray();
ActorArray->TargetActorArray = {Enemy1, Enemy2, Enemy3};
TargetDataHandle.Add(ActorArray);

// Apply effect to all targets
for (int32 i = 0; i < TargetDataHandle.Num(); i++)
{
    const FGameplayAbilityTargetData* Data = TargetDataHandle.Get(i);
    TArray<TWeakObjectPtr<AActor>> Actors = Data->GetActors();
    
    for (AActor* Actor : Actors)
    {
        ApplyGameplayEffectToTarget(Actor);
    }
}
Ability Cost & Cooldown
Cost GE = Resource consumption (stamina, mana, ammo)
cpp// In ability constructor:
CostGameplayEffectClass = UGE_StaminaCost::StaticClass();

// GE_StaminaCost setup:
// - Duration: Instant
// - Modifier: Stamina, Additive, -20
// Result: Deducts 20 stamina when ability activates
Cooldown GE = Time before ability can be used again
cpp// In ability constructor:
CooldownGameplayEffectClass = UGE_AttackCooldown::StaticClass();

// GE_AttackCooldown setup:
// - Duration: Has Duration, 2.0 seconds
// - Granted Tags: Cooldown.Ability.Attack
// - While this effect active, ability can't activate (ActivationBlockedTags)

// Check remaining cooldown:
float RemainingCooldown = ASC->GetCooldownTimeRemaining(CooldownTag);

// Modify cooldown duration:
FGameplayEffectSpec* CooldownSpec = GetCooldownGameplayEffect();
CooldownSpec->SetDuration(1.5f);  // Override to 1.5 seconds
Granted Abilities & Effects
Granted by GE = Effects can give abilities/other effects
cpp// In Gameplay Effect:
GrantedAbilities.Add(UGA_Dash::StaticClass());
// When GE applied → Target gains Dash ability
// When GE removed → Dash ability removed

GrantedApplicationImmunityTags.AddTag(FGameplayTag::RequestGameplayTag("Status.Stunned"));
// While this GE active → Immune to stun effects

OngoingTagRequirements.IgnoreTags.AddTag(FGameplayTag::RequestGameplayTag("Status.Dead"));
// If actor gets "Status.Dead" tag → This GE auto-removed
Attribute Aggregators
Aggregators = Combine multiple modifiers on same attribute
cpp// Example: Multiple effects modify Damage attribute
// GE 1: +10 damage (additive)
// GE 2: +20% damage (multiplicative)
// GE 3: +5 damage (additive)

// Aggregator calculation:
// 1. Sum all additives: 10 + 5 = 15
// 2. Apply multiplicatives: 15 * 1.20 = 18
// 3. Final damage modifier: +18

// Query aggregator:
float EvaluatedValue = 0.f;
FAggregatorEvaluateParameters EvalParams;
ASC->GetGameplayAttributeValue(DamageAttribute, EvalParams, EvaluatedValue);

Debugging & Troubleshooting
Console Commands
cpp// ABILITY SYSTEM DEBUG DISPLAY
showdebug abilitysystem
// Shows: Active abilities, active effects, attribute values, granted tags

// Cycle through debug categories
AbilitySystem.Debug.NextCategory

// Target specific actor for debugging
AbilitySystem.Debug.Target PlayerCharacter_1

// GAMEPLAY TAG DEBUGGING
GameplayTags.PrintReplicationIndices
// Shows tag replication info

// Print all registered tags
GameplayTags.PrintReport

// ATTRIBUTE DEBUGGING
// In C++, add to PostGameplayEffectExecute:
UE_LOG(LogTemp, Warning, TEXT("Health changed: %.2f -> %.2f (Damage: %.2f)"), 
    OldHealth, GetHealth(), DamageDone);

// NETWORK DEBUGGING
net PktLag=100  // Simulate 100ms latency
net PktLoss=5   // Simulate 5% packet loss
stat net        // Show network stats
Common Debug Breakpoints
cpp// 1. ABILITY NOT ACTIVATING
// Breakpoint in:
bool UGameplayAbility::CanActivateAbility(...)
{
    // Check return value and why it's false
    if (!CheckCost(...)) return false;  // ← Breakpoint here
    if (!CheckCooldown(...)) return false;  // ← And here
    return Super::CanActivateAbility(...);
}

// 2. ATTRIBUTE NOT REPLICATING
// Breakpoint in:
void UAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    // Verify this function is called on clients
    UE_LOG(LogTemp, Warning, TEXT("OnRep_Health called: %.2f"), GetHealth());
}

// 3. DAMAGE NOT APPLYING
// Breakpoint in:
void UAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        // Check damage value
        const float DamageDone = GetDamage();  // ← Breakpoint here
        // Check health calculation
        const float NewHealth = GetHealth() - DamageDone;  // ← And here
    }
}

// 4. GAMEPLAY EVENT NOT TRIGGERING
// Breakpoint in:
int32 UAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
    // Verify event tag matches
    // Check if any abilities have matching trigger
    return TriggeredAbilityCount;  // ← Breakpoint here
}
Visual Debugging
cpp// DRAW DEBUG HELPERS

// In ActivateAbility or OnMontageHit:
DrawDebugSphere(
    GetWorld(),
    HitLocation,
    50.f,  // Radius
    12,    // Segments
    FColor::Red,
    false,  // Persistent
    2.0f   // Lifetime
);

// Draw hit direction
DrawDebugDirectionalArrow(
    GetWorld(),
    AttackerLocation,
    VictimLocation,
    50.f,  // Arrow size
    FColor::Yellow,
    false,
    2.0f
);

// Draw trace line (for melee attacks)
DrawDebugLine(
    GetWorld(),
    TraceStart,
    TraceEnd,
    FColor::Green,
    false,
    1.0f
);

// Display text above actor
DrawDebugString(
    GetWorld(),
    Actor->GetActorLocation() + FVector(0, 0, 100),
    FString::Printf(TEXT("Health: %.0f"), Health),
    nullptr,
    FColor::White,
    2.0f
);
Logging Best Practices
cpp// DECLARE LOG CATEGORY (in header or cpp top)
DEFINE_LOG_CATEGORY_STATIC(LogGAS, Log, All);

// USE APPROPRIATE LOG LEVELS

// Verbose: Extremely detailed (usually disabled in shipping)
UE_LOG(LogGAS, Verbose, TEXT("Checking ability activation..."));

// Log: General information
UE_LOG(LogGAS, Log, TEXT("Ability activated: %s"), *GetName());

// Warning: Something unusual happened
UE_LOG(LogGAS, Warning, TEXT("Ability activation failed: Not enough stamina"));

// Error: Something went wrong
UE_LOG(LogGAS, Error, TEXT("Failed to apply damage effect: Target ASC is null"));

// NETWORK CONTEXT LOGGING
UE_LOG(LogGAS, Log, TEXT("[%s] ComboIndex: %d"), 
    HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), 
    CurrentComboIndex);
// Output: [SERVER] ComboIndex: 2
// Output: [CLIENT] ComboIndex: 2

// CONDITIONAL LOGGING
if (GAS_VERBOSE_LOGGING)
{
    UE_LOG(LogGAS, Log, TEXT("Detailed attribute changes..."));
}
Common Issues & Solutions
Issue 1: Ability Activates on Client but Not Server
cpp// PROBLEM: Client prediction runs, but server rejects

// CHECK 1: Network execution policy
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;  // ✓ Correct

// CHECK 2: Authority check mistakes
void ActivateAbility(...)
{
    // ❌ WRONG: Only runs on server
    if (HasAuthority())
    {
        PlayMontage();  // Clients won't see this!
    }
    
    // ✓ CORRECT: Runs on both, replicates
    PlayMontage();  // Montage system handles replication
}

// CHECK 3: Validate cost/cooldown on both
if (!CommitAbility(...))  // This checks both client and server
{
    EndAbility(...);
    return;
}
Issue 2: Attributes Not Syncing Between Clients
cpp// PROBLEM: Health changes on server but clients don't see it

// CHECK 1: Replication setup
void UAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // ✓ MUST HAVE THIS:
    DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
}

// CHECK 2: OnRep function exists
void UAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Health, OldHealth);
}

// CHECK 3: Actor replicates
AMyCharacter::AMyCharacter()
{
    bReplicates = true;  // ✓ MUST BE TRUE
    NetUpdateFrequency = 66.0f;  // How often to replicate (Hz)
}

// CHECK 4: ASC replication mode
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
// Full: Replicate everything (expensive)
// Mixed: GE replicated to owner, minimal to simulated (recommended)
// Minimal: Only tags/cues replicate (competitive games)
Issue 3: Montage Plays but Damage Doesn't Apply
cpp// PROBLEM: Animation plays but no damage happens

// CHECK 1: AnimNotify exists and calls correct function
// In montage: Add AnimNotify_GameplayEvent
// Event Tag: "Event.Montage.Hit"

// CHECK 2: Ability listens for event
void UGA_Meleecombo::ActivateAbility(...)
{
    // Create task to wait for montage event
    UAbilityTask_WaitGameplayEvent* EventTask = 
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
            this,
            FGameplayTag::RequestGameplayTag("Event.Montage.Hit")
        );
    
    EventTask->EventReceived.AddDynamic(this, &UGA_Meleecombo::OnHitEvent);
    EventTask->ReadyForActivation();
}

void UGA_Meleecombo::OnHitEvent(FGameplayEventData Payload)
{
    // ✓ This should fire when AnimNotify triggers
    ApplyDamage();  // ← If this doesn't run, event not wired correctly
}

// CHECK 3: Damage application has authority
void UGA_Meleecombo::ApplyDamage()
{
    if (!HasAuthority())
    {
        // ❌ WRONG: Damage must be server-authoritative
        return;
    }
    
    // ✓ CORRECT: Only server applies damage
    ASC->ApplyGameplayEffectToTarget(...);
}
Issue 4: Combo System Desyncs
cpp// PROBLEM: Client shows combo 3, server shows combo 2

// CHECK 1: CurrentComboIndex is replicated
UPROPERTY(Replicated)
int32 CurrentComboIndex;

// CHECK 2: Only server modifies replicated variable
void TriggerNextCombo()
{
    if (HasAuthority())  // ✓ Server-only
    {
        CurrentComboIndex++;
    }
    
    // Client predicts locally (will be corrected if wrong)
    int32 PredictedIndex = CurrentComboIndex + 1;
    PlayComboMontage(PredictedIndex);
}

// CHECK 3: OnRep function synchronizes clients
UFUNCTION()
void OnRep_CurrentComboIndex()
{
    // Client receives server's correct value
    // If prediction was wrong, correct it here
    if (GetWorld()->GetNetMode() != NM_DedicatedServer)
    {
        RefreshComboUI(CurrentComboIndex);
    }
}
Issue 5: Block Not Reducing Damage
cpp// PROBLEM: Block animation plays but damage taken is full

// CHECK 1: BlockEffect is assigned
if (!CachedWeaponData->BlockEffect)
{
    UE_LOG(LogGAS, Error, TEXT("BlockEffect is null! Assign in weapon data asset"));
    return;
}

// CHECK 2: BlockEffect is applied
FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BlockEffect, 1, Context);
ActiveBlockEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

if (!ActiveBlockEffectHandle.IsValid())
{
    UE_LOG(LogGAS, Error, TEXT("Failed to apply BlockEffect"));
}

// CHECK 3: BlockEffect modifies damage correctly
// In GE_Block blueprint:
// - Modifier: Damage (incoming), Multiplicative, 0.5
// - Granted Tags: State.Blocking
// This reduces incoming damage by 50%

// CHECK 4: Damage system respects modifiers
// In damage GE, ensure it modifies the Damage attribute
// Damage flows: GE_Damage → Damage attribute → PostGameplayEffectExecute → Health
Issue 6: Ability Not Ending
cpp// PROBLEM: Ability stays active forever

// CHECK 1: EndAbility is called
void UGA_Meleecombo::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    // ✓ MUST CALL EndAbility when done
}

// CHECK 2: Task cleanup
void UGA_Meleecombo::EndAbility(...)
{
    // ✓ Cancel tasks before ending
    if (MontageTask)
    {
        MontageTask->EndTask();
        MontageTask = nullptr;
    }
    
    Super::EndAbility(...);
}

// CHECK 3: Check for infinite loops
void ActivateAbility(...)
{
    // ❌ WRONG: Never-ending ability
    while (bIsAttacking)  // This will hang!
    {
        Attack();
    }
    
    // ✓ CORRECT: Use timers or tasks
    GetWorld()->GetTimerManager().SetTimer(
        AttackTimerHandle,
        this,
        &UGA_Meleecombo::PerformNextAttack,
        0.5f,
        true  // Repeating
    );
}
Network Testing Setup
cpp// EDITOR SETTINGS for multiplayer testing

// 1. Play as Listen Server + Clients
// Editor Preferences → Play → Multiplayer Options
// Number of Players: 3
// Net Mode: Play As Listen Server

// 2. Simulate lag
// Console command: net PktLag=100

// 3. Simulate packet loss
// Console command: net PktLoss=10

// 4. Dedicated server testing
// Editor Preferences → Play
// Net Mode: Play As Client
// Run Dedicated Server: true

// 5. Package and test
// Project Settings → Packaging
// Build Configuration: Development
// Package project, run with: MyGame.exe -log
GAS-Specific Debug Cvars
cpp// Enable verbose GAS logging
AbilitySystem.Debug.NextCategory

// Show ability costs/cooldowns
showdebug AbilitySystem

// Log all ability activations
AbilitySystem.LogAbilityActivations 1

// Log all gameplay effect applications
AbilitySystem.LogGameplayEffects 1

// Log attribute changes
AbilitySystem.LogAttributeChanges 1

// Show prediction keys
AbilitySystem.Debug.ShowPredictionKeys 1
Memory Leak Detection
cpp// CHECK 1: Ability tasks not cleaned up
void UGA_MyAbility::EndAbility(...)
{
    // ✓ Always end tasks
    if (WaitTask)
    {
        WaitTask->EndTask();
        WaitTask = nullptr;  // Clear reference
    }
    
    Super::EndAbility(...);
}

// CHECK 2: Gameplay effects not removed
void UGA_Block::CancelAbility(...)
{
    // ✓ Remove applied effects
    if (ActiveBlockEffectHandle.IsValid())
    {
        ASC->RemoveActiveGameplayEffect(ActiveBlockEffectHandle);
    }
    
    Super::CancelAbility(...);
}

// CHECK 3: Timers not cleared
void UGA_MyAbility::EndAbility(...)
{
    // ✓ Clear timers
    GetWorld()->GetTimerManager().ClearTimer(MyTimerHandle);
    
    Super::EndAbility(...);
}

// CHECK 4: Delegates not unbound
void UGA_MyAbility::EndAbility(...)
{
    // ✓ Unbind delegates
    if (MontageTask)
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
    }
    
    Super::EndAbility(...);
}

Additional Concepts & Patterns
Ability Queue System
cpp// Queue abilities when player input comes during ability execution

class UAbilityQueueComponent : public UActorComponent
{
public:
    void QueueAbility(TSubclassOf<UGameplayAbility> AbilityClass)
    {
        AbilityQueue.Add(AbilityClass);
    }
    
    void ProcessQueue()
    {
        if (AbilityQueue.Num() > 0 && !ASC->IsAbilityActive())
        {
            TSubclassOf<UGameplayAbility> NextAbility = AbilityQueue[0];
            AbilityQueue.RemoveAt(0);
            
            ASC->TryActivateAbilityByClass(NextAbility);
        }
    }
    
private:
    TArray<TSubclassOf<UGameplayAbility>> AbilityQueue;
};

// Usage in combo system:
if (CurrentAbilityRunning)
{
    QueueComponent->QueueAbility(NextComboAbility);
}
Loosely-Coupled Event System
cpp// Instead of tight coupling: Ability → Character → Weapon
// Use event tags: Ability → ASC Event → Any Listener

// Weapon listens for equip event
void AWeapon::Begincontinue in from block correctly fromwhere you left offDec 13, 20252 / 2Pinpointed continuation point within event system documentation.The user wants me to continue from where I left off. I was in the middle of the "Loosely-Coupled Event System" section under "Additional Concepts & Patterns". Specifically, I was writing about how weapons listen for equip events. Let me continue from there.cpp// Weapon listens for equip event
void AWeapon::BeginPlay()
{
    Super::BeginPlay();
    
    // Bind to gameplay events
    if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
    {
        ASC->GenericGameplayEventCallbacks.FindOrAdd(
            FGameplayTag::RequestGameplayTag("Event.Weapon.Equipped")
        ).AddUObject(this, &AWeapon::OnEquipped);
        
        ASC->GenericGameplayEventCallbacks.FindOrAdd(
            FGameplayTag::RequestGameplayTag("Event.Weapon.Unequipped")
        ).AddUObject(this, &AWeapon::OnUnequipped);
    }
}

void AWeapon::OnEquipped(const FGameplayEventData* Payload)
{
    // React to being equipped
    SetActorHiddenInGame(false);
    AttachToCharacter();
}

// Ability fires event instead of direct function call
void UGA_EquipWeapon::ActivateAbility(...)
{
    FGameplayEventData EventData;
    EventData.OptionalObject = WeaponToEquip;
    
    ASC->HandleGameplayEvent(
        FGameplayTag::RequestGameplayTag("Event.Weapon.Equipped"),
        &EventData
    );
}
Snapshot vs Non-Snapshot Attributes
Snapshot = Capture attribute value when GE created
Non-Snapshot = Use current attribute value when GE executes
cpp// In MMC (Modifier Magnitude Calculation):

FGameplayEffectAttributeCaptureDefinition CaptureDef;
CaptureDef.AttributeToCapture = UAttributeSetBase::GetDamageAttribute();
CaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;

// SNAPSHOT: Use value at GE creation time
CaptureDef.bSnapshot = true;
// Example: Arrow fired with 50 damage, attacker buffs to 100 damage
// Arrow still does 50 damage (snapshotted)

// NON-SNAPSHOT: Use value at GE execution time
CaptureDef.bSnapshot = false;
// Example: Arrow fired with 50 damage, attacker buffs to 100 damage
// Arrow now does 100 damage (uses current value)

// Use snapshot for: Projectiles, delayed effects
// Use non-snapshot for: DOT effects, auras that scale with current stats
Gameplay Effect Stacking Policies
cpp// In Gameplay Effect blueprint:

// STACK OVERFLOW POLICY
StackOverflowPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
// Options:
// - ClearEntireStack: Remove all stacks when limit reached
// - DenyNewApplication: Refuse new stack, keep existing
// - RemoveOldestStack: Remove first stack, add new one

// STACK DURATION REFRESH
StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
// Options:
// - RefreshOnSuccessfulApplication: Reset timer when new stack added
// - NeverRefresh: Each stack has independent timer

// Example: Poison stacking
// MaxStacks: 5
// Duration: 10 seconds per stack
// StackOverflowPolicy: DenyNewApplication
// StackDurationRefreshPolicy: RefreshOnSuccessfulApplication

// Scenario:
// Apply stack 1 at t=0s  → 1 stack, expires at t=10s
// Apply stack 2 at t=3s  → 2 stacks, both expire at t=13s (refreshed)
// Apply stack 3 at t=5s  → 3 stacks, all expire at t=15s
// Apply stack 4-5...
// Try stack 6 at t=7s   → DENIED (overflow policy)
Immunity Systems
cpp// GRANTED APPLICATION IMMUNITY TAGS
// In Gameplay Effect: Prevents other GEs with certain tags from applying

// GE_ShieldBuff grants immunity to fire
GrantedApplicationImmunityTags.AddTag(FGameplayTag::RequestGameplayTag("Damage.Type.Fire"));

// When character has this effect:
// - Fire damage GEs fail to apply
// - Other damage types still work

// CHECK IMMUNITY QUERY
FGameplayTagContainer SourceTags;
SourceTags.AddTag(FGameplayTag::RequestGameplayTag("Damage.Type.Fire"));

FGameplayEffectQuery Query;
Query.IgnoringTags = SourceTags;

TArray<FActiveGameplayEffectHandle> Effects = 
    ASC->GetActiveEffectsWithAllTags(Query.OwningTagQuery.GetGameplayTagArray());

if (Effects.Num() > 0)
{
    // Character is immune to fire
}
Cooldown Remaining Time
cpp// Get remaining cooldown for ability
FGameplayTagContainer CooldownTags;
CooldownTags.AddTag(FGameplayTag::RequestGameplayTag("Cooldown.Ability.Attack"));

float TimeRemaining = 0.f;
float CooldownDuration = 0.f;

if (ASC->GetCooldownTimeRemaining(CooldownTags, TimeRemaining, CooldownDuration))
{
    // Ability is on cooldown
    float Progress = TimeRemaining / CooldownDuration;  // 0.0 to 1.0
    
    // Update UI cooldown wheel
    UpdateCooldownUI(Progress);
}
else
{
    // Ability ready to use
}

// Modify cooldown duration dynamically
FGameplayEffectQuery Query;
Query.EffectDefinition = CooldownEffectClass;

TArray<FActiveGameplayEffectHandle> ActiveCooldowns = 
    ASC->GetActiveEffects(Query);

if (ActiveCooldowns.Num() > 0)
{
    // Reduce cooldown by 50%
    FActiveGameplayEffect* ActiveEffect = 
        ASC->GetActiveGameplayEffect(ActiveCooldowns[0]);
    
    if (ActiveEffect)
    {
        float NewDuration = ActiveEffect->GetDuration() * 0.5f;
        ASC->SetActiveGameplayEffectDuration(ActiveCooldowns[0], NewDuration);
    }
}
Animation-Driven Gameplay
Root Motion in GAS abilities:
cppvoid UGA_Dash::ActivateAbility(...)
{
    // Play montage with root motion enabled
    UAbilityTask_PlayMontageAndWait* Task = 
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            DashMontage,
            1.0f,
            NAME_None,
            true,   // bStopWhenAbilityEnds
            1.0f,
            0.0f,
            true    // bReplicateMontage (important for multiplayer)
        );
    
    Task->ReadyForActivation();
    
    // Root motion automatically moves character
    // No need to manually update location
}

// In montage: Enable "Root Motion Root Lock"
// Character smoothly moves with animation
Animation Notifies as Ability Events:
cpp// In AnimNotify_GameplayEvent:
// - Notify name: "AttackHit"
// - Gameplay Tag: Event.Montage.AttackHit

// In ability:
void UGA_Attack::ActivateAbility(...)
{
    // Listen for all anim notify events
    UAbilityTask_WaitGameplayEvent* EventTask = 
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
            this,
            FGameplayTag::RequestGameplayTag("Event.Montage"),
            nullptr,
            false,  // Don't trigger only once
            false   // Match child tags
        );
    
    EventTask->EventReceived.AddDynamic(this, &UGA_Attack::OnAnimEvent);
    EventTask->ReadyForActivation();
}

void UGA_Attack::OnAnimEvent(FGameplayEventData EventData)
{
    // EventData.EventTag tells you which notify fired
    if (EventData.EventTag.MatchesTag(FGameplayTag::RequestGameplayTag("Event.Montage.AttackHit")))
    {
        PerformHitDetection();
    }
    else if (EventData.EventTag.MatchesTag(FGameplayTag::RequestGameplayTag("Event.Montage.ComboWindow")))
    {
        bComboWindowOpen = true;
    }
}
Advanced Tag Queries
cpp// COMPLEX TAG LOGIC

FGameplayTagQuery Query;

// Query: (Has "Class.Warrior" OR "Class.Knight") AND NOT "Status.Dead"
FGameplayTagQuery ClassQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(
    FGameplayTagContainer::CreateFromArray(
        {
            FGameplayTag::RequestGameplayTag("Class.Warrior"),
            FGameplayTag::RequestGameplayTag("Class.Knight")
        }
    )
);

FGameplayTagQuery DeadQuery = FGameplayTagQuery::MakeQuery_MatchNoTags(
    FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Status.Dead"))
);

// Combine queries
Query = FGameplayTagQuery::BuildQuery(
    FGameplayTagQueryExpression()
        .AllTagsMatch()
        .AddTag(ClassQuery)
        .AddTag(DeadQuery)
);

// Use in ability
bool CanActivate = Query.Matches(ASC->GetOwnedGameplayTags());
Gameplay Effect Removal
cpp// REMOVE BY HANDLE
FActiveGameplayEffectHandle EffectHandle = 
    ASC->ApplyGameplayEffectToSelf(Effect);

// Later remove:
ASC->RemoveActiveGameplayEffect(EffectHandle);

// REMOVE BY TAG
FGameplayTagContainer TagsToRemove;
TagsToRemove.AddTag(FGameplayTag::RequestGameplayTag("Status.Burning"));

int32 NumRemoved = ASC->RemoveActiveEffectsWithTags(TagsToRemove);

// REMOVE BY SOURCE
// Remove all effects applied by specific actor
ASC->RemoveActiveEffectsWithSourceTags(
    SourceActor->GetAbilitySystemComponent()->GetOwnedGameplayTags()
);

// REMOVE BY QUERY
FGameplayEffectQuery Query;
Query.EffectDefinition = BuffEffectClass;
Query.IgnoreHandles.Add(PermanentBuffHandle);

ASC->RemoveActiveEffects(Query);

// REMOVE ALL EFFECTS
ASC->RemoveAllActiveEffects();
Effect Context Handles - Extended Data
cpp// CUSTOM CONTEXT for damage types, hit directions, etc.

USTRUCT()
struct FGASGameplayEffectContext : public FGameplayEffectContext
{
    GENERATED_BODY()
    
public:
    // Custom data
    UPROPERTY()
    FVector HitDirection;
    
    UPROPERTY()
    bool bIsCriticalHit = false;
    
    UPROPERTY()
    EDamageType DamageType = EDamageType::Physical;
    
    // Required for replication
    virtual UScriptStruct* GetScriptStruct() const override
    {
        return FGASGameplayEffectContext::StaticStruct();
    }
    
    virtual FGameplayEffectContext* Duplicate() const override
    {
        FGASGameplayEffectContext* NewContext = new FGASGameplayEffectContext();
        *NewContext = *this;
        NewContext->AddActors(Actors);
        if (GetHitResult())
        {
            NewContext->AddHitResult(*GetHitResult(), true);
        }
        return NewContext;
    }
    
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override
    {
        Super::NetSerialize(Ar, Map, bOutSuccess);
        
        // Serialize custom data
        Ar << HitDirection;
        Ar << bIsCriticalHit;
        uint8 DamageTypeInt = (uint8)DamageType;
        Ar << DamageTypeInt;
        DamageType = (EDamageType)DamageTypeInt;
        
        return true;
    }
};

// Usage:
FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
FGASGameplayEffectContext* CustomContext = 
    static_cast<FGASGameplayEffectContext*>(ContextHandle.Get());

CustomContext->HitDirection = (Target->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
CustomContext->bIsCriticalHit = FMath::RandRange(0.f, 1.f) < CritChance;
CustomContext->DamageType = EDamageType::Fire;

// Later extract:
void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    const FGASGameplayEffectContext* Context = 
        static_cast<const FGASGameplayEffectContext*>(Data.EffectSpec.GetContext().Get());
    
    if (Context->bIsCriticalHit)
    {
        // Apply extra damage
        float CritMultiplier = 2.0f;
        SetDamage(GetDamage() * CritMultiplier);
    }
}
Gameplay Effect Period & Execution
cpp// PERIODIC EFFECTS with dynamic period

// In GE blueprint:
// Period: 1.0 seconds (base)
// bExecutePeriodicEffectOnApplication: true

// Modify period at runtime:
FGameplayEffectSpec* Spec = EffectHandle.Data.Get();
if (Spec)
{
    // Speed up periodic effect
    Spec->Period = 0.5f;  // Now ticks every 0.5 seconds instead of 1.0
}

// STAGGERED PERIODIC EFFECTS
// Multiple DOTs ticking at different intervals:
// Poison: Ticks every 1.0s
// Bleed: Ticks every 0.5s
// Burn: Ticks every 1.5s
// Result: Damage comes in waves, not all at once
Server-Side Validation Patterns
cpp// CLIENT PREDICTION + SERVER VALIDATION

void UGA_Attack::ActivateAbility(...)
{
    // Client predicts attack
    if (IsLocallyControlled())
    {
        PlayAttackMontage();  // Instant feedback
    }
    
    // Server validates
    if (HasAuthority())
    {
        // Check stamina again (client could be cheating)
        if (GetStamina() < StaminaCost)
        {
            // Client tried to activate without stamina
            UE_LOG(LogGAS, Warning, TEXT("Client failed server-side stamina check"));
            EndAbility(...);
            return;
        }
        
        // Validate attack range
        float Distance = FVector::Dist(GetAvatarActorFromActorInfo()->GetActorLocation(), 
                                       TargetActor->GetActorLocation());
        if (Distance > MaxAttackRange)
        {
            UE_LOG(LogGAS, Warning, TEXT("Client outside attack range"));
            EndAbility(...);
            return;
        }
        
        // Server approves, apply damage
        ApplyDamageEffect();
    }
}

// If server rejects: Client's prediction rolled back, ability cancelled
Attribute Curves & Scaling
cpp// CURVE-BASED SCALING for attributes

UCLASS()
class UMMC_ScaledDamage : public UGameplayModMagnitudeCalculation
{
public:
    UMMC_ScaledDamage()
    {
        // Capture Level
        LevelDef.AttributeToCapture = UAttributeSetBase::GetLevelAttribute();
        LevelDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
        LevelDef.bSnapshot = false;
        RelevantAttributesToCapture.Add(LevelDef);
    }
    
    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override
    {
        float Level = 0.f;
        GetCapturedAttributeMagnitude(LevelDef, Spec, FAggregatorEvaluateParameters(), Level);
        
        // Use curve table for scaling
        if (DamageCurve)
        {
            float ScaledDamage = DamageCurve->GetFloatValue(Level);
            return BaseDamage * ScaledDamage;
        }
        
        return BaseDamage;
    }
    
private:
    FGameplayEffectAttributeCaptureDefinition LevelDef;
    
    UPROPERTY(EditDefaultsOnly)
    UCurveFloat* DamageCurve;  // Set in editor
    
    UPROPERTY(EditDefaultsOnly)
    float BaseDamage = 10.f;
};

// In DamageCurve:
// Level 1:  1.0x damage
// Level 5:  1.5x damage
// Level 10: 2.5x damage
// Level 20: 5.0x damage
// Smooth curve, not linear jumps
Cheat Detection & Anti-Cheat
cpp// SERVER-SIDE SANITY CHECKS

void UAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    // Server validates attribute changes
    if (GetOwningActor()->HasAuthority())
    {
        // Check for impossible values
        if (GetHealth() > GetMaxHealth())
        {
            UE_LOG(LogGAS, Error, TEXT("Health exceeded MaxHealth! Possible cheat attempt."));
            
            // Clamp and log player
            SetHealth(GetMaxHealth());
            LogSuspiciousActivity(GetOwningActor());
        }
        
        // Check for negative critical attributes
        if (GetHealth() < 0.f)
        {
            UE_LOG(LogGAS, Error, TEXT("Negative health detected!"));
            SetHealth(0.f);
        }
    }
}

// Rate limit ability activations
void UAbilitySystemComponent::TryActivateAbility(...)
{
    if (HasAuthority())
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float TimeSinceLastActivation = CurrentTime - LastAbilityActivationTime;
        
        if (TimeSinceLastActivation < MinTimeBetweenAbilities)
        {
            // Spamming abilities too fast
            UE_LOG(LogGAS, Warning, TEXT("Ability activation rate limit exceeded"));
            return false;
        }
        
        LastAbilityActivationTime = CurrentTime;
    }
    
    // Continue with activation...
}

Performance Optimization
Attribute Set Optimization
cpp// DON'T create many small AttributeSets
// ❌ BAD:
class UHealthAttributeSet : public UAttributeSet { float Health; };
class UStaminaAttributeSet : public UAttributeSet { float Stamina; };
class UManaAttributeSet : public UAttributeSet { float Mana; };

// ✓ GOOD: Combine related attributes
class UCombatAttributeSet : public UAttributeSet 
{
    float Health;
    float Stamina;
    float Mana;
};

// Reason: Fewer attribute sets = fewer replication calls, less overhead
Gameplay Effect Pooling
cpp// For frequently applied effects (damage, healing), pool specs

class UGameplayEffectPool
{
public:
    FGameplayEffectSpecHandle GetPooledSpec(TSubclassOf<UGameplayEffect> EffectClass)
    {
        if (SpecPool.Contains(EffectClass))
        {
            TArray<FGameplayEffectSpecHandle>& Pool = SpecPool[EffectClass];
            if (Pool.Num() > 0)
            {
                FGameplayEffectSpecHandle Spec = Pool.Pop();
                return Spec;
            }
        }
        
        // Create new spec if pool empty
        return ASC->MakeOutgoingSpec(EffectClass, 1, ASC->MakeEffectContext());
    }
    
    void ReturnSpec(FGameplayEffectSpecHandle Spec)
    {
        // Reset spec data
        Spec.Data->SetLevel(1);
        Spec.Data->CapturedRelevantAttributes.Empty();
        
        // Return to pool
        TSubclassOf<UGameplayEffect> EffectClass = Spec.Data->Def->GetClass();
        SpecPool.FindOrAdd(EffectClass).Add(Spec);
    }
    
private:
    TMap<TSubclassOf<UGameplayEffect>, TArray<FGameplayEffectSpecHandle>> SpecPool;
};
Lazy Attribute Calculation
cpp// Don't recalculate attributes every frame

class UAttributeSetBase : public UAttributeSet
{
public:
    // ❌ BAD: Recalculate in Tick
    void TickComponent(float DeltaTime)
    {
        float MovementSpeed = CalculateMovementSpeed();  // Expensive calculation
        Character->GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
    }
    
    // ✓ GOOD: Only recalculate when relevant attributes change
    void OnRep_Stamina(const FGameplayAttributeData& OldStamina)
    {
        GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Stamina, OldStamina);
        
        // Only update when stamina actually changes
        if (bStaminaAffectsMovementSpeed)
        {
            UpdateMovementSpeed();
        }
    }
    
private:
    bool bStaminaAffectsMovementSpeed = true;
    
    void UpdateMovementSpeed()
    {
        float MovementSpeed = CalculateMovementSpeed();
        Character->GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
    }
};
Network Optimization
cpp// REPLICATION MODES comparison:

// Full Replication: Replicate everything to everyone
// - GEs, attributes, tags all replicate fully
// - Use for: Co-op games, small player counts (< 10)
ASC->SetReplicationMode(EGameplayEffectReplicationMode::Full);

// Mixed Replication: Full to owner, minimal to others
// - Owner gets full data, others get tags/cues only
// - Use for: Competitive games, medium player counts (10-50)
ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

// Minimal Replication: Tags and cues only
// - No GE or attribute replication
// - Use for: Battle royale, high player counts (50+)
ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

// CONDITIONAL REPLICATION for attributes:
DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Mana, COND_OwnerOnly, REPNOTIFY_Always);
// Only owner needs to see mana, save bandwidth for other clients
Tag Container Optimization
cpp// ✓ Cache frequently used tags
class UMyGameplayAbility : public UGameplayAbility
{
protected:
    // Cache tags in constructor
    FGameplayTag AttackingTag;
    FGameplayTag StunnedTag;
    FGameplayTag DeadTag;
    
public:
    UMyGameplayAbility()
    {
        // Initialize once
        AttackingTag = FGameplayTag::RequestGameplayTag("State.Attacking");
        StunnedTag = FGameplayTag::RequestGameplayTag("Status.Stunned");
        DeadTag = FGameplayTag::RequestGameplayTag("Status.Dead");
    }
    
    bool CanActivateAbility(...) override
    {
        // ✓ Use cached tags (fast)
        if (ASC->HasMatchingGameplayTag(StunnedTag)) return false;
        
        // ❌ Don't create tags every check (slow)
        if (ASC->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag("Status.Stunned"))) return false;
    }
};
```

---

## Summary & Quick Reference

### GAS Activation Flow (One-Page Reference)
```
INPUT → ASC → ABILITY CHECK → ACTIVATE → EXECUTE → END

1. Player Input
   PlayerController::PressedButton()
   ↓
2. Send to ASC
   ASC->AbilityLocalInputPressed(InputID)
   ↓
3. Find Matching Ability
   ASC checks ActivatableAbilities for matching InputID
   ↓
4. Can Activate?
   - CanActivateAbility(): Tags, resources, custom logic
   - CheckCost(): Enough stamina/mana?
   - CheckCooldown(): Off cooldown?
   ↓
5. Commit
   CommitAbility(): Deduct costs, apply cooldown
   ↓
6. Activate
   ActivateAbility(): Run ability logic
   ↓
7. Apply Effects
   ASC->ApplyGameplayEffectToTarget()
   ↓
8. Modify Attributes
   PostGameplayEffectExecute(): Extract meta, update persistent
   ↓
9. Replicate
   OnRep_Attribute(): Broadcast to clients
   ↓
10. Trigger Events
    HandleGameplayEvent(): Trigger reactive abilities (HitReact, Death)
    ↓
11. End Ability
    EndAbility(): Cleanup, remove tags
Common Patterns Cheat Sheet
cpp// ABILITY ACTIVATION
if (!CommitAbility(...)) { EndAbility(...); return; }

// DAMAGE APPLICATION
const float DamageDone = GetDamage();
SetDamage(0.f);  // Reset meta-attribute
SetHealth(FMath::Max(GetHealth() - DamageDone, 0.f));

// EVENT TRIGGERING
FGameplayEventData EventData;
EventData.Instigator = SourceActor;
EventData.EventMagnitude = Damage;
ASC->HandleGameplayEvent(EventTag, &EventData);

// TAG CHECKING
if (ASC->HasMatchingGameplayTag(BlockedTag)) return false;

// MONTAGE PLAYING
UAbilityTask_PlayMontageAndWait* Task = 
    UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(...);
Task->OnCompleted.AddDynamic(this, &UMyAbility::OnComplete);
Task->ReadyForActivation();

// EFFECT APPLICATION
FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1, Context);
ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

// REPLICATION SETUP
DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Health, COND_None, REPNOTIFY_Always);

void OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Health, OldHealth);
}

// AUTHORITY CHECK
if (HasAuthority()) { /* Server-only logic */ }
if (IsLocallyControlled()) { /* Local player logic */ }
Debugging Checklist
Ability Won't Activate:

 CanActivateAbility() returns true?
 Enough resources (stamina/mana)?
 Not on cooldown?
 Required tags present?
 Blocked tags absent?
 Input ID correctly bound?

Attributes Not Replicating:

 DOREPLIFETIME_CONDITION_NOTIFY in GetLifetimeReplicatedProps?
 OnRep_[Attribute] function exists?
 Actor has bReplicates = true?
 ASC replication mode set?

Damage Not Applying:

 PostGameplayEffectExecute implemented?
 Damage attribute extracted and reset?
 Server authority for damage application?
 Gameplay Effect has correct magnitude?

Network Desync:

 Replicated variables only modified on server?
 OnRep functions handle client correction?
 NetExecutionPolicy set correctly?
 Prediction keys tracked properly?


This completes the comprehensive GAS documentation covering:

Core architecture & attribute system
Abilities, effects, tags, and events
Network replication & prediction
Ability tasks & execution chains
Advanced concepts (cues, MMCs, contexts)
Performance optimization
Complete debugging guide

The notes now provide a full reference for understanding GAS multiplayer combat systems with proper replication, event-driven abilities, and networked gameplay.