#pragma once

#include "Aliases.hpp"

#include <QRegularExpression>

namespace Notetags {
// https://www.yanfly.moe/wiki/Category:Notetags_(MV)

#define ABSORPTION_BARRIER_NOTETAGS \
    "[Absorption Barrier (YEP)](https://www.yanfly.moe/wiki/Absorption_Barrier_(YEP)#Notetags)\n\n"

#define ACTION_CUTIN_NOTETAGS "[Action Cutin (Irina)](https://www.yanfly.moe/wiki/Action_Cutin_(Irina)#Notetags)\n\n"

#define ACTION_SEQUENCE_FAST_FORWARD_SKIP_NOTETAGS \
    "[Action Sequence Fast Forward/Skip (Irina)](https://www.yanfly.moe/wiki/Action_Sequence_Fast_Forward/Skip_(Irina)#Notetags)\n\n"

#define AUTO_PASSIVE_STATES_NOTETAGS \
    "[Auto Passive States (YEP)](https://www.yanfly.moe/wiki/Auto_Passive_States_(YEP)#Notetags)\n\n"

#define LUNATIC_PACK_PASSIVE_CONDITION_CASES_NOTETAGS \
    "[Lunatic Pack - Passive Condition Cases (YEP)](https://www.yanfly.moe/wiki/Lunatic_Pack_-_Passive_Condition_Cases_(YEP)#Notetags)\n\n"

#define PASSIVE_AURA_EFFECTS_NOTETAGS \
    "[Passive Aura Effects (YEP)](https://www.yanfly.moe/wiki/Passive_Aura_Effects_(YEP)#Notetags)\n\n"

#define BASE_PARAMETER_CONTROL_NOTETAGS \
    "[Base Parameter Control (YEP)](https://www.yanfly.moe/wiki/Base_Parameter_Control_(YEP)#Notetags)\n\n"

#define BATTLE_AI_CORE_NOTETAGS \
    "[Battle A.I. Core (YEP)](https://www.yanfly.moe/wiki/Battle_A.I._Core_(YEP)#Notetags)\n\n"

#define ACTOR_AUTO_BATTLE_AI_NOTETAGS \
    "[Actor Auto Battle A.I. (YEP)](https://www.yanfly.moe/wiki/Actor_Auto_Battle_A.I._(YEP)#Notetags)\n\n"

#define BATTLE_EFFECTS_PACK_1_NOTETAGS \
    "[Battle Effects Pack 1 (Olivia)](https://www.yanfly.moe/wiki/Battle_Effects_Pack_1_(Olivia)#Notetags)\n\n"

#define BATTLE_EFFECTS_PACK_2_NOTETAGS \
    "[Battle Effects Pack 2 (Olivia)](https://www.yanfly.moe/wiki/Battle_Effects_Pack_2_(Olivia)#Notetags)\n\n"

#define BATTLE_ENGINE_CORE_NOTETAGS \
    "[Battle Engine Core (YEP)](https://www.yanfly.moe/wiki/Battle_Engine_Core_(YEP)#Notetags)\n\n"

#define ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS \
    "[Animated Sideview Enemies (YEP)](https://www.yanfly.moe/wiki/Animated_Sideview_Enemies_(YEP)#Notetags)\n\n"

#define BATTLE_SYSTEM_ATB_NOTETAGS \
    "[Battle System - ATB (YEP)](https://www.yanfly.moe/wiki/Battle_System_-_ATB_(YEP)#Notetags)\n\n"

#define VISUAL_ATB_GAUGE_NOTETAGS \
    "[Visual ATB Gauge (YEP)](https://www.yanfly.moe/wiki/Visual_ATB_Gauge_(YEP)#Notetags)\n\n"

#define BATTLE_SYSTEM_CTB_NOTETAGS \
    "[Battle System - CTB (YEP)](https://www.yanfly.moe/wiki/Battle_System_-_CTB_(YEP)#Notetags)\n\n"

#define ORDER_TURN_BATTLE_NOTETAGS \
    "[Order Turn Battle (Olivia)](https://www.yanfly.moe/wiki/Order_Turn_Battle_(Olivia)#Notetags)\n\n"

#define BATTLE_SYSTEM_STB_NOTETAGS \
    "[Battle System - STB (YEP)](https://www.yanfly.moe/wiki/Battle_System_-_STB_(YEP)#Notetags)\n\n"

#define COUNTER_CONTROL_NOTETAGS \
    "[Counter Control (YEP)](https://www.yanfly.moe/wiki/Counter_Control_(YEP)#Notetags)\n\n"

#define IN_BATTLE_STATUS_NOTETAGS \
    "[In-Battle Status (YEP)](https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)#Notetags)\n\n"

// In-Battle Status's <Help Description>/</Help Description> literal string coincidentally
// matches a CommentTags entry for a different plugin (Common Event Menu (YEP))

#define LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS \
    "[Lunatic Pack - Action Beginning and End Effects (YEP)](https://www.yanfly.moe/wiki/Lunatic_Pack_-_Action_Beginning_and_End_Effects_(YEP)#Notetags)\n\n"

#define TURN_ORDER_DISPLAY_NOTETAGS \
    "[Turn Order Display (YEP)](https://www.yanfly.moe/wiki/Turn_Order_Display_(YEP)#Notetags)\n\n"

#define VISUAL_HP_GAUGES_NOTETAGS \
    "[Visual HP Gauges (YEP)](https://www.yanfly.moe/wiki/Visual_HP_Gauges_(YEP)#Notetags)\n\n"

#define WEAK_ENEMY_POSES_NOTETAGS \
    "[Weak Enemy Poses (YEP)](https://www.yanfly.moe/wiki/Weak_Enemy_Poses_(YEP)#Notetags)\n\n"

#define BATTLE_IMPACT_NOTETAGS \
    "[Battle Impact (Olivia)](https://www.yanfly.moe/wiki/Battle_Impact_(Olivia)#Notetags)\n\n"

#define BATTLE_SELECT_CURSOR_NOTETAGS \
    "[Battle Select Cursor (YEP)](https://www.yanfly.moe/wiki/Battle_Select_Cursor_(YEP)#Notetags)\n\n"

#define BOOST_POINT_SYSTEM_NOTETAGS \
    "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Notetags)\n\n"

#define BRIGHT_EFFECTS_NOTETAGS \
    "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Notetags)\n\n"

#define BUFFS_AND_STATES_CORE_NOTETAGS \
    "[Buffs & States Core (YEP)](https://www.yanfly.moe/wiki/Buffs_%26_States_Core_(YEP)#Notetags)\n\n"

#define EXTENDED_DAMAGE_OVER_TIME_NOTETAGS \
    "[Extended Damage Over Time (YEP)](https://www.yanfly.moe/wiki/Extended_Damage_Over_Time_(YEP)#Notetags)\n\n"

#define LUNATIC_PACK_STATE_PROTECTION_NOTETAGS \
    "[Lunatic Pack - State Protection (YEP)](https://www.yanfly.moe/wiki/Lunatic_Pack_-_State_Protection_(YEP)#Notetags)\n\n"

#define STATE_CATEGORIES_NOTETAGS \
    "[State Categories (YEP)](https://www.yanfly.moe/wiki/State_Categories_(YEP)#Notetags)\n\n"

#define VISUAL_STATE_EFFECTS_NOTETAGS \
    "[Visual State Effects (YEP)](https://www.yanfly.moe/wiki/Visual_State_Effects_(YEP)#Notetags)\n\n"

#define CARD_GAME_CORE_NOTETAGS \
    "[Card Game Core (Irina)](https://www.yanfly.moe/wiki/Card_Game_Core_(Irina)#Notetags)\n\n"

#define CARD_GAME_DECK_BUILDER_NOTETAGS \
    "[Card Game Deck Builder (Irina)](https://www.yanfly.moe/wiki/Card_Game_Deck_Builder_(Irina)#Notetags)\n\n"

#define CARD_GAME_MECHANICS_NOTETAGS \
    "[Card Game Mechanics (Irina)](https://www.yanfly.moe/wiki/Card_Game_Mechanics_(Irina)#Notetags)\n\n"

#define CLASS_CHANGE_CORE_NOTETAGS \
    "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Notetags)\n\n"

#define SUBCLASS_NOTETAGS "[Subclass (YEP)](https://www.yanfly.moe/wiki/Subclass_(YEP)#Notetags)\n\n"

#define COUNTER_STATE_NOTETAGS "[Counter State (Arisu)](https://www.yanfly.moe/wiki/Counter_State_(Arisu)#Notetags)\n\n"

#define CORE_ENGINE_NOTETAGS "[Core Engine (YEP)](https://www.yanfly.moe/wiki/Core_Engine_(YEP)#Notetags)\n\n"

#define DAMAGE_CORE_NOTETAGS "[Damage Core (YEP)](https://www.yanfly.moe/wiki/Damage_Core_(YEP)#Notetags)\n\n"

#define ARMOR_SCALING_NOTETAGS "[Armor Scaling (YEP)](https://www.yanfly.moe/wiki/Armor_Scaling_(YEP)#Notetags)\n\n"

#define CRITICAL_CONTROL_NOTETAGS \
    "[Critical Control (YEP)](https://www.yanfly.moe/wiki/Critical_Control_(YEP)#Notetags)\n\n"

#define LUNATIC_PACK_CRITICAL_SWAY_NOTETAGS \
    "[Lunatic Pack - Critical Sway (YEP)](https://www.yanfly.moe/wiki/Lunatic_Pack_-_Critical_Sway_(YEP)#Notetags)\n\n"

#define DASH_TOGGLE_NOTETAGS "[Dash Toggle (YEP)](https://www.yanfly.moe/wiki/Dash_Toggle_(YEP)#Notetags)\n\n"

#define DISABLE_AUTO_SHADOW_EXTENDED_NOTETAGS \
    "[Disable Auto Shadow Extended (YEP)](https://www.yanfly.moe/wiki/Disable_Auto_Shadow_Extended_(YEP)#Notetags)\n\n"

#define DRAGONBONES_INTEGRATION_NOTETAGS \
    "[Dragonbones Integration (YEP)](https://www.yanfly.moe/wiki/Dragonbones_Integration_(YEP)#Notetags)\n\n"

#define DRAGONBONES_MAP_SPRITES_NOTETAGS \
    "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Notetags)\n\n"

#define ELEMENT_CORE_NOTETAGS "[Element Core (YEP)](https://www.yanfly.moe/wiki/Element_Core_(YEP)#Notetags)\n\n"

#define ENEMY_LEVELS_NOTETAGS "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Notetags)\n\n"

#define DIFFICULTY_SLIDER_NOTETAGS \
    "[Difficulty Slider (YEP)](https://www.yanfly.moe/wiki/Difficulty_Slider_(YEP)#Notetags)\n\n"

#define ENEMY_BASE_PARAMETERS_NOTETAGS \
    "[Enemy Base Parameters (YEP)](https://www.yanfly.moe/wiki/Enemy_Base_Parameters_(YEP)#Notetags)\n\n"

#define MAP_ENEMY_LEVELS_NOTETAGS \
    "[Map Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Map_Enemy_Levels_(YEP)#Notetags)\n\n"

#define ENHANCED_TP_NOTETAGS "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Notetags)\n\n"

#define EQUIP_BATTLE_SKILLS_NOTETAGS \
    "[Equip Battle Skills (YEP)](https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)#Notetags)\n\n"

#define EQUIP_SKILL_TIERS_NOTETAGS \
    "[Equip Skill Tiers (YEP)](https://www.yanfly.moe/wiki/Equip_Skill_Tiers_(YEP)#Notetags)\n\n"

#define EQUIP_CORE_NOTETAGS "[Equip Core (YEP)](https://www.yanfly.moe/wiki/Equip_Core_(YEP)#Notetags)\n\n"

#define CHANGE_BATTLE_EQUIP_NOTETAGS \
    "[Change Battle Equip (YEP)](https://www.yanfly.moe/wiki/Change_Battle_Equip_(YEP)#Notetags)\n\n"

#define EQUIP_REQUIREMENTS_NOTETAGS \
    "[Equip Requirements (YEP)](https://www.yanfly.moe/wiki/Equip_Requirements_(YEP)#Notetags)\n\n"

#define EQUIPMENT_SET_BONUSES_NOTETAGS \
    "[Equipment Set Bonuses (Olivia)](https://www.yanfly.moe/wiki/Equipment_Set_Bonuses_(Olivia)#Notetags)\n\n"

#define EVENT_COPIER_NOTETAGS "[Event Copier (YEP)](https://www.yanfly.moe/wiki/Event_Copier_(YEP)#Notetags)\n\n"

#define EVENT_ENCOUNTER_AID_NOTETAGS \
    "[Event Encounter Aid (YEP)](https://www.yanfly.moe/wiki/Event_Encounter_Aid_(YEP)#Notetags)\n\n"

#define EXTRA_ENEMY_DROPS_NOTETAGS \
    "[Extra Enemy Drops (YEP)](https://www.yanfly.moe/wiki/Extra_Enemy_Drops_(YEP)#Notetags)\n\n"

#define EXTRA_PARAMETER_FORMULA_NOTETAGS \
    "[Extra Parameter Formula (YEP)](https://www.yanfly.moe/wiki/Extra_Parameter_Formula_(YEP)#Notetags)\n\n"

#define FLOOR_DAMAGE_NOTETAGS "[Floor Damage (YEP)](https://www.yanfly.moe/wiki/Floor_Damage_(YEP)#Notetags)\n\n"

#define FOOTSTEP_SOUNDS_NOTETAGS \
    "[Footstep Sounds (YEP)](https://www.yanfly.moe/wiki/Footstep_Sounds_(YEP)#Notetags)\n\n"

#define HIT_DAMAGE_SOUNDS_NOTETAGS \
    "[Hit Damage Sounds (YEP)](https://www.yanfly.moe/wiki/Hit_Damage_Sounds_(YEP)#Notetags)\n\n"

#define INSTANT_CAST_NOTETAGS "[Instant Cast (YEP)](https://www.yanfly.moe/wiki/Instant_Cast_(YEP)#Notetags)\n\n"

#define ITEM_CONCOCTIONS_NOTETAGS \
    "[Item Concoctions (Olivia)](https://www.yanfly.moe/wiki/Item_Concoctions_(Olivia)#Notetags)\n\n"

#define ITEM_CORE_NOTETAGS "[Item Core (YEP)](https://www.yanfly.moe/wiki/Item_Core_(YEP)#Notetags)\n\n"

#define ATTACHABLE_AUGMENTS_NOTETAGS \
    "[Attachable Augments (YEP)](https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)#Notetags)\n\n"

#define ITEM_DISASSEMBLE_NOTETAGS \
    "[Item Disassemble (YEP)](https://www.yanfly.moe/wiki/Item_Disassemble_(YEP)#Notetags)\n\n"

#define ITEM_DISCARD_NOTETAGS "[Item Discard (YEP)](https://www.yanfly.moe/wiki/Item_Discard_(YEP)#Notetags)\n\n"

#define ITEM_DURABILITY_NOTETAGS \
    "[Item Durability (YEP)](https://www.yanfly.moe/wiki/Item_Durability_(YEP)#Notetags)\n\n"

#define ITEM_MENU_CATEGORIES_NOTETAGS \
    "[Item Menu Categories (YEP)](https://www.yanfly.moe/wiki/Item_Menu_Categories_(YEP)#Notetags)\n\n"

#define ITEM_PICTURE_IMAGES_NOTETAGS \
    "[Item Picture Images (YEP)](https://www.yanfly.moe/wiki/Item_Picture_Images_(YEP)#Notetags)\n\n"

#define ITEM_RENAME_NOTETAGS "[Item Rename (YEP)](https://www.yanfly.moe/wiki/Item_Rename_(YEP)#Notetags)\n\n"

#define ITEM_REQUIREMENTS_NOTETAGS \
    "[Item Requirements (YEP)](https://www.yanfly.moe/wiki/Item_Requirements_(YEP)#Notetags)\n\n"

#define ITEM_UPGRADE_SLOTS_NOTETAGS \
    "[Item Upgrade Slots (YEP)](https://www.yanfly.moe/wiki/Item_Upgrade_Slots_(YEP)#Notetags)\n\n"

#define ITEM_SYNTHESIS_NOTETAGS "[Item Synthesis (YEP)](https://www.yanfly.moe/wiki/Item_Synthesis_(YEP)#Notetags)\n\n"

#define JOB_POINTS_NOTETAGS "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Notetags)\n\n"

#define LEVEL_UP_GROWTH_EFFECTS_NOTETAGS \
    "[Level Up Growth Effects (YEP)](https://www.yanfly.moe/wiki/Level_Up_Growth_Effects_(YEP)#Notetags)\n\n"

#define LIFE_STEAL_NOTETAGS "[Life Steal (YEP)](https://www.yanfly.moe/wiki/Life_Steal_(YEP)#Notetags)\n\n"

#define MAIN_MENU_ACTOR_EVENTS_NOTETAGS \
    "[Main Menu Actor Events (YEP)](https://www.yanfly.moe/wiki/Main_Menu_Actor_Events_(YEP)#Notetags)\n\n"

#define MAP_DISPLAY_NAME_CORE_NOTETAGS \
    "[Map Display Name Core (Olivia)](https://www.yanfly.moe/wiki/Map_Display_Name_Core_(Olivia)#Notetags)\n\n"

static constexpr auto category(const u32 idx) -> QStringView {
    if (idx >= 562) {
        return u"Map Display Name Core (Olivia)";
    }
    if (idx >= 561) {
        return u"Main Menu Actor Events (YEP)";
    }
    if (idx >= 555) {
        return u"Life Steal (YEP)";
    }
    if (idx >= 549) {
        return u"Level Up Growth Effects (YEP)";
    }
    if (idx >= 543) {
        return u"Job Points (YEP)";
    }
    if (idx >= 537) {
        return u"Item Synthesis (YEP)";
    }
    if (idx >= 529) {
        return u"Item Upgrade Slots (YEP)";
    }
    if (idx >= 525) {
        return u"Item Requirements (YEP)";
    }
    if (idx >= 524) {
        return u"Item Rename (YEP)";
    }
    if (idx >= 521) {
        return u"Item Picture Images (YEP)";
    }
    if (idx >= 520) {
        return u"Item Menu Categories (YEP)";
    }
    if (idx >= 501) {
        return u"Item Durability (YEP)";
    }
    if (idx >= 500) {
        return u"Item Discard (YEP)";
    }
    if (idx >= 494) {
        return u"Item Disassemble (YEP)";
    }
    if (idx >= 487) {
        return u"Attachable Augments (YEP)";
    }
    if (idx >= 477) {
        return u"Item Core (YEP)";
    }
    if (idx >= 474) {
        return u"Item Concoctions (Olivia)";
    }
    if (idx >= 469) {
        return u"Instant Cast (YEP)";
    }
    if (idx >= 468) {
        return u"Hit Damage Sounds (YEP)";
    }
    if (idx >= 465) {
        return u"Footstep Sounds (YEP)";
    }
    if (idx >= 462) {
        return u"Floor Damage (YEP)";
    }
    if (idx >= 459) {
        return u"Extra Parameter Formula (YEP)";
    }
    if (idx >= 455) {
        return u"Extra Enemy Drops (YEP)";
    }
    if (idx >= 453) {
        return u"Event Encounter Aid (YEP)";
    }
    if (idx >= 452) {
        return u"Event Copier (YEP)";
    }
    if (idx >= 448) {
        return u"Equipment Set Bonuses (Olivia)";
    }
    if (idx >= 442) {
        return u"Equip Requirements (YEP)";
    }
    if (idx >= 440) {
        return u"Change Battle Equip (YEP)";
    }
    if (idx >= 434) {
        return u"Equip Core (YEP)";
    }
    if (idx >= 432) {
        return u"Equip Skill Tiers (YEP)";
    }
    if (idx >= 425) {
        return u"Equip Battle Skills (YEP)";
    }
    if (idx >= 421) {
        return u"Enhanced TP (YEP)";
    }
    if (idx >= 417) {
        return u"Map Enemy Levels (YEP)";
    }
    if (idx >= 411) {
        return u"Enemy Base Parameters (YEP)";
    }
    if (idx >= 410) {
        return u"Difficulty Slider (YEP)";
    }
    if (idx >= 389) {
        return u"Enemy Levels (YEP)";
    }
    if (idx >= 382) {
        return u"Element Core (YEP)";
    }
    if (idx >= 374) {
        return u"Dragonbones Map Sprites (Irina)";
    }
    if (idx >= 367) {
        return u"Dragonbones Integration (YEP)";
    }
    if (idx >= 366) {
        return u"Disable Auto Shadow Extended (YEP)";
    }
    if (idx >= 365) {
        return u"Dash Toggle (YEP)";
    }
    if (idx >= 361) {
        return u"Lunatic Pack - Critical Sway (YEP)";
    }
    if (idx >= 349) {
        return u"Critical Control (YEP)";
    }
    if (idx >= 338) {
        return u"Armor Scaling (YEP)";
    }
    if (idx >= 333) {
        return u"Damage Core (YEP)";
    }
    if (idx >= 325) {
        return u"Core Engine (YEP)";
    }
    if (idx >= 324) {
        return u"Counter State (Arisu)";
    }
    if (idx >= 317) {
        return u"Subclass (YEP)";
    }
    if (idx >= 306) {
        return u"Class Change Core (YEP)";
    }
    if (idx >= 287) {
        return u"Card Game Mechanics (Irina)";
    }
    if (idx >= 286) {
        return u"Card Game Deck Builder (Irina)";
    }
    if (idx >= 259) {
        return u"Card Game Core (Irina)";
    }
    if (idx >= 257) {
        return u"Visual State Effects (YEP)";
    }
    if (idx >= 253) {
        return u"State Categories (YEP)";
    }
    if (idx >= 242) {
        return u"Lunatic Pack - State Protection (YEP)";
    }
    if (idx >= 237) {
        return u"Extended Damage Over Time (YEP)";
    }
    if (idx >= 219) {
        return u"Buffs & States Core (YEP)";
    }
    if (idx >= 211) {
        return u"Bright Effects (Olivia)";
    }
    if (idx >= 204) {
        return u"Boost Point System (Olivia)";
    }
    if (idx >= 199) {
        return u"Battle Select Cursor (YEP)";
    }
    if (idx >= 198) {
        return u"Battle Impact (Olivia)";
    }
    if (idx >= 195) {
        return u"Weak Enemy Poses (YEP)";
    }
    if (idx >= 192) {
        return u"Visual HP Gauges (YEP)";
    }
    if (idx >= 188) {
        return u"Turn Order Display (YEP)";
    }
    if (idx >= 180) {
        return u"Lunatic Pack - Action Beginning and End Effects (YEP)";
    }
    if (idx >= 178) {
        return u"In-Battle Status (YEP)";
    }
    if (idx >= 156) {
        return u"Counter Control (YEP)";
    }
    if (idx >= 154) {
        return u"Battle System - STB (YEP)";
    }
    if (idx >= 151) {
        return u"Order Turn Battle (Olivia)";
    }
    if (idx >= 135) {
        return u"Battle System - CTB (YEP)";
    }
    if (idx >= 133) {
        return u"Visual ATB Gauge (YEP)";
    }
    if (idx >= 120) {
        return u"Battle System - ATB (YEP)";
    }
    if (idx >= 97) {
        return u"Animated Sideview Enemies (YEP)";
    }
    if (idx >= 89) {
        return u"Battle Engine Core (YEP)";
    }
    if (idx >= 82) {
        return u"Battle Effects Pack 2 (Olivia)";
    }
    if (idx >= 66) {
        return u"Battle Effects Pack 1 (Olivia)";
    }
    if (idx >= 64) {
        return u"Actor Auto Battle A.I. (YEP)";
    }
    if (idx >= 61) {
        return u"Battle A.I. Core (YEP)";
    }
    if (idx >= 57) {
        return u"Base Parameter Control (YEP)";
    }
    if (idx >= 55) {
        return u"Passive Aura Effects (YEP)";
    }
    if (idx >= 54) {
        return u"Lunatic Pack - Passive Condition Cases (YEP)";
    }
    if (idx >= 47) {
        return u"Auto Passive States (YEP)";
    }
    if (idx >= 44) {
        return u"Action Sequence Fast Forward/Skip (Irina)";
    }
    if (idx >= 11) {
        return u"Action Cutin (Irina)";
    }
    if (idx >= 0) {
        return u"Absorption Barrier (YEP)";
    }

    std::unreachable();
};

static constexpr array<QStringView, 563> PATTERN_PARTS = {
    // https://www.yanfly.moe/wiki/Absorption_Barrier_(YEP)
    // Skill and Item Notetags
    uR"((<(?:User|Target) Barrier: (?:\+|-)\d+?>))",
    uR"((<(?:User|Target) Barrier \d+? Turns: (?:\+|-)\d+?>))",
    uR"((<Bypass Barrier>))",
    uR"((<Barrier Penetration: \d+?%>))",
    uR"((<Barrier Penetration: \d+?>))",
    // Actor, Class, Enemy, Weapon, Armor, State Notetags
    uR"((<Barrier Penetration: (?:\+|-)\d+?%>))",
    uR"((<Barrier Penetration: (?:\+|-)\d+?>))",
    uR"((<Barrier Points: (?:\+|-)\d+?>))",
    uR"((<Barrier Points \d+? Turns: (?:\+|-)\d+?>))",
    uR"((<Barrier Regen: (?:\+|-)\d+?>))",
    uR"((<Barrier Regen \d+? Turns: (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Action_Cutin_(Irina)
    // Actor, Actor, and Enemy Notetags
    uR"((<Action Cutin Picture: .+?>))",
    uR"((<Action Cutin Face: .+?, \d+?>))",
    uR"((<Action Cutin SV: .+?, \d+?>))",
    uR"((<Action Cutin Battler: .+?(?:, \d+?)?>))",
    uR"((<Action Cutin Scale: [\d.]+?>))",
    uR"((<Action Cutin Offset (?:X|Y): (?:\+|-)\d+?>))",
    uR"((<Action Cutin (?:No )?Anti-Alias>))",
    // Skill and Item Notetags
    uR"((<(?:No )?Action Cutin>))",
    uR"((<Action Cutin Image: .+?>))",
    uR"((<Action Cutin Hue: \d+?>))",
    uR"((<Action Cutin Blend Mode: (?:Normal|Additive|Multiply|Screen)>))",
    uR"((<Action Cutin Border Thickness: \d+?>))",
    uR"((<Action Cutin Border Color: .+?>))",
    uR"((<Action Cutin Screen (?:X|Y): \d+?>))",
    uR"((<Action Cutin Shift Distance (?:X|Y): \d+?>))",
    uR"((<Action Cutin Shift Frames: \d+?>))",
    uR"((<Action Cutin (?:No )?Flash>))",
    uR"((<Action Cutin Flash Color: \d+?, \d+?, \d+?, \d+?>))",
    uR"((<Action Cutin Flash Duration: \d+?>))",
    uR"((<Action Cutin (?:No )?Shake>))",
    uR"((<Action Cutin Shake Power: \d+?>))",
    uR"((<Action Cutin Shake Speed: \d+?>))",
    uR"((<Action Cutin Shake Duration: \d+?>))",
    uR"((<Action Cutin (?:No )?Sound>))",
    uR"((<Action Cutin Sound Filename: .+?>))",
    uR"((<Action Cutin Sound Volume: \d+?>))",
    uR"((<Action Cutin Sound Pitch: \d+?>))",
    uR"((<Action Cutin Sound Pan: (?:\+|-)?\d+?>))",
    uR"((<Action Cutin Duration: \d+?>))",
    uR"((<Action Cutin Opacity Speed: \d+?>))",
    uR"((<Action Cutin Forced Scale: [\d.]+?>))",
    uR"((<Action Cutin Forced (?:No )?Anti-Alias>))",
    uR"((<Action Cutin Forced Offset (?:X|Y): (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Action_Sequence_Fast_Forward/Skip_(Irina)
    uR"((<Cannot (?:Fast|Skip) Forward>))",
    uR"((<Fast Forward Speed: \d+?>))",
    uR"((<Skip Forward Speed: \d+?>))",

    // https://www.yanfly.moe/wiki/Auto_Passive_States_(YEP)
    // Actor, Class, Skills, Weapon, Armor, Enemy Notetags
    uR"((<Passive State: \d+?(?:, \d+?)*>))",
    uR"((<Passive State: \d+? to \d+?>))",
    // State Notetags
    uR"((<Passive Condition: (?:HP|MP) (?:Above|Below) \d+?%>))",
    uR"((<Passive Condition: (?:HP|MP|TP|MAXHP|ATK|DEF|MAT|MDF|AGI|LUK) (?:Above|Below) \d+?>))",
    uR"((<Passive Condition: Switch \d+? (?:ON|OFF)>))",
    uR"((<Passive Condition: Variable \d+? (?:Above|Below) \d+?>))",
    // Lunatic Mode Notetags
    uR"((<Custom Passive Condition>))",

    // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Passive_Condition_Cases_(YEP)
    uR"((<Passive Condition Cases>))",

    // https://www.yanfly.moe/wiki/Passive_Aura_Effects_(YEP)
    uR"((<(?:Alive Ally|Dead Ally|Ally|Alive Foe|Dead Foe|Foe|Alive Party|Dead Party|Party|Alive Troop|Dead Troop|Troop|Everybody|Alive|Dead) Aura: \d+?(?:(?: to |, )\d+?)?>))",
    // Lunatic Mode
    uR"((<Custom Aura Condition>))",

    // https://www.yanfly.moe/wiki/Base_Parameter_Control_(YEP)
    uR"((<(?:MaxHP|MaxMP|Atk|Def|Mat|Mdf|Agi|Luk) Plus: (?:\+|-)\d+?>))",
    uR"((<(?:MaxHP|MaxMP|Atk|Def|Mat|Mdf|Agi|Luk) Rate: [\d.]+?%?>))",
    uR"((<(?:MaxHP|MaxMP|Atk|Def|Mat|Mdf|Agi|Luk) Flat: (?:\+|-)\d+?>))",
    uR"((<(?:MaxHP|MaxMP|Atk|Def|Mat|Mdf|Agi|Luk) (?:Max|Min): \d+?>))",

    // https://www.yanfly.moe/wiki/Battle_A.I._Core_(YEP)
    uR"((<AI Level: \d+?>))",
    uR"((<\/?AI Priority>))",
    uR"((<AI Consider Taunt>))",

    // https://www.yanfly.moe/wiki/Actor_Auto_Battle_A.I._(YEP)
    uR"((<AI Level: \d+?>))",
    uR"((<\/?AI Priority>))",

    // https://www.yanfly.moe/wiki/Battle_Effects_Pack_1_(Olivia)
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<(?:Skill|Item) Target Change: Self to All>))",
    uR"((<(?:Skill|Item) Target Change (?:Allies|Enemies): (?:All to One|One to All)>))",
    // Skill, and Item Notetags
    uR"((<(?:Bypass Target Change|Divine)>))",
    uR"((<(?:JP|EXP|Gold) x\d+?>))",
    // Skill Notetags
    uR"((<Destroy Weapon>))",
    uR"((<Extra Skill List: \d+?(?:, \d+?)*>))",
    // State Notetags
    uR"((<All Element Damage Rate: \d+?%>))",
    uR"((<Break Popup>))",
    uR"((<Buff Immunity: \d+?(?:, \d+?)*>))",
    uR"((<Debuff Immunity: \d+?(?:, \d+?)*>))",
    uR"((<Damage Color: \d+?, \d+?, \d+?, \d+?>))",
    uR"((<Item Seal>))",
    uR"((<Max Turns: \d+?>))",
    uR"((<No Weak Popup>))",
    uR"((<(?:Physical |Magical |Certain )?Follow Up Skill: \d+?>))",
    uR"((<State Immunity: \d+?(?:, \d+?)*>))",

    // https://www.yanfly.moe/wiki/Battle_Effects_Pack_2_(Olivia)
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<(?:High|Low) Health (?:ATK|DEF|MAT|MDF|AGI|LUK): \+\d+?%>))",
    uR"((<Damage Cut: \d+?%>))",
    uR"((<Element (?:\d+?|.+?) Cut: \d+?%>))",
    uR"((<Overheal (?:HP|MP|TP)>))",
    uR"((<Swap (?:mhp|mmp|atk|def|mat|mdf|agi|luk) with (?:mhp|mmp|atk|def|mat|mdf|agi|luk)>))",
    // State Notetags
    uR"((<Dissolve State: \d+?(?:, \d+?)*>))",
    uR"((<(?:Set|Add) State Counter: \d+?>))",

    // https://www.yanfly.moe/wiki/Battle_Engine_Core_(YEP)
    // Battle Messages
    uR"((<Display (?:Text: .+?|Icon: \d+?)>))",
    // Battle Order
    uR"((<speed: (?:\+|-)\d+?>))",
    // Casting Animations
    uR"((<Cast Animation: \d+?>))",
    // Sideview Actions
    uR"((<Reflect Animation ID: \d+?>))",
    uR"((<Sprite Cannot Move>))",
    // Custom Sideview Battler Anchor
    uR"((<Anchor (?:X|Y): [\d.]+?>))",
    // Enemy Attack Animation
    uR"((<Attack Animation: \d+?>))",
    // Automatic State Removal Conditions
    uR"((<(?:Action|Turn) Start: \d+?(?: to \d+?)?>))",

    // https://www.yanfly.moe/wiki/Animated_Sideview_Enemies_(YEP)
    // Enemy Notetags - General
    uR"((<(?:No )?Breathing>))",
    uR"((<Breathing Speed: \d+?>))",
    uR"((<Breathing Rate (?:X|Y): [\d.]+?>))",
    uR"((<(?:Enable|Disable) HP Link Breathing>))",
    uR"((<Floating>))",
    uR"((<Floating Speed: \d+?>))",
    uR"((<Floating Rate: [\d.]+?>))",
    uR"((<Floating Height: \d+?>))",
    uR"((<(?:No )?Floating Death>))",
    uR"((<Scale Sprite: \d+?%>))",
    uR"((<Scale Sprite (?:Width|Height): \d+?%>))",
    // Enemy Notetags - Sideview
    uR"((<Sideview Battler: .+?>))",
    uR"((<Sideview Anchor (?:X|Y): [\d.]+?>))",
    uR"((<Sideview (?:Width|Height): \d+?>))",
    uR"((<Sideview (?:No )?Collapse>))",
    uR"((<Sideview Frame Speed: \d+?>))",
    uR"((<Sideview (?:Show|Hide) State Overlay>))",
    uR"((<Sideview Attack Motion: .+?>))",
    uR"((<Sideview Weapon: \d+?(?:, .+?, \d+?)?>))",
    uR"((<Sideview (?:Idle|Damage|Evade|Escape|Guard|Abnormal|Sleep|Dying|Dead) Motion: .+?>))",
    uR"((<Sideview (?:Show|Hide) Shadow>))",
    uR"((<Sideview Shadow (?:Width|Height): \d+?%>))",
    // State Notetags
    uR"((<Hide Sideview Weapon>))",

    // https://www.yanfly.moe/wiki/Battle_System_-_ATB_(YEP)
    // Skill and Item Notetags
    uR"((<ATB Help>))",
    uR"((<\/ATB Help>))",
    uR"((<ATB (?:Speed|Charge|Gauge): (?:\+|-)?\d+?%?>))",
    uR"((<After ATB: \d+?%?>))",
    uR"((<ATB Interrupt(?:: \d+?%)?>))",
    uR"((<Cannot ATB Interrupt>))",
    // Lunatic Mode Notetags
    uR"((<Target ATB Eval>))",
    uR"((<\/Target ATB Eval>))",
    uR"((<After ATB Eval>))",
    uR"((<\/After ATB Eval>))",
    uR"((<ATB Interrupt Eval>))",
    uR"((<\/ATB Interrupt Eval>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<ATB (?:Start|Turn): \+\d+?%?>))",

    // https://www.yanfly.moe/wiki/Visual_ATB_Gauge_(YEP)
    // Enemy Notetags
    uR"((<(?:Show|Hide) ATB Gauge>))",
    uR"((<ATB Gauge Width: \d+?>))",

    // https://www.yanfly.moe/wiki/Battle_System_-_CTB_(YEP)
    // Actor and Enemy Notetags
    uR"((<CTB Icon: \d+?>))",
    uR"((<CTB (?:Border|Background) Color: \d+?>))",
    // Actor only Notetags
    uR"((<Class \d+? CTB Icon: \d+?>))",
    uR"((<[A-Za-z]+? CTB Icon: \d+?>))",
    // Skill and Item Notetags
    uR"((<CTB Help>))",
    uR"((<\/CTB Help>))",
    uR"((<CTB Speed: (?:\+|-)?\d+?%?>))",
    uR"((<CTB Order: (?:\+|-)\d+?>))",
    uR"((<After CTB: \d+?%?>))",
    // Lunatic Mode - Skill and Item Notetags
    uR"((<Target CTB Speed Eval>))",
    uR"((<\/Target CTB Speed Eval>))",
    uR"((<Target CTB Order Eval>))",
    uR"((<\/Target CTB Order Eval>))",
    uR"((<After CTB Eval>))",
    uR"((<\/After CTB Eval>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<CTB (?:Start|Turn): \+\d+?%?>))",

    // https://www.yanfly.moe/wiki/Order_Turn_Battle_(Olivia)
    // Skill and Item Notetags
    uR"((<OTB User Next Turn: (?:\+|-)\d+?>))",
    uR"((<OTB Target (?:Current|Next|Follow) Turn: (?:\+|-)\d+?>))",
    uR"((<OTB (?:User|Target) Add (?:Current|Next) Turn Actions: \d+?>))",

    // https://www.yanfly.moe/wiki/Battle_System_-_STB_(YEP)
    // Skill and Item Notetags
    uR"((<STB Help>))",
    uR"((<\/STB Help>))",

    // https://www.yanfly.moe/wiki/Counter_Control_(YEP)
    // Actor and Enemy Notetags
    uR"((<Default Counter: (?:\d+?|.+?)>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<Counter Skills?: (?:\d+?(?:, \d+?)*|\d+? to \d+?|.+?)>))",
    uR"((<Counter Total: (?:\+|-)\d+?>))",
    uR"((<Target Counter: (?:\+|-)?\d+?%>))",
    uR"((<(?:Evade|Hit) Counter>))",
    // Lunatic Mode: Custom Counter Skills
    uR"((<Custom Counter Skills>))",
    uR"((<\/Custom Counter Skills>))",
    // Lunatic Mode: Custom Counter Total
    uR"((<Custom Counter Total>))",
    uR"((<\/Custom Counter Total>))",
    // Lunatic Mode: Custom Target Counter Rate
    uR"((<Custom Target Counter Rate>))",
    uR"((<\/Custom Target Counter Rate>))",
    // Skill and Item Notetags
    uR"((<Ally (?:Cannot )?Counter>))",
    uR"((<Cannot Counter>))",
    uR"((<Counter Rate: (?:\+|-)?\d+?%>))",
    // Lunatic Mode: Custom Counter Rates
    uR"((<Custom Counter Rate>))",
    uR"((<\/Custom Counter Rate>))",
    // Skill Notetags
    uR"((<Counter Name: .+?>))",
    uR"((<Counter Icon: \d+?>))",
    // Counter Conditions
    uR"((<Counter Condition>))",
    uR"((<\/Counter Condition>))",
    // Lunatic Mode: Custom Counter Condition
    uR"((<Custom Counter Condition>))",
    uR"((<\/Custom Counter Condition>))",

    // https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)
    // State Notetags
    uR"((<Help Description>))",
    uR"((<\/Help Description>))",

    // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Action_Beginning_and_End_Effects_(YEP)
    // Skill, Item, and State Notetags
    uR"((<(?:Begin|End) Action: Animation \d+?(?:, Mirror)?(?:, Delay \d+?)?>))",
    uR"((<(?:Begin|End) Action: (?:\+|-)\d+? HP%?>))",
    uR"((<End Action: (?:Drain|Recoil) \d+?% Total HP Damage>))",
    uR"((<(?:Begin|End) Action: (?:\+|-)\d+? MP%?>))",
    uR"((<End Action: (?:Drain|Recoil) \d+?% Total MP Damage>))",
    uR"((<(?:Begin|End) Action: (?:\+|-)\d+? TP%?>))",
    uR"((<(?:Begin|End) Action: (?:Add \d+? (?:Buff|Debuff)(?:, \d+? Turns)?|Remove \d+? (?:Buff|Debuff))>))",
    uR"((<(?:Begin|End) Action: (?:Add|Remove) State \d+?>))",

    // https://www.yanfly.moe/wiki/Turn_Order_Display_(YEP)
    // Actor and Enemy Notetags
    uR"((<Turn Order Icon: \d+?>))",
    uR"((<Turn Order (?:Border|Background) Color: \d+?>))",
    // Actor Only Notetags
    uR"((<Class \d+? Turn Order Icon: \d+?>))",
    uR"((<(?:Hero|Warrior|Mage|Priest) Turn Order Icon: \d+?>))",

    // https://www.yanfly.moe/wiki/Visual_HP_Gauges_(YEP)
    // Class and Enemy Notetags
    uR"((<(?:Hide|Show) HP Gauge>))",
    uR"((<HP Gauge (?:Width|Height): \d+?>))",
    uR"((<HP Gauge (?:Back Color|Color 1|Color 2): \d+?>))",

    // https://www.yanfly.moe/wiki/Weak_Enemy_Poses_(YEP)
    // Enemy Notetags
    uR"((<\d+?% Health Pose: .+?(?:, \d+?)?>))",
    // State Notetags
    uR"((<Force Enemy Pose: .+?(?:, \d+?)?>))",
    // Lunatic Mode (Enemy and State)
    uR"((<Custom Enemy Pose>))",

    // https://www.yanfly.moe/wiki/Battle_Impact_(Olivia)
    // State/Buff Icon Notetags
    uR"((<Ignore Icon Effect>))",

    // https://www.yanfly.moe/wiki/Battle_Select_Cursor_(YEP)
    // Actor and Enemy Notetags
    uR"((<Battle Select Cursor: .+?>))",
    uR"((<Battle Select Cursor Anchor X: (?:Left|Center|Right)>))",
    uR"((<Battle Select Cursor Anchor Y: (?:Top|Middle|Bottom)>))",
    uR"((<Battle Select Cursor Position X: (?:Left|Center|Right)>))",
    uR"((<Battle Select Cursor Position Y: (?:Top|Middle|Bottom)>))",

    // https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)
    // Skill and Item Notetags
    uR"((<Require (?:>=|<=|>|<|=)? ?\d+? BP>))",
    uR"((<(?:Target|User) BP: (?:\+|-)\d+?>))",
    uR"((<Boost (?:Damage|Turns|Repeats|Analyze|BP Effect)>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<BP Battle Start: (?:\+|-)\d+?%?>))",
    uR"((<BP Regen: (?:\+|-)\d+?%?>))",
    // Enemy Notetags
    uR"((<Boost (?:Skill \d+?|.+?): (?:Full|At Least \d+?|At Most \d+?)>))",
    // State Notetags
    uR"((<Boost Sealed>))",

    // https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)
    // Bloom Map Notetags and Troop Name Tags
    uR"((<Bloom (?:Scale|Brightness|Threshold): [\d.]+?>))",
    uR"((<Bloom (?:Horz|Vert) (?:Scale|Brightness|Threshold): [\d.]+? to [\d.]+?>))",
    // Godray Map Notetags and Troop Name Tags
    uR"((<(?:No )?Godray>))",
    uR"((<Godray (?:Speed|Gain|Lacunarity): [\d.]+?>))",
    uR"((<Godray Angle: (?:\+|-)?\d+?>))",
    uR"((<Godray (?:Horz|Vert) (?:Speed|Gain|Lacunarity|Angle): [\d.]+? to [\d.]+?>))",
    // Color Adjust Map Notetags and Troop Name Tags
    uR"((<Color Adjust (?:Brightness|Contrast|Saturate): [\d.]+?>))",
    uR"((<Color Adjust (?:Horz|Vert) (?:Brightness|Contrast|Saturate): [\d.]+? to [\d.]+?>))",

    // https://www.yanfly.moe/wiki/Buffs_%26_States_Core_(YEP)
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<Max (?:MaxHP|MaxMP|ATK|DEF|MAT|MDF|AGI|LUK) (?:Buff|Debuff): (?:\+|-)\d+?>))",
    // Skill and Item Notetags
    uR"((<(?:MaxHP|MaxMP|ATK|DEF|MAT|MDF|AGI|LUK) (?:Buff|Debuff) Turns: (?:\+|-)\d+?>))",
    uR"((<State (?:\d+?|.+?) Turns: (?:\+|-)\d+?>))",
    // State Notetags - Display
    uR"((<(?:Show|Hide) Turns>))",
    uR"((<Turn Font Size: \d+?>))",
    uR"((<Turn Alignment: (?:Left|Center|Right)>))",
    uR"((<Turn Buffer (?:X|Y): (?:\+|-)\d+?>))",
    uR"((<Turn Color: \d+?>))",
    // State Notetags - Reapplication
    uR"((<Reapply (?:Ignore|Reset|Add) Turns>))",
    // Enemy Notetags
    uR"((<(?:Show|Hide) State Turns>))",
    // State Notetags - Lunatic Mode: Custom Turn Modifiers
    uR"((<Custom (?:MaxHP|MaxMP|ATK|DEF|MAT|MDF|AGI|LUK) (?:Buff|Debuff) Turn>))",
    uR"((<Custom State (?:\d+?|.+?) Turn>))",
    // State Notetags - Lunatic Mode: Custom Timing Effects
    uR"((<Custom (?:Apply|Remove|Leave|Turn Start|Action Start|Action End|Regenerate|Turn End|Battle|Victory|Escape|Defeat) Effect>))",
    // State Notetags - Lunatic Mode: Custom Action Effects
    uR"((<Custom (?:Initiate|Select|Confirm|React|Respond|Establish|Deselect|Conclude) Effect>))",
    // State Notetags - Counters
    uR"((<Counter Font Size: \d+?>))",
    uR"((<Counter Alignment: (?:left|center|right)>))",
    uR"((<Counter Buffer (?:X|Y): (?:\+|-)\d+?>))",
    uR"((<Counter Text Color: \d+?>))",

    // https://www.yanfly.moe/wiki/Extended_Damage_Over_Time_(YEP)
    // State Notetags
    uR"((<(?:Regen|DoT) Animation: \d+?>))",
    uR"((<(?:Regen|DoT) Formula: .+?>))",
    uR"((<(?:Regen|DoT) Element: \d+?>))",
    uR"((<(?:Regen|DoT) Variance: \d+?%>))",
    // Lunatic Mode
    uR"((<Custom (?:DoT|Regen) Formula>))",

    // https://www.yanfly.moe/wiki/Lunatic_Pack_-_State_Protection_(YEP)
    // State Notetags
    uR"((<Protection Animation: \d+?>))",
    uR"((<(?:HP|MP|Both) Protection: Damage Cut \d+?%>))",
    uR"((<(?:HP|MP|Both) Protection: Damage Block (?:\+|-)\d+?>))",
    uR"((<(?:HP|MP|Both) Protection: Damage (?:Null|Barrier|Ceiling|Floor) \d+?%>))",
    uR"((<HP Protection: (?:True )?Guts \d+?%>))",
    uR"((<HP Protection: Fatal Damage Absorb \d+?%>))",
    uR"((<(?:HP|MP|Both) Protection: Trigger Removal \d+?%>))",
    uR"((<(?:HP|MP|Both) Protection: Trigger Add \d+? (?:Buff|Debuff)(?:, \d+? Turns)?>))",
    uR"((<(?:HP|MP|Both) Protection: Trigger Remove \d+? (?:Buff|Debuff)>))",
    uR"((<(?:HP|MP|Both) Protection: Trigger Add State \d+?>))",
    uR"((<(?:HP|MP|Both) Protection: Trigger Remove State \d+?>))",

    // https://www.yanfly.moe/wiki/State_Categories_(YEP)
    // State Notetags
    uR"((<Category: .+?>))",
    // Skill and Item Notetags
    uR"((<Remove(?: \d+?)? State Category: .+?>))",
    uR"((<Custom Remove State Category: .+?>))",
    uR"((<\/Custom Remove State Category: .+?>))",

    // https://www.yanfly.moe/wiki/Visual_State_Effects_(YEP)
    // State Notetags
    uR"((<State Motion: (?:Walk|Wait|Chant|Guard|Damage|Evade|Thrust|Swing|Missile|Skill|Spell|Item|Escape|Victory|Dying|Abnormal|Sleep|Dead)>))",
    uR"((<State Animation: \d+?>))",

    // https://www.yanfly.moe/wiki/Card_Game_Core_(Irina)
    // Card Notetags
    uR"((<Card Art: .+?, .+?(?:, \d+?)?>))",
    uR"((<Card Dragonbones: .+?>))",
    uR"((<Card Dragonbones Animation: .+?>))",
    uR"((<Card Art Offset: (?:\+|-)\d+?, (?:\+|-)\d+?>))",
    uR"((<Card Art Scale: [\d.]+?>))",
    uR"((<Card Art Cells: \d+?x\d+?>))",
    uR"((<Cell Art Cell Index: \d+?>))",
    uR"((<Card Name: .+?>))",
    uR"((<Card Element: .+?>))",
    uR"((<Card (?:Level|Power|Health): \d+?>))",
    uR"((<Card Template: .+?>))",
    uR"((<Card Rarity: \d+?>))",
    uR"((<Card Parallax (?:BG|FG): .+?>))",
    uR"((<Card Parallax (?:BG|FG) Speed: (?:\+|-)\d+?, (?:\+|-)\d+?>))",
    uR"((<Card Parallax (?:BG|FG) Opacity: \d+?>))",
    uR"((<Card Holo (?:BG|FG): .+?>))",
    uR"((<Card Holo (?:BG|FG) Speed: \d+?>))",
    uR"((<Card Holo (?:BG|FG) Opacity: \d+?>))",
    uR"((<Card (?:Parallax BG|Holo BG|Parallax FG|Holo FG) Blend Mode: (?:Normal|Additive|Multiply|Screen)>))",
    uR"((<Card Text>))",
    uR"((<\/Card Text>))",
    uR"((<Card Text Scale: [\d.]+?>))",
    uR"((<Not a Card>))",
    // Booster Notetags
    uR"((<Booster Art: .+?>))",
    uR"((<Booster BG: .+?>))",
    uR"((<Booster Rarities: \d+?(?:, \d+?)*>))",
    uR"((<Card Pool: (?:\d+?(?:, \d+?)*|\d+? to \d+?)>))",

    // https://www.yanfly.moe/wiki/Card_Game_Deck_Builder_(Irina)
    // Item Notetags
    uR"((<Deck (?:Box|Sleeves): .+?>))",

    // https://www.yanfly.moe/wiki/Card_Game_Mechanics_(Irina)
    // Start/Upkeep/Draw/End Phase Notetags (Item)
    uR"((<Card (?:Hand|Discard|Exhaust|Library) (?:Start|Upkeep|Draw|End) JS>))",
    uR"((<\/Card (?:Hand|Discard|Exhaust|Library) (?:Start|Upkeep|Draw|End) JS>))",
    uR"((<Card (?:Hand|Discard|Exhaust|Library) (?:Start|Upkeep|Draw|End) Common Event: \d+?>))",
    // Main Phase Notetags (Item)
    uR"((<Card No Battle>))",
    uR"((<Card Spell \d+? JS>))",
    uR"((<\/Card Spell \d+? JS>))",
    uR"((<Card Spell \d+? Common Event: \d+?>))",
    uR"((<Card Spell \d+? Enable>))",
    uR"((<\/Card Spell \d+? Enable>))",
    uR"((<Card Spell \d+? Icon: \d+?>))",
    uR"((<Card Spell \d+? Name: .+?>))",
    uR"((<Card Spell \d+? Animation: \d+?>))",
    uR"((<Card Spell \d+? Allow AI Use>))",
    uR"((<Card Spell \d+? Check AI Use>))",
    uR"((<\/Card Spell \d+? Check AI Use>))",
    // Battle Phase Notetags (Item)
    uR"((<Card AI Priority-.+?: \d+?>))",
    uR"((<Card (?:Pre-Battle|Tie Battle|Lose Battle|Win Battle|Post-Battle|Resolve Battle) JS>))",
    uR"((<\/Card (?:Pre-Battle|Tie Battle|Lose Battle|Win Battle|Post-Battle|Resolve Battle) JS>))",
    uR"((<Card (?:Pre-Battle|Tie Battle|Lose Battle|Win Battle|Post-Battle|Resolve Battle) Common Event: \d+?>))",

    // https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)
    // Actor Notetags
    uR"((<Unlock Class: (?:\d+?(?:, \d+?)*|\d+? to \d+?)>))",
    uR"((<Cannot Change Class>))",
    uR"((<.+? Character: .+? \d+?>))",
    uR"((<.+? Face: .+? \d+?>))",
    uR"((<.+? Battler: .+?>))",
    // Class Notetags
    uR"((<Icon: \d+?>))",
    uR"((<Use Nickname>))",
    uR"((<Help Description>))",
    uR"((<\/Help Description>))",
    uR"((<Level Unlock Requirements>))",
    uR"((<\/Level Unlock Requirements>))",

    // https://www.yanfly.moe/wiki/Subclass_(YEP)
    // Actor Notetags
    uR"((<Subclass: \d+?>))",
    uR"((<Cannot Change Subclass>))",
    uR"((<Restrict (?:Class|Subclass): (?:\d+?(?:, \d+?)*|\d+? to \d+?)>))",
    // Class Notetags
    uR"((<Primary Only>))",
    uR"((<Subclass Only>))",
    uR"((<.+? Combo Name: .+?>))",
    // Skill and Item Notetags
    uR"((<Require (?:Class|Subclass): (?:\d+?(?:, \d+?)*|\d+? to \d+?)>))",

    // https://www.yanfly.moe/wiki/Counter_State_(Arisu)
    // Actor, Class, Skill, Weapon, Armor, Enemy, State Notetags
    uR"((<(?:Physical |Magical |Certain Hit )?Counter State \d+?: \d+?%>))",

    // https://www.yanfly.moe/wiki/Core_Engine_(YEP)
    // Item, Weapon, Armor Notetags
    uR"((<Price: \d+?>))",
    uR"((<Max Item: \d+?>))",
    // Enemy Notetags
    uR"((<Gold: \d+?>))",
    uR"((<(?:hp|mp|atk|def|mat|mdf|agi|luk): \d+?>))",
    uR"((<exp: \d+?>))",
    // Actor Notetags
    uR"((<(?:Initial|Max) Level: \d+?>))",
    // Class Skills to Learn Notetag
    uR"((<Learn at Level: \d+?>))",
    // Weapon and Armor Notetags
    uR"((<(?:hp|mp|atk|def|mat|mdf|agi|luk): (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Damage_Core_(YEP)
    // Skill and Item Notetags
    uR"((<Bypass Damage Cap>))",
    uR"((<damage formula>))",
    uR"((<\/damage formula>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<Damage Cap: \d+?>))",
    uR"((<Heal Cap: \d+?>))",

    // https://www.yanfly.moe/wiki/Armor_Scaling_(YEP)
    // Skill and Item Notetags
    uR"((<Armor Reduction: \d+?>))",
    uR"((<Armor Reduction: \d+?%>))",
    uR"((<Armor Penetration: \d+?%>))",
    uR"((<Armor Penetration: \d+?>))",
    uR"((<Bypass Armor Scaling>))",
    // Actor, Class, Enemy, Weapon, Armor, State Notetags
    uR"((<(?:Physical|Magical|Certain) Armor Reduction: \d+?>))",
    uR"((<(?:Physical|Magical|Certain) Armor Reduction: \d+?%>))",
    uR"((<(?:Physical|Magical|Certain) Armor Penetration: \d+?%>))",
    uR"((<(?:Physical|Magical|Certain) Armor Penetration: \d+?>))",
    // Lunatic Mode Notetags
    uR"((<(?:Positive Armor Rate|Negative Armor Rate|Base Armor)>))",
    uR"((<\/(?:Positive Armor Rate|Negative Armor Rate|Base Armor)>))",

    // https://www.yanfly.moe/wiki/Critical_Control_(YEP)
    // Skill and Item Notetags
    uR"((<Critical Rate: \d+?%>))",
    uR"((<Critical Rate: \d+?\.\d+?>))",
    uR"((<Critical Multiplier: \d+?%>))",
    uR"((<Critical Multiplier: \d+?\.\d+?>))",
    uR"((<Flat Critical: \d+?% (?:hp|mp|atk|def|mat|mdf|agi|luk)>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<Critical Multiplier: (?:\+|-)\d+?%>))",
    uR"((<Flat Critical: (?:\+|-)\d+?>))",
    uR"((<Certain Hit Critical Rate: (?:\+|-)\d+?%>))",
    uR"((<Physical Critical Rate: (?:\+|-)\d+?%>))",
    uR"((<Magical Critical Rate: (?:\+|-)\d+?%>))",
    // Lunatic Mode Notetags
    uR"((<(?:Critical Rate|Critical Multiplier|Flat Critical) Formula>))",
    uR"((<\/(?:Critical Rate|Critical Multiplier|Flat Critical) Formula>))",

    // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Critical_Sway_(YEP)
    // User Param Rate Section
    uR"((<Custom Critical Rate: \d+? (?:Pride|Crisis) \d+?%(?:, nonstackable)?>))",
    // Target Param Rate Section
    uR"((<Custom Critical Rate: \d+? (?:Hero|Bully) \d+?%(?:, nonstackable)?>))",
    // Element Rate Section
    uR"((<Custom Critical Rate: Element Rate \d+?%(?:, nonstackable)?>))",
    // State Modifiers / Buff Modifiers / Debuff Modifiers Sections
    uR"((<Custom Critical Rate: (?:User|Target) (?:States|Buffs|Debuffs) (?:\+|-)\d+?%(?:, nonstackable)?>))",

    // https://www.yanfly.moe/wiki/Dash_Toggle_(YEP)
    // Actor, Class, Weapon, Armor, State Notetags
    uR"((<Disable Dashing>))",

    // https://www.yanfly.moe/wiki/Disable_Auto_Shadow_Extended_(YEP)
    // Map and Tileset Notetags
    uR"((<(?:Hide|Show) Shadows>))",

    // https://www.yanfly.moe/wiki/Dragonbones_Integration_(YEP)
    // Actor and Enemy Notetags
    uR"((<DragonBone(?: Battler)?: .+?>))",
    uR"((<DragonBone Scale(?:X|Y): -?[\d.]+?>))",
    uR"((<DragonBone (?:Width|Height): \d+?>))",
    uR"((<DragonBone (?:Keep|Replace) Sprite>))",
    uR"((<DragonBone Ani (?:attack|walk|thrust|escape|wait|swing|victory|chant|missile|dying|guard|skill|abnormal|damage|evade|spell|sleep|dead|item): .+?>))",
    uR"((<DragonBone Settings>))",
    uR"((<\/DragonBone Settings>))",

    // https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)
    // Actor, Enemy, Event Notetags, Event Page Comment Tags
    uR"((<Dragonbones Sprite: .+?>))",
    uR"((<Dragonbones Sprite Scale(?: X| Y)?: -?[\d.]+?(?:, -?[\d.]+?)?>))",
    uR"((<Dragonbones Sprite Time Scale: [\d.]+?>))",
    uR"((<Dragonbones Sprite (?:Size: \d+?, \d+?|Width: \d+?|Height: \d+?)>))",
    uR"((<Dragonbones Sprite (?:No )?Flip (?:Left|Right)>))",
    uR"((<Dragonbones Sprite Motion (?:Idle|Walk|Dash|Jump|LadderIdle|LadderClimb): .+?>))",
    uR"((<Dragonbones Sprite Settings>))",
    uR"((<\/Dragonbones Sprite Settings>))",

    // https://www.yanfly.moe/wiki/Element_Core_(YEP)
    // Skill and Item Notetags
    uR"((<Bypass Element Reflect>))",
    uR"((<Multiple Elements: .+?>))",
    uR"((<Multi-Element Rule: (?:Lowest|Add|Multiply|Highest|Average)>))",
    // Actor, Class, Enemy, Weapon, Armor, and State Notetags
    uR"((<Element Absorb: .+?>))",
    uR"((<Element (?:Reflect|Magnify|Amplify) .+?: (?:\+|-)\d+?%>))",
    uR"((<Element Null>))",
    uR"((<Force Element .+? Rate: \d+?%>))",

    // https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)
    // Enemy Notetags
    uR"((<(?:Show|Hide) Level>))",
    uR"((<Minimum Level: \d+?>))",
    uR"((<Maximum Level: \d+?>))",
    uR"((<Static Level: \d+?>))",
    uR"((<Starting Level Type: [0-5]>))",
    uR"((<Positive Level Fluctuation: \d+?>))",
    uR"((<Negative Level Fluctuation: \d+?>))",
    uR"((<Level Fluctuation: \d+?>))",
    uR"((<(?:maxhp|maxmp|atk|def|mat|mdf|agi|luk|exp|gold) Rate: (?:\+|-)\d+?% per level>))",
    uR"((<(?:maxhp|maxmp|atk|def|mat|mdf|agi|luk|exp|gold) Flat: (?:\+|-)\d+? per level>))",
    uR"((<Resist Level Change>))",
    uR"((<Skill .+? Require Level: \d+?>))",
    uR"((<Ignore Level Bonus>))",
    uR"((<Custom Starting Level>))",
    uR"((<\/Custom Starting Level>))",
    uR"((<Custom Parameter (?:maxhp|maxmp|atk|def|mat|mdf|agi|luk) Formula>))",
    uR"((<\/Custom Parameter (?:maxhp|maxmp|atk|def|mat|mdf|agi|luk) Formula>))",
    // Skill and Item Notetags
    uR"((<Reset Enemy Level>))",
    uR"((<Change Enemy Level: (?:\+|-)\d+?>))",
    uR"((<Custom Change Enemy Level>))",
    uR"((<\/Custom Change Enemy Level>))",

    // https://www.yanfly.moe/wiki/Difficulty_Slider_(YEP)
    // Enemy Notetags
    uR"((<Unaffected by Difficulty Slider>))",

    // https://www.yanfly.moe/wiki/Enemy_Base_Parameters_(YEP)
    // Enemy Notetags
    uR"((<Base Parameters on Class: \d+?>))",
    uR"((<Base (?:maxhp|maxmp|atk|def|mat|mdf|agi|luk) Parameter on Class: \d+?>))",
    uR"((<Custom Enemy Parameters>))",
    uR"((<\/Custom Enemy Parameters>))",
    uR"((<Custom Base (?:maxhp|maxmp|atk|def|mat|mdf|agi|luk|exp|gold) Formula>))",
    uR"((<\/Custom Base (?:maxhp|maxmp|atk|def|mat|mdf|agi|luk|exp|gold) Formula>))",

    // https://www.yanfly.moe/wiki/Map_Enemy_Levels_(YEP)
    // Map Notetags
    uR"((<Enemy Level Base: \d+?>))",
    uR"((<Enemy Level Base Variable: \d+?>))",
    uR"((<Enemy Level Range: \d+? to \d+?>))",
    uR"((<Enemy Level Range Variables: \d+? to \d+?>))",

    // https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)
    // Actor and Enemy Notetags
    uR"((<TP Mode: \d+?>))",
    uR"((<Unlock TP Mode: \d+?(?:(?:, \d+?)+| to \d+?)?>))",
    // Skill and Item Notetags
    uR"((<Unlock TP Mode: \d+?(?:(?:, \d+?)+| to \d+?)?>))",
    // Skill Notetags
    uR"((<Learn Unlock TP Mode: \d+?(?:(?:, \d+?)+| to \d+?)?>))",

    // https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)
    // Actor Notetags
    uR"((<Starting Skill Slots: \d+?>))",
    // Skill Notetags
    uR"((<Equip (?:hp|mp|atk|def|mat|mdf|agi|luk): (?:\+|-)\d+?>))",
    uR"((<Equip State: \d+?(?:(?:, \d+?)+| through \d+?)?>))",
    uR"((<Unequippable>))",
    uR"((<All Access Equippable>))",
    uR"((<Access Only Equippable>))",
    // Class, Skill, Weapon, Armor, and State Notetags
    uR"((<Equip Skill Slots: (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Equip_Skill_Tiers_(YEP)
    // Skill Notetags
    uR"((<Skill Tier: \d+?>))",
    // Actor, Class, Skill, Weapon, Armor, and State Notetags
    uR"((<Skill Tier \d+? Slots: (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Equip_Core_(YEP)
    // Class Notetags
    uR"((<Equip Slot: \d+?(?:, \d+?)*>))",
    uR"((<Equip Slot>))",
    uR"((<\/Equip Slot>))",
    // Weapon and Armor Notetags
    uR"((<(?:hp|mp|atk|def|mat|mdf|agi|luk): (?:\+|-)\d+?>))",
    // Lunatic Mode (Advanced JavaScript) - Weapon and Armor Notetags
    uR"((<Custom Parameters>))",
    uR"((<\/Custom Parameters>))",

    // https://www.yanfly.moe/wiki/Change_Battle_Equip_(YEP)
    // Actor, Class, Weapons, Armors, and State Notetags
    uR"((<Change Battle Equip Cooldown: (?:\+|-)\d+?>))",
    uR"((<Disable Change Battle Equip>))",

    // https://www.yanfly.moe/wiki/Equip_Requirements_(YEP)
    // Weapon and Armor Notetags
    uR"((<Equip Requirement>))",
    uR"((<\/Equip Requirement>))",
    uR"((<Custom Equip Requirement Condition>))",
    uR"((<\/Custom Equip Requirement Condition>))",
    uR"((<Custom Equip Requirement Text>))",
    uR"((<\/Custom Equip Requirement Text>))",

    // https://www.yanfly.moe/wiki/Equipment_Set_Bonuses_(Olivia)
    // Weapon and Armor Notetags
    uR"((<Equip Set: .+?>))",
    // Actor Notetags
    uR"((<.+? Set, \d+? Pieces Character: .+?, \d+?>))",
    uR"((<.+? Set, \d+? Pieces Face: .+?, \d+?>))",
    uR"((<.+? Set, \d+? Pieces Battler: .+?>))",

    // https://www.yanfly.moe/wiki/Event_Copier_(YEP)
    uR"((<Copy Event: (?:Map \d+?, Event \d+?|\d+?, \d+?|.+?)>))",

    // https://www.yanfly.moe/wiki/Event_Encounter_Aid_(YEP)
    uR"((<Encounter (?:Direction )?Lock>))",
    uR"((<Follower (?:Touch|Trigger)>))",

    // https://www.yanfly.moe/wiki/Extra_Enemy_Drops_(YEP)
    // Generic Drop Notetags
    uR"((<(?:Item|Weapon|Armor) \d+?: \d+?%>))",
    uR"((<\/?Enemy Drops>))",
    uR"((<Drop .+?: \d+?%>))",
    // Conditional Drop Notetags
    uR"((<\/?Conditional (?:(?:Item|Weapon|Armor) \d+?|Named) Drop>))",

    // https://www.yanfly.moe/wiki/Extra_Parameter_Formula_(YEP)
    // Actor, Class, Enemy, Weapon, Armor, State Notetags
    uR"((<(?:hit|eva|cri|cev|mev|mrf|cnt|hrg|mrg|trg) Plus: (?:\+|-)\d+?(?:\.\d+?)?%?>))",
    uR"((<(?:hit|eva|cri|cev|mev|mrf|cnt|hrg|mrg|trg) Rate: \d+?(?:\.\d+?)?%?>))",
    uR"((<(?:hit|eva|cri|cev|mev|mrf|cnt|hrg|mrg|trg) Flat: (?:\+|-)\d+?(?:\.\d+?)?%?>))",

    // https://www.yanfly.moe/wiki/Floor_Damage_(YEP)
    // Tileset Notetags
    uR"((<Floor Damage \d+?: \d+?>))",
    uR"((<Floor Flash \d+?: \d+?, \d+?, \d+?, \d+?>))",
    // Lunatic Mode
    uR"((<\/?Custom Floor Damage \d+?>))",

    // https://www.yanfly.moe/wiki/Footstep_Sounds_(YEP)
    // Event Notetags
    uR"((<No Footsteps>))",
    // Tileset Notetags
    uR"((<Terrain Tag \d+? Footstep Sound: .+?(?:, \d+?)?(?:, \d+?)?>))",
    // Map Notetags
    uR"((<Region \d+? Footstep Sound: .+?(?:, \d+?)?(?:, \d+?)?>))",

    // https://www.yanfly.moe/wiki/Hit_Damage_Sounds_(YEP)
    // Armor and Enemy Notetags
    uR"((<Hit Damage Sound: .+?(?:, \d+?)?(?:, \d+?)?(?:, \d+?)?>))",

    // https://www.yanfly.moe/wiki/Instant_Cast_(YEP)
    // Skill and Item Notetags
    uR"((<Instant(?: Cast)?>))",
    uR"((<Instant Eval>))",
    uR"((<\/Instant Eval>))",
    // Actor, Class, Enemy, Weapon, Armor, State Notetags
    uR"((<(?:Instant|Cancel Instant) (?:Skill|Item): \d+?(?:, \d+?)*>))",
    uR"((<(?:Instant|Cancel Instant) (?:Skill|Item): \d+? to \d+?>))",

    // https://www.yanfly.moe/wiki/Item_Concoctions_(Olivia)
    // Actor, Class, Weapon, Armor, State Notetags / Item Notetags
    uR"((<Item Concoct>))",
    uR"((<\/Item Concoct>))",
    // State Notetag
    uR"((<Item Concoct Seal>))",

    // https://www.yanfly.moe/wiki/Item_Core_(YEP)
    // Item, Weapon, Armor Notetags
    uR"((<Random Variance: \d+?>))",
    uR"((<Not Independent Item>))",
    uR"((<Priority Name>))",
    uR"((<Text Color: \d+?>))",
    // Lunatic Mode Notetags
    uR"((<On Creation Eval>))",
    uR"((<\/On Creation Eval>))",
    uR"((<Info Text (?:Top|Bottom)>))",
    uR"((<\/Info Text (?:Top|Bottom)>))",
    uR"((<Info Eval>))",
    uR"((<\/Info Eval>))",

    // https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)
    // Weapon and Armor Notetags
    uR"((<Augment Slots>))",
    uR"((<\/Augment Slots>))",
    uR"((<No Augment Slots>))",
    // Item, Weapon, Armor Notetags
    uR"((<Augment(?: Attach| Detach)?: .+?>))",
    uR"((<\/Augment(?: Attach| Detach)?: .+?>))",
    // Lunatic Mode Notetags
    uR"((<Augment (?:Attach|Detach) Eval: .+?>))",
    uR"((<\/Augment (?:Attach|Detach) Eval: .+?>))",

    // https://www.yanfly.moe/wiki/Item_Disassemble_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<Disassemble Pool(?:: .+?)?>))",
    uR"((<\/Disassemble Pool(?:: .+?)?>))",
    uR"((<Disassembler(?:: .+?)?>))",
    uR"((<Disassemble Sound (?:Name|Volume|Pitch|Pan): .+?>))",
    // Lunatic Mode Notetags
    uR"((<Custom Disassembl(?:ed|er) Effect>))",
    uR"((<\/Custom Disassembl(?:ed|er) Effect>))",

    // https://www.yanfly.moe/wiki/Item_Discard_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<Can(?:not)? Discard>))",

    // https://www.yanfly.moe/wiki/Item_Durability_(YEP)
    // Weapon and Armor Notetags
    uR"((<Durability(?: Variance| Maximum)?: \d+?>))",
    uR"((<Bypass Durability>))",
    uR"((<Unbreakable>))",
    uR"((<Break Sound Name: .+?>))",
    uR"((<Break Sound (?:Volume|Pitch): \d+?>))",
    uR"((<Break Sound Pan: (?:\+|-)\d+?>))",
    // Item, Weapon, Armor Notetags
    uR"((<Repair (?:Durability|Weapon|Armor): \d+?>))",
    uR"((<Repair (?:WType|AType) \d+?: \d+?>))",
    uR"((<Repair Sound Name: .+?>))",
    uR"((<Repair Sound (?:Volume|Pitch): \d+?>))",
    uR"((<Repair Sound Pan: (?:\+|-)\d+?>))",
    uR"((<Unbreakable (?:Durability|Weapon|Armor)>))",
    uR"((<Unbreakable (?:WType|AType) \d+?>))",
    // Skill and Item Notetags
    uR"((<(?:User|Target) (?:All |Random )?Weapon Durability: (?:\+|-)\d+?>))",
    uR"((<(?:User|Target) (?:All |Random )?Armor Durability: (?:\+|-)\d+?>))",
    // Lunatic Mode Notetags
    uR"((<Custom (?:Break|Repair) Effect>))",
    uR"((<\/Custom (?:Break|Repair) Effect>))",
    uR"((<Custom (?:User|Target) (?:All|Random) (?:Weapon|Armor) Durability>))",
    uR"((<\/Custom (?:User|Target) (?:All|Random) (?:Weapon|Armor) Durability>))",

    // https://www.yanfly.moe/wiki/Item_Menu_Categories_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<Menu Category: .+?>))",

    // https://www.yanfly.moe/wiki/Item_Picture_Images_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<Picture: .+?>))",
    uR"((<Picture Image: .+?>))",
    uR"((<Picture Hue: \d+?>))",

    // https://www.yanfly.moe/wiki/Item_Rename_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<(?:Can|Cannot) Rename>))",

    // https://www.yanfly.moe/wiki/Item_Requirements_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<Enable Requirements>))",
    uR"((<\/Enable Requirements>))",
    // Lunatic Mode
    uR"((<Custom Enable Requirement>))",
    uR"((<\/Custom Enable Requirement>))",

    // https://www.yanfly.moe/wiki/Item_Upgrade_Slots_(YEP)
    // Weapon and Armor Notetags
    uR"((<Upgrade Slots: \d+?>))",
    uR"((<Slot Variance: \d+?>))",
    uR"((<Upgrade Sound: .+?>))",
    uR"((<Upgrade Effect>))",
    uR"((<\/Upgrade Effect>))",
    // Item Notetags
    uR"((<Upgrade Item Type: .+?>))",
    uR"((<Type: .+?>))",
    // Weapon and Armor Notetags
    uR"((<Upgrade (?:Weapon|Armor) Type: \d+?(?:, \d+?)*(?: through \d+?)?>))",

    // https://www.yanfly.moe/wiki/Item_Synthesis_(YEP)
    // Item, Weapon, and Armor Notetags
    uR"((<(?:Item|Weapon|Armor) Recipe: \d+?(?:, \d+?)*(?: to \d+?)?>))",
    uR"((<Synthesis Ingredients>))",
    uR"((<\/Synthesis Ingredients>))",
    uR"((<Mask Name: .+?>))",
    // Lunatic Mode
    uR"((<Custom Synthesis Effect>))",
    uR"((<\/Custom Synthesis Effect>))",

    // https://www.yanfly.moe/wiki/Job_Points_(YEP)
    // Actor Notetags
    uR"((<Starting JP: \d+?>))",
    uR"((<Class \d+? Starting JP: \d+?>))",
    // Actor, Class, Weapon, Armor, and State Notetags
    uR"((<JP Rate: \d+?%>))",
    // Skill and Item Notetags
    uR"((<JP Gain: \d+?>))",
    uR"((<Target JP Gain: \d+?>))",
    // Enemy Notetag
    uR"((<JP: \d+?>))",

    // https://www.yanfly.moe/wiki/Level_Up_Growth_Effects_(YEP)
    // Actor, Class, Skill, Weapon, Armor, State Notetags
    uR"((<Level Up (?:MaxHP|MaxMP|ATK|DEF|MAT|MDF|AGI|LUK) Growth: (?:\+|-)\d+?>))",
    uR"((<Level Up Learn Skill: \d+?>))",
    uR"((<Level Up Switch (?:On|Off): \d+?(?:, \d+?)*(?: to \d+?)?>))",
    uR"((<Level Up Recover All>))",
    // Lunatic Mode
    uR"((<Custom Level Up Effect>))",
    uR"((<\/Custom Level Up Effect>))",

    // https://www.yanfly.moe/wiki/Life_Steal_(YEP)
    // Skill and Item Notetags
    uR"((<(?:HP|MP) Life Steal: \d+?%>))",
    uR"((<(?:HP|MP) Life Steal: \d+?>))",
    // Skill and Item, and Actor, Class, Enemy, Weapon, Armor, State Notetags
    uR"((<Cancel (?:HP |MP )?Life Steal>))",
    // Actor, Class, Enemy, Weapon, Armor, State Notetags
    uR"((<(?:HP|MP) Life Steal (?:Physical|Magical|Certain): (?:\+|-)\d+?%>))",
    uR"((<(?:HP|MP) Life Steal (?:Physical|Magical|Certain): (?:\+|-)\d+?>))",
    uR"((<Guard (?:HP |MP )?Life Steal>))",

    // https://www.yanfly.moe/wiki/Main_Menu_Actor_Events_(YEP)
    // Actor Notetags
    uR"((<Menu .+? Event: \d+?>))",

    // https://www.yanfly.moe/wiki/Map_Display_Name_Core_(Olivia)
    // Map Notetags
    uR"((<Dim Color (?:1|2): \d+?, \d+?, \d+?, \d+?>))",
};

