#pragma once

#include "Aliases.hpp"

#include <QRegularExpression>

// 253 capturing groups total
namespace TextCodes {
    constexpr static QStringView PATTERN =
        // Advanced Text System script from modern algebra. Topic:
        // https://rmrk.net/index.php?topic=44525.0.html
        uR"((\\nb\{.+?\})|(\\nbl\{.+?\})|(\\nbr\{.+?\})|(\\nbt\{.+?\})|(\\nbb\{.+?\})|(\\nblt\{.+?\})|(\\nblb\{.+?\})|(\\nbrt\{.+?\})|(\\nbrb\{.+?\})|(\\fit)|(\\e\[\d+?\])|(\\et\[\d+?\])|(\\eb\[\d+?\])|(\\el\[\d+?\])|(\\er\[\d+?\])|(\\lse)|(\/lse)|(\\pse)|(\/pse)|(\\fse)|(\/fse)|(\\tse)|(\/tse)|(\\pSE\[.+?,\d+?,\d+?\])|(\\pME\[.+?,\d+?,\d+?\])|(\\pANIM\[\d+?,\d+?\])|(\\pBLN\[\d+?,\d+?\]))"

        // https://www.yanfly.moe/wiki/Message_Core_(YEP)
        uR"(|(\\V\[\d+?\])|(\\N\[\d+?\])|(\\P\[\d+?\])|(\\G)|(\\C\[\d+?\])|(\\I\[\d+?\])|(\\\{)|(\\\})|(\\\\)|(\\\$)|(\\\.)|(\\\|)|(\\!)|(\\>)|(\\<)|(\\\^)|(\\w\[\d+?\])|(\\n<.+?>)|(\\nc<.+?>)|(\\nr<.+?>)|(<br>)|(\\px\[\d+?\])|(\\py\[\d+?\])|(\\oc\[\d+?\])|(\\ow\[\d+?\])|(\\fr)|(\\fs\[\d+?\])|(\\fn<.+?>)|(\\fb)|(\\fi)|(\\af\[\d+?\])|(\\ac\[\d+?\])|(\\an\[\d+?\])|(\\pf\[\d+?\])|(\\pc\[\d+?\])|(\\pn\[\d+?\])|(\\nc\[\d+?\])|(\\ni\[\d+?\])|(\\nw\[\d+?\])|(\\na\[\d+?\])|(\\ns\[\d+?\])|(\\nt\[\d+?\])|(\\ii\[\d+?\])|(\\iw\[\d+?\])|(\\ia\[\d+?\])|(\\is\[\d+?\])|(\\it\[\d+?\]))"

        // https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)
        uR"(|(\\lson)|(\\lsoff)|(\\lsn<filename>)|(\\lsv\[\d+?\])|(\\lspi\[\d+?\])|(\\lspiv\[\d+?\])|(\\lspa\[\d+?\])|(\\lspav\[\d+?\])|(\\lsi\[\d+?\])|(\\en\[\d+?\])|(\\et\[\d+?\])|(\\n<.+?>)|(\\nc<.+?>)|(\\nr<.+?>)|(\\nd<.+?>)|(\\ndc<.+?>)|(\\ndr<.+?>)|(\\nt<.+?>)|(\\ntc<.+?>)|(\\ntr<.+?>)|(\\msgposx\[\d+?\])|(\\msgposx\[auto\])|(\\msgposy\[\d+?\])|(\\msgposy\[auto\])|(\\msgevent\[\d+?\])|(\\msgactor\[\d+?\])|(\\msgparty\[\d+?\])|(\\msgenemy\[\d+?\])|(\\msgwidth\[\d+?\])|(\\msgwidth\[auto\])|(\\msgrows\[\d+?\])|(\\msgrows\[auto\])|(\\auto)|(\\autoevent\[\d+?\])|(\\autoactor\[\d+?\])|(\\autoparty\[\d+?\])|(\\autoenemy\[\d+?\])|(\\msgreset)|(\\faceindex\[\d+?\])|(\\dg\[\d+?\])|(\\hc\[([a-fA-F0-9]{6})\])|(\\map\[\d+?\]))"

