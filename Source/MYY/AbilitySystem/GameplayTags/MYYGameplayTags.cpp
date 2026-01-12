#include "MYY/AbilitySystem/GameplayTags/MYYGameplayTags.h"

namespace MYYTags
{
    /* --------------------------- Ability: Ranged --------------------------- */
    UE_DEFINE_GAMEPLAY_TAG(Ability_Ranged_Aim,  "Ability.Ranged.Aim");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Ranged_Fire, "Ability.Ranged.Fire");

    /* ------------------------- GameplayAbility.* --------------------------- */
    UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Movement_Dash, "GameplayAbility.Movement.Dash");

    /* ------------------------------ Ability.* ------------------------------ */
    UE_DEFINE_GAMEPLAY_TAG(Ability_Combat_Block,      "Ability.Combat.Block");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Melee,      "Ability.Attack.Melee");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Dodge,      "Ability.Action.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Defense_Block,     "Ability.Defense.Block");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Movement_Dodge,    "Ability.Movement.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Unequip,    "Ability.Weapon.Unequip");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Equip,      "Ability.Weapon.Equip");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Stagger,    "Ability.Attack.Stagger");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Ranged,     "Ability.Attack.Ranged");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Combat_Parry,      "Ability.Combat.Parry");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Interact,          "Ability.Interact");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Movement_Vault,          "Ability.Movement.Vault");
    

    /* ------------------------------- Event.* ------------------------------- */
    UE_DEFINE_GAMEPLAY_TAG(Event_Abilities_Changed, "Event.Abilities.Changed");
    

    /* ----------------------------- GameplayEvent.* -------------------------- */
    UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Fire, "GameplayEvent.Fire");
    UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_HitReact, "GameplayEvent.HitReact"); // ✅ ADD THIS
    UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Death, "GameplayEvent.Death");       // ✅ ADD THIS

    /* -------------------------------- Data.* ------------------------------- */
    UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
    UE_DEFINE_GAMEPLAY_TAG(Data_Heal,   "Data.Heal");

    /* -------------------------- GameplayCue.* ------------------------------ */
    UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Dash_Activate, "GameplayCue.Dash.Activate");
    UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage_Burst,  "GameplayCue.Damage.Burst");
    UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Heal_Burst,    "GameplayCue.Heal.Burst");

    /* -------------------------------- State.* ------------------------------ */
    UE_DEFINE_GAMEPLAY_TAG(State_Combat_BlockWindow,   "State.Combat.BlockWindow");
    UE_DEFINE_GAMEPLAY_TAG(State_Combat_Blocking,      "State.Combat.Blocking");
    UE_DEFINE_GAMEPLAY_TAG(State_Combat_Attacking,     "State.Combat.Attacking");
    UE_DEFINE_GAMEPLAY_TAG(State_Attack,               "State.Attack");
    UE_DEFINE_GAMEPLAY_TAG(State_Defense_Blocking,     "State.Defense.Blocking");
    UE_DEFINE_GAMEPLAY_TAG(State_Action_Swap,          "State.Action.Swap");
    UE_DEFINE_GAMEPLAY_TAG(State_Action_Dodge,         "State.Action.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(State_Invincible,           "State.Invincible");
    UE_DEFINE_GAMEPLAY_TAG(State_Combat_ParrySuccess,  "State.Combat.ParrySuccess");
    UE_DEFINE_GAMEPLAY_TAG(State_Combat_ParryWindow,  "State.Combat.ParryWindow");
    UE_DEFINE_GAMEPLAY_TAG(State_Combat_HitReact,      "State.Combat.HitReact");
    UE_DEFINE_GAMEPLAY_TAG(State_Aiming,"State.Aiming");
    UE_DEFINE_GAMEPLAY_TAG(State_Firing,"State.Firing");

    /* ---------------------------- Weapon.Type.* ---------------------------- */
    UE_DEFINE_GAMEPLAY_TAG(Weapon_Melee_Sword, "Weapon.Melee.Sword");
    UE_DEFINE_GAMEPLAY_TAG(Weapon_Melee_Axe,   "Weapon.Melee.Axe");
    UE_DEFINE_GAMEPLAY_TAG(Weapon_Ranged_Bow,  "Weapon.Ranged.Bow");

    /* ----------------------------- Cooldown.* ------------------------------ */
    UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dash, "Cooldown.Dash");

    /* ------------------------------- Status.* ------------------------------ */
    UE_DEFINE_GAMEPLAY_TAG(Status_Stamina_Regen, "Status.Stamina.Regen");

    /* -------------------------- Fail.* ------------------------------ */
    UE_DEFINE_GAMEPLAY_TAG(Ability_Failed_NoAmmo, "Ability.Failed.NoAmmo");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Failed_NoStamina, "Ability.Failed.NoStamina");
    

}