static constexpr usize PATTERN_SIZE = joinedPatternSize(PATTERN_PARTS);
static constexpr array<char16_t, PATTERN_SIZE> PATTERN_STORAGE = buildPattern<PATTERN_SIZE>(PATTERN_PARTS);
static constexpr QStringView PATTERN = QStringView(std::u16string_view(PATTERN_STORAGE.data(), PATTERN_STORAGE.size()));

static constexpr array<const char*, 563>
    TOOLTIPS = {
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<User Barrier: +x> / <Target Barrier: +x> adjusts unexpiring Barrier Points for the user or target by ±x. A negative x removes barrier points from both unexpiring and temporary pools.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<User Barrier x Turns: +y> / <Target Barrier x Turns: +y> adjusts Barrier Points for the user or target by ±y, expiring after x turns.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Bypass Barrier> lets this skill/item bypass Barrier Points, dealing damage directly to the target.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Penetration: x%> causes x% of this skill/item's damage to bypass the target's Barrier Points.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Penetration: x> causes a flat x value of this skill/item's damage to bypass the target's Barrier Points.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Penetration: +x%> / <Barrier Penetration: -x%> gives the attacker a ±x% multiplicative bonus to Barrier Penetration on any damaging action.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Penetration: +x> / <Barrier Penetration: -x> gives the attacker a flat ±x bonus to Barrier Penetration.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Points: +x> / <Barrier Points: -x> grants (or removes) x unexpiring Barrier Points at the start of battle.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Points x Turns: +y> / <Barrier Points x Turns: -y> grants (or removes) y Barrier Points lasting x turns at the start of battle.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Regen: +x> / <Barrier Regen: -x> regenerates ±x unexpiring Barrier Points during the regeneration phase.)"
        ),
        QT_TR_NOOP(
            ABSORPTION_BARRIER_NOTETAGS
            R"(<Barrier Regen x Turns: +y> / <Barrier Regen x Turns: -y> regenerates Barrier Points lasting x turns with a ±y value during the regeneration phase.)"
        ),

        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Picture: filename> changes this actor/enemy's cutin to use a picture from img/pictures/.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Face: filename, index> changes this actor/enemy's cutin to use a face graphic cell from img/faces/ (index starts at 0).)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin SV: filename, index> changes this actor/enemy's cutin to use a Side View Actor graphic from img/sv_actors/.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Battler: filename, hue> changes this actor/enemy's cutin to use a battler graphic, with an optional hue (0-360).)"
        ),
        QT_TR_NOOP(ACTION_CUTIN_NOTETAGS R"(<Action Cutin Scale: n> changes the cutin graphic's scale (1.0 = 100%).)"),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Offset X: ±n> / <Action Cutin Offset Y: ±n> offsets the cutin graphic's position in pixels.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Anti-Alias> / <Action Cutin No Anti-Alias> toggles anti-aliasing for the cutin graphic.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin> / <No Action Cutin> toggles whether this skill/item shows an action cutin.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Image: filename> changes the speedline graphic behind the face cutin, loaded from img/pictures/.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Hue: n> changes the hue (0-360) of the speedline graphic behind the face cutin.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Blend Mode: mode> sets the speedline graphic's blend mode to Normal, Additive, Multiply, or Screen.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Border Thickness: n> sets the border thickness (pixels) around the action cutin.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS R"(<Action Cutin Border Color: color> sets the border color, by name or hex code.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Screen X: n> / <Action Cutin Screen Y: n> sets the speedline graphic's screen position (X = far-left coordinate, Y = middle coordinate).)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Shift Distance X: n> / <Action Cutin Shift Distance Y: n> sets how far (pixels) the face shifts side to side / vertically.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS R"(<Action Cutin Shift Frames: n> sets the duration (frames) of the shift animation.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Flash> / <Action Cutin No Flash> toggles the screen flash on this action cutin.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Flash Color: red, green, blue, intensity> sets the screen flash color (RGB and intensity 0-255 each).)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS R"(<Action Cutin Flash Duration: n> sets the duration (frames) of the screen flash.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Shake> / <Action Cutin No Shake> toggles screen shake on this action cutin.)"
        ),
        QT_TR_NOOP(ACTION_CUTIN_NOTETAGS R"(<Action Cutin Shake Power: n> sets the power of the screen shake.)"),
        QT_TR_NOOP(ACTION_CUTIN_NOTETAGS R"(<Action Cutin Shake Speed: n> sets the speed of the screen shake.)"),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS R"(<Action Cutin Shake Duration: n> sets the duration (frames) of the screen shake.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Sound> / <Action Cutin No Sound> toggles whether a sound effect plays for this action cutin.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Sound Filename: filename> sets the sound effect played, from audio/se/.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Sound Volume: n> sets the volume of the action cutin's sound effect.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS R"(<Action Cutin Sound Pitch: n> sets the pitch of the action cutin's sound effect.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Sound Pan: n> sets the pan of the action cutin's sound effect (negative = left, positive = right).)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS R"(<Action Cutin Duration: n> sets the total duration (frames) of the action cutin.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Opacity Speed: n> sets the rate at which the action cutin fades in and out.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Forced Scale: n> forces the battler's face graphic to a specific scale, overriding the actor/enemy notetag.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Forced Anti-Alias> / <Action Cutin Forced No Anti-Alias> forces anti-aliasing on/off for the battler's face graphic.)"
        ),
        QT_TR_NOOP(
            ACTION_CUTIN_NOTETAGS
            R"(<Action Cutin Forced Offset X: ±n> / <Action Cutin Forced Offset Y: ±n> forces the cutin graphic's offset position, overriding the actor/enemy notetag.)"
        ),

        QT_TR_NOOP(
            ACTION_SEQUENCE_FAST_FORWARD_SKIP_NOTETAGS
            R"(<Cannot Fast Forward> / <Cannot Skip Forward> prevents this item/skill's action sequence from being fast-forwarded or skipped, e.g. for button-input or scan-display actions.)"
        ),
        QT_TR_NOOP(
            ACTION_SEQUENCE_FAST_FORWARD_SKIP_NOTETAGS
            R"(<Fast Forward Speed: x> sets how fast Fast Forward plays through this item/skill's action sequence (x > 1).)"
        ),
        QT_TR_NOOP(
            ACTION_SEQUENCE_FAST_FORWARD_SKIP_NOTETAGS
            R"(<Skip Forward Speed: x> sets how fast Skip Forward plays through this item/skill's action sequence (x > 1).)"
        ),

        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS
            R"(<Passive State: x> or <Passive State: x, x, x> lets the battler passively hold state(s) x. In a weapon/armor notebox, the state is granted to whoever equips it.)"
        ),
        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS R"(<Passive State: x to y> passively grants states x through y in sequence.)"
        ),
        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS
            R"(<Passive Condition: HP Above x%> / <Passive Condition: HP Below x%> / same for MP - the passive state only applies while the condition holds.)"
        ),
        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS
            R"(<Passive Condition: stat Above x> / <Passive Condition: stat Below x> - replace 'stat' with HP, MP, TP, MAXHP, ATK, DEF, MAT, MDF, AGI, or LUK; the passive state applies only while the stat's raw value satisfies the comparison.)"
        ),
        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS
            R"(<Passive Condition: Switch x ON> / <Passive Condition: Switch x OFF> - the passive state applies only while switch x matches the given value.)"
        ),
        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS
            R"(<Passive Condition: Variable x Above y> / <Passive Condition: Variable x Below y> - the passive state applies only while variable x satisfies the comparison against y.)"
        ),
        QT_TR_NOOP(
            AUTO_PASSIVE_STATES_NOTETAGS
            R"(<Custom Passive Condition> ... </Custom Passive Condition> wraps JavaScript that returns true/false to decide whether the passive state applies. Evaluated only after all standard conditions on the state already pass.)"
        ),

        QT_TR_NOOP(
            LUNATIC_PACK_PASSIVE_CONDITION_CASES_NOTETAGS
            R"(<Passive Condition Cases> ... </Passive Condition Cases> wraps a list of case-style conditions (switch state, numeric comparisons, has/not state or buff, turn count, etc.) that all decide whether the state's passive effect is active.)"
        ),

        QT_TR_NOOP(
            PASSIVE_AURA_EFFECTS_NOTETAGS
            R"(<Ally Aura: x> and its variants (Alive/Dead Ally, Foe, Party, Troop, Everybody, Alive, Dead) grant state(s) x (optionally a range "x to y") to the named group of battlers while the holder is affected by this state.)"
        ),
        QT_TR_NOOP(
            PASSIVE_AURA_EFFECTS_NOTETAGS
            R"(<Custom Aura Condition> ... </Custom Aura Condition> wraps JavaScript that sets the 'condition' variable to true/false, deciding whether the aura state is applied.)"
        ),

        QT_TR_NOOP(
            BASE_PARAMETER_CONTROL_NOTETAGS
            R"(<stat Plus: ±x> adds x to the base parameter before rate and flat values are applied. Replace 'stat' with MaxHP, MaxMP, Atk, Def, Mat, Mdf, Agi, or Luk.)"
        ),
        QT_TR_NOOP(
            BASE_PARAMETER_CONTROL_NOTETAGS
            R"(<stat Rate: x%> or <stat Rate: x.y> multiplies the sum of the base and plus values of the stat before the buff rate and flat value are applied.)"
        ),
        QT_TR_NOOP(
            BASE_PARAMETER_CONTROL_NOTETAGS
            R"(<stat Flat: ±x> adds x at the very end, after base+plus have been summed and multiplied by rate.)"
        ),
        QT_TR_NOOP(
            BASE_PARAMETER_CONTROL_NOTETAGS
            R"(<stat Max: x> / <stat Min: x> caps the stat's final value. When multiple sources apply, the largest cap notetag wins.)"
        ),

        QT_TR_NOOP(
            BATTLE_AI_CORE_NOTETAGS
            R"(<AI Level: x> sets the enemy's AI level. Lower x means more random behavior; higher x means it follows the AI Priority list more strictly.)"
        ),
        QT_TR_NOOP(
            BATTLE_AI_CORE_NOTETAGS
            R"(<AI Priority> ... </AI Priority> wraps a "condition: skill, target" list the enemy AI checks top to bottom, using the first fulfilled condition's action. Conditions can be combined with +++.)"
        ),
        QT_TR_NOOP(
            BATTLE_AI_CORE_NOTETAGS
            R"(<AI Consider Taunt> makes the enemy's AI decision-making account for taunted battlers (requires YEP_Taunt.js).)"
        ),

        QT_TR_NOOP(
            ACTOR_AUTO_BATTLE_AI_NOTETAGS
            R"(<AI Level: x> sets the actor's AI level. Lower x means more random behavior; higher x means it follows the AI Priority list more strictly.)"
        ),
        QT_TR_NOOP(
            ACTOR_AUTO_BATTLE_AI_NOTETAGS
            R"(<AI Priority> ... </AI Priority> wraps a "condition: skill, target" list the actor's auto-battle AI checks top to bottom, taking the first fulfilled entry's action.)"
        ),

        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Skill Target Change: Self to All> / <Item Target Change: Self to All> turns self-scope skills/items into all-scope in battle. Ignored by skills/items with <Bypass Target Change>.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Skill Target Change Allies: All to One> and its variants (Skill/Item x Allies/Enemies x All-to-One/One-to-All) remap a skill/item's all-vs-one targeting scope in battle.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Bypass Target Change> / <Divine> makes this skill/item immune to the target-scope-change notetags above.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<JP x5> / <EXP x10> / <Gold x200> multiplies the respective reward earned in the current battle by the given amount.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Destroy Weapon> destroys the actor's currently equipped weapon after this skill is used.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Extra Skill List: x> or <Extra Skill List: x, x, x> turns this skill into a folder: using it opens a new window listing skill(s) x to choose from, in battle only.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<All Element Damage Rate: x%> makes the battler take x% damage from every element while affected.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Break Popup> shows the Break popup when the battler receives this state.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Buff Immunity: x> or <Buff Immunity: x, x, x> prevents the listed parameter(s) (0-7) from being buffed while affected.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Debuff Immunity: x> or <Debuff Immunity: x, x, x> prevents the listed parameter(s) (0-7) from being debuffed while affected.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Damage Color: r, g, b, a> changes the HP-damage popup color for the battler while affected (each channel 0-255).)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Item Seal> prevents the actor from using items from the actor command menu while affected.)"
        ),
        QT_TR_NOOP(BATTLE_EFFECTS_PACK_1_NOTETAGS R"(<Max Turns: x> caps how many turns this state can last to x.)"),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<No Weak Popup> suppresses the weakness popup when the battler is hit by an elemental weakness while affected.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<Follow Up Skill: x> (or Physical/Magical/Certain Follow Up Skill) performs skill x automatically after a matching hit-type skill completes.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_1_NOTETAGS
            R"(<State Immunity: x> or <State Immunity: x, x, x> prevents the battler from receiving the listed state(s) while affected.)"
        ),

        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<High Health param: +x%> / <Low Health param: +x%> boosts the battler's parameter (ATK/DEF/MAT/MDF/AGI/LUK) by up to x% as HP approaches full (High Health) or 0% (Low Health).)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<Damage Cut: x%> reduces incoming damage by x%. Stacks additively across sources.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<Element id Cut: x%> / <Element name Cut: x%> reduces damage from the specified element by x%.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<Overheal HP/MP/TP> allows the battler to be healed above its maximum for that parameter.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<Swap param1 with param2> swaps the values of two base parameters (mhp, mmp, atk, def, mat, mdf, agi, luk) for the battler.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<Dissolve State: x> removes the current state if the battler becomes affected by state x, and prevents state x from being applied while this state is active.

        # Examples
        <Dissolve State: 4, 5, 6> lists multiple dissolving state IDs.)"
        ),
        QT_TR_NOOP(
            BATTLE_EFFECTS_PACK_2_NOTETAGS
            R"(<Set State Counter: x> / <Add State Counter: x> sets or adds to this state's displayed counter value. Requires YEP_BuffsStatesCore.js.)"
        ),

        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Display Text: x> / <Display Icon: x> overrides the skill/item's displayed action text or icon during battle.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<speed: +x> / <speed: -x> adjusts this skill/item's action speed beyond the database editor's limits.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Cast Animation: x> sets the animation played while this skill is being cast. x = 0 disables the cast animation.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Reflect Animation ID: x> changes the animation played when this battler reflects a skill.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Sprite Cannot Move> prevents this battler's sideview sprite from moving during actions.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Anchor X: y.z> / <Anchor Y: y.z> sets the sideview sprite's anchor point along that axis.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Attack Animation: x> sets this enemy's default basic-attack animation to ID x.)"
        ),
        QT_TR_NOOP(
            BATTLE_ENGINE_CORE_NOTETAGS
            R"(<Action Start: x> / <Turn Start: x> automatically removes state x at the start of the next action or turn, respectively.

        # Examples
        <Action Start: 4 to 6> removes any state within the ID range 4-6.)"
        ),

        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Breathing> / <No Breathing> enables or disables the enemy's idle breathing effect.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS R"(<Breathing Speed: x> sets the breathing cycle length in frames.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Breathing Rate X: x.y> / <Breathing Rate Y: x.y> sets the breathing effect's horizontal/vertical scale variance.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Enable HP Link Breathing> / <Disable HP Link Breathing> ties breathing speed to the enemy's current HP percentage.)"
        ),
        QT_TR_NOOP(ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS R"(<Floating> enables a floating idle animation for the enemy.)"),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS R"(<Floating Speed: x> sets the floating cycle length in frames.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Floating Rate: x.y> sets the floating effect's vertical scale variance.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Floating Height: x> sets the minimum height in pixels the enemy floats above its base position.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Floating Death> / <No Floating Death> controls whether the enemy keeps floating after being defeated.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Scale Sprite: x%> scales the enemy's sideview sprite uniformly by x%.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Scale Sprite Width: x%> / <Scale Sprite Height: x%> scales the sprite along a single axis.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Battler: filename> sets the sideview actor-style graphic used for this enemy, from img/sv_actors/.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Anchor X: y.z> / <Sideview Anchor Y: y.z> sets the sideview battler's anchor point.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Width: x> / <Sideview Height: x> overrides the sideview battler's frame dimensions.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Collapse> / <Sideview No Collapse> controls whether the enemy's sprite disappears when defeated.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Frame Speed: x> sets the sideview battler's animation frame update speed.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Show State Overlay> / <Sideview Hide State Overlay> controls whether state icons overlay the sideview sprite.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Attack Motion: motion> sets the motion used for this enemy's basic attack.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Weapon: x> sets the weapon graphic ID used by the enemy's sprite. Optionally specify motion and animation: <Sideview Weapon: x, motion, animationId>.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview [Idle/Damage/Evade/Escape/Guard/Abnormal/Sleep/Dying/Dead] Motion: motion> sets the sprite motion used for that battler state.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Show Shadow> / <Sideview Hide Shadow> controls the sprite's shadow visibility.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Sideview Shadow Width: x%> / <Sideview Shadow Height: x%> scales the shadow graphic.)"
        ),
        QT_TR_NOOP(
            ANIMATED_SIDEVIEW_ENEMIES_NOTETAGS
            R"(<Hide Sideview Weapon> hides the weapon sprite effect for animated enemies affected by this state.)"
        ),

        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<ATB Help> text </ATB Help> displays alternate help text for this skill/item while the ATB battle system is active.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_ATB_NOTETAGS R"(Closing tag for <ATB Help>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<ATB Speed: x> / <ATB Charge: x> / <ATB Gauge: x> sets or adjusts the target's ATB speed/charge gauge during the corresponding phase.

        # Examples
        <ATB Speed: 50%>, <ATB Charge: +20>, <ATB Gauge: -10%>)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<After ATB: x> / <After ATB: x%> sets the user's ATB speed value immediately after using this skill/item.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<ATB Interrupt> / <ATB Interrupt: x%> allows this skill/item to interrupt the target's queued action, optionally with x% success chance.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<Cannot ATB Interrupt> prevents this skill/item's queued action from being interrupted.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<Target ATB Eval> speed = x; charge = x; </Target ATB Eval> runs JavaScript to conditionally set the target's ATB speed/charge gauge.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_ATB_NOTETAGS R"(Closing tag for <Target ATB Eval>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<After ATB Eval> speed = x; </After ATB Eval> runs JavaScript to set the user's ATB speed after the action resolves.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_ATB_NOTETAGS R"(Closing tag for <After ATB Eval>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<ATB Interrupt Eval> interrupt = true; </ATB Interrupt Eval> runs JavaScript to determine whether this action interrupts the target.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_ATB_NOTETAGS R"(Closing tag for <ATB Interrupt Eval>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_ATB_NOTETAGS
            R"(<ATB Start: +x> / <ATB Turn: +x> grants a partial ATB gauge at battle start / at the start of each turn, respectively.)"
        ),

        QT_TR_NOOP(
            VISUAL_ATB_GAUGE_NOTETAGS
            R"(<Show ATB Gauge> / <Hide ATB Gauge> forces the ATB gauge to display or hide for this enemy, overriding the plugin's default setting.)"
        ),
        QT_TR_NOOP(
            VISUAL_ATB_GAUGE_NOTETAGS
            R"(<ATB Gauge Width: x> sets a custom pixel width for this enemy's ATB gauge instead of using the battler graphic's width.)"
        ),

        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<CTB Icon: x> sets the icon displayed for this actor/enemy in the CTB Turn Order display.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<CTB Border Color: x> / <CTB Background Color: x> sets a text color (color ID x) for the border/background of this battler's CTB icon.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<Class x CTB Icon: y> sets an actor's CTB Turn Order icon to y whenever they belong to class ID x.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<ClassName CTB Icon: x> sets an actor's CTB Turn Order icon to x whenever they belong to the class named ClassName.

        # Examples
        <Mage CTB Icon: 79>)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<CTB Help> text </CTB Help> displays alternate help text for this skill/item while the CTB battle system is active.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_CTB_NOTETAGS R"(Closing tag for <CTB Help>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<CTB Speed: x> sets, and <CTB Speed: +x>/<CTB Speed: -x> adjusts, the target's current CTB speed. A % suffix applies the value as a percentage of the CTB turn-completion target.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<CTB Order: +x> / <CTB Order: -x> shifts the target's position in the CTB turn order; positive values delay their turn.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<After CTB: x> / <After CTB: x%> sets the user's CTB speed immediately after using this skill/item.)"
        ),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<Target CTB Speed Eval> speed = x; charge = x; </Target CTB Speed Eval> runs JavaScript to dynamically set the target's CTB speed/charge.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_CTB_NOTETAGS R"(Closing tag for <Target CTB Speed Eval>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<Target CTB Order Eval> order = x; </Target CTB Order Eval> runs JavaScript to dynamically alter the target's turn order.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_CTB_NOTETAGS R"(Closing tag for <Target CTB Order Eval>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<After CTB Eval> speed = x; </After CTB Eval> runs JavaScript to set the user's CTB speed after the action resolves.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_CTB_NOTETAGS R"(Closing tag for <After CTB Eval>.)"),
        QT_TR_NOOP(
            BATTLE_SYSTEM_CTB_NOTETAGS
            R"(<CTB Start: +x> / <CTB Turn: +x> grants bonus CTB speed at battle start / at the start of each turn, respectively.)"
        ),

        QT_TR_NOOP(
            ORDER_TURN_BATTLE_NOTETAGS
            R"(<OTB User Next Turn: +x> / <OTB User Next Turn: -x> shifts the user's position in next turn's OTB order. Applies once per use regardless of hit count.)"
        ),
        QT_TR_NOOP(
            ORDER_TURN_BATTLE_NOTETAGS
            R"(<OTB Target Current/Next/Follow Turn: +x/-x> shifts the target's position in the current, next, or follow-up turn order. "Follow" applies to whichever turn the target hasn't acted in yet. Applies per hit.)"
        ),
        QT_TR_NOOP(
            ORDER_TURN_BATTLE_NOTETAGS
            R"(<OTB User Add Current/Next Turn Actions: x> / <OTB Target Add Current/Next Turn Actions: x> grants x extra actions to the user (once per use) or target (once per hit) for the current or next turn.)"
        ),

        QT_TR_NOOP(
            BATTLE_SYSTEM_STB_NOTETAGS
            R"(<STB Help> text </STB Help> displays alternate help text for this skill/item while the STB battle system is active.)"
        ),
        QT_TR_NOOP(BATTLE_SYSTEM_STB_NOTETAGS R"(Closing tag for <STB Help>.)"),

        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Default Counter: x> sets the actor/enemy's default counterattack skill, by ID or name.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Counter Skills: x> adds skill(s) to the battler's counter pool, by ID list, ID range ("x to y"), or <Counter Skill: name> by name.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Counter Total: +x> / <Counter Total: -x> increases or decreases the battler's total counterattack frequency.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Target Counter: x%> multiplies, and <Target Counter: +x%>/<Target Counter: -x%> additively adjusts, this battler's counter rate when targeted.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Evade Counter> / <Hit Counter> sets whether the battler's counter skills trigger on evading or on being hit by an action.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Custom Counter Skills> ... </Custom Counter Skills> runs JavaScript to dynamically determine the battler's counter skill pool.)"
        ),
        QT_TR_NOOP(COUNTER_CONTROL_NOTETAGS R"(Closing tag for <Custom Counter Skills>.)"),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Custom Counter Total> ... </Custom Counter Total> runs JavaScript to dynamically calculate the battler's counter total.)"
        ),
        QT_TR_NOOP(COUNTER_CONTROL_NOTETAGS R"(Closing tag for <Custom Counter Total>.)"),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Custom Target Counter Rate> ... </Custom Target Counter Rate> runs JavaScript to modify counter rate based on the attacker.)"
        ),
        QT_TR_NOOP(COUNTER_CONTROL_NOTETAGS R"(Closing tag for <Custom Target Counter Rate>.)"),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Ally Counter> permits allied battlers to trigger counters on behalf of the target; <Ally Cannot Counter> prevents it.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS R"(<Cannot Counter> makes this skill/item's action impossible to counter.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Counter Rate: x%> multiplies, and <Counter Rate: +x%>/<Counter Rate: -x%> additively adjusts, the target's chance to counter this action.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Custom Counter Rate> ... </Custom Counter Rate> runs JavaScript to dynamically modify this skill/item's counter rate.)"
        ),
        QT_TR_NOOP(COUNTER_CONTROL_NOTETAGS R"(Closing tag for <Custom Counter Rate>.)"),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Counter Name: text> changes the displayed name when this skill triggers as a counterattack.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Counter Icon: x> changes the displayed icon when this skill triggers as a counterattack.)"
        ),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Counter Condition> ... </Counter Condition> establishes additional requirements this skill must meet to trigger as a counter.)"
        ),
        QT_TR_NOOP(COUNTER_CONTROL_NOTETAGS R"(Closing tag for <Counter Condition>.)"),
        QT_TR_NOOP(
            COUNTER_CONTROL_NOTETAGS
            R"(<Custom Counter Condition> ... </Custom Counter Condition> runs JavaScript for a fully custom counter-triggering condition.)"
        ),
        QT_TR_NOOP(COUNTER_CONTROL_NOTETAGS R"(Closing tag for <Custom Counter Condition>.)"),

        QT_TR_NOOP(
            IN_BATTLE_STATUS_NOTETAGS
            R"(<Help Description> text </Help Description> sets the help text shown for this state in the In-Battle Status window. Supports text codes.)"
        ),
        QT_TR_NOOP(IN_BATTLE_STATUS_NOTETAGS R"(Closing tag for <Help Description>.)"),

        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<Begin Action: Animation x> / <End Action: Animation x> plays animation x on the user at the start/end of the action.

        # Arguments
        - x: animation ID.
        - Mirror (optional): mirrors the animation.
        - Delay y (optional): delays playback by y frames.

        # Examples
        - <Begin Action: Animation 5>
        - <End Action: Animation 5, Mirror, Delay 6>)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<Begin Action: +x HP> / <Begin Action: -x HP> / <Begin Action: +x HP%> / <Begin Action: -x HP%> (also with End Action) applies a flat or percentile HP change to the user at the start/end of the action.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<End Action: Drain x% Total HP Damage> / <End Action: Recoil x% Total HP Damage> recovers (Drain) or self-inflicts (Recoil) x% of all HP damage the user dealt this action, as HP. End actions only.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<Begin Action: +x MP> / <Begin Action: -x MP> / <Begin Action: +x MP%> / <Begin Action: -x MP%> (also with End Action) applies a flat or percentile MP change to the user at the start/end of the action.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<End Action: Drain x% Total MP Damage> / <End Action: Recoil x% Total MP Damage> recovers (Drain) or self-inflicts (Recoil) x% of all MP damage the user dealt this action, as MP. End actions only.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<Begin Action: +x TP> / <Begin Action: -x TP> / <Begin Action: +x TP%> / <Begin Action: -x TP%> (also with End Action) applies a flat or percentile TP change to the user at the start/end of the action.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<Begin Action: Add x Buff> / <Begin Action: Add x Debuff> / <Begin Action: Remove x Buff> / <Begin Action: Remove x Debuff> (also with End Action) adds or removes a buff/debuff on the user for parameter x.

        # Arguments
        - x: parameter index (MaxHP, MaxMP, ATK, DEF, MAT, MDF, AGI, LUK).
        - Turns y (optional, add only): duration in turns; defaults to 5 if omitted.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_ACTION_BEGINNING_AND_END_EFFECTS_NOTETAGS
            R"(<Begin Action: Add State x> / <Begin Action: Remove State x> (also with End Action) adds or removes state x on the user at the start/end of the action.)"
        ),

        QT_TR_NOOP(
            TURN_ORDER_DISPLAY_NOTETAGS
            R"(<Turn Order Icon: x> sets the icon index used to represent this actor/enemy in the Turn Order Display.)"
        ),
        QT_TR_NOOP(
            TURN_ORDER_DISPLAY_NOTETAGS
            R"(<Turn Order Border Color: x> / <Turn Order Background Color: x> sets the border/background color of the Turn Order Display icon to text color x.)"
        ),
        QT_TR_NOOP(
            TURN_ORDER_DISPLAY_NOTETAGS
            R"(<Class x Turn Order Icon: y> gives the actor icon y in the Turn Order Display while their current class is class ID x.)"
        ),
        QT_TR_NOOP(
            TURN_ORDER_DISPLAY_NOTETAGS
            R"(<Hero Turn Order Icon: x> / <Warrior Turn Order Icon: x> / <Mage Turn Order Icon: x> / <Priest Turn Order Icon: x> gives the actor icon x in the Turn Order Display while their current class name matches the tag. If multiple classes share that name, the highest class ID takes priority.)"
        ),

        QT_TR_NOOP(
            VISUAL_HP_GAUGES_NOTETAGS
            R"(<Hide HP Gauge> / <Show HP Gauge> forces the battler's HP gauge to always be hidden, or always shown while selected/taking damage.)"
        ),
        QT_TR_NOOP(
            VISUAL_HP_GAUGES_NOTETAGS
            R"(<HP Gauge Width: x> / <HP Gauge Height: x> sets the battler's HP gauge width/height to x pixels.)"
        ),
        QT_TR_NOOP(VISUAL_HP_GAUGES_NOTETAGS R"(<HP Gauge Back Color: x> / <HP Gauge Color 1: x> / <HP Gauge Color 2: x> sets the HP gauge's back color or gradient color 1/2 to text color x.)"),

        QT_TR_NOOP(
            WEAK_ENEMY_POSES_NOTETAGS
            R"(<x% Health Pose: filename> / <x% Health Pose: filename, hue> sets the static enemy's battler image to filename (with optional hue, 0-360) once its HP falls to x% or lower.)"
        ),
        QT_TR_NOOP(
            WEAK_ENEMY_POSES_NOTETAGS
            R"(<Force Enemy Pose: filename> / <Force Enemy Pose: filename, hue> forces the static enemy's battler image to filename (with optional hue, 0-360) at highest priority while afflicted by this state.)"
        ),
        QT_TR_NOOP(
            WEAK_ENEMY_POSES_NOTETAGS
            R"(<Custom Enemy Pose> determines the static enemy's pose battler image (and hue) via custom JavaScript code, using the name and hue variables.)"
        ),

        QT_TR_NOOP(
            BATTLE_IMPACT_NOTETAGS
            R"(<Ignore Icon Effect> prevents this state/buff's icon from playing the flashy state/buff icon effect in the notebox.)"
        ),

        QT_TR_NOOP(
            BATTLE_SELECT_CURSOR_NOTETAGS
            R"(<Battle Select Cursor: filename> changes the cursor image used for this actor/enemy when selected in battle.)"
        ),
        QT_TR_NOOP(
            BATTLE_SELECT_CURSOR_NOTETAGS
            R"(<Battle Select Cursor Anchor X: Left/Center/Right> sets the horizontal origin point of the cursor sprite.)"
        ),
        QT_TR_NOOP(
            BATTLE_SELECT_CURSOR_NOTETAGS
            R"(<Battle Select Cursor Anchor Y: Top/Middle/Bottom> sets the vertical origin point of the cursor sprite.)"
        ),
        QT_TR_NOOP(
            BATTLE_SELECT_CURSOR_NOTETAGS
            R"(<Battle Select Cursor Position X: Left/Center/Right> sets where the cursor appears horizontally on the actor/enemy when targeted.)"
        ),
        QT_TR_NOOP(
            BATTLE_SELECT_CURSOR_NOTETAGS
            R"(<Battle Select Cursor Position Y: Top/Middle/Bottom> sets where the cursor appears vertically on the actor/enemy when targeted.)"
        ),

        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<Require x BP> requires at least x BP to use this skill/item. Optionally prefix x with a comparator (<Require > x BP>, <Require >= x BP>, <Require = x BP>, <Require <= x BP>, <Require < x BP>) to change the comparison.)"
        ),
        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<Target BP: +x> / <Target BP: -x> / <User BP: +x> / <User BP: -x> changes the target's or user's BP by x when this skill/item is used.)"
        ),
        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<Boost Damage> / <Boost Turns> / <Boost Repeats> / <Boost Analyze> / <Boost BP Effect> boosts this action's damage multiplier, state/buff turns, repeated hit count, revealed weaknesses, or BP effect numbers respectively, scaling with the BP spent by the user.)"
        ),
        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<BP Battle Start: x%> / <BP Battle Start: +x> / <BP Battle Start: -x> changes the battler's starting BP by a percentage or flat amount.)"
        ),
        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<BP Regen: x%> / <BP Regen: +x> / <BP Regen: -x> changes the battler's BP regeneration by a percentage or flat amount.)"
        ),
        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<Boost Skill x: Full> / <Boost skillname: Full> / <Boost Skill x: At Least y> / <Boost Skill x: At Most y> (and name variants) controls how much BP the enemy uses when it uses the named/numbered skill: as much as possible, only once BP reaches y, or capped at y.)"
        ),
        QT_TR_NOOP(
            BOOST_POINT_SYSTEM_NOTETAGS
            R"(<Boost Sealed> prevents the battler from boosting (spending BP) while affected by this state.)"
        ),

        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Bloom Scale: x> / <Bloom Brightness: x> / <Bloom Threshold: x> overrides the bloom scale/brightness/threshold for this map or troop. Lower values mean less bloom/darker/less picky; higher values mean more bloom/brighter/more picky.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Bloom Horz Scale: x to y> / <Bloom Vert Scale: x to y> (also Brightness, Threshold) sets a bloom value that interpolates from x to y as the player moves left-to-right (Horz) or top-to-bottom (Vert) across the map. Map only.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Godray> / <No Godray> forces godrays on or off for this map/troop, overriding the plugin's default setting.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Godray Speed: x> / <Godray Gain: x> / <Godray Lacunarity: x> sets the flicker speed / gain (intensity) / lacunarity (density) of the godrays for this map/troop.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Godray Angle: x> sets the angle of the godrays. Negative values come from the left, positive values come from the right.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Godray Horz Speed: x to y> / <Godray Vert Speed: x to y> (also Gain, Lacunarity, Angle) sets a godray value that interpolates from x to y across the map, left-to-right (Horz) or top-to-bottom (Vert). Map only.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Color Adjust Brightness: x> / <Color Adjust Contrast: x> / <Color Adjust Saturate: x> overrides the screen brightness/contrast/saturation for this map/troop.)"
        ),
        QT_TR_NOOP(
            BRIGHT_EFFECTS_NOTETAGS
            R"(<Color Adjust Horz Brightness: x to y> / <Color Adjust Vert Brightness: x to y> (also Contrast, Saturate) sets a screen color-adjust value that interpolates from x to y across the map, left-to-right (Horz) or top-to-bottom (Vert). Map only.)"
        ),

        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Max stat Buff: +x> / <Max stat Buff: -x> / <Max stat Debuff: +x> / <Max stat Debuff: -x> changes the maximum buff/debuff stack limit for parameter stat by x. stat is one of MaxHP, MaxMP, ATK, DEF, MAT, MDF, AGI, LUK.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<stat Buff Turns: +x> / <stat Buff Turns: -x> / <stat Debuff Turns: +x> / <stat Debuff Turns: -x> adds or subtracts x turns to/from an existing buff/debuff on the target for parameter stat.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<State x Turns: +y> / <State x Turns: -y> (also by state name) adds or subtracts y turns to/from state x when applied by this skill/item.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Show Turns> / <Hide Turns> forces the state's remaining turn count to display or stay hidden.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Turn Font Size: x> sets the font size used for this state's turn counter.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Turn Alignment: Left/Center/Right> sets the text alignment of the state's turn counter.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Turn Buffer X: +x> / <Turn Buffer X: -x> / <Turn Buffer Y: +x> / <Turn Buffer Y: -x> offsets the turn counter's display position.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Turn Color: x> sets the turn counter's text color to text color index x.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Reapply Ignore Turns> / <Reapply Reset Turns> / <Reapply Add Turns> controls how the state's remaining turns are affected when it is reapplied: left unchanged, reset to default, or increased.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Show State Turns> / <Hide State Turns> forces this enemy's afflicted state turn counts to show or hide in the help window.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Custom stat Buff Turn> / <Custom stat Debuff Turn> runs custom JavaScript code to determine the buff/debuff turn count for parameter stat.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Custom State x Turn> (also by state name) runs custom JavaScript code to determine the turn count applied for state x.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Custom Apply Effect> / <Custom Remove Effect> / <Custom Leave Effect> / <Custom Turn Start Effect> / <Custom Action Start Effect> / <Custom Action End Effect> / <Custom Regenerate Effect> / <Custom Turn End Effect> / <Custom Battle Effect> / <Custom Victory Effect> / <Custom Escape Effect> / <Custom Defeat Effect> runs custom JavaScript code on the corresponding state lifecycle timing (added, removed, expired naturally, turn/action start or end, regeneration, battle start while passive, victory, escape, or defeat).)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Custom Initiate Effect> / <Custom Select Effect> / <Custom Confirm Effect> / <Custom React Effect> / <Custom Respond Effect> / <Custom Establish Effect> / <Custom Deselect Effect> / <Custom Conclude Effect> runs custom JavaScript code at the corresponding stage of an action sequence for a battler afflicted by this state.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Counter Font Size: x> sets the font size used for this state's counter display.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Counter Alignment: left/center/right> sets the text alignment of the state's counter display.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Counter Buffer X: +x> / <Counter Buffer X: -x> / <Counter Buffer Y: +y> / <Counter Buffer Y: -y> offsets the counter display's position.)"
        ),
        QT_TR_NOOP(
            BUFFS_AND_STATES_CORE_NOTETAGS
            R"(<Counter Text Color: x> sets the counter display's text color to text color index x.)"
        ),

        QT_TR_NOOP(
            EXTENDED_DAMAGE_OVER_TIME_NOTETAGS
            R"(<Regen Animation: x> / <DoT Animation: x> plays animation x when this state's regen/damage-over-time formula yields a non-zero result.)"
        ),
        QT_TR_NOOP(
            EXTENDED_DAMAGE_OVER_TIME_NOTETAGS
            R"(<Regen Formula: x> / <DoT Formula: x> makes the affected battler regenerate/lose x HP each turn, where x is a formula or numeric value.)"
        ),
        QT_TR_NOOP(
            EXTENDED_DAMAGE_OVER_TIME_NOTETAGS
            R"(<Regen Element: x> / <DoT Element: x> makes this state's regen healing/damage-over-time damage consider the target's elemental rate for element x.)"
        ),
        QT_TR_NOOP(
            EXTENDED_DAMAGE_OVER_TIME_NOTETAGS
            R"(<Regen Variance: x%> / <DoT Variance: x%> sets the variance percentage for this state's regen/damage-over-time effect, overriding the plugin default.)"
        ),
        QT_TR_NOOP(
            EXTENDED_DAMAGE_OVER_TIME_NOTETAGS
            R"(<Custom DoT Formula> / <Custom Regen Formula> runs custom JavaScript code to calculate this state's damage-over-time/regen value, with access to value, variance, and element variables.)"
        ),

        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<Protection Animation: x> replaces the default protection-trigger animation with animation x.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Damage Cut x%> reduces incoming HP/MP damage by x% of its original value.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Damage Block +x> / <...: Damage Block -x> adds or subtracts a flat x amount from incoming damage.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Damage Null x%> / Damage Barrier x% nullifies damage that is at or below x% of the target's max HP/MP. <...: Damage Ceiling x%> caps damage at x%, <...: Damage Floor x%> sets a minimum of x%.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP Protection: Guts x%> gives an x% chance to survive fatal damage with 1 HP (does not trigger while already at 1 HP). <HP Protection: True Guts x%> is the same but also triggers at 1 HP.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP Protection: Fatal Damage Absorb x%> gives an x% chance to convert what would be fatal damage into healing instead.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Trigger Removal x%> gives an x% chance to remove the protection state whenever its effect triggers.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Trigger Add x Buff> / <...: Trigger Add x Debuff> applies a buff/debuff to parameter x when the protection triggers; append ", y Turns" to set its duration.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Trigger Remove x Buff> / <...: Trigger Remove x Debuff> removes a buff/debuff from parameter x when the protection triggers.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Trigger Add State x> applies state x when the protection effect triggers.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_STATE_PROTECTION_NOTETAGS
            R"(<HP/MP/Both Protection: Trigger Remove State x> removes state x when the protection effect triggers.)"
        ),

        QT_TR_NOOP(
            STATE_CATEGORIES_NOTETAGS
            R"(<Category: text> assigns a custom category to the state; use multiple instances to assign several categories. The reserved category strings "Bypass Death Removal", "Bypass Recover All Removal", and "Group Defeat" have special meaning (skip removal on death/Recover All, or count toward a full-party defeat check).)"
        ),
        QT_TR_NOOP(
            STATE_CATEGORIES_NOTETAGS
            R"(<Remove State Category: text> removes all non-passive states of category "text" from the action's target. <Remove x State Category: text> removes only x of them, prioritizing highest-priority states first.)"
        ),
        QT_TR_NOOP(
            STATE_CATEGORIES_NOTETAGS
            R"(<Custom Remove State Category: text> opening tag of a Lunatic Mode block computing how many "text"-category states to remove via the 'value' variable, seeded from a preceding <Remove x State Category> tag or 1 by default.)"
        ),
        QT_TR_NOOP(
            STATE_CATEGORIES_NOTETAGS
            R"(</Custom Remove State Category: text> closing tag of the <Custom Remove State Category> block.)"
        ),

        QT_TR_NOOP(
            VISUAL_STATE_EFFECTS_NOTETAGS
            R"(<State Motion: motion> gives the battler a custom sprite motion while affected by this state (e.g. Walk, Chant, Dying, Sleep). If multiple active states set a motion, the one on the highest-priority state wins.)"
        ),
        QT_TR_NOOP(
            VISUAL_STATE_EFFECTS_NOTETAGS
            R"(<State Animation: x> plays animation x on repeat over the battler while this state is active. If multiple active states set an animation, the one on the highest-priority state wins.)"
        ),

        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Art: foldername, filename> or <Card Art: foldername, filename, hue> sets the card's artwork from an img/ subfolder, with an optional 0-360 hue shift.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Dragonbones: armature> uses a DragonBones armature as the card's art instead of a static image.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Dragonbones Animation: name> sets which DragonBones animation plays for the card's armature.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Art Offset: +x, +y> offsets the card art from center; x and y are independently signed.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS R"(<Card Art Scale: x> scales the card art, e.g. 1.0 for 100%, 0.5 for 50%.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Art Cells: ColxRow> declares the card art as a sprite sheet with the given column x row grid.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Cell Art Cell Index: x> selects cell x (0-indexed) of the sprite sheet declared by <Card Art Cells>.)"
        ),
        QT_TR_NOOP(CARD_GAME_CORE_NOTETAGS R"(<Card Name: text> sets the name text displayed on the card.)"),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Element: name> associates the card with an element, which determines its default template.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Level: x> / <Card Power: x> / <Card Health: x> set the card's numeric level, power, and health stats respectively.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS R"(<Card Template: name> overrides the card's default template with a custom one.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Rarity: x> sets the card's rarity level, which affects its aura and which booster packs can pull it.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Parallax BG: filename> / <Card Parallax FG: filename> adds a scrolling parallax background/foreground behind/in front of the card art, loaded from img/parallaxes/.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Parallax BG Speed: +x, +y> / <Card Parallax FG Speed: +x, +y> sets the parallax layer's scroll speed; x and y are independently signed.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Parallax BG Opacity: x> / <Card Parallax FG Opacity: x> sets the parallax layer's opacity, 0-255.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Holo BG: filename> / <Card Holo FG: filename> adds a holographic background/foreground layer behind/in front of the card art, loaded from img/pictures/.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Holo BG Speed: x> / <Card Holo FG Speed: x> controls the holographic layer's hue-shift speed.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Holo BG Opacity: x> / <Card Holo FG Opacity: x> sets the holographic layer's opacity, 0-255.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Parallax BG/Holo BG/Parallax FG/Holo FG Blend Mode: Normal|Additive|Multiply|Screen> sets that layer's blend mode.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Text> opening tag for the multi-line text block displayed on the card, up to 4 lines.)"
        ),
        QT_TR_NOOP(CARD_GAME_CORE_NOTETAGS R"(</Card Text> closing tag for the card's text block.)"),
        QT_TR_NOOP(CARD_GAME_CORE_NOTETAGS R"(<Card Text Scale: x> scales the size of the card's text block.)"),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Not a Card> excludes this item from the card pool even if it would otherwise be treated as a card.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Booster Art: filename> sets the booster pack's own artwork, loaded from img/pictures/.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Booster BG: filename> sets the background shown during the booster-opening scene, loaded from img/parallaxes/.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Booster Rarities: x> or <Booster Rarities: x, x, x> defines which rarity level(s) this booster pack can pull cards from.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_CORE_NOTETAGS
            R"(<Card Pool: x> / <Card Pool: x, x, x> / <Card Pool: x to y> lists (or ranges) the item IDs eligible to be pulled from this booster pack.)"
        ),

        QT_TR_NOOP(CARD_GAME_DECK_BUILDER_NOTETAGS R"(<Deck Box: filename> / <Deck Sleeves: filename> unlocks a deck box / card sleeves cosmetic (from img/pictures/, case-sensitive) for the player as long as this item is in their inventory.)"),

        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Hand/Discard/Exhaust/Library Start/Upkeep/Draw/End JS> opening tag; runs JavaScript when the card is in the given zone during the given phase.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(</Card Hand/Discard/Exhaust/Library Start/Upkeep/Draw/End JS> closing tag of the corresponding phase JS block.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Hand/Discard/Exhaust/Library Start/Upkeep/Draw/End Common Event: x> runs common event x when the card is in the given zone during the given phase.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card No Battle> prevents this card from being usable to battle another card.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Spell x JS> opening tag; runs JavaScript when the card's spell slot x (1-4) is used.)"
        ),
        QT_TR_NOOP(CARD_GAME_MECHANICS_NOTETAGS R"(</Card Spell x JS> closing tag of the spell slot JS block.)"),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Spell x Common Event: y> runs common event y when the card's spell slot x is used.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Spell x Enable> opening tag of a JavaScript block that determines whether spell slot x is currently usable.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS R"(</Card Spell x Enable> closing tag of the spell slot enable block.)"
        ),
        QT_TR_NOOP(CARD_GAME_MECHANICS_NOTETAGS R"(<Card Spell x Icon: y> sets the icon shown for spell slot x.)"),
        QT_TR_NOOP(CARD_GAME_MECHANICS_NOTETAGS R"(<Card Spell x Name: text> sets the display name for spell slot x.)"),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS R"(<Card Spell x Animation: y> plays animation y when spell slot x is used.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Spell x Allow AI Use> allows AI-controlled opponents to use spell slot x.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Spell x Check AI Use> opening tag of a JavaScript block deciding whether the AI should use spell slot x.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS R"(</Card Spell x Check AI Use> closing tag of the spell slot AI-check block.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card AI Priority-behavior: x> sets the AI's selection priority value x for the named behavior when picking cards to play.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Pre-Battle/Tie Battle/Lose Battle/Win Battle/Post-Battle/Resolve Battle JS> opening tag; runs JavaScript at the corresponding stage of a card-vs-card battle.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(</Card Pre-Battle/Tie Battle/Lose Battle/Win Battle/Post-Battle/Resolve Battle JS> closing tag of the corresponding battle-stage JS block.)"
        ),
        QT_TR_NOOP(
            CARD_GAME_MECHANICS_NOTETAGS
            R"(<Card Pre-Battle/Tie Battle/Lose Battle/Win Battle/Post-Battle/Resolve Battle Common Event: x> runs common event x at the corresponding stage of a card-vs-card battle.)"
        ),

        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Unlock Class: x> / <Unlock Class: x, x, x> / <Unlock Class: x to y> unlocks the listed/ranged class(es) for the actor from the start of the game.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Cannot Change Class> prevents the actor from changing their primary class (reversible via plugin commands).)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Class x Character: filename y> (or the class-name variant, e.g. <Hero Character: filename y>) swaps the actor's map sprite to 'filename' at index y while they have class x.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Class x Face: filename y> (or the class-name variant) swaps the actor's menu face graphic while they have class x.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Class x Battler: filename> (or the class-name variant) swaps the actor's battler sprite while they have class x.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS R"(<Icon: x> sets the icon shown for this class in the Class Change menu.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Use Nickname> displays the actor's nickname instead of the class name in class listings.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Help Description> opening tag for custom multi-line help text describing the class.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS R"(</Help Description> closing tag of the class's help description block.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(<Level Unlock Requirements> opening tag listing "Class x: Level y" (or class-name: Level y) lines that must all be met to unlock this class.)"
        ),
        QT_TR_NOOP(
            CLASS_CHANGE_CORE_NOTETAGS
            R"(</Level Unlock Requirements> closing tag of the level unlock requirements block.)"
        ),

        QT_TR_NOOP(SUBCLASS_NOTETAGS R"(<Subclass: x> sets the actor's default subclass to class ID x.)"),
        QT_TR_NOOP(
            SUBCLASS_NOTETAGS
            R"(<Cannot Change Subclass> prevents the actor from changing subclasses (overridable via plugin commands).)"
        ),
        QT_TR_NOOP(
            SUBCLASS_NOTETAGS
            R"(<Restrict Class: x> / <Restrict Subclass: x> (also accepting a list "x, x, x" or a range "x to y") forbids the actor from switching their primary class / subclass to the listed class ID(s).)"
        ),
        QT_TR_NOOP(
            SUBCLASS_NOTETAGS
            R"(<Primary Only> restricts this class to being used only as a primary class, never a subclass.)"
        ),
        QT_TR_NOOP(
            SUBCLASS_NOTETAGS
            R"(<Subclass Only> restricts this class to being used only as a subclass, never a primary class.)"
        ),
        QT_TR_NOOP(
            SUBCLASS_NOTETAGS
            R"(<Subclass x Combo Name: text> (or the class-name variant, e.g. <Hero Combo Name: text>) sets a custom display name shown when this class is primary and the actor's subclass is class x.)"
        ),
        QT_TR_NOOP(
            SUBCLASS_NOTETAGS
            R"(<Require Class: x> / <Require Subclass: x> (also accepting a list or a range "x to y") restricts this skill/item to only be usable by actors with the given primary class / subclass.)"
        ),

        QT_TR_NOOP(
            COUNTER_STATE_NOTETAGS
            R"(<Counter State x: n%> gives an n% chance to inflict state x on the attacker when countering any attack. <Physical Counter State x: n%> / <Magical Counter State x: n%> / <Certain Hit Counter State x: n%> restrict the trigger to that specific hit type.)"
        ),

        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS
            R"(<Price: x> sets the item/weapon/armor's price to x, bypassing the default 999,999 gold cap.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS R"(<Max Item: x> sets the maximum inventory count for this item/weapon/armor to x.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS R"(<Gold: x> sets the enemy's gold drop to x, bypassing the default 9,999,999 cap.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS R"(<hp/mp/atk/def/mat/mdf/agi/luk: x> sets the enemy's base value for that stat to x.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS R"(<exp: x> sets the enemy's EXP reward to x, exceeding the default 9,999,999 cap.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS
            R"(<Initial Level: x> / <Max Level: x> set the actor's starting/maximum level to x, bypassing the level 99 cap.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS
            R"(<Learn at Level: x>, placed inside a class's "Skills to Learn" notebox, makes the class learn that skill at level x.)"
        ),
        QT_TR_NOOP(
            CORE_ENGINE_NOTETAGS
            R"(<hp/mp/atk/def/mat/mdf/agi/luk: +x> / <...: -x> grants the weapon/armor a flat bonus or penalty to that stat.)"
        ),

        QT_TR_NOOP(
            DAMAGE_CORE_NOTETAGS
            R"(<Bypass Damage Cap> - the skill/item or battler ignores damage/heal cap effects, using the raw uncapped calculated value instead. Takes priority over any cap-breaking effects.)"
        ),
        QT_TR_NOOP(
            DAMAGE_CORE_NOTETAGS
            R"(<damage formula> - opens a Lunatic Mode block replacing the skill/item's damage formula.

        # Arguments
        - value: the base calculated damage
        - user/subject: the attacker
        - target: the recipient)"
        ),
        QT_TR_NOOP(DAMAGE_CORE_NOTETAGS R"(</damage formula> - closes a <damage formula> Lunatic Mode block.)"),
        QT_TR_NOOP(
            DAMAGE_CORE_NOTETAGS
            R"(<Damage Cap: x> - sets the battler's damage cap to x. When multiple sources apply, the highest cap value wins.)"
        ),
        QT_TR_NOOP(
            DAMAGE_CORE_NOTETAGS
            R"(<Heal Cap: x> - sets the battler's healing cap to x. When multiple sources apply, the highest cap value wins.)"
        ),

        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS R"(<Armor Reduction: x> - reduces the target's armor by a flat x, calculated first.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Armor Reduction: x%> - reduces the target's armor by x%, calculated second (skipped if armor already below 0).)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Armor Penetration: x%> - reduces the target's armor by x% without letting it go below 0, calculated third.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Armor Penetration: x> - reduces the target's armor by a flat x without letting it go below 0, calculated last.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS R"(<Bypass Armor Scaling> - the skill/item skips the entire armor scaling process.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Physical/Magical/Certain Armor Reduction: x> - flat armor reduction applied to this battler's physical/magical/certain-hit damage taken.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Physical/Magical/Certain Armor Reduction: x%> - percentage armor reduction applied to this battler's physical/magical/certain-hit damage taken.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Physical/Magical/Certain Armor Penetration: x%> - percentage armor penetration applied when this battler deals physical/magical/certain-hit damage.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Physical/Magical/Certain Armor Penetration: x> - flat armor penetration applied when this battler deals physical/magical/certain-hit damage.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(<Positive Armor Rate>, <Negative Armor Rate>, or <Base Armor> - opens a Lunatic Mode block with a custom JavaScript formula for the respective armor calculation.)"
        ),
        QT_TR_NOOP(
            ARMOR_SCALING_NOTETAGS
            R"(</Positive Armor Rate>, </Negative Armor Rate>, or </Base Armor> - closes the corresponding Lunatic Mode block.)"
        ),

        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Critical Rate: x%> - sets this skill/item's critical hit rate to x%, disregarding user/target bonuses, and forces critical hits to be possible.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Critical Rate: x.y> - sets this skill/item's critical hit rate using a decimal multiplier (x.y), disregarding other modifiers.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Critical Multiplier: x%> - sets this skill/item's critical damage multiplier to x%, still factoring in the user's multiplier bonuses.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Critical Multiplier: x.y> - sets this skill/item's critical damage multiplier using a decimal value (x.y), still factoring in the user's bonuses.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Flat Critical: x% stat> - adds a flat critical rate bonus equal to x% of the user's specified stat (hp, mp, atk, def, mat, mdf, agi, or luk).)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Critical Multiplier: +x%>/<Critical Multiplier: -x%> - additively adjusts this battler's critical damage multiplier.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Flat Critical: +x>/<Flat Critical: -x> - additively adjusts this battler's flat critical damage bonus.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Certain Hit Critical Rate: +x%>/<Certain Hit Critical Rate: -x%> - additively adjusts this battler's critical rate for certain-hit skills/items.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Physical Critical Rate: +x%>/<Physical Critical Rate: -x%> - additively adjusts this battler's critical rate for physical skills/items.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Magical Critical Rate: +x%>/<Magical Critical Rate: -x%> - additively adjusts this battler's critical rate for magical skills/items.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(<Critical Rate Formula>, <Critical Multiplier Formula>, or <Flat Critical Formula> - opens a Lunatic Mode block with a custom JavaScript formula for the respective critical calculation.)"
        ),
        QT_TR_NOOP(
            CRITICAL_CONTROL_NOTETAGS
            R"(</Critical Rate Formula>, </Critical Multiplier Formula>, or </Flat Critical Formula> - closes the corresponding Lunatic Mode block.)"
        ),

        QT_TR_NOOP(
            LUNATIC_PACK_CRITICAL_SWAY_NOTETAGS
            R"(<Custom Critical Rate: x Pride y%>/<Custom Critical Rate: x Crisis y%> - modifies critical hit rate by y% based on the user's current HP/MP/TP rate for param x (Pride = higher when the rate is high, Crisis = higher when the rate is low). Append ", nonstackable" to prevent stacking with similar effects.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_CRITICAL_SWAY_NOTETAGS
            R"(<Custom Critical Rate: x Hero y%>/<Custom Critical Rate: x Bully y%> - modifies critical hit rate by y% based on the target's current HP/MP/TP rate for param x. Append ", nonstackable" to prevent stacking with similar effects.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_CRITICAL_SWAY_NOTETAGS
            R"(<Custom Critical Rate: Element Rate x%> - modifies critical hit rate by x% based on the target's damage rate to the action's element. Append ", nonstackable" to prevent stacking with similar effects.)"
        ),
        QT_TR_NOOP(
            LUNATIC_PACK_CRITICAL_SWAY_NOTETAGS
            R"(<Custom Critical Rate: User/Target States/Buffs/Debuffs +x%> - modifies critical hit rate by ±x% per matching state/buff/debuff on the user or target. Append ", nonstackable" to prevent stacking with similar effects.)"
        ),

        QT_TR_NOOP(
            DASH_TOGGLE_NOTETAGS
            R"(<Disable Dashing> - when the party's leading actor has a trait carrying this notetag, the player cannot dash while that actor leads the party.)"
        ),

        QT_TR_NOOP(
            DISABLE_AUTO_SHADOW_EXTENDED_NOTETAGS
            R"(<Hide Shadows>/<Show Shadows> - placed in a map's or tileset's note field to force-disable/enable event shadows there. Map notetags take priority over tileset notetags; if neither is present, shadow visibility follows the plugin parameter default.)"
        ),

        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS
            R"(<DragonBone: name>/<DragonBone Battler: name> - assigns a DragonBones skeletal animation asset to this actor/enemy, matched against name_ske.json/name_tex.json/name_tex.png. Case-sensitive.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS
            R"(<DragonBone ScaleX: n>/<DragonBone ScaleY: n> - overrides the horizontal/vertical scale of the skeletal battler (values below 1 shrink it, above 1 enlarge it, negative values mirror it).)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS
            R"(<DragonBone Width: x>/<DragonBone Height: x> - sets the collision/click detection box (in pixels) for the skeletal battler, since armature assets don't carry inherent dimensions.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS
            R"(<DragonBone Keep Sprite>/<DragonBone Replace Sprite> - keeps the original battler sprite visible alongside the skeletal animation, or hides it in favor of the skeletal animation.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS
            R"(<DragonBone Ani motion: animation> - maps a standard battler motion (attack, walk, thrust, escape, wait, swing, victory, chant, missile, dying, guard, skill, abnormal, damage, evade, spell, sleep, dead, item) to a case-sensitive custom skeletal animation name.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS
            R"(<DragonBone Settings> - opens a consolidated block for specifying the battler name, scale, dimensions, sprite behavior, and animation mappings in one place.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_INTEGRATION_NOTETAGS R"(</DragonBone Settings> - closes a <DragonBone Settings> block.)"
        ),

        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite: filename> - replaces this actor's/event's default map sprite with the named DragonBones armature (matched against _ske.json/_tex.json/_tex.png).)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite Scale: x, y>/<Dragonbones Sprite Scale X: x>/<Dragonbones Sprite Scale Y: y> - sets the armature's base scale; negative values flip it.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite Time Scale: x> - multiplies the armature's animation playback speed (1.0 = default, higher = faster, lower = slower).)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite Size: width, height>/<Dragonbones Sprite Width: x>/<Dragonbones Sprite Height: x> - sets pixel dimensions for the armature sprite, used for collision and plugin compatibility since armatures carry no inherent size.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite Flip Left>/<Dragonbones Sprite Flip Right>/<Dragonbones Sprite No Flip Left>/<Dragonbones Sprite No Flip Right> - enables/disables automatic mirroring of the armature when facing left/right.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite Motion Idle/Walk/Dash/Jump/LadderIdle/LadderClimb: animation> - maps the given movement state to a case-sensitive custom armature animation name.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(<Dragonbones Sprite Settings> - opens a consolidated block for specifying the filename, scale, time scale, dimensions, flip behavior, and motion animations in one place.)"
        ),
        QT_TR_NOOP(
            DRAGONBONES_MAP_SPRITES_NOTETAGS
            R"(</Dragonbones Sprite Settings> - closes a <Dragonbones Sprite Settings> block.)"
        ),

        QT_TR_NOOP(
            ELEMENT_CORE_NOTETAGS
            R"(<Bypass Element Reflect> - this skill/item ignores elemental reflection properties (other reflect effects still apply).)"
        ),
        QT_TR_NOOP(
            ELEMENT_CORE_NOTETAGS
            R"(<Multiple Elements: x>/<Multiple Elements: x to y>/<Multiple Elements: x, x, x>/<Multiple Elements: name, name, name> - adds extra elements (by ID or name) to this skill/item beyond its base element; damage is combined per the Multi-Element Rule.)"
        ),
        QT_TR_NOOP(
            ELEMENT_CORE_NOTETAGS
            R"(<Multi-Element Rule: Lowest/Add/Multiply/Highest/Average> - determines how the elemental damage rate is computed when a skill/item has multiple elements.)"
        ),
        QT_TR_NOOP(
            ELEMENT_CORE_NOTETAGS
            R"(<Element Absorb: x>/<Element Absorb: name> - the designated element(s) heal this battler instead of damaging it (rate reduced by 200%, floor 0.01%; more resistant battlers absorb more, more vulnerable ones absorb less).)"
        ),
        QT_TR_NOOP(
            ELEMENT_CORE_NOTETAGS
            R"(<Element Reflect/Magnify/Amplify x: +y%>/<... -y%> - for element x (by ID or name): Reflect adjusts reflection chance, Magnify adjusts damage multiplicatively, Amplify adjusts damage additively, when this battler performs skills/items of that element.)"
        ),
        QT_TR_NOOP(
            ELEMENT_CORE_NOTETAGS
            R"(<Element Null> - this battler's skills/items never carry an elemental attribute (does not bypass a Force Element action sequence).)"
        ),
        QT_TR_NOOP(ELEMENT_CORE_NOTETAGS R"(<Force Element x Rate: y%>/<Force Element name Rate: y%> - forces this battler's elemental rate for element x to equal y% (negative values cause absorption). Priority: states (highest to lowest) > equipment (first to last) > class > actor/enemy.)"),

        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Show Level>/<Hide Level> - controls whether this enemy displays its level during target selection.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Minimum Level: x> - sets the lowest possible level this enemy can have at battle start.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Maximum Level: x> - sets the highest possible level this enemy can have at battle start.)"
        ),
        QT_TR_NOOP(ENEMY_LEVELS_NOTETAGS R"(<Static Level: x> - fixes this enemy's starting level to exactly x.)"),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Starting Level Type: x> - selects the calculation used for this enemy's starting level.

        # Arguments
        - 0: lowest level among all joined actors
        - 1: lowest level among active battle members
        - 2: average level of all joined actors
        - 3: average level of active battle members
        - 4: highest level among all joined actors
        - 5: highest level among active battle members)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Positive Level Fluctuation: x> - sets the maximum random level increase applied at battle start.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Negative Level Fluctuation: x> - sets the maximum random level decrease applied at battle start.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Level Fluctuation: x> - shorthand for setting both positive and negative level fluctuation to the same range x.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<stat Rate: +x% per level>/<stat Rate: -x% per level> - modifies the given stat (maxhp, maxmp, atk, def, mat, mdf, agi, luk, exp, or gold) by ±x% for each level above 1.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<stat Flat: +x per level>/<stat Flat: -x per level> - adds a flat ±x to the given stat (maxhp, maxmp, atk, def, mat, mdf, agi, luk, exp, or gold) for each level above 1.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS R"(<Resist Level Change> - prevents any skill/item from altering this enemy's level.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Skill x Require Level: y>/<Skill name Require Level: y> - this enemy cannot use the given skill until it reaches level y.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Ignore Level Bonus> - this enemy keeps its base stats regardless of level changes.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Custom Starting Level> - opens a Lunatic Mode block with custom JavaScript logic for this enemy's starting level.)"
        ),
        QT_TR_NOOP(ENEMY_LEVELS_NOTETAGS R"(</Custom Starting Level> - closes a <Custom Starting Level> block.)"),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Custom Parameter stat Formula> - opens a Lunatic Mode block with a custom JavaScript formula for how the given stat (maxhp, maxmp, atk, def, mat, mdf, agi, luk) scales by level.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(</Custom Parameter stat Formula> - closes a <Custom Parameter stat Formula> block.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS R"(<Reset Enemy Level> - restores the target enemy's level to its pre-battle state.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Change Enemy Level: +x>/<Change Enemy Level: -x> - modifies the target enemy's level by ±x.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS
            R"(<Custom Change Enemy Level> - opens a Lunatic Mode block with custom JavaScript logic for modifying the target enemy's level.)"
        ),
        QT_TR_NOOP(
            ENEMY_LEVELS_NOTETAGS R"(</Custom Change Enemy Level> - closes a <Custom Change Enemy Level> block.)"
        ),

        QT_TR_NOOP(
            DIFFICULTY_SLIDER_NOTETAGS
            R"(<Unaffected by Difficulty Slider> - This enemy's level is not affected by the difficulty slider setting and always uses its base calculated level.)"
        ),

        QT_TR_NOOP(
            ENEMY_BASE_PARAMETERS_NOTETAGS
            R"(<Base Parameters on Class: x> - Bases this enemy's parameters (except EXP and Gold) on class x, taking priority over custom enemy parameters.

        # Arguments
        - x: the ID of the class to base parameters on)"
        ),
        QT_TR_NOOP(
            ENEMY_BASE_PARAMETERS_NOTETAGS
            R"(<Base stat Parameter on Class: x> - Bases this enemy's specific parameter on class x, overriding custom parameters for that stat only.

        # Arguments
        - stat: maxhp, maxmp, atk, def, mat, mdf, agi, or luk
        - x: the ID of the class to base the parameter on)"
        ),
        QT_TR_NOOP(
            ENEMY_BASE_PARAMETERS_NOTETAGS
            R"(<Custom Enemy Parameters> - Opening tag for a block of formulas that calculate this enemy's maxhp, maxmp, atk, def, mat, mdf, agi, luk, exp, and gold using the 'level' variable. Closed by </Custom Enemy Parameters>.)"
        ),
        QT_TR_NOOP(
            ENEMY_BASE_PARAMETERS_NOTETAGS
            R"(</Custom Enemy Parameters> - Closing tag for the custom enemy parameters formula block.)"
        ),
        QT_TR_NOOP(
            ENEMY_BASE_PARAMETERS_NOTETAGS
            R"(<Custom Base stat Formula> - Opening tag for a JavaScript formula block that calculates the base value for a single parameter using the 'value' and 'level' variables. Closed by the matching </Custom Base stat Formula>.

        # Arguments
        - stat: maxhp, maxmp, atk, def, mat, mdf, agi, luk, exp, or gold)"
        ),
        QT_TR_NOOP(
            ENEMY_BASE_PARAMETERS_NOTETAGS
            R"(</Custom Base stat Formula> - Closing tag for the custom base parameter formula block.)"
        ),

        QT_TR_NOOP(
            MAP_ENEMY_LEVELS_NOTETAGS
            R"(<Enemy Level Base: x> - Sets a static base level for all enemies encountered on this map. Further fluctuations from the Enemy Levels plugin are applied afterward.

        # Arguments
        - x: the base level)"
        ),
        QT_TR_NOOP(
            MAP_ENEMY_LEVELS_NOTETAGS
            R"(<Enemy Level Base Variable: x> - Sets the base level for enemies on this map to the value of variable x. Further fluctuations are applied afterward.

        # Arguments
        - x: the variable ID holding the base level)"
        ),
        QT_TR_NOOP(
            MAP_ENEMY_LEVELS_NOTETAGS
            R"(<Enemy Level Range: x to y> - Sets the minimum and maximum base level for enemies on this map. Further fluctuations are applied afterward.

        # Arguments
        - x: minimum base level
        - y: maximum base level)"
        ),
        QT_TR_NOOP(
            MAP_ENEMY_LEVELS_NOTETAGS
            R"(<Enemy Level Range Variables: x to y> - Sets the minimum and maximum base level for enemies on this map using two variables. Further fluctuations are applied afterward.

        # Arguments
        - x: variable ID holding the minimum base level
        - y: variable ID holding the maximum base level)"
        ),

        QT_TR_NOOP(
            ENHANCED_TP_NOTETAGS
            R"(<TP Mode: x> - Sets this actor's default TP mode to mode x. If unused, the actor uses the plugin parameter default.

        # Arguments
        - x: the TP mode ID)"
        ),
        QT_TR_NOOP(
            ENHANCED_TP_NOTETAGS
            R"(<Unlock TP Mode: x> - Makes TP mode(s) x available for this actor in addition to its default unlocked modes. Accepts a single ID, a comma-separated list, or an "x to y" range.

        # Arguments
        - x: TP mode ID, list of IDs, or a range "x to y")"
        ),
        QT_TR_NOOP(
            ENHANCED_TP_NOTETAGS
            R"(<Unlock TP Mode: x> - When this skill or item is used, unlocks TP mode(s) x for the target, making them selectable in the TP menu. Accepts a single ID, a comma-separated list, or an "x to y" range.

        # Arguments
        - x: TP mode ID, list of IDs, or a range "x to y")"
        ),
        QT_TR_NOOP(
            ENHANCED_TP_NOTETAGS
            R"(<Learn Unlock TP Mode: x> - Upon learning this skill, the actor automatically unlocks TP mode(s) x. Accepts a single ID, a comma-separated list, or an "x to y" range.

        # Arguments
        - x: TP mode ID, list of IDs, or a range "x to y")"
        ),

        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS
            R"(<Starting Skill Slots: x> - Sets this actor's initial number of equippable battle skill slots, respecting the maximum limit.

        # Arguments
        - x: the number of starting slots)"
        ),
        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS
            R"(<Equip stat: +x> - Increases or decreases the actor's stat by x while this skill is equipped for battle.

        # Arguments
        - stat: hp, mp, atk, def, mat, mdf, agi, or luk
        - x: the amount to change the stat by (use +x or -x))"
        ),
        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS
            R"(<Equip State: x> - Applies state(s) x to the actor while this skill is equipped for battle. Accepts a single ID, a comma-separated list, or an "x through x" range.

        # Arguments
        - x: state ID, list of IDs, or a range "x through x")"
        ),
        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS R"(<Unequippable> - This skill can never be equipped as a battle skill.)"
        ),
        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS
            R"(<All Access Equippable> - This skill can be equipped by any actor regardless of whether they have the required skill type.)"
        ),
        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS
            R"(<Access Only Equippable> - This skill can only be equipped by actors who meet its skill type requirement.)"
        ),
        QT_TR_NOOP(
            EQUIP_BATTLE_SKILLS_NOTETAGS
            R"(<Equip Skill Slots: +x> - Adds or subtracts x equippable battle skill slots for the actor, respecting the maximum limit.

        # Arguments
        - x: the number of slots to add or subtract (use +x or -x))"
        ),

        QT_TR_NOOP(
            EQUIP_SKILL_TIERS_NOTETAGS
            R"(<Skill Tier: x> - Assigns this skill to tier x, making it subject to the actor's tier-based equip slot restrictions.

        # Arguments
        - x: the tier ID)"
        ),
        QT_TR_NOOP(
            EQUIP_SKILL_TIERS_NOTETAGS
            R"(<Skill Tier x Slots: +y> - Increases or decreases the actor's available equip slots for tier x by y. The result never goes below 0 or exceeds the maximum battle skills equip limit.

        # Arguments
        - x: the tier ID
        - y: the number of slots to add or subtract (use +y or -y))"
        ),

        QT_TR_NOOP(
            EQUIP_CORE_NOTETAGS
            R"(<Equip Slot: x> - Changes this class's equipment slots to the listed equipment type IDs. Repeating an ID allows equipping multiple items of that type.

        # Arguments
        - x: a comma-separated list of equipment type IDs)"
        ),
        QT_TR_NOOP(
            EQUIP_CORE_NOTETAGS
            R"(<Equip Slot> - Opening tag for an alternative equip slot list using equipment type names (case-sensitive) instead of IDs, one per line. Closed by </Equip Slot>.)"
        ),
        QT_TR_NOOP(EQUIP_CORE_NOTETAGS R"(</Equip Slot> - Closing tag for the name-based equip slot list.)"),
        QT_TR_NOOP(EQUIP_CORE_NOTETAGS R"(<stat: +x> - Adds or subtracts x from this weapon or armor's stat.

        # Arguments
        - stat: hp, mp, atk, def, mat, mdf, agi, or luk
        - x: the amount to change the stat by (use +x or -x))"),
        QT_TR_NOOP(
            EQUIP_CORE_NOTETAGS
            R"(<Custom Parameters> - Opening tag for a JavaScript block that computes this weapon or armor's parameter bonuses (maxhp, maxmp, atk, def, mat, mdf, agi, luk, or all). Closed by </Custom Parameters>.)"
        ),
        QT_TR_NOOP(
            EQUIP_CORE_NOTETAGS R"(</Custom Parameters> - Closing tag for the custom parameters JavaScript block.)"
        ),

        QT_TR_NOOP(
            CHANGE_BATTLE_EQUIP_NOTETAGS
            R"(<Change Battle Equip Cooldown: +x> - Increases or decreases the number of turns before the actor can change equips again in battle.

        # Arguments
        - x: the number of turns to add or subtract (use +x or -x))"
        ),
        QT_TR_NOOP(
            CHANGE_BATTLE_EQUIP_NOTETAGS
            R"(<Disable Change Battle Equip> - Prevents the actor from changing equipment mid-battle.)"
        ),

        QT_TR_NOOP(
            EQUIP_REQUIREMENTS_NOTETAGS
            R"(<Equip Requirement> - Opening tag for a block of equip requirement conditions (parameter comparisons, class/skill/switch requirements, "unique only") that must be met to equip this weapon or armor. Closed by </Equip Requirement>.)"
        ),
        QT_TR_NOOP(
            EQUIP_REQUIREMENTS_NOTETAGS R"(</Equip Requirement> - Closing tag for the equip requirement block.)"
        ),
        QT_TR_NOOP(
            EQUIP_REQUIREMENTS_NOTETAGS
            R"(<Custom Equip Requirement Condition> - Opening tag for a JavaScript block that sets the 'condition' variable to true or false to permit or deny equipping. Closed by </Custom Equip Requirement Condition>.)"
        ),
        QT_TR_NOOP(
            EQUIP_REQUIREMENTS_NOTETAGS
            R"(</Custom Equip Requirement Condition> - Closing tag for the custom equip requirement condition block.)"
        ),
        QT_TR_NOOP(
            EQUIP_REQUIREMENTS_NOTETAGS
            R"(<Custom Equip Requirement Text> - Opening tag for a JavaScript block that sets the 'text' variable to custom text shown in the equip requirement window. Closed by </Custom Equip Requirement Text>.)"
        ),
        QT_TR_NOOP(
            EQUIP_REQUIREMENTS_NOTETAGS
            R"(</Custom Equip Requirement Text> - Closing tag for the custom equip requirement text block.)"
        ),

        QT_TR_NOOP(
            EQUIPMENT_SET_BONUSES_NOTETAGS
            R"(<Equip Set: name> - Assigns this weapon or armor to equipment set "name". Use multiple instances of this notetag if the item should belong to more than one set.

        # Arguments
        - name: the equipment set's identifying name)"
        ),
        QT_TR_NOOP(
            EQUIPMENT_SET_BONUSES_NOTETAGS
            R"(<name Set, x Pieces Character: filename, index> - Changes the actor's map character sprite when at least x pieces from set "name" are equipped.

        # Arguments
        - name: the equipment set's name
        - x: minimum number of equipped pieces required
        - filename: character sprite sheet filename (case-sensitive)
        - index: the sprite's index on the sheet, starting from 0)"
        ),
        QT_TR_NOOP(
            EQUIPMENT_SET_BONUSES_NOTETAGS
            R"(<name Set, x Pieces Face: filename, index> - Changes the actor's face graphic when at least x pieces from set "name" are equipped.

        # Arguments
        - name: the equipment set's name
        - x: minimum number of equipped pieces required
        - filename: face image filename (case-sensitive)
        - index: the face's index on the sheet, starting from 0)"
        ),
        QT_TR_NOOP(
            EQUIPMENT_SET_BONUSES_NOTETAGS
            R"(<name Set, x Pieces Battler: filename> - Changes the actor's sideview battler graphic when at least x pieces from set "name" are equipped.

        # Arguments
        - name: the equipment set's name
        - x: minimum number of equipped pieces required
        - filename: sideview battler image filename (case-sensitive))"
        ),

        QT_TR_NOOP(
            EVENT_COPIER_NOTETAGS
            R"(<Copy Event: Map x, Event y> / <Copy Event: mapId, eventId> / <Copy Event: template> copies all event data (except ID, X, Y) from another event.

        # Arguments
        - Map x, Event y / mapId, eventId: the source map ID and event ID to copy from.
        - template: a name from the 'Template Names' plugin parameter, used instead of raw IDs.)"
        ),

        QT_TR_NOOP(
            EVENT_ENCOUNTER_AID_NOTETAGS
            R"(<Encounter Lock> / <Encounter Direction Lock> prevents the event from immediately facing the player when approached, allowing directional-facing checks.)"
        ),
        QT_TR_NOOP(
            EVENT_ENCOUNTER_AID_NOTETAGS
            R"(<Follower Touch> / <Follower Trigger> allows the event to trigger when touched by a follower, not just the leading player character.)"
        ),

        QT_TR_NOOP(
            EXTRA_ENEMY_DROPS_NOTETAGS
            R"(<Item x: y%> / <Weapon x: y%> / <Armor x: y%> single-line drop notetag; enemy drops item/weapon/armor x with a y% chance.

        # Arguments
        - x: the item/weapon/armor's database ID.
        - y: drop chance percentage.)"
        ),
        QT_TR_NOOP(
            EXTRA_ENEMY_DROPS_NOTETAGS
            R"(<Enemy Drops> / </Enemy Drops> multi-line block wrapping several drop entries (by ID or by item name) for one enemy.

        # Examples
        <Enemy Drops>
         Item 5: 50%
         Weapon 3: 25%
        </Enemy Drops>)"
        ),
        QT_TR_NOOP(
            EXTRA_ENEMY_DROPS_NOTETAGS
            R"(<Drop name: x%> named single-line drop notetag; enemy drops the item/weapon/armor called 'name' with an x% chance, used instead of a database ID.)"
        ),
        QT_TR_NOOP(
            EXTRA_ENEMY_DROPS_NOTETAGS
            R"(<Conditional Item x Drop> / <Conditional Weapon x Drop> / <Conditional Armor x Drop> / <Conditional Named Drop> block notetag defining conditional chance modifiers for a drop, closed by the matching </Conditional ... Drop> tag.

        # Examples
        <Conditional Item 5 Drop>
         $gameSwitches.value(10): +25%
         $gameSwitches.value(20): -10%
        </Conditional Item 5 Drop>)"
        ),

        QT_TR_NOOP(
            EXTRA_PARAMETER_FORMULA_NOTETAGS
            R"(<stat Plus: +x%> / <stat Plus: -x%> / <stat Plus: +x.y> / <stat Plus: -x.y> value added to the base extra parameter before the rate and flat values apply.

        # Arguments
        - stat: one of hit, eva, cri, cev, mev, mrf, cnt, hrg, mrg, trg.)"
        ),
        QT_TR_NOOP(
            EXTRA_PARAMETER_FORMULA_NOTETAGS
            R"(<stat Rate: x%> / <stat Rate: x.y> value multiplied against the sum of the base and plus values of the extra parameter, before the flat value is added.)"
        ),
        QT_TR_NOOP(
            EXTRA_PARAMETER_FORMULA_NOTETAGS
            R"(<stat Flat: +x%> / <stat Flat: -x%> / <stat Flat: +x.y> / <stat Flat: -x.y> value added last, after the base/plus sum has been multiplied by the rate value.)"
        ),

        QT_TR_NOOP(
            FLOOR_DAMAGE_NOTETAGS
            R"(<Floor Damage x: y> tiles with terrain tag x marked as damage tiles inflict y damage on party members standing on them.

        # Arguments
        - x: terrain tag (0-7).
        - y: damage amount.)"
        ),
        QT_TR_NOOP(
            FLOOR_DAMAGE_NOTETAGS
            R"(<Floor Flash x: r, g, b, o> sets the screen flash color/opacity shown when a party member takes floor damage from terrain tag x.

        # Arguments
        - x: terrain tag.
        - r, g, b, o: red, green, blue, opacity, each 0-255.)"
        ),
        QT_TR_NOOP(
            FLOOR_DAMAGE_NOTETAGS
            R"(<Custom Floor Damage x> / </Custom Floor Damage x> Lunatic Mode block; runs JavaScript code to compute floor damage for terrain tag x, adding to the base <Floor Damage x: y> value.

        # Examples
        <Custom Floor Damage 2>
         value = actor.level;
        </Custom Floor Damage 2>)"
        ),

        QT_TR_NOOP(
            FOOTSTEP_SOUNDS_NOTETAGS
            R"(<No Footsteps> prevents this event from generating footstep sounds when it moves.)"
        ),
        QT_TR_NOOP(
            FOOTSTEP_SOUNDS_NOTETAGS
            R"(<Terrain Tag x Footstep Sound: filename> / <Terrain Tag x Footstep Sound: filename, volume> / <Terrain Tag x Footstep Sound: filename, volume, pitch> sets the footstep sound played while standing on tiles with terrain tag x.

        # Arguments
        - x: terrain tag ID (1-7; 0 is ignored).
        - filename: case-sensitive audio filename, no extension.
        - volume, pitch: integers 0-100.

        # Examples
        <Terrain Tag 1 Footstep Sound: Move1>
        <Terrain Tag 2 Footstep Sound: Move2, 80>
        <Terrain Tag 3 Footstep Sound: Move3, 75, 150>)"
        ),
        QT_TR_NOOP(
            FOOTSTEP_SOUNDS_NOTETAGS
            R"(<Region x Footstep Sound: filename> / <Region x Footstep Sound: filename, volume> / <Region x Footstep Sound: filename, volume, pitch> sets the footstep sound played while standing on tiles with region ID x.

        # Arguments
        - x: region ID (1-255; 0 uses the default footstep sound).
        - filename: case-sensitive audio filename, no extension.
        - volume, pitch: integers 0-100.

        # Examples
        <Region 10 Footstep Sound: Move1>
        <Region 20 Footstep Sound: Move2, 80>
        <Region 30 Footstep Sound: Move3, 75, 150>)"
        ),

        QT_TR_NOOP(
            HIT_DAMAGE_SOUNDS_NOTETAGS
            R"(<Hit Damage Sound: filename> plays 'filename' as a sound effect whenever this armor or enemy is struck by damage, with optional volume/pitch/pan overrides.

        # Arguments
        - filename: sound effect filename, case sensitive, no extension.
        - volume: optional, 0-100.
        - pitch: optional, 0-100.
        - pan: optional, -100 to 100.

        # Examples
        - <Hit Damage Sound: Sword3>
        - <Hit Damage Sound: Sword3, 90>
        - <Hit Damage Sound: Sword3, 90, 120>
        - <Hit Damage Sound: Sword3, 90, 120, 0>)"
        ),

        QT_TR_NOOP(
            INSTANT_CAST_NOTETAGS
            R"(<Instant> or <Instant Cast> forces this skill/item to be used instantly (no turn consumed) when selected first by an actor, or granted as a bonus action for enemies.)"
        ),
        QT_TR_NOOP(
            INSTANT_CAST_NOTETAGS
            R"(<Instant Eval> opening tag of a block of JavaScript code deciding instant-cast eligibility at run time; set the 'instant' variable to true/false inside.)"
        ),
        QT_TR_NOOP(INSTANT_CAST_NOTETAGS R"(</Instant Eval> closing tag of the <Instant Eval> code block.)"),
        QT_TR_NOOP(
            INSTANT_CAST_NOTETAGS
            R"(<Instant Skill: x> / <Instant Item: x> grants instant-cast properties to skill/item x. <Cancel Instant Skill: x> / <Cancel Instant Item: x> revoke it. Accepts a comma-separated list of IDs.

        # Arguments
        - x: skill or item ID, or a comma-separated list of IDs.

        # Examples
        - <Instant Skill: 5>
        - <Instant Item: 10, 12, 14>
        - <Cancel Instant Skill: 5>)"
        ),
        QT_TR_NOOP(
            INSTANT_CAST_NOTETAGS
            R"(<Instant Skill: x to y> / <Instant Item: x to y> grants instant-cast properties to all skills/items in the ID range x-y. <Cancel Instant Skill: x to y> / <Cancel Instant Item: x to y> revoke it for that range.)"
        ),

        QT_TR_NOOP(
            ITEM_CONCOCTIONS_NOTETAGS
            R"(<Item Concoct> on an actor/class/weapon/armor/state, enables the Concoct battle command for that actor. On an item, opens a block listing secondary components and the resulting item produced when combined with this item; each line is "component: result" using "Item x"/name for either side.

        # Examples
        - <Item Concoct>
          Item 51: Item 61
          Plum Essence: Inspiriting Mist
          </Item Concoct>)"
        ),
        QT_TR_NOOP(
            ITEM_CONCOCTIONS_NOTETAGS
            R"(</Item Concoct> closing tag of the item's <Item Concoct> component list block.)"
        ),
        QT_TR_NOOP(
            ITEM_CONCOCTIONS_NOTETAGS
            R"(<Item Concoct Seal> on a state, disables the Concoct battle command for any actor affected by it.)"
        ),

        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<Random Variance: x> if this independent item is acquired outside a shop, its stats are randomly offset by up to +/-x.)"
        ),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<Not Independent Item> forces an otherwise-independent item to behave as a regular stacking item, immune to independent-item modifiers.)"
        ),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<Priority Name> forces the item/weapon/armor to always display its database name, ignoring name-scheme changes from other plugins.)"
        ),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<Text Color: x> sets the windowskin text color index used to display this item/weapon/armor's name.)"
        ),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<On Creation Eval> opening tag of a JavaScript block run when an independent copy of this item is created; 'item' refers to the new copy, 'baseItem' to the database original.)"
        ),
        QT_TR_NOOP(ITEM_CORE_NOTETAGS R"(</On Creation Eval> closing tag of the <On Creation Eval> code block.)"),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<Info Text Top> / <Info Text Bottom> opening tag of extra lore/info text (text codes allowed) shown at the top or bottom of the item info window.)"
        ),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(</Info Text Top> / </Info Text Bottom> closing tag of the corresponding <Info Text Top>/<Info Text Bottom> block.)"
        ),
        QT_TR_NOOP(
            ITEM_CORE_NOTETAGS
            R"(<Info Eval> opening tag of a JavaScript block run before the item info window displays, useful for syncing displayed variable values.)"
        ),
        QT_TR_NOOP(ITEM_CORE_NOTETAGS R"(</Info Eval> closing tag of the <Info Eval> code block.)"),

        QT_TR_NOOP(
            ATTACHABLE_AUGMENTS_NOTETAGS
            R"(<Augment Slots> opening tag of a list of augment slot categories (e.g. Rune, Glyph, Orb, Mark) this weapon/armor can accept.)"
        ),
        QT_TR_NOOP(ATTACHABLE_AUGMENTS_NOTETAGS R"(</Augment Slots> closing tag of the <Augment Slots> list.)"),
        QT_TR_NOOP(
            ATTACHABLE_AUGMENTS_NOTETAGS
            R"(<No Augment Slots> removes all augment slot functionality from this weapon/armor, preventing any augment from being attached.)"
        ),
        QT_TR_NOOP(
            ATTACHABLE_AUGMENTS_NOTETAGS
            R"(<Augment: type> / <Augment Attach: type> / <Augment Detach: type> opening tag turning this item into an augment component for slot category 'type'. Plain <Augment: type> applies effects both attached and detached; Attach/Detach variants apply only on that transition.

        # Arguments
        - type: the augment slot category this item's effects apply to.)"
        ),
        QT_TR_NOOP(
            ATTACHABLE_AUGMENTS_NOTETAGS
            R"(</Augment: type> / </Augment Attach: type> / </Augment Detach: type> closing tag of the corresponding <Augment[...]: type> effect list block; must repeat the same 'type'.)"
        ),
        QT_TR_NOOP(ATTACHABLE_AUGMENTS_NOTETAGS R"(<Augment Attach Eval: type> / <Augment Detach Eval: type> opening tag of a JavaScript block run on attach/detach of this augment; 'item' is the equipment being modified, 'effectItem' the augment component.)"),
        QT_TR_NOOP(
            ATTACHABLE_AUGMENTS_NOTETAGS
            R"(</Augment Attach Eval: type> / </Augment Detach Eval: type> closing tag of the corresponding Eval block; must repeat the same 'type'.)"
        ),

        QT_TR_NOOP(
            ITEM_DISASSEMBLE_NOTETAGS
            R"(<Disassemble Pool> or <Disassemble Pool: type> opening tag of a list of items/weapons/armors dropped when this item is disassembled. Typed form only drops when the disassembler's type matches.

        # Examples
        - <Disassemble Pool>
          item 5: 50%
          x2-3 weapon 3
          </Disassemble Pool>)"
        ),
        QT_TR_NOOP(
            ITEM_DISASSEMBLE_NOTETAGS
            R"(</Disassemble Pool> or </Disassemble Pool: type> closing tag of the <Disassemble Pool> drop list.)"
        ),
        QT_TR_NOOP(
            ITEM_DISASSEMBLE_NOTETAGS
            R"(<Disassembler> marks this item as a disassembler tool. Optional colon form restricts it to specific pool types and/or a success-rate modifier.

        # Examples
        - <Disassembler>
        - <Disassembler: +25%>
        - <Disassembler: Metal>
        - <Disassembler: Metal -10%>)"
        ),
        QT_TR_NOOP(
            ITEM_DISASSEMBLE_NOTETAGS
            R"(<Disassemble Sound Name/Volume/Pitch/Pan: x> overrides the sound effect (name/volume/pitch/pan) played when this item is disassembled.)"
        ),
        QT_TR_NOOP(
            ITEM_DISASSEMBLE_NOTETAGS
            R"(<Custom Disassembled Effect> / <Custom Disassembler Effect> opening tag of a JavaScript block for custom effects when this item is disassembled / used as a disassembler; uses 'results', 'targetItem', 'effectItem' variables.)"
        ),
        QT_TR_NOOP(
            ITEM_DISASSEMBLE_NOTETAGS
            R"(</Custom Disassembled Effect> / </Custom Disassembler Effect> closing tag of the corresponding custom effect block.)"
        ),

        QT_TR_NOOP(
            ITEM_DISCARD_NOTETAGS
            R"(<Can Discard> / <Cannot Discard> forces this item/weapon/armor to be discardable or non-discardable, overriding the plugin's default discard setting.)"
        ),

        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Durability: x> / <Durability Variance: x> / <Durability Maximum: x> sets this weapon/armor's starting durability, its +/-x random starting variance, or its maximum repairable durability.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Bypass Durability> makes this independent weapon/armor bypass the durability system entirely.)"
        ),
        QT_TR_NOOP(ITEM_DURABILITY_NOTETAGS R"(<Unbreakable> makes this independent weapon/armor unbreakable.)"),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Break Sound Name: filename> overrides the sound effect played when this weapon/armor breaks in battle (case sensitive, no extension).)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Break Sound Volume: x> / <Break Sound Pitch: x> overrides the volume or pitch of the break sound effect.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Break Sound Pan: +x> / <Break Sound Pan: -x> overrides the pan of the break sound effect.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Repair Durability: x> / <Repair Weapon: x> / <Repair Armor: x> on a repair item, restores x durability to any weapon/armor, or to weapons only, or to armors only.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Repair WType x: y> / <Repair AType x: y> on a repair item, restores y durability specifically to weapon-type x / armor-type x.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Repair Sound Name: filename> overrides the sound effect played when this item repairs equipment.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Repair Sound Volume: x> / <Repair Sound Pitch: x> overrides the volume or pitch of the repair sound effect.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Repair Sound Pan: +x> / <Repair Sound Pan: -x> overrides the pan of the repair sound effect.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Unbreakable Durability> / <Unbreakable Weapon> / <Unbreakable Armor> on a repair item, removes durability tracking (makes unbreakable) for any/weapon/armor equipment it's used on.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Unbreakable WType x> / <Unbreakable AType x> on a repair item, removes durability tracking for weapon-type x / armor-type x equipment.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<User Weapon Durability: +x/-x> and its All/Random variants (<User All Weapon Durability>, <User Random Weapon Durability>), plus the Target-side equivalents - each hit alters the user's/target's weapon durability by x; the weapon breaks if durability reaches 0.

        # Arguments
        - x: signed durability change, e.g. +5 or -5.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<User Armor Durability: +x/-x> and its All/Random variants (<User All Armor Durability>, <User Random Armor Durability>), plus the Target-side equivalents - each hit alters the user's/target's armor durability by x; the armor breaks if durability reaches 0.

        # Arguments
        - x: signed durability change, e.g. +5 or -5.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Custom Break Effect> / <Custom Repair Effect> opening tag of a JavaScript block run when equipment breaks / when a repair item is used; 'item'/'newItem'/'targetItem' refer to the relevant equipment.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(</Custom Break Effect> / </Custom Repair Effect> closing tag of the corresponding custom effect block.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(<Custom User All Weapon Durability> and its User/Target x All/Random x Weapon/Armor variants opening tag of a JavaScript block computing a custom durability change via the 'value' variable; negative reduces, positive increases, 0-or-below breaks the equipment.)"
        ),
        QT_TR_NOOP(
            ITEM_DURABILITY_NOTETAGS
            R"(</Custom User All Weapon Durability> and its User/Target x All/Random x Weapon/Armor variants closing tag of the corresponding custom durability block.)"
        ),

        QT_TR_NOOP(
            ITEM_MENU_CATEGORIES_NOTETAGS
            R"(<Menu Category: x> assigns this item/weapon/armor to a custom item-menu category named x, referenced elsewhere via the plugin's Category:x parameter syntax. Multiple tags on one item add it to multiple categories; a comma-separated list assigns several categories in one tag. Does not apply to the built-in Items/Weapons/Armors categories.

        # Examples
        - <Menu Category: Herbs>
        - <Menu Category: Herbs, Potions, Rare>)"
        ),

        QT_TR_NOOP(
            ITEM_PICTURE_IMAGES_NOTETAGS
            R"(<Picture: filename> sets the item/weapon/armor's picture to filename (no extension, case sensitive), defaulting its hue to 0.)"
        ),
        QT_TR_NOOP(
            ITEM_PICTURE_IMAGES_NOTETAGS
            R"(<Picture Image: filename> sets the item/weapon/armor's picture to filename (no extension, case sensitive), without altering hue.)"
        ),
        QT_TR_NOOP(ITEM_PICTURE_IMAGES_NOTETAGS R"(<Picture Hue: x> sets the picture image's hue to x (0-360).)"),

        QT_TR_NOOP(
            ITEM_RENAME_NOTETAGS
            R"(<Can Rename> / <Cannot Rename> force this item/weapon/armor to be (or not be) renameable, overriding the Default Rename plugin parameter.)"
        ),

        QT_TR_NOOP(
            ITEM_REQUIREMENTS_NOTETAGS
            R"(<Enable Requirements> opens a block of conditions (one per line, e.g. Actor: x, Switch On: x, Eval: code) that must all be met for this item/weapon/armor to be usable.)"
        ),
        QT_TR_NOOP(
            ITEM_REQUIREMENTS_NOTETAGS R"(</Enable Requirements> closes an <Enable Requirements> condition block.)"
        ),
        QT_TR_NOOP(
            ITEM_REQUIREMENTS_NOTETAGS
            R"(<Custom Enable Requirement> opens a Lunatic Mode block where a 'condition' variable (with access to 'battler' and 'item') determines whether the item is enabled.)"
        ),
        QT_TR_NOOP(
            ITEM_REQUIREMENTS_NOTETAGS R"(</Custom Enable Requirement> closes a <Custom Enable Requirement> block.)"
        ),

        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Upgrade Slots: x> sets the number of times this weapon/armor can be upgraded, overriding the default.)"
        ),
        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Slot Variance: x> gives a random variance to the number of upgrade slots this weapon/armor provides, overriding the parameter setting.)"
        ),
        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Upgrade Sound: filename> changes the sound effect played on upgrade to filename, overriding the Default Sound parameter.)"
        ),
        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Upgrade Effect> opens a block listing the upgrade effects applied, in order, to this weapon/armor.)"
        ),
        QT_TR_NOOP(ITEM_UPGRADE_SLOTS_NOTETAGS R"(</Upgrade Effect> closes an <Upgrade Effect> block.)"),
        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Upgrade Item Type: type> restricts this item to upgrading only items matching type, which can be a fixed occasion keyword (All, Regular, Key, Hidden A, Hidden B, Always, Battle, Menu, Never) or a custom item type string.)"
        ),
        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Type: string> (items only) assigns a custom type label of string to the item, shown in its notebox.)"
        ),
        QT_TR_NOOP(
            ITEM_UPGRADE_SLOTS_NOTETAGS
            R"(<Upgrade Weapon Type: x> / <Upgrade Armor Type: x> restricts this item to upgrading only the listed weapon/armor type(s). Accepts a single id, a comma list, or an "x through x" range; type 0 allows all weapons/armor.)"
        ),

        QT_TR_NOOP(
            ITEM_SYNTHESIS_NOTETAGS
            R"(<Item Recipe: x> / <Weapon Recipe: x> / <Armor Recipe: x> turns this item into a recipe that synthesizes the listed item/weapon/armor id(s) once the party has the required ingredients. Accepts a single id, a comma list, or an "x to y" range.)"
        ),
        QT_TR_NOOP(
            ITEM_SYNTHESIS_NOTETAGS
            R"(<Synthesis Ingredients> opens a block listing the ingredients (items, weapons, armors, gold, or named entries, optionally with ": quantity") required to synthesize this recipe.)"
        ),
        QT_TR_NOOP(ITEM_SYNTHESIS_NOTETAGS R"(</Synthesis Ingredients> closes a <Synthesis Ingredients> block.)"),
        QT_TR_NOOP(
            ITEM_SYNTHESIS_NOTETAGS
            R"(<Mask Name: x> changes the displayed name of this item to x while its identity is masked as unknown.)"
        ),
        QT_TR_NOOP(
            ITEM_SYNTHESIS_NOTETAGS
            R"(<Custom Synthesis Effect> opens a Lunatic Mode block of code that runs when this item is synthesized; the 'item' variable refers to the item being synthesized.)"
        ),
        QT_TR_NOOP(ITEM_SYNTHESIS_NOTETAGS R"(</Custom Synthesis Effect> closes a <Custom Synthesis Effect> block.)"),

        QT_TR_NOOP(
            JOB_POINTS_NOTETAGS R"(<Starting JP: x> sets the actor's starting JP for their initial class to x.)"
        ),
        QT_TR_NOOP(JOB_POINTS_NOTETAGS R"(<Class x Starting JP: y> sets the actor's starting JP for class x to y.)"),
        QT_TR_NOOP(
            JOB_POINTS_NOTETAGS
            R"(<JP Rate: x%> changes the rate of JP gained by x% (default 100%); stacks across actor, class, weapon, armor, and state sources.)"
        ),
        QT_TR_NOOP(
            JOB_POINTS_NOTETAGS
            R"(<JP Gain: x> makes the actor using this skill/item gain x JP, overriding the default JP gain from the parameters.)"
        ),
        QT_TR_NOOP(
            JOB_POINTS_NOTETAGS R"(<Target JP Gain: x> makes the target actor affected by this skill/item gain x JP.)"
        ),
        QT_TR_NOOP(JOB_POINTS_NOTETAGS R"(<JP: x> makes each party member gain x JP when this enemy is defeated.)"),

        QT_TR_NOOP(
            LEVEL_UP_GROWTH_EFFECTS_NOTETAGS
            R"(<Level Up stat Growth: +x> / <Level Up stat Growth: -x> raises or lowers a stat on level up.

        # Arguments
        - stat: MaxHP, MaxMP, ATK, DEF, MAT, MDF, AGI, or LUK.
        - x: amount of growth to apply.)"
        ),
        QT_TR_NOOP(
            LEVEL_UP_GROWTH_EFFECTS_NOTETAGS
            R"(<Level Up Learn Skill: x> teaches skill x to the actor upon leveling up.)"
        ),
        QT_TR_NOOP(
            LEVEL_UP_GROWTH_EFFECTS_NOTETAGS
            R"(<Level Up Switch On: x> / <Level Up Switch Off: x> turns the listed switch(es) on/off upon leveling up. Accepts a single id, a comma list, or an "x to y" range; insert multiple notetags to affect multiple groups.)"
        ),
        QT_TR_NOOP(
            LEVEL_UP_GROWTH_EFFECTS_NOTETAGS
            R"(<Level Up Recover All> triggers the Recover All effect on the actor upon leveling up.)"
        ),
        QT_TR_NOOP(
            LEVEL_UP_GROWTH_EFFECTS_NOTETAGS
            R"(<Custom Level Up Effect> opens a Lunatic Mode block of code that runs upon leveling up.)"
        ),
        QT_TR_NOOP(
            LEVEL_UP_GROWTH_EFFECTS_NOTETAGS R"(</Custom Level Up Effect> closes a <Custom Level Up Effect> block.)"
        ),

        QT_TR_NOOP(
            LIFE_STEAL_NOTETAGS
            R"(<HP Life Steal: x%> / <MP Life Steal: x%> makes this action life steal x% of the HP or MP damage dealt.)"
        ),
        QT_TR_NOOP(
            LIFE_STEAL_NOTETAGS
            R"(<HP Life Steal: x> / <MP Life Steal: x> makes this action life steal exactly x HP or MP, regardless of damage dealt.)"
        ),
        QT_TR_NOOP(
            LIFE_STEAL_NOTETAGS
            R"(<Cancel Life Steal> / <Cancel HP Life Steal> / <Cancel MP Life Steal> either stops this skill/item from triggering passive Life Steal effects, or (on an actor/class/enemy/weapon/armor/state) stops the battler from passively life stealing HP and/or MP.)"
        ),
        QT_TR_NOOP(
            LIFE_STEAL_NOTETAGS
            R"(<HP Life Steal Physical: +x%> and its Magical/Certain, HP/MP variants multiplicatively increase (or decrease with -x%) the battler's passive Life Steal by x% of damage dealt for that hit type.)"
        ),
        QT_TR_NOOP(
            LIFE_STEAL_NOTETAGS
            R"(<HP Life Steal Physical: +x> and its Magical/Certain, HP/MP variants additively increase (or decrease with -x) the battler's passive Life Steal by a flat amount for that hit type.)"
        ),
        QT_TR_NOOP(
            LIFE_STEAL_NOTETAGS
            R"(<Guard Life Steal> / <Guard HP Life Steal> / <Guard MP Life Steal> prevents the battler from having HP and/or MP life stolen from it.)"
        ),

        QT_TR_NOOP(
            MAIN_MENU_ACTOR_EVENTS_NOTETAGS
            R"(<Menu keyword Event: x> (actors only) runs common event x when the actor's ext-entry keyword is selected in the Main Menu Manager.

        # Arguments
        - keyword: the exact keyword used in the Main Menu Manager command's ext entry setup.
        - x: ID of the common event to run.)"
        ),

        QT_TR_NOOP(
            MAP_DISPLAY_NAME_CORE_NOTETAGS
            R"(<Dim Color 1: r, g, b, a> / <Dim Color 2: r, g, b, a> overrides the map display name's gradient dim colors for this map.

        # Arguments
        - r, g, b: red/green/blue, 0-255.
        - a: alpha/opacity, 0-100.

        # Examples
        - <Dim Color 1: 255, 255, 0, 60>)"
        ),
    };

const static QRegularExpression REGEXP =
    QRegularExpression(PATTERN.toString(), QRegularExpression::CaseInsensitiveOption);
}  // namespace Notetags