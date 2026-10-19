#pragma once

#include "Aliases.hpp"

#include <QRegularExpression>

namespace PluginCommands {

static constexpr auto category(const u32 idx) -> QStringView {
    if (idx >= 121) {
        return u"Turn Order Display (YEP)";
    }
    if (idx >= 117) {
        return u"Subclass (YEP)";
    }
    if (idx >= 115) {
        return u"Proximity Compass (Olivia)";
    }
    if (idx >= 113) {
        return u"Order Turn Battle (Olivia)";
    }
    if (idx >= 110) {
        return u"Message Core (YEP)";
    }
    if (idx >= 104) {
        return u"Job Points (YEP)";
    }
    if (idx >= 102) {
        return u"Item Upgrade Slots (YEP)";
    }
    if (idx >= 100) {
        return u"Item Synthesis (YEP)";
    }
    if (idx >= 98) {
        return u"Item Durability (YEP)";
    }
    if (idx >= 96) {
        return u"Item Disassemble (YEP)";
    }
    if (idx >= 94) {
        return u"Item Core (YEP)";
    }
    if (idx >= 92) {
        return u"In-Battle Status (YEP)";
    }
    if (idx >= 91) {
        return u"Footstep Sounds (YEP)";
    }
    if (idx >= 69) {
        return u"Extended Message Pack 1 (YEP)";
    }
    if (idx >= 68) {
        return u"Event Mini Label (YEP)";
    }
    if (idx >= 65) {
        return u"Equip Battle Skills (YEP)";
    }
    if (idx >= 58) {
        return u"Enhanced TP (YEP)";
    }
    if (idx >= 50) {
        return u"Enemy Levels (YEP)";
    }
    if (idx >= 43) {
        return u"Dragonbones Map Sprites (Irina)";
    }
    if (idx >= 42) {
        return u"Difficulty Slider (YEP)";
    }
    if (idx >= 41) {
        return u"Dash Toggle (YEP)";
    }
    if (idx >= 38) {
        return u"Damage Core (YEP)";
    }
    if (idx >= 24) {
        return u"Common Event Menu (YEP)";
    }
    if (idx >= 16) {
        return u"Class Change Core (YEP)";
    }
    if (idx >= 9) {
        return u"Bright Effects (Olivia)";
    }
    if (idx >= 8) {
        return u"Battle System - STB (YEP)";
    }
    if (idx >= 7) {
        return u"Battle System - CTB (YEP)";
    }
    if (idx >= 6) {
        return u"Battle System - ATB (YEP)";
    }
    if (idx >= 2) {
        return u"Attachable Augments (YEP)";
    }
    if (idx >= 0) {
        return u"Action Sequence Fast Forward/Skip (Irina)";
    }

    std::unreachable();
};

static constexpr array<QStringView, 123> PATTERN_PARTS = {
    // https://www.yanfly.moe/wiki/Action_Sequence_Fast_Forward/Skip_(Irina)
    uR"((STOP ACTION SEQUENCE FORWARD))",
    uR"((ALLOW ACTION SEQUENCE FORWARD))",

    // https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)
    uR"((EnableAugments))",
    uR"((DisableAugments))",
    uR"((ShowAugments))",
    uR"((HideAugments))",

    // https://www.yanfly.moe/wiki/Battle_System_-_ATB_(YEP)
    uR"((setBattleSys (?:ATB|DTB)))",

    // https://www.yanfly.moe/wiki/Battle_System_-_CTB_(YEP)
    uR"((setBattleSys (?:CTB|DTB)))",

    // https://www.yanfly.moe/wiki/Battle_System_-_STB_(YEP)
    uR"((setBattleSys (?:STB|DTB)))",

    // https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)
    uR"((BrightEffects Bloom (?:Scale|Brightness|Threshold) (?:\+|-)?\d+?(?:\.\d+?)? \d+?))",
    uR"((BrightEffects Bloom Reset))",
    uR"((BrightEffects Godray (?:Show|Hide)))",
    uR"((BrightEffects Godray (?:Speed|Gain|Lacunarity|Angle) (?:\+|-)?\d+?(?:\.\d+?)? \d+?))",
    uR"((BrightEffects Godray Reset))",
    uR"((BrightEffects ColorAdjust (?:Brightness|Contrast|Saturate) (?:\+|-)?\d+?(?:\.\d+?)? \d+?))",
    uR"((BrightEffects ColorAdjust Reset))",

    // https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)
    uR"((OpenClass))",
    uR"(((?:Show|Hide)Class))",
    uR"(((?:Enable|Disable)Class))",
    uR"((UnlockClass \d+? \d+?))",
    uR"((RemoveClass \d+? \d+?))",
    uR"((UnlockClassAll \d+?))",
    uR"((RemoveClassAll \d+?))",
    uR"(((?:Enable|Disable)PrimaryClassChange \d+?))",

    // https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)
    uR"((ClearCommonEventMenu))",
    uR"((AddCommonEventMenu .+?))",
    uR"((SetCommonEventMenuCancel \d+?))",
    uR"(((?:Disable|Enable)CommonEventMenuCancel))",
    uR"(((?:Disable|Enable)CommonEventMenuConfirm))",
    uR"((OpenCommonEventMenu))",
    uR"((CommonEventMenu(?:X|Y|Width|Height|Opacity|Columns) .+?))",
    uR"(((?:Show|Hide)CommonEventMenuHelp))",
    uR"((CommonEventMenuHelp(?:X|Y|Width|Height|Opacity) .+?))",
    uR"(((?:Show|Hide)CommonEventMenuPicture))",
    uR"((CommonEventMenuPicture(?:X|Y|Width|Height|Opacity) .+?))",
    uR"(((?:Show|Hide)CommonEventMenuSubtext))",
    uR"((CommonEventMenuSubtext(?:X|Y|Width|Height|Opacity) .+?))",
    uR"((SetCommonEventMenuSettings (?:Default|Basic) Setup))",

    // https://www.yanfly.moe/wiki/Damage_Core_(YEP)
    uR"((SetDamageCap \d+?))",
    uR"((SetHealingCap \d+?))",
    uR"(((?:Enable|Disable)DamageCap))",

    // https://www.yanfly.moe/wiki/Dash_Toggle_(YEP)
    uR"(((?:Enable|Disable|Toggle)Dashing))",

    // https://www.yanfly.moe/wiki/Difficulty_Slider_(YEP)
    uR"(((?:Show|Hide)DifficultySlider))",

    // https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)
    uR"(((?:Actor \d+?|Event \d+?|Party \d+?) Dragonbones Sprite Animation: .+?))",
    uR"(((?:Actor \d+?|Party \d+?) Dragonbones Sprite Filename: .+?))",
    uR"(((?:Actor \d+?|Party \d+?) Dragonbones Sprite Scale: \d+?, \d+?))",
    uR"(((?:Actor \d+?|Party \d+?) Dragonbones Sprite Speed: \d+?))",
    uR"(((?:Actor \d+?|Party \d+?) Dragonbones Sprite Size: \d+?, \d+?))",
    uR"(((?:Actor \d+?|Party \d+?) Dragonbones Sprite Flip (?:Left|Right): (?:true|false)))",
    uR"(((?:Actor \d+?|Party \d+?) Dragonbones Motion .+?: .+?))",

    // https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)
    uR"((EnemyLevelChange \d+? to \d+?))",
    uR"((EnemyLevelChangeAll \d+?))",
    uR"((EnemyGainLevel \d+? by \d+?))",
    uR"((EnemyGainLevelAll \d+?))",
    uR"((EnemyLoseLevel \d+? by \d+?))",
    uR"((EnemyLoseLevelAll \d+?))",
    uR"((EnemyLevelReset \d+?))",
    uR"((EnemyLevelResetAll))",

    // https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)
    uR"(((?:ShowTpMode|HideTpMode)))",
    uR"(((?:EnableTpMode|DisableTpMode)))",
    uR"((ChangeTpMode (?:Actor|Party) \d+? to \d+?))",
    uR"((UnlockTpMode (?:Actor|Party) \d+? Mode \d+?))",
    uR"((RemoveTpMode (?:Actor|Party) \d+? Mode \d+?))",
    uR"((UnlockAllTpModes (?:Actor|Party) \d+?))",
    uR"((RemoveAllTpModes (?:Actor|Party) \d+?))",

    // https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)
    uR"(((?:Increase|Decrease)ActorBattleSlots \d+? by \d+?))",
    uR"(((?:Enable|Disable)EquipBattleSkills))",
    uR"(((?:Show|Hide)EquipBattleSkills))",

    // https://www.yanfly.moe/wiki/Event_Mini_Label_(YEP)
    uR"(((?:Hide|Show|Refresh)MiniLabel))",

    // https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)
    uR"(((?:Enable|Disable)LetterSound))",
    uR"((LetterSoundName .+?))",
    uR"((LetterSoundVolume \d+?))",
    uR"((LetterSoundPitch \d+?))",
    uR"((LetterSoundPitchVariance \d+?))",
    uR"((LetterSoundPan \d+?))",
    uR"((LetterSoundPanVariance \d+?))",
    uR"((LetterSoundInterval \d+?))",
    uR"((LetterSoundReset))",
    uR"((ChoiceRowMax \d+?))",
    uR"(((?:Hide|Show)Choice \d+?))",
    uR"((ClearHiddenChoices))",
    uR"(((?:Disable|Enable)Choice \d+?))",
    uR"((ClearDisabledChoices))",
    uR"((ClearChoiceSettings))",
    uR"((MessageRows \d+?))",
    uR"((MessageWidth \d+?))",
    uR"((MessagePositionX \d+?))",
    uR"((MessagePositionY \d+?))",
    uR"((MessagePosition[XY]Auto))",
    uR"((MessageAnchor[XY] .+?))",
    uR"((MessagePositionReset))",

    // https://www.yanfly.moe/wiki/Footstep_Sounds_(YEP)
    uR"(((?:Enable|Disable)Footsteps))",

    // https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)
    uR"((ShowInBattleStatus))",
    uR"((HideInBattleStatus))",

    // https://www.yanfly.moe/wiki/Item_Core_(YEP)
    uR"((EnableVarianceStock))",
    uR"((DisableVarianceStock))",

    // https://www.yanfly.moe/wiki/Item_Disassemble_(YEP)
    uR"((ShowItemDisassemble))",
    uR"((HideItemDisassemble))",

    // https://www.yanfly.moe/wiki/Item_Durability_(YEP)
    uR"(((?:Show|Hide)RepairDurability))",
    uR"(((?:Enable|Disable)RepairDurability))",

    // https://www.yanfly.moe/wiki/Item_Synthesis_(YEP)
    uR"(((?:Open|Show|Hide|Enable|Disable)Synthesis))",
    uR"((OpenSynthesis (?:Item|Weapon|Armor) \d+? Recipe))",

    // https://www.yanfly.moe/wiki/Item_Upgrade_Slots_(YEP)
    uR"(((?:Show|Hide)ItemUpgrade))",
    uR"(((?:Enable|Disable)ItemUpgrade))",

    // https://www.yanfly.moe/wiki/Job_Points_(YEP)
    uR"((gainJp \d+? \d+?))",
    uR"((gainJp \d+? \d+? \d+?))",
    uR"((loseJp \d+? \d+?))",
    uR"((loseJp \d+? \d+? \d+?))",
    uR"((setJp \d+? \d+?))",
    uR"((setJp \d+? \d+? \d+?))",

    // https://www.yanfly.moe/wiki/Message_Core_(YEP)
    uR"((MessageRows \d+?))",
    uR"((MessageWidth \d+?))",
    uR"((Enable|Disable)(?:WordWrap|FastForward))",

    // https://www.yanfly.moe/wiki/Order_Turn_Battle_(Olivia)
    uR"((setBattleSys OTB))",
    uR"((setBattleSys DTB))",

    // https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)
    uR"(((?:ShowCompass|HideCompass|ToggleCompass)))",
    uR"((PlayerCompassIcon \d+?))",

    // https://www.yanfly.moe/wiki/Subclass_(YEP)
    uR"(((?:ShowSubclass|HideSubclass)))",
    uR"(((?:EnableSubclass|DisableSubclass)))",
    uR"((ChangeSubclass \d+? \d+?))",
    uR"(((?:EnableSubclassChange|DisableSubclassChange) \d+?))",

    // https://www.yanfly.moe/wiki/Turn_Order_Display_(YEP)
    uR"((EnableTurnOrderDisplay))",
    uR"((DisableTurnOrderDisplay))",
};

static constexpr usize PATTERN_SIZE = joinedPatternSize(PATTERN_PARTS);
static constexpr array<char16_t, PATTERN_SIZE> PATTERN_STORAGE = buildPattern<PATTERN_SIZE>(PATTERN_PARTS);
static constexpr QStringView PATTERN = QStringView(std::u16string_view(PATTERN_STORAGE.data(), PATTERN_STORAGE.size()));

static constexpr array<const char*, 123> TOOLTIPS = {
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Action Sequence Fast Forward/Skip (Irina)](https://www.yanfly.moe/wiki/Action_Sequence_Fast_Forward/Skip_(Irina)#Plugin_Commands)\n\n"
        R"(STOP ACTION SEQUENCE FORWARD - Stops Fast Forward and Skip Forward from this point onward in the action sequence.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Action Sequence Fast Forward/Skip (Irina)](https://www.yanfly.moe/wiki/Action_Sequence_Fast_Forward/Skip_(Irina)#Plugin_Commands)\n\n"
        R"(ALLOW ACTION SEQUENCE FORWARD - Re-enables Fast Forward and Skip Forward from this point onward in the action sequence.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Attachable Augments (YEP)](https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)#Plugin_Commands)\n\n"
        R"(EnableAugments - Enables augments in the item action menu, allowing the player to attach and detach augments to their items.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Attachable Augments (YEP)](https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)#Plugin_Commands)\n\n"
        R"(DisableAugments - Disables augments in the item action menu and hides their options, preventing the player from attaching or detaching augments.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Attachable Augments (YEP)](https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)#Plugin_Commands)\n\n"
        R"(ShowAugments - Shows augments in the item info window when viewing item details.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Attachable Augments (YEP)](https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)#Plugin_Commands)\n\n"
        R"(HideAugments - Hides augments in the item info window when viewing item details.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Battle System - ATB (YEP)](https://www.yanfly.moe/wiki/Battle_System_-_ATB_(YEP)#Plugin_Commands)\n\n"
        R"(setBattleSys ATB / setBattleSys DTB toggles the active battle system between Active Turn Battle and the Default Turn Battle.

        # Arguments
        - ATB: switches to Active Turn Battle.
        - DTB: switches back to the Default Turn Battle.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Battle System - CTB (YEP)](https://www.yanfly.moe/wiki/Battle_System_-_CTB_(YEP)#Plugin_Commands)\n\n"
        R"(setBattleSys CTB / setBattleSys DTB toggles the active battle system between Charge Turn Battle and the Default Turn Battle.

        # Arguments
        - CTB: switches to Charge Turn Battle.
        - DTB: switches back to the Default Turn Battle.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Battle System - STB (YEP)](https://www.yanfly.moe/wiki/Battle_System_-_STB_(YEP)#Plugin_Commands)\n\n"
        R"(setBattleSys STB / setBattleSys DTB toggles the active battle system between Standard Turn Battle and the Default Turn Battle.

        # Arguments
        - STB: switches to Standard Turn Battle.
        - DTB: switches back to the Default Turn Battle.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects Bloom Scale/Brightness/Threshold x d changes a bloom post-processing parameter over d frames, overriding any Map Horz/Vert notetag for that parameter.

        # Arguments
        - Scale: bloom intensity strength.
        - Brightness: bloom brightness (lower = subtler, higher = blown-out).
        - Threshold: how bright a color must be before it contributes to bloom.
        - x: new decimal value for the chosen parameter.
        - d: duration in frames over which the change occurs.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects Bloom Reset restores the map/battle's default bloom settings.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects Godray Show / BrightEffects Godray Hide toggles visibility of the godray effect.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects Godray Speed/Gain/Lacunarity/Angle x d changes a godray post-processing parameter over d frames, overriding any Map Horz/Vert notetag for that parameter.

        # Arguments
        - Speed: flicker rate of the light.
        - Gain: overall intensity of the effect.
        - Lacunarity: density of the fractal noise.
        - Angle: light-source direction of the rays (positive or negative).
        - x: new decimal value for the chosen parameter.
        - d: duration in frames over which the change occurs.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects Godray Reset restores the map/battle's default godray settings.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects ColorAdjust Brightness/Contrast/Saturate x d changes a screen color-adjustment parameter over d frames, overriding any Map Horz/Vert notetag for that parameter.

        # Arguments
        - Brightness: overall screen brightness.
        - Contrast: separation between dark and light colors.
        - Saturate: intensity of color on screen.
        - x: new decimal value for the chosen parameter.
        - d: duration in frames over which the change occurs.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Bright Effects (Olivia)](https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)#Plugin_Commands)\n\n"
        R"(BrightEffects ColorAdjust Reset restores the map/battle's default color-adjustment settings.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(OpenClass - Opens the class changing scene.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(ShowClass / HideClass - Shows or hides the Class option from the main menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(EnableClass / DisableClass - Enables or disables the Class option in the main menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(UnlockClass actorId classId - Allows actor actorId to unlock class classId.

