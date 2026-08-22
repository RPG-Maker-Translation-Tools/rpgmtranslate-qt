#pragma once

#include "Aliases.hpp"

#include <QRegularExpression>

namespace CommentTags {
#define BATTLE_BGM_CONTROL_COMMENT_TAGS \
    "[Battle BGM Control (YEP)](https://www.yanfly.moe/wiki/Battle_BGM_Control_(YEP)#Comment_Tags)\n\n"

#define COMMON_EVENT_MENU_COMMENT_TAGS \
    "[Common Event Menu (YEP)](https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)#Comment_Tags)\n\n"

#define EVENT_CLICK_TRIGGER_COMMENT_TAGS \
    "[Event Click Trigger (YEP)](https://www.yanfly.moe/wiki/Event_Click_Trigger_(YEP)#Comment_Tags)\n\n"

#define EVENT_HITBOX_RESIZE_COMMENT_TAGS \
    "[Event Hitbox Resize (YEP)](https://www.yanfly.moe/wiki/Event_Hitbox_Resize_(YEP)#Comment_Tags)\n\n"

#define EVENT_MINI_LABEL_COMMENT_TAGS \
    "[Event Mini Label (YEP)](https://www.yanfly.moe/wiki/Event_Mini_Label_(YEP)#Comment_Tags)\n\n"

#define EVENT_PROXIMITY_ACTIVATE_COMMENT_TAGS \
    "[Event Proximity Activate (YEP)](https://www.yanfly.moe/wiki/Event_Proximity_Activate_(YEP)#Comment_Tags)\n\n"

#define EVENT_REGION_TRIGGER_COMMENT_TAGS \
    "[Event Region Trigger (YEP)](https://www.yanfly.moe/wiki/Event_Region_Trigger_(YEP)#Comment_Tags)\n\n"

#define EVENT_SPRITE_OFFSET_COMMENT_TAGS \
    "[Event Sprite Offset (YEP)](https://www.yanfly.moe/wiki/Event_Sprite_Offset_(YEP)#Comment_Tags)\n\n"

#define EVENT_STEP_ANIMATION_OPTIONS_COMMENT_TAGS \
    "[Event Step Animation Options (YEP)](https://www.yanfly.moe/wiki/Event_Step_Animation_Options_(YEP)#Comment_Tags)\n\n"

#define ICONS_ON_EVENTS_COMMENT_TAGS \
    "[Icons on Events (YEP)](https://www.yanfly.moe/wiki/Icons_on_Events_(YEP)#Comment_Tags)\n\n"

#define KEYBOARD_CONFIG_COMMENT_TAGS \
    "[Keyboard Config (YEP)](https://www.yanfly.moe/wiki/Keyboard_Config_(YEP)#Comment_Tags)\n\n"

#define PROXIMITY_COMPASS_COMMENT_TAGS \
    "[Proximity Compass (Olivia)](https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)#Comment_Tags)\n\n"

#define SYNCHRONIZED_EVENT_MOVEMENT_COMMENT_TAGS \
    "[Synchronized Event Movement (Arisu)](https://www.yanfly.moe/wiki/Synchronized_Event_Movement_(Arisu)#Comment_Tags)\n\n"

#define DRAGONBONES_MAP_SPRITES_COMMENT_TAGS \
    "[Dragonbones Map Sprites (Irina)](https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)#Comment_Tags)\n\n"

static constexpr auto category(const u32 idx) -> QStringView {
    if (idx >= 45) {
        return u"Dragonbones Map Sprites (Irina)";
    }
    if (idx >= 42) {
        return u"Synchronized Event Movement (Arisu)";
    }
    if (idx >= 40) {
        return u"Proximity Compass (Olivia)";
    }
    if (idx >= 37) {
        return u"Keyboard Config (YEP)";
    }
    if (idx >= 33) {
        return u"Icons on Events (YEP)";
    }
    if (idx >= 29) {
        return u"Event Step Animation Options (YEP)";
    }
    if (idx >= 28) {
        return u"Event Sprite Offset (YEP)";
    }
    if (idx >= 27) {
        return u"Event Region Trigger (YEP)";
    }
    if (idx >= 23) {
        return u"Event Proximity Activate (YEP)";
    }
    if (idx >= 17) {
        return u"Event Mini Label (YEP)";
    }
    if (idx >= 16) {
        return u"Event Hitbox Resize (YEP)";
    }
    if (idx >= 15) {
        return u"Event Click Trigger (YEP)";
    }
    if (idx >= 4) {
        return u"Common Event Menu (YEP)";
    }
    if (idx >= 0) {
        return u"Battle BGM Control (YEP)";
    }

    std::unreachable();
};

static constexpr array<QStringView, 52> PATTERN_PARTS = {
    // https://www.yanfly.moe/wiki/Battle_BGM_Control_(YEP)
    // Troop event
    uR"((<Battle BGM Name: .+?>))",
    uR"((<Battle BGM Volume: \d+?>))",
    uR"((<Battle BGM Pitch: \d+?>))",
    uR"((<Battle BGM Pan: (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)
    uR"((<Menu Name: .+?>))",
    uR"((<Icon: .+?>))",
    uR"((<Picture: .+?>))",
    uR"((<Help Description>))",
    uR"((<\/Help Description>))",
    uR"((<Subtext>))",
    uR"((<\/Subtext>))",
    uR"((<Menu Enable Eval>))",
    uR"((<\/Menu Enable Eval>))",
    uR"((<Menu Visible Eval>))",
    uR"((<\/Menu Visible Eval>))",

    // https://www.yanfly.moe/wiki/Event_Click_Trigger_(YEP)
    uR"((<Click Trigger>))",

    // https://www.yanfly.moe/wiki/Event_Hitbox_Resize_(YEP)
    uR"((<Hitbox (?:Up|Left|Right|Down): .+?>))",

    // https://www.yanfly.moe/wiki/Event_Mini_Label_(YEP)
    uR"((<Mini Label: .+?>))",
    uR"((<Mini Label Font Size: \d+?>))",
    uR"((<Mini Label (?:X|Y) Buffer: (?:\+|-)\d+?>))",
    uR"((<Always Show Mini Label>))",
    uR"((<Mini Label Range: \d+?>))",
    uR"((<Mini Label Require Facing>))",

    // https://www.yanfly.moe/wiki/Event_Proximity_Activate_(YEP)
    uR"((<Activation Square: \d+?>))",
    uR"((<Activation Radius: \d+?>))",
    uR"((<Activation Row: \d+?>))",
    uR"((<Activation Column: \d+?>))",

    // https://www.yanfly.moe/wiki/Event_Region_Trigger_(YEP)
    uR"((<Region Triggers?: \d+?(?:, \d+?, \d+?)>))",

    // https://www.yanfly.moe/wiki/Event_Sprite_Offset_(YEP)
    uR"((<Sprite Offset(?: X| Y)?: (?:\+|-)\d+?>))",

    // https://www.yanfly.moe/wiki/Event_Step_Animation_Options_(YEP)
    uR"((<Step Animation: Left to Right>))",
    uR"((<Step Animation: Right to Left>))",
    uR"((<Step Animation: Spin (?:Clockwise|CW)>))",
    uR"((<Step Animation: Spin (?:CounterClockwise|CCW|AntiClockwise|ACW)>))",

    // https://www.yanfly.moe/wiki/Icons_on_Events_(YEP)
    uR"((<Icon on Event: \d+?>))",
    uR"((<Icon on Event Buffer (?:X|Y)?: (?:\+|-)\d+?>))",
    uR"((<Icon on Event Buffer: \+\d+?, \+\d+?>))",
    uR"((<Icon on Event Buffer: -\d+?, -\d+?>))",

    // https://www.yanfly.moe/wiki/Keyboard_Config_(YEP)
    uR"((<Config Key: .+?>))",
    uR"((<Config Text: .+?>))",
    uR"((<Config Required>))",

    // https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)
    uR"((<Compass Icon: \d+?>))",
    uR"((<Compass Proximity: \d+?>))",

    // https://www.yanfly.moe/wiki/Synchronized_Event_Movement_(Arisu)
    uR"((<Move Synch Target: (?:Player|Event \d+?)>))",
    uR"((<Move Synch Type: (?:Random|Approach|Away|Custom|Mimic|Reverse Mimic|Mirror Horizontal|Mirror Vertical)>))",
    uR"((<Move Synch Delay: \d+?>))",

    // https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)
    // Event Page Comment Tags
    uR"((<Dragonbones Sprite: .+?>))",
    uR"((<Dragonbones Sprite Scale(?: X| Y)?: -?[\d.]+?(?:, -?[\d.]+?)?>))",
    uR"((<Dragonbones Sprite Time Scale: [\d.]+?>))",
    uR"((<Dragonbones Sprite (?:No )?Flip (?:Left|Right)>))",
    uR"((<Dragonbones Sprite Motion (?:Idle|Walk|Dash|Jump|LadderIdle|LadderClimb): .+?>))",
    uR"((<Dragonbones Sprite Settings>))",
    uR"((<\/Dragonbones Sprite Settings>))",
};

static constexpr usize PATTERN_SIZE = joinedPatternSize(PATTERN_PARTS);
static constexpr array<char16_t, PATTERN_SIZE> PATTERN_STORAGE = buildPattern<PATTERN_SIZE>(PATTERN_PARTS);
static constexpr QStringView PATTERN = QStringView(std::u16string_view(PATTERN_STORAGE.data(), PATTERN_STORAGE.size()));

static constexpr array<const char*, 52> TOOLTIPS = {
    QT_TR_NOOP(
        BATTLE_BGM_CONTROL_COMMENT_TAGS
        R"(<Battle BGM Name: filename> sets the battle BGM played for this troop when the comment tag is placed in a troop event page.)"
    ),
    QT_TR_NOOP(
        BATTLE_BGM_CONTROL_COMMENT_TAGS R"(<Battle BGM Volume: x> sets the volume of the battle BGM for this troop.)"
    ),
    QT_TR_NOOP(
        BATTLE_BGM_CONTROL_COMMENT_TAGS R"(<Battle BGM Pitch: x> sets the pitch of the battle BGM for this troop.)"
    ),
    QT_TR_NOOP(
        BATTLE_BGM_CONTROL_COMMENT_TAGS
        R"(<Battle BGM Pan: +x> / <Battle BGM Pan: -x> sets the pan of the battle BGM for this troop.)"
    ),

    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Menu Name: text> sets the name displayed for this common event in the Common Event Menu.)"
    ),
    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Icon: x> sets the icon displayed for this common event in the Common Event Menu.)"
    ),
    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Picture: filename> sets the picture displayed for this common event in the Common Event Menu.)"
    ),
    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Help Description> text </Help Description> sets the help window text shown for this common event.)"
    ),
    QT_TR_NOOP(COMMON_EVENT_MENU_COMMENT_TAGS R"(Closing tag for <Help Description>.)"),
    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Subtext> text </Subtext> sets the subtext line shown for this common event in the menu list.)"
    ),
    QT_TR_NOOP(COMMON_EVENT_MENU_COMMENT_TAGS R"(Closing tag for <Subtext>.)"),
    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Menu Enable Eval> code </Menu Enable Eval> runs JavaScript to determine whether this common event is enabled/selectable in the menu.)"
    ),
    QT_TR_NOOP(COMMON_EVENT_MENU_COMMENT_TAGS R"(Closing tag for <Menu Enable Eval>.)"),
    QT_TR_NOOP(
        COMMON_EVENT_MENU_COMMENT_TAGS
        R"(<Menu Visible Eval> code </Menu Visible Eval> runs JavaScript to determine whether this common event is visible in the menu.)"
    ),
    QT_TR_NOOP(COMMON_EVENT_MENU_COMMENT_TAGS R"(Closing tag for <Menu Visible Eval>.)"),

    QT_TR_NOOP(
        EVENT_CLICK_TRIGGER_COMMENT_TAGS
        R"(<Click Trigger> makes this event page trigger when clicked/tapped by the player, instead of only by touch/action button.)"
    ),

    QT_TR_NOOP(
        EVENT_HITBOX_RESIZE_COMMENT_TAGS
        R"(<Hitbox Up: x> / <Hitbox Left: x> / <Hitbox Right: x> / <Hitbox Down: x> extends this event's collision hitbox by x tiles in the given direction.)"
    ),

    QT_TR_NOOP(
        EVENT_MINI_LABEL_COMMENT_TAGS
        R"(<Mini Label: text> gives this event a small floating label showing text above it.)"
    ),
    QT_TR_NOOP(EVENT_MINI_LABEL_COMMENT_TAGS R"(<Mini Label Font Size: x> sets the font size of the mini label.)"),
    QT_TR_NOOP(
        EVENT_MINI_LABEL_COMMENT_TAGS
        R"(<Mini Label X Buffer: +x> / <Mini Label Y Buffer: +x> offsets the mini label's display position.)"
    ),
    QT_TR_NOOP(
        EVENT_MINI_LABEL_COMMENT_TAGS
        R"(<Always Show Mini Label> keeps the mini label visible regardless of player proximity/facing requirements.)"
    ),
    QT_TR_NOOP(
        EVENT_MINI_LABEL_COMMENT_TAGS
        R"(<Mini Label Range: x> sets the range in tiles within which the mini label becomes visible.)"
    ),
    QT_TR_NOOP(
        EVENT_MINI_LABEL_COMMENT_TAGS
        R"(<Mini Label Require Facing> requires the player to be facing the event for its mini label to show.)"
    ),

    QT_TR_NOOP(
        EVENT_PROXIMITY_ACTIVATE_COMMENT_TAGS
        R"(<Activation Square: x> triggers this event automatically once the player enters a square-shaped region of radius x tiles.)"
    ),
    QT_TR_NOOP(
        EVENT_PROXIMITY_ACTIVATE_COMMENT_TAGS
        R"(<Activation Radius: x> triggers this event automatically once the player enters a circular radius of x tiles.)"
    ),
    QT_TR_NOOP(
        EVENT_PROXIMITY_ACTIVATE_COMMENT_TAGS
        R"(<Activation Row: x> triggers this event automatically once the player enters the same row within x tiles.)"
    ),
    QT_TR_NOOP(
        EVENT_PROXIMITY_ACTIVATE_COMMENT_TAGS
        R"(<Activation Column: x> triggers this event automatically once the player enters the same column within x tiles.)"
    ),

    QT_TR_NOOP(
        EVENT_REGION_TRIGGER_COMMENT_TAGS
        R"(<Region Trigger: x> / <Region Triggers: x, y, z> makes this event page only active while the player stands on the listed region ID(s).)"
    ),

    QT_TR_NOOP(
        EVENT_SPRITE_OFFSET_COMMENT_TAGS
        R"(<Sprite Offset: +x> / <Sprite Offset X: +x> / <Sprite Offset Y: +x> offsets this event's sprite rendering position without moving its actual tile position.)"
    ),

    QT_TR_NOOP(
        EVENT_STEP_ANIMATION_OPTIONS_COMMENT_TAGS
        R"(<Step Animation: Left to Right> makes this event's step animation frames cycle left to right.)"
    ),
    QT_TR_NOOP(
        EVENT_STEP_ANIMATION_OPTIONS_COMMENT_TAGS
        R"(<Step Animation: Right to Left> makes this event's step animation frames cycle right to left.)"
    ),
    QT_TR_NOOP(
        EVENT_STEP_ANIMATION_OPTIONS_COMMENT_TAGS
        R"(<Step Animation: Spin Clockwise> / <Step Animation: Spin CW> cycles the event's frames in a clockwise spin pattern.)"
    ),
    QT_TR_NOOP(
        EVENT_STEP_ANIMATION_OPTIONS_COMMENT_TAGS
        R"(<Step Animation: Spin CounterClockwise/CCW/AntiClockwise/ACW> cycles the event's frames in a counter-clockwise spin pattern.)"
    ),

    QT_TR_NOOP(ICONS_ON_EVENTS_COMMENT_TAGS R"(<Icon on Event: x> displays icon x above this event.)"),
    QT_TR_NOOP(
        ICONS_ON_EVENTS_COMMENT_TAGS
        R"(<Icon on Event Buffer: +x> / <Icon on Event Buffer X: +x> / <Icon on Event Buffer Y: +x> offsets the icon's display position above the event.)"
    ),
    QT_TR_NOOP(
        ICONS_ON_EVENTS_COMMENT_TAGS
        R"(<Icon on Event Buffer: +x, +y> offsets the icon's position on both axes with positive values.)"
    ),
    QT_TR_NOOP(
        ICONS_ON_EVENTS_COMMENT_TAGS
        R"(<Icon on Event Buffer: -x, -y> offsets the icon's position on both axes with negative values.)"
    ),

    QT_TR_NOOP(
        KEYBOARD_CONFIG_COMMENT_TAGS
        R"(<Config Key: keyname> binds this event's action to the keyboard key keyname when used with the Keyboard Config system.)"
    ),
    QT_TR_NOOP(
        KEYBOARD_CONFIG_COMMENT_TAGS
        R"(<Config Text: text> sets the display text used for this key binding entry in the config menu.)"
    ),
    QT_TR_NOOP(
        KEYBOARD_CONFIG_COMMENT_TAGS
        R"(<Config Required> marks this key binding as required, so it cannot be left unbound.)"
    ),

    QT_TR_NOOP(
        PROXIMITY_COMPASS_COMMENT_TAGS
        R"(<Compass Icon: x> sets the icon used to represent this event on the proximity compass.)"
    ),
    QT_TR_NOOP(
        PROXIMITY_COMPASS_COMMENT_TAGS
        R"(<Compass Proximity: x> sets the distance in tiles at which this event begins showing on the proximity compass.)"
    ),

    QT_TR_NOOP(
        SYNCHRONIZED_EVENT_MOVEMENT_COMMENT_TAGS
        R"(<Move Synch Target: Player> / <Move Synch Target: Event x> sets which character this event synchronizes its movement to.)"
    ),
    QT_TR_NOOP(
        SYNCHRONIZED_EVENT_MOVEMENT_COMMENT_TAGS
        R"(<Move Synch Type: type> sets how this event's movement synchronizes with its target: Random, Approach, Away, Custom, Mimic, Reverse Mimic, Mirror Horizontal, or Mirror Vertical.)"
    ),
    QT_TR_NOOP(
        SYNCHRONIZED_EVENT_MOVEMENT_COMMENT_TAGS
        R"(<Move Synch Delay: x> sets a delay in frames before this event's synchronized movement takes effect.)"
    ),

    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(<Dragonbones Sprite: filename> - replaces this event's default map sprite with the named DragonBones armature (matched against _ske.json/_tex.json/_tex.png).)"
    ),
    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(<Dragonbones Sprite Scale: x, y>/<Dragonbones Sprite Scale X: x>/<Dragonbones Sprite Scale Y: y> - sets the armature's base scale; negative values flip it.)"
    ),
    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(<Dragonbones Sprite Time Scale: x> - multiplies the armature's animation playback speed (1.0 = default, higher = faster, lower = slower).)"
    ),
    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(<Dragonbones Sprite Flip Left>/<Dragonbones Sprite Flip Right>/<Dragonbones Sprite No Flip Left>/<Dragonbones Sprite No Flip Right> - enables/disables automatic mirroring of the armature when facing left/right.)"
    ),
    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(<Dragonbones Sprite Motion Idle/Walk/Dash/Jump/LadderIdle/LadderClimb: animation> - maps the given movement state to a case-sensitive custom armature animation name.)"
    ),
    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(<Dragonbones Sprite Settings> - opens a consolidated block for specifying the filename, scale, time scale, flip behavior, and motion animations in one place (event page comments cap at 6 lines, so this may not fit).)"
    ),
    QT_TR_NOOP(
        DRAGONBONES_MAP_SPRITES_COMMENT_TAGS
        R"(</Dragonbones Sprite Settings> - closes a <Dragonbones Sprite Settings> block.)"
    ),
};

const static QRegularExpression REGEXP =
    QRegularExpression(PATTERN.toString(), QRegularExpression::CaseInsensitiveOption);
}  // namespace CommentTags