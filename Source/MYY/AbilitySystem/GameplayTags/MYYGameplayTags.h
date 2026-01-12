#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace MYYTags
{
    /* --------------------------- Ability: Ranged --------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranged_Aim);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranged_Fire);

    /* --------------------------- GameplayAbility.* -------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Movement_Dash);

    /* ----------------------------- Ability.* ------------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combat_Block);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Melee);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Dodge);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Defense_Block);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Movement_Dodge);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Unequip);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Equip);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Stagger);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Ranged);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Combat_Parry);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Movement_Vault);

    /* ------------------------------- Event.* ------------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Abilities_Changed);
    

    /* ----------------------------- GameplayEvent.* -------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Fire);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_HitReact);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death);

    /* ------------------------------ Data.* -------------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Heal);

    /* -------------------------- GameplayCue.* ------------------------------ */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Dash_Activate);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage_Burst);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Heal_Burst);

    /* -------------------------------- State.* ------------------------------ */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_BlockWindow);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Blocking);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Attacking);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attack);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Defense_Blocking);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Swap);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Dodge);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_ParrySuccess);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_ParryWindow);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_HitReact);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aiming);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Firing);

    /* ---------------------------- Weapon.Type.* ---------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Melee_Sword);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Melee_Axe);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Ranged_Bow);

    /* ----------------------------- Cooldown.* ------------------------------ */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash);

    /* ----------------------------- Status.* -------------------------------- */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stamina_Regen);

    /* -------------------------- Fail.* ------------------------------ */
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Failed_NoAmmo);
    MYY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Failed_NoStamina);

    
}