        // https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)
        uR"(|(\\qi\[\d+?\])|(\\qw\[\d+?\])|(\\qa\[\d+?\])|(\\compare<x:y>)|(\\compare1<x:y>)|(\\compare2<x:y>)|(\\compare3<x:y>)|(\\compare4<x:y>)|(\\compare5<x:y>)|(\\compare6<x:y>)|(\\compare7<x:y>)|(\\compare8<x:y>)|(\\compare9<x:y>)|(\\caseSwitch\{s\?x:y\})|(\\caseEval\{e\?x:y\})|(\\amhp\[\d+?\])|(\\ahp\[\d+?\])|(\\ahp%\[\d+?\])|(\\ammp\[\d+?\])|(\\amp\[\d+?\])|(\\amp%\[\d+?\])|(\\amtp\[\d+?\])|(\\atp\[\d+?\])|(\\atp%\[\d+?\])|(\\aatk\[\d+?\])|(\\adef\[\d+?\])|(\\amat\[\d+?\])|(\\amdf\[\d+?\])|(\\aagi\[\d+?\])|(\\ahit\[\d+?\])|(\\aeva\[\d+?\])|(\\acri\[\d+?\])|(\\acev\[\d+?\])|(\\amev\[\d+?\])|(\\amrf\[\d+?\])|(\\acnt\[\d+?\])|(\\ahrg\[\d+?\])|(\\amrg\[\d+?\])|(\\atrg\[\d+?\])|(\\emph\[\d+?\])|(\\ehp\[\d+?\])|(\\ehp%\[\d+?\])|(\\emmp\[\d+?\])|(\\emp\[\d+?\])|(\\emp%\[\d+?\])|(\\emtp\[\d+?\])|(\\etp\[\d+?\])|(\\etp\[\d+?\])|(\\eatk\[\d+?\])|(\\edef\[\d+?\])|(\\emat\[\d+?\])|(\\emdf\[\d+?\])|(\\eagi\[\d+?\])|(\\eluk\[\d+?\])|(\\eexp\[\d+?\])|(\\egold\[\d+?\])|(\\ehit\[\d+?\])|(\\eeva\[\d+?\])|(\\ecri\[\d+?\])|(\\ecev\[\d+?\])|(\\emev\[\d+?\])|(\\emrf\[\d+?\])|(\\ecnt\[\d+?\])|(\\ehrg\[\d+?\])|(\\emrg\[\d+?\])|(\\etrg\[\d+?\])|(\\etgr\[\d+?\])|(\\egrd\[\d+?\])|(\\erec\[\d+?\])|(\\epha\[\d+?\])|(\\emcr\[\d+?\])|(\\etcr\[\d+?\])|(\\epdr\[\d+?\])|(\\emdr\[\d+?\])|(\\efdr\[\d+?\])|(\\eexr\[\d+?\]))"

        // https://www.yanfly.moe/wiki/Message_Eval_Text_(YEP)
        uR"(|\\evalText<<.+?>>)"

        // https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)
        uR"(|\\th\[\d+?\])"

        // https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)
        uR"(|(\\bpDamage\[\d+?\])|(\\bpTurn\[\d+?\])|(\\bpRepeat\[\d+?\])|(\\bpAnalyze\[\d+?\])|(\\bpEffect\[\d+?\])|(\\bp\[.+?\])|(\\bp0\[.+?\])|(\\bp>\d+?\[.+?\])|(\\bp>=\d+?\[.+?\])|(\\bp=\d+?\[.+?\])|(\\bp<=\d+?\[.+?\])|(\\bp<\d+?\[.+?\]))"

        // https://www.yanfly.moe/wiki/Real_Time_System_(Irina)
        uR"(|(\\Year)|(\\YearA)|(\\Month)|(\\MonthID)|(\\Date)|(\\WeekDay)|(\\Hour)|(\\Hour24)|(\\Hour12)|(\\Minute)|(\\Second)|(\\Meridiem))"

        // https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)
        uR"(|(\\bust\[\d+?\])|(\\bustClear\[\d+?, \d+?\])|(\\bustExp\[\d+?, .+?\])|(\\bustExpression\[\d+?, .+?\])|(\\bustAni\[\d+?, .+?\])|(\\bustAnimation\[\d+?, .+?\])|(\\bustRepAni\[\d+?, .+?\])|(\\bustRepeatAnimation\[\d+?, .+?\])|(\\bustClearAni\[\d+?, .+?\])|(\\bustClearAnimation\[\d+?, .+?\])|(\\bustMoveTo\[\d+?, .+?\])|(\\bustMoveBy\[\d+?, .+?\])|(\\bustMoveHome\[\d+?, .+?\])|(\\bustMoveType\[\d+?, .+?\])|(\\bustFadeIn\[\d+?, .+?\])|(\\bustFadeOut\[\d+?, .+?\])|(\\bustOpacityTo\[\d+?, .+?\])|(\\bustOpacityBy\[\d+?, .+?\])|(\\bustSlideIn\[\d+?, .+?\])|(\\bustSlideInFromLeft\[\d+?, .+?\])|(\\bustSlideInFromRight\[\d+?, .+?\])|(\\bustSlideOut\[\d+?, .+?\])|(\\bustSlideOutToLeft\[\d+?, .+?\])|(\\bustSlideOutToRight\[\d+?, .+?\])|(\\bustMirror\[\d+?, .+?\])|(\\bustUnmirror\[\d+?, .+?\])|(\\bustMirrorToggle\[\d+?, .+?\])|(\\bustScaleTo\[\d+?, .+?\])|(\\bustScaleToX\[\d+?, .+?\])|(\\bustScaleToY\[\d+?, .+?\])|(\\bustScaleBy\[\d+?, .+?\])|(\\bustScaleByX\[\d+?, .+?\])|(\\bustScaleByY\[\d+?, .+?\])|(\\bustTone\[\d+?, .+?\])|(\\bustDim\[\d+?, .+?\])|(\\bustLight\[\d+?, .+?\]))";

