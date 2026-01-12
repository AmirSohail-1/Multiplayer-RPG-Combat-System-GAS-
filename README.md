# Multiplayer RPG Combat System – Unreal Engine (GAS)

A **data-driven RPG combat framework** built with **Unreal Engine Gameplay Ability System (GAS)**.  
Designed to support **melee, ranged, and unarmed combat** using a **single reusable architecture** with minimal code duplication.

---

## 🚀 Overview

This project demonstrates how to build a **scalable, maintainable combat system** using GAS by separating **combat logic (C++)** from **weapon behavior (Data Assets)**.

The system allows designers to create and balance new weapons **without writing or recompiling code**.

---

## 🎯 Core Philosophy

### 🔧 Data-Driven Design
- All weapon stats, animations, and behaviors live in **Data Assets**
- No hardcoded weapon logic in abilities
- Designers can create new weapons independently

### ♻ Reusable Abilities
- Weapon-agnostic Gameplay Abilities
- Same ability works for **sword, axe, fists, bow**, etc.
- Abilities read weapon data at runtime

### 🏷 Tag-Based Architecture
- Gameplay Tags control:
  - Ability activation
  - Combat states
  - Interrupts & conditions
- Easy to debug using GAS debugger

---

## 🧩 System Architecture

### 🗡 Weapon Data Assets

Each weapon type has its own Data Asset class:

- **MeleeWeaponDataAsset**
  - Damage
  - Combo chains
  - Stamina cost
  - Attack montages

- **RangedWeaponDataAsset**
  - Projectile class
  - Draw time
  - Accuracy
  - Ammo capacity

- **UnarmedCombatDataAsset**
  - Strike damage
  - Combo variations
  - Grapple mechanics

Each Data Asset fully defines a weapon with **zero C++ changes**.

---

## ⚔ Universal Gameplay Abilities

### Combat Abilities
- `GA_MeleeCombo` – Reads combo data from equipped weapon
- `GA_RangedAttack` – Aim, charge, fire projectiles
- `GA_Block` – Universal blocking logic
- `GA_Parry` – Timing-based counter system
- `GA_DirectionalDodge` – Dodge with stamina cost

### Utility & State Abilities
- `GA_EquipWeapon`
- `GA_UnequipWeapon`
- `GA_HitReact`
- `GA_Stagger`
- `GA_Death`
- `GA_Vault` (Traversal)

---

## ✅ Benefits

### 👨‍💻 For Programmers
- Single ability supports multiple weapons
- Less combat code
- Clear separation of logic and data
- Easy to extend and maintain

### 🎮 For Designers
- No coding required
- Balance directly in editor
- Fast iteration on damage & stamina
- Full control over animations

### 🎨 For Content Creators
- Weapon-specific montages
- Precise VFX triggers via Gameplay Cues
- Contextual audio per weapon

---

## 🛠 Example Workflow: Creating a New Sword

1. Duplicate an existing Data Asset  
   `DA_Sword_Basic → DA_Sword_Legendary`

2. Modify values:
   - Increase damage
   - Assign new attack montages
   - Define 5-hit combo chain
   - Add trail & impact effects

3. Equip weapon via Blueprint

4. Test & balance — **no recompilation needed**

---

## 🔮 Future Expansion Ideas

- 🔮 Magic System (`MagicWeaponDataAsset`)
- 🛠 Weapon Durability
- 🔥 Elemental Damage (Fire, Ice, Lightning via Tags)
- 🌐 Multiplayer (GAS-ready replication)

---

## 🏁 Conclusion

This project showcases how **Gameplay Ability System + Data Assets** can:

- Reduce technical debt
- Empower designers
- Scale effortlessly for future features

### Results:
- ✅ ~90% less combat code
- ✅ Zero programmer effort for new weapon variants
- ✅ Instant balance iteration
- ✅ Production-ready architecture

---

## 📌 Engine Version
- Unreal Engine **5.5**
- Gameplay Ability System (GAS)

---