        # Arguments
        - actorId: ID of the actor.
        - classId: ID of the class to unlock.

        # Examples
        - UnlockClass 5 6)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(RemoveClass actorId classId - Causes actor actorId to no longer be able to access class classId.

        # Examples
        - RemoveClass 5 7)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(UnlockClassAll classId - Unlocks class classId for the global class pool (all actors).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(RemoveClassAll classId - Removes class classId from the global class pool.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Class Change Core (YEP)](https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)#Plugin_Commands)\n\n"
        R"(EnablePrimaryClassChange actorId / DisablePrimaryClassChange actorId - Enables or disables primary class changing for actor actorId.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(ClearCommonEventMenu - Clears all listed common events from the Common Event Menu data pool; the pool must be refilled with AddCommonEventMenu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(AddCommonEventMenu ids - Adds the given common event number(s) to the list shown in the common event menu.

        # Arguments
        - ids: a single ID, a comma-separated list, or a "x through y" range.

        # Examples
        - AddCommonEventMenu 1
        - AddCommonEventMenu 2, 3, 4, 5
        - AddCommonEventMenu 6 through 10)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(SetCommonEventMenuCancel eventId - Sets the common event to run when the cancel button is pressed in the menu. 0 disables running an event but keeps cancel usable.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(DisableCommonEventMenuCancel / EnableCommonEventMenuCancel - Disables or re-enables the cancel button for the common event menu. Enable resets the cancel target to 0.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(DisableCommonEventMenuConfirm / EnableCommonEventMenuConfirm - Disables or re-enables the confirm button, for using the menu as a plain list.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(OpenCommonEventMenu - Opens the common event menu using the settings configured by the preceding plugin commands.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(CommonEventMenuX/Y/Width/Height/Opacity/Columns value - Sets the position, size, opacity, or column count of the main common event list window. Must be set before OpenCommonEventMenu.

        # Examples
        - CommonEventMenuX 0
        - CommonEventMenuWidth Graphics.boxWidth / 2
        - CommonEventMenuColumns 1)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(ShowCommonEventMenuHelp / HideCommonEventMenuHelp - Shows or hides the help window for the next OpenCommonEventMenu call.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(CommonEventMenuHelpX/Y/Width/Height/Opacity value - Sets the position, size, and opacity of the help window. Must be set before OpenCommonEventMenu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(ShowCommonEventMenuPicture / HideCommonEventMenuPicture - Shows or hides the picture window for the next OpenCommonEventMenu call.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(CommonEventMenuPictureX/Y/Width/Height/Opacity value - Sets the position, size, and opacity of the picture window. Must be set before OpenCommonEventMenu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(ShowCommonEventMenuSubtext / HideCommonEventMenuSubtext - Shows or hides the subtext window for the next OpenCommonEventMenu call.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(CommonEventMenuSubtextX/Y/Width/Height/Opacity value - Sets the position, size, and opacity of the subtext window. Must be set before OpenCommonEventMenu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Plugin_Commands)\n\n"
        R"(SetCommonEventMenuSettings Default Setup / SetCommonEventMenuSettings Basic Setup - Positions all menu windows to a preset layout (full setup, or just the main list plus help window).)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Damage Core (YEP)](https://www.yanfly.moe/wiki/Damage_Core_(YEP)#Plugin_Commands)\n\n"
        R"(SetDamageCap value - Sets the default damage cap.

        # Examples
        - SetDamageCap 9999)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Damage Core (YEP)](https://www.yanfly.moe/wiki/Damage_Core_(YEP)#Plugin_Commands)\n\n"
        R"(SetHealingCap value - Sets the default healing cap.

        # Examples
        - SetHealingCap 9999)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Damage Core (YEP)](https://www.yanfly.moe/wiki/Damage_Core_(YEP)#Plugin_Commands)\n\n"
        R"(EnableDamageCap / DisableDamageCap - Enables or disables the default damage/healing cap for both damage and healing.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dash Toggle (YEP)](https://www.yanfly.moe/wiki/Dash_Toggle_(YEP)#Plugin_Commands)\n\n"
        R"(EnableDashing / DisableDashing / ToggleDashing - Enables, disables, or toggles the player's ability to dash. Does not override maps that disable dashing altogether.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Difficulty Slider (YEP)](https://www.yanfly.moe/wiki/Difficulty_Slider_(YEP)#Plugin_Commands)\n\n"
        R"(ShowDifficultySlider / HideDifficultySlider - Shows/enables or hides/disables the Difficulty Slider option and its effect on enemy levels.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Event id / Party index Dragonbones Sprite Animation: animation - Plays a Dragonbones animation on the target's map sprite.

        # Arguments
        - id/index: actor ID, event ID, or party index to target.
        - animation: Dragonbones animation name to play (case sensitive); use "Clear Animation" to clear it.

        # Examples
        - Actor 1 Dragonbones Sprite Animation: Idle
        - Party 0 Dragonbones Sprite Animation: Clear Animation)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Party index Dragonbones Sprite Filename: filename - Permanently changes the Dragonbones armature filename used by the target's map sprite (case sensitive).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Party index Dragonbones Sprite Scale: x, y - Permanently sets the Dragonbones map sprite's scale for the target.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Party index Dragonbones Sprite Speed: x - Permanently sets the Dragonbones animation base speed for the target (1 is normal speed).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Party index Dragonbones Sprite Size: width, height - Permanently sets the sprite's width/height values used by other plugins (Dragonbones armatures have no inherent size).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Party index Dragonbones Sprite Flip Left/Right: true/false - Turns horizontal flipping on/off for the target's Dragonbones map sprite.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Plugin_Commands)\n\n"
        R"(Actor id / Party index Dragonbones Motion type: animation - Permanently sets the animation used for a specific motion type on the target.

        # Arguments
        - type: one of Idle, Walk, Dash, Jump, Ladder Idle, Ladder Climb.
        - animation: Dragonbones animation name (case sensitive).

        # Examples
        - Actor 1 Dragonbones Motion Walk: move)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyLevelChange x to y sets the enemy in troop position x's level to y.

        # Arguments
        - x: enemy troop position.
        - y: new level.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyLevelChangeAll x sets the level of every enemy in the troop to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyGainLevel x by y causes the enemy in troop position x to gain y levels.

        # Arguments
        - x: enemy troop position.
        - y: levels to gain.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyGainLevelAll x causes every enemy in the troop to gain x levels.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyLoseLevel x by y causes the enemy in troop position x to lose y levels.

        # Arguments
        - x: enemy troop position.
        - y: levels to lose.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyLoseLevelAll x causes every enemy in the troop to lose x levels.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyLevelReset x resets the enemy in troop position x's level back to the level it had at the start of battle.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enemy Levels (YEP)](https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)#Plugin_Commands)\n\n"
        R"(EnemyLevelResetAll resets every enemy's level back to the level it had at the start of battle.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(ShowTpMode / HideTpMode shows or hides the TP Mode command from the Skill Menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(EnableTpMode / DisableTpMode enables or disables the TP Mode command in the Skill Menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(ChangeTpMode Actor/Party x to y changes the TP Mode of actor or party member x to TP Mode y.

        # Arguments
        - x: actor ID or party member index.
        - y: TP Mode ID to switch to.

        # Examples
        - ChangeTpMode Actor 1 to 5
        - ChangeTpMode Party 2 to 6)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(UnlockTpMode Actor/Party x Mode y makes actor or party member x unlock TP Mode y.

        # Arguments
        - x: actor ID or party member index.
        - y: TP Mode ID to unlock.

        # Examples
        - UnlockTpMode Actor 3 Mode 7
        - UnlockTpMode Party 4 Mode 8)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(RemoveTpMode Actor/Party x Mode y makes actor or party member x remove (relock) TP Mode y.

        # Arguments
        - x: actor ID or party member index.
        - y: TP Mode ID to remove.

        # Examples
        - RemoveTpMode Actor 1 Mode 9
        - RemoveTpMode Party 2 Mode 10)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(UnlockAllTpModes Actor/Party x makes actor or party member x unlock all TP Modes.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Enhanced TP (YEP)](https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)#Plugin_Commands)\n\n"
        R"(RemoveAllTpModes Actor/Party x makes actor or party member x remove (relock) all TP Modes.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Equip Battle Skills (YEP)](https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)#Plugin_Commands)\n\n"
        R"(IncreaseActorBattleSlots x by y / DecreaseActorBattleSlots x by y adjusts actor x's number of battle skill slots by y. Cannot exceed the 'Maximum Skills' plugin parameter or drop below 1.

        # Arguments
        - x: actor ID.
        - y: amount of slots to add/remove.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Equip Battle Skills (YEP)](https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)#Plugin_Commands)\n\n"
        R"(EnableEquipBattleSkills / DisableEquipBattleSkills enables or disables the 'Equip Skills' command in the skill menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Equip Battle Skills (YEP)](https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)#Plugin_Commands)\n\n"
        R"(ShowEquipBattleSkills / HideEquipBattleSkills shows or hides the 'Equip Skills' command in the skill menu.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Event Mini Label (YEP)](https://www.yanfly.moe/wiki/Event_Mini_Label_(YEP)#Plugin_Commands)\n\n"
        R"(HideMiniLabel / ShowMiniLabel hides or shows all Event Mini Labels; RefreshMiniLabel refreshes all Event Mini Labels on the map.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(EnableLetterSound / DisableLetterSound - enables or disables the letter-by-letter sound effect in messages.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundName filename - replaces the current letter sound SE with 'filename' (case-sensitive, no extension).

        # Arguments
        - filename: sound effect filename)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundVolume value - sets the letter sound's volume.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundPitch value - sets the letter sound's pitch.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundPitchVariance value - sets the +/- range the letter sound's pitch randomly fluctuates within.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundPan value - sets the letter sound's pan.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundPanVariance value - sets the +/- range the letter sound's pan randomly fluctuates within.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundInterval value - sets how many letters pass between each played letter sound. 0 plays a sound on every letter.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(LetterSoundReset - resets the letter sound settings to their plugin parameter defaults.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(ChoiceRowMax value - sets the maximum number of visible choices before scrolling.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(HideChoice / ShowChoice index - hides or reveals the given choice index.

        # Arguments
        - index: choice number)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(ClearHiddenChoices - makes all previously hidden choices visible again.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(DisableChoice / EnableChoice index - disables (greys out) or re-enables the given choice index.

        # Arguments
        - index: choice number)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(ClearDisabledChoices - re-enables all previously disabled choices.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(ClearChoiceSettings - clears both hidden and disabled choice states, showing and enabling every choice.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessageRows value - sets the number of message rows displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessageWidth value - sets the message window's width in pixels.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessagePositionX value - sets the message window's X position relative to its horizontal anchor.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessagePositionY value - sets the message window's Y position relative to its vertical anchor.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessagePositionXAuto / MessagePositionYAuto - restores automatic positioning for the given axis instead of a fixed coordinate.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessageAnchorX left|center|right / MessageAnchorY top|center|bottom - sets the message window's anchor point on the given axis.

        # Arguments
        - anchor keyword for the given axis)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Plugin_Commands)\n\n"
        R"(MessagePositionReset - resets the message window's X/Y position, width, and row count to automatic defaults.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Footstep Sounds (YEP)](https://www.yanfly.moe/wiki/Footstep_Sounds_(YEP)#Plugin_Commands)\n\n"
        R"(EnableFootsteps / DisableFootsteps - turns footstep sounds on or off.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[In-Battle Status (YEP)](https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)#Plugin_Commands)\n\n"
        R"(ShowInBattleStatus - Causes the 'Status' command to show in the party menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[In-Battle Status (YEP)](https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)#Plugin_Commands)\n\n"
        R"(HideInBattleStatus - Causes the 'Status' command to not show in the party menu.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Core (YEP)](https://www.yanfly.moe/wiki/Item_Core_(YEP)#Plugin_Commands)\n\n"
        R"(EnableVarianceStock - Causes all items acquired from this point forward to have nonrandom variance stock values.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Core (YEP)](https://www.yanfly.moe/wiki/Item_Core_(YEP)#Plugin_Commands)\n\n"
        R"(DisableVarianceStock - Causes all items acquired from this point forward to have random variance values.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Disassemble (YEP)](https://www.yanfly.moe/wiki/Item_Disassemble_(YEP)#Plugin_Commands)\n\n"
        R"(ShowItemDisassemble - Shows the Disassemble command in the item menu if the item permits disassembling.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Disassemble (YEP)](https://www.yanfly.moe/wiki/Item_Disassemble_(YEP)#Plugin_Commands)\n\n"
        R"(HideItemDisassemble - Hides the Disassemble command in the item menu regardless.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Durability (YEP)](https://www.yanfly.moe/wiki/Item_Durability_(YEP)#Plugin_Commands)\n\n"
        R"(ShowRepairDurability / HideRepairDurability - Shows or hides the Repair command in the Item Action Window.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Durability (YEP)](https://www.yanfly.moe/wiki/Item_Durability_(YEP)#Plugin_Commands)\n\n"
        R"(EnableRepairDurability / DisableRepairDurability - Enables or disables the Repair command in the Item Action Window.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Synthesis (YEP)](https://www.yanfly.moe/wiki/Item_Synthesis_(YEP)#Plugin_Commands)\n\n"
        R"(OpenSynthesis - Opens the Synthesis scene from the field.
        ShowSynthesis - Shows the Synthesis command in the main menu.
        HideSynthesis - Hides the Synthesis command from the main menu.
        EnableSynthesis - Enables the Synthesis command in the main menu.
        DisableSynthesis - Disables the Synthesis command in the main menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Synthesis (YEP)](https://www.yanfly.moe/wiki/Item_Synthesis_(YEP)#Plugin_Commands)\n\n"
        R"(OpenSynthesis type id Recipe - Opens the synthesis scene restricted to the recipe of a single item/weapon/armor, ignoring recipe items in the player's inventory.

        # Arguments
        - type: Item, Weapon, or Armor
        - id: database ID of the item/weapon/armor

        # Examples
        - OpenSynthesis Item 15 Recipe
        - OpenSynthesis Weapon 20 Recipe
        - OpenSynthesis Armor 30 Recipe)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Upgrade Slots (YEP)](https://www.yanfly.moe/wiki/Item_Upgrade_Slots_(YEP)#Plugin_Commands)\n\n"
        R"(ShowItemUpgrade / HideItemUpgrade - Shows or hides the upgrade option in the item menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Item Upgrade Slots (YEP)](https://www.yanfly.moe/wiki/Item_Upgrade_Slots_(YEP)#Plugin_Commands)\n\n"
        R"(EnableItemUpgrade / DisableItemUpgrade - Enables or disables the upgrade option in the item menu.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Plugin_Commands)\n\n"
        R"(gainJp actorId jp - actor gains jp amount of JP.

        # Arguments
        - actorId: ID of the actor.
        - jp: amount of JP to gain.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Plugin_Commands)\n\n"
        R"(gainJp actorId jp classId - actor gains jp amount of JP for a specific class.

        # Arguments
        - actorId: ID of the actor.
        - jp: amount of JP to gain.
        - classId: ID of the class whose JP is altered.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Plugin_Commands)\n\n"
        R"(loseJp actorId jp - actor loses jp amount of JP.

        # Arguments
        - actorId: ID of the actor.
        - jp: amount of JP to lose.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Plugin_Commands)\n\n"
        R"(loseJp actorId jp classId - actor loses jp amount of JP for a specific class.

        # Arguments
        - actorId: ID of the actor.
        - jp: amount of JP to lose.
        - classId: ID of the class whose JP is altered.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Plugin_Commands)\n\n"
        R"(setJp actorId jp - sets the actor's JP to a specific value.

        # Arguments
        - actorId: ID of the actor.
        - jp: value to set JP to.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Job Points (YEP)](https://www.yanfly.moe/wiki/Job_Points_(YEP)#Plugin_Commands)\n\n"
        R"(setJp actorId jp classId - sets the actor's JP to a specific value for a specific class.

        # Arguments
        - actorId: ID of the actor.
        - jp: value to set JP to.
        - classId: ID of the class whose JP is altered.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Plugin_Commands)\n\n"
        R"(MessageRows rows - changes the number of message rows displayed.

        # Arguments
        - rows: number of rows to display; text beyond this limit is cut off until the next message.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Plugin_Commands)\n\n"
        R"(MessageWidth width - changes the message window width, in pixels.

        # Arguments
        - width: window width in pixels; content extending past it is cut off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Plugin_Commands)\n\n"
        R"(EnableWordWrap / DisableWordWrap / EnableFastForward / DisableFastForward - toggles word wrapping or Fast Forward key support for messages.

        EnableWordWrap wraps overflowing words onto the next line (manual breaks still need \br). DisableWordWrap restores editor-defined line breaks. EnableFastForward/DisableFastForward toggle whether the Fast Forward key works during messages.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Order Turn Battle (Olivia)](https://www.yanfly.moe/wiki/Order_Turn_Battle_(Olivia)#Plugin_Commands)\n\n"
        R"(setBattleSys OTB - Sets the battle system to Order Turn Battle.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Order Turn Battle (Olivia)](https://www.yanfly.moe/wiki/Order_Turn_Battle_(Olivia)#Plugin_Commands)\n\n"
        R"(setBattleSys DTB - Sets the battle system to Default Turn Battle.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Proximity Compass (Olivia)](https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)#Plugin_Commands)\n\n"
        R"(ShowCompass / HideCompass / ToggleCompass - Shows, hides, or toggles the compass from view. If shown, the compass becomes visible unless the player has opted to hide it or the map has a <Hide Compass> notetag.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Proximity Compass (Olivia)](https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)#Plugin_Commands)\n\n"
        R"(PlayerCompassIcon x - Changes the player's compass icon to x.

        # Arguments
        - x: icon index to use for the player's compass marker.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Subclass (YEP)](https://www.yanfly.moe/wiki/Subclass_(YEP)#Plugin_Commands)\n\n"
        R"(ShowSubclass / HideSubclass - Shows or hides the Subclass command in the Class Change Menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Subclass (YEP)](https://www.yanfly.moe/wiki/Subclass_(YEP)#Plugin_Commands)\n\n"
        R"(EnableSubclass / DisableSubclass - Enables or disables the Subclass command in the Class Change Menu.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Subclass (YEP)](https://www.yanfly.moe/wiki/Subclass_(YEP)#Plugin_Commands)\n\n"
        R"(ChangeSubclass x y - Changes actor x's subclass to y. Replace y with 0 to remove a subclass.

        # Arguments
        - x: actor ID.
        - y: class ID to set as the subclass, or 0 to remove it.

        # Examples
        - ChangeSubclass 1 5)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Subclass (YEP)](https://www.yanfly.moe/wiki/Subclass_(YEP)#Plugin_Commands)\n\n"
        R"(EnableSubclassChange x / DisableSubclassChange x - Enables or disables subclass changing for actor x.

        # Arguments
        - x: actor ID.

        # Examples
        - EnableSubclassChange 5
        - DisableSubclassChange 5)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Turn Order Display (YEP)](https://www.yanfly.moe/wiki/Turn_Order_Display_(YEP)#Plugin_Commands)\n\n"
        R"(EnableTurnOrderDisplay - Turns on the Turn Order Display to be shown in battle, if the current battle system supports it.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Turn Order Display (YEP)](https://www.yanfly.moe/wiki/Turn_Order_Display_(YEP)#Plugin_Commands)\n\n"
        R"(DisableTurnOrderDisplay - Turns off the Turn Order Display so it is hidden in battle.)"
    ),
};

const static QRegularExpression REGEXP =
    QRegularExpression(PATTERN.toString(), QRegularExpression::CaseInsensitiveOption);
}  // namespace PluginCommands