    constexpr static array<cstr, 253> TOOLTIPS = {
        QT_TR_NOOP(
            R"(ATS Message Options: \nb{text} - Shows the namebox with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nbl{text} - Shows the namebox left of the message with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nbr{text} - Shows the namebox right of the message with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nbt{text} - Shows the namebox above the message with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nbb{text} - Shows the namebox below the message with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nblt{text} - Shows the namebox left and above with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nblb{text} - Shows the namebox left and below with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nbrt{text} - Shows the namebox right and above with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \nbrb{text} - Shows the namebox right and below with text displayed.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \fit - Fits the window to this message (:paragraph_format must be false).)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \e[n] - Sets the text box in reference to character n (0 = Player; >0 = Event).)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \et[n] - Sets the text box above character n (0 = Player; >0 = Event).)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \eb[n] - Sets the text box below character n (0 = Player; >0 = Event).)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \el[n] - Sets the text box to the left of character n (0 = Player; >0 = Event).)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \er[n] - Sets the text box to the right of character n (0 = Player; >0 = Event).)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \lse - Turns the letter by letter SE on.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: /lse - Turns the letter by letter SE off.)"
        ),
        QT_TR_NOOP(R"(ATS Message Options: \pse - Turns the pause SE on.)"),
        QT_TR_NOOP(R"(ATS Message Options: /pse - Turns the pause SE off.)"),
        QT_TR_NOOP(R"(ATS Message Options: \fse - Turns the finish SE on.)"),
        QT_TR_NOOP(R"(ATS Message Options: /fse - Turns the finish SE off.)"),
        QT_TR_NOOP(R"(ATS Message Options: \tse - Turns the terminate SE on.)"),
        QT_TR_NOOP(
            R"(ATS Message Options: /tse - Turns the terminate SE off.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \pSE[file,x,y] - Play the "file" SE at volume x and pitch y.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \pME[file,x,y] - Play the "file" ME at volume x and pitch y.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \pANIM[x,n] - Play the animation with ID n over character x.)"
        ),
        QT_TR_NOOP(
            R"(ATS Message Options: \pBLN[x,n] - Play the balloon with ID n over character x.)"
        ),

        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \V[n] is replaced by the value of the nth variable.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \N[n] is replaced by the name of the nth actor.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \P[n] is replaced by the name of the nth party member.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \G is replaced by the currency unit.)"
        ),
        QT_TR_NOOP(
            "Yanfly Message Core (YEP): \\C[n] draws the subsequent text in the nth color.\nThis tag should always be closed with \\C[0] tag to reset the color to default.\n\\C[n] uses the colors from img/system/Window.png file."
        ),
        QT_TR_NOOP(
            "Yanfly Message Core (YEP): \\I[n] is replaced by the nth icon.\n\\I[n] uses the icons from img/system/IconSet.rpgmvp file."
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \{ increases the text size by one step.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \} descreases the text size by one step.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \\ inserts a literal backslash (\).)"
        ),
        QT_TR_NOOP(R"(Yanfly Message Core (YEP): \$ opens the gold window.)"),
        QT_TR_NOOP(R"(Yanfly Message Core (YEP): \. waits 1/4th seconds.)"),
        QT_TR_NOOP(R"(Yanfly Message Core (YEP): \| waits 1 second.)"),
        QT_TR_NOOP(R"(Yanfly Message Core (YEP): \! waits for button input.)"),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \> displays the remaining text on same line all at once.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \< cancels the effect that displays text all at once.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \^ does not wait for input after displaying text.)"
        ),

        // Wait
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \w[x] waits x frames (60 frames = 1 second). Message window only.)"
        ),

        // NameWindow
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \n<x> creates a name box with x string. Left side. Works for message window only.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \nc<x> creates a name box with x string. Centered. Works for message window only.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \nr<x> creates a name box with x string. Right side. Works for message window only.)"
        ),

        // Line break
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): if using word wrap mode, this will cause a line break.)"
        ),

        // Position
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \px[x] sets x position of text to x.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \py[x] sets y position of text to x.)"
        ),

        // Outline
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \oc[x] sets outline color to x.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \ow[x] sets outline width to x.)"
        ),

        // Font
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \fr resets all font changes.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \fs[x] changes font size to x.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \fn<x> changes font name to x.)"
        ),
        QT_TR_NOOP(R"(Yanfly Message Core (YEP): \fb toggles font boldness.)"),
        QT_TR_NOOP(R"(Yanfly Message Core (YEP): \fi toggles font italic.)"),

        // Actor
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \af[x] shows face of actor x. Works for message window only.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \ac[x] writes out actor's class name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \an[x] writes out actor's nickname.)"
        ),

        // Party
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \pf[x] shows face of party member x. Works for message window only.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \pc[x] writes out party member x's class name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \pn[x] writes out party member x's nickname.)"
        ),

        // Names
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \nc[x] writes out class x's name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \ni[x] writes out item x's name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \nw[x] writes out weapon x's name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \na writes out armor x's name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \ns[x] writes out skill x's name.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \nt writes out state x's name.)"
        ),

        // Icon Names
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \ii[x] writes out item x's name including icon.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \iw[x] writes out weapon x's name including icon.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \ia[x] writes out armor x's weapon including icon.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \is[x] writes out skill x's name including icon.)"
        ),
        QT_TR_NOOP(
            R"(Yanfly Message Core (YEP): \it[x] writes out state x's name including icon.)"
        )

        // TODO
    };

    const static QRegularExpression REGEXP = QRegularExpression(
        PATTERN.toString(),
        QRegularExpression::CaseInsensitiveOption
    );
}  // namespace TextCodes

namespace AbsorptionBarrier {
    constexpr array NOTE_TAGS = {
        uR"(<User Barrier: \+\d+?>)",
        uR"(<Target Barrier: \+\d+?>)",

        uR"(<User Barrier: \-\d+?>)",
        uR"(<Target Barrier: \-\d+?>)",

        uR"(<User Barrier \d+? Turns: \+\d+?>)",
        uR"(<Target Barrier \d+? Turns: \+\d+?>)",

        uR"(<User Barrier \d+? Turns: \-\d+?>)",
        uR"(<Target Barrier \d+? Turns: \-\d+?>)",

        uR"(<Bypass Barrier>)",

        uR"(<Barrier Penetration: \d+?%>)",
        uR"(<Barrier Penetration: \d+?>)",

        uR"(<Barrier Penetration: \+\d+?%>)",
        uR"(<Barrier Penetration: \-\d+?%>)",

        uR"(<Barrier Penetration: \+\d+?>)",
        uR"(<Barrier Penetration: \-\d+?>)",

        uR"(<Barrier Points: \+\d+?>)",
        uR"(<Barrier Points: \-\d+?>)",

        uR"(<Barrier Points \d+? Turns: \+\d+?>)",
        uR"(<Barrier Points \d+? Turns: \-\d+?>)",

        uR"(<Barrier Regen: \+\d+?>)",
        uR"(<Barrier Regen: \-\d+?>)",

        uR"(<Barrier Regen \d+? Turns: \+\d+?>)",
        uR"(<Barrier Regen \d+? Turns: \-\d+?>)",
    };
}  // namespace AbsorptionBarrier

// CATEGORIES
constexpr array NOTE_TAGS = { uR"(<Menu Category: .+?(?:, .+?, .+?)?>)" };

namespace Notetags {
    constexpr static QStringView PATTERN =
        // https://www.yanfly.moe/wiki/Absorption_Barrier_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Action_Cutin_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Action_Sequence_Fast_Forward/Skip_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Auto_Passive_States_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Passive_Condition_Cases_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Passive_Aura_Effects_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Base_Parameter_Control_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_A.I._Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Actor_Auto_Battle_A.I._(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_Effects_Pack_1_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_Effects_Pack_2_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_Engine_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Animated_Sideview_Enemies_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_System_-_ATB_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Visual_ATB_Gauge_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_System_-_CTB_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Order_Turn_Battle_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_System_-_STB_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Counter_Control_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Action_Beginning_and_End_Effects_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Turn_Order_Display_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Visual_HP_Gauges_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Weak_Enemy_Poses_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_Impact_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Battle_Select_Cursor_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Bright_Effects_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Buffs_%26_States_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Extended_Damage_Over_Time_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Lunatic_Pack_-_State_Protection_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/State_Categories_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Visual_State_Effects_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Card_Game_Core_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Card_Game_Deck_Builder_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Card_Game_Mechanics_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Class_Change_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Subclass_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Counter_State_(Arisu)
        uR"()"

        // https://www.yanfly.moe/wiki/Core_Engine_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Damage_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Armor_Scaling_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Critical_Control_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Critical_Sway_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Dash_Toggle_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Disable_Auto_Shadow_Extended_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Dragonbones_Integration_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Dragonbones_Map_Sprites_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Element_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Enemy_Levels_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Difficulty_Slider_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Enemy_Base_Parameters_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Map_Enemy_Levels_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Enhanced_TP_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Equip_Battle_Skills_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Equip_Skill_Tiers_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Equip_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Change_Battle_Equip_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Equip_Requirements_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Equipment_Set_Bonuses_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Click_Trigger_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Copier_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Encounter_Aid_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Hitbox_Resize_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Sprite_Offset_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Step_Animation_Options_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Extra_Enemy_Drops_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Extra_Parameter_Formula_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Floor_Damage_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Footstep_Sounds_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Hit_Damage_Sounds_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Icons_on_Events_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Instant_Cast_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Concoctions_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Attachable_Augments_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Disassemble_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Discard_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Durability_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Menu_Categories_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Picture_Images_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Rename_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Requirements_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Upgrade_Slots_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Item_Synthesis_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Job_Points_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Level_Up_Growth_Effects_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Life_Steal_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Main_Menu_Actor_Events_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Map_Display_Name_Core_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Meta_Controls_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Move_Route_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Multi-Layer_HP_Gauge_(Arisu)
        uR"()"

        // https://www.yanfly.moe/wiki/Overkill_Bonus_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Provoke_Effect_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Recruiting_Board_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Region_Battlebacks_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Region_Events_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Region_Restrictions_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Vehicle_Restrictions_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Row_Formation_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/New_Game%2B_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Save_Event_Locations_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Sealed_Battle_Commands_(Arisu)
        uR"()"

        // https://www.yanfly.moe/wiki/Shop_Menu_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Conditional_Shop_Prices_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Hide/Show_Shop_Items_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/More_Currencies_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Skill_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Limited_Skill_Uses_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Multi-Type_Skills_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Lunatic_Pack_-_Skill_Rewards_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Party_Limit_Gauge_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Skill_Cooldowns_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Skill_Cost_Items_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Skill_Learn_System_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Skill_Mastery_Levels_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Slippery_Tiles_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Smart_Jump_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Special_Parameter_Formula_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Stat_Allocation_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/State_Tooltip_Display_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Actor_Variables_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/More_Status_Menu_Pages_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Profile_Status_Page_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Steal_%26_Snatch_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Swap_Enemies_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Target_Core_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Area_of_Effect_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Selection_Control_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Taunt_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Tool_Ring_System_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Unison_Attack_(Irina)
        uR"()"

        // https://www.yanfly.moe/wiki/Visual_Item_Inventory_(Arisu)
        uR"()"

        // https://www.yanfly.moe/wiki/Weakness_Display_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Weapon_Animation_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Weapon_Swap_System_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Weapon_Unleash_(YEP)
        uR"()";

    const static QRegularExpression REGEXP = QRegularExpression(
        PATTERN.toString(),
        QRegularExpression::CaseInsensitiveOption
    );
}  // namespace Notetags

namespace PluginCommands {
    // TODO: https://www.yanfly.moe/wiki/Category:Plugin_Commands_(MV)

    constexpr static QStringView PATTERN = uR"()";

    const static QRegularExpression REGEXP = QRegularExpression(
        PATTERN.toString(),
        QRegularExpression::CaseInsensitiveOption
    );
}  // namespace PluginCommands

namespace CommentTags {
    constexpr static QStringView PATTERN =
        // https://www.yanfly.moe/wiki/Battle_BGM_Control_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Common_Event_Menu_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Click_Trigger_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Hitbox_Resize_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Mini_Label_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Proximity_Activate_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Region_Trigger_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Sprite_Offset_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Event_Step_Animation_Options_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Icons_on_Events_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Keyboard_Config_(YEP)
        uR"()"

        // https://www.yanfly.moe/wiki/Proximity_Compass_(Olivia)
        uR"()"

        // https://www.yanfly.moe/wiki/Synchronized_Event_Movement_(Arisu)
        uR"()";

    const static QRegularExpression REGEXP = QRegularExpression(
        PATTERN.toString(),
        QRegularExpression::CaseInsensitiveOption
    );
}  // namespace CommentTags