#pragma once

#include "Aliases.hpp"

#include <QRegularExpression>

namespace TextCodes {
// https://www.yanfly.moe/wiki/Category:Text_Codes_(MV)

static constexpr auto category(const u32 idx) -> QStringView {
    if (idx >= 235) {
        return u"RPG Maker 2000/2003";
    }
    if (idx >= 216) {
        return u"Visual Novel Busts (Irina)";
    }
    if (idx >= 205) {
        return u"Real Time System (Irina)";
    }
    if (idx >= 193) {
        return u"Boost Point System (Olivia)";
    }
    if (idx >= 192) {
        return u"In-Battle Status (YEP)";
    }
    if (idx >= 191) {
        return u"Message Eval Text (YEP)";
    }
    if (idx >= 113) {
        return u"Extended Message Pack 2 (YEP)";
    }
    if (idx >= 74) {
        return u"Extended Message Pack 1 (YEP)";
    }
    if (idx >= 27) {
        return u"Yanfly Message Core (YEP)";
    }
    if (idx >= 0) {
        return u"Advanced Text System (ATS)";
    }

    std::unreachable();
};

static constexpr array<QStringView, 237> PATTERN_PARTS = {
    // Advanced Text System script from modern algebra. Topic:
    // https://rmrk.net/index.php?topic=44525.0.html
    uR"((\\nb\{.+?\}))",
    uR"((\\nbl\{.+?\}))",
    uR"((\\nbr\{.+?\}))",
    uR"((\\nbt\{.+?\}))",
    uR"((\\nbb\{.+?\}))",
    uR"((\\nblt\{.+?\}))",
    uR"((\\nblb\{.+?\}))",
    uR"((\\nbrt\{.+?\}))",
    uR"((\\nbrb\{.+?\}))",
    uR"((\\fit))",
    uR"((\\e\[\d+?\]))",
    uR"((\\et\[\d+?\]))",
    uR"((\\eb\[\d+?\]))",
    uR"((\\el\[\d+?\]))",
    uR"((\\er\[\d+?\]))",
    uR"((\\lse))",
    uR"((\/lse))",
    uR"((\\pse))",
    uR"((\/pse))",
    uR"((\\fse))",
    uR"((\/fse))",
    uR"((\\tse))",
    uR"((\/tse))",
    uR"((\\pSE\[.+?,\d+?,\d+?\]))",
    uR"((\\pME\[.+?,\d+?,\d+?\]))",
    uR"((\\pANIM\[\d+?,\d+?\]))",
    uR"((\\pBLN\[\d+?,\d+?\]))",

    // https://www.yanfly.moe/wiki/Message_Core_(YEP)
    uR"((\\V\[\d+?\]))",
    uR"((\\N\[\d+?\]))",
    uR"((\\P\[\d+?\]))",
    uR"((\\G))",
    uR"((\\C\[\d+?\]))",
    uR"((\\I\[\d+?\]))",
    uR"((\\\{))",
    uR"((\\\}))",
    uR"((\\\\))",
    uR"((\\\$))",
    uR"((\\\.))",
    uR"((\\\|))",
    uR"((\\!))",
    uR"((\\>))",
    uR"((\\<))",
    uR"((\\\^))",
    uR"((\\w\[\d+?\]))",
    uR"((\\n<.+?>))",
    uR"((\\nc<.+?>))",
    uR"((\\nr<.+?>))",
    uR"((<br>))",
    uR"((\\px\[\d+?\]))",
    uR"((\\py\[\d+?\]))",
    uR"((\\oc\[\d+?\]))",
    uR"((\\ow\[\d+?\]))",
    uR"((\\fr))",
    uR"((\\fs\[\d+?\]))",
    uR"((\\fn<.+?>))",
    uR"((\\fb))",
    uR"((\\fi))",
    uR"((\\af\[\d+?\]))",
    uR"((\\ac\[\d+?\]))",
    uR"((\\an\[\d+?\]))",
    uR"((\\pf\[\d+?\]))",
    uR"((\\pc\[\d+?\]))",
    uR"((\\pn\[\d+?\]))",
    uR"((\\nc\[\d+?\]))",
    uR"((\\ni\[\d+?\]))",
    uR"((\\nw\[\d+?\]))",
    uR"((\\na\[\d+?\]))",
    uR"((\\ns\[\d+?\]))",
    uR"((\\nt\[\d+?\]))",
    uR"((\\ii\[\d+?\]))",
    uR"((\\iw\[\d+?\]))",
    uR"((\\ia\[\d+?\]))",
    uR"((\\is\[\d+?\]))",
    uR"((\\it\[\d+?\]))",

    // https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)
    uR"((\\lson))",
    uR"((\\lsoff))",
    uR"((\\lsn<.+?>))",
    uR"((\\lsv\[\d+?\]))",
    uR"((\\lspi\[\d+?\]))",
    uR"((\\lspiv\[\d+?\]))",
    uR"((\\lspa\[\d+?\]))",
    uR"((\\lspav\[\d+?\]))",
    uR"((\\lsi\[\d+?\]))",
    uR"((\\en\[\d+?\]))",
    uR"((\\et\[\d+?\]))",
    uR"((\\n<.+?>))",
    uR"((\\nc<.+?>))",
    uR"((\\nr<.+?>))",
    uR"((\\nd<.+?>))",
    uR"((\\ndc<.+?>))",
    uR"((\\ndr<.+?>))",
    uR"((\\nt<.+?>))",
    uR"((\\ntc<.+?>))",
    uR"((\\ntr<.+?>))",
    uR"((\\msgposx\[(?:\d+?|auto)\]))",
    uR"((\\msgposy\[(?:\d+?|auto)\]))",
    uR"((\\msgevent\[\d+?\]))",
    uR"((\\msgactor\[\d+?\]))",
    uR"((\\msgparty\[\d+?\]))",
    uR"((\\msgenemy\[\d+?\]))",
    uR"((\\msgwidth\[(?:\d+?|auto)\]))",
    uR"((\\msgrows\[(?:\d+?|auto)\]))",
    uR"((\\autoevent\[\d+?\]))",
    uR"((\\autoactor\[\d+?\]))",
    uR"((\\autoparty\[\d+?\]))",
    uR"((\\autoenemy\[\d+?\]))",
    uR"((\\auto))",
    uR"((\\msgreset))",
    uR"((\\faceindex\[\d+?\]))",
    uR"((\\dg\[\d+?\]))",
    uR"((\\hc\[[a-fA-F\d]{6}\]))",
    uR"((\\playtime))",
    uR"((\\map\[\d+?\]))",

    // https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)
    uR"((\\qi\[\d+?\]))",
    uR"((\\qw\[\d+?\]))",
    uR"((\\qa\[\d+?\]))",
    uR"((\\compare(?:\d)?<.+?:.+?>))",
    uR"((\\caseSwitch\{\d+?\?.+?:.+?\}))",
    uR"((\\caseEval\{.+?\?.+?:.+?\}))",
    uR"((\\amhp\[\d+?\]))",
    uR"((\\ahp\[\d+?\]))",
    uR"((\\ahp%\[\d+?\]))",
    uR"((\\ammp\[\d+?\]))",
    uR"((\\amp\[\d+?\]))",
    uR"((\\amp%\[\d+?\]))",
    uR"((\\amtp\[\d+?\]))",
    uR"((\\atp\[\d+?\]))",
    uR"((\\atp%\[\d+?\]))",
    uR"((\\aatk\[\d+?\]))",
    uR"((\\adef\[\d+?\]))",
    uR"((\\amat\[\d+?\]))",
    uR"((\\amdf\[\d+?\]))",
    uR"((\\aagi\[\d+?\]))",
    uR"((\\aluk\[\d+?\]))",
    uR"((\\ahit\[\d+?\]))",
    uR"((\\aeva\[\d+?\]))",
    uR"((\\acri\[\d+?\]))",
    uR"((\\acev\[\d+?\]))",
    uR"((\\amev\[\d+?\]))",
    uR"((\\amrf\[\d+?\]))",
    uR"((\\acnt\[\d+?\]))",
    uR"((\\ahrg\[\d+?\]))",
    uR"((\\amrg\[\d+?\]))",
    uR"((\\atrg\[\d+?\]))",
    uR"((\\atgr\[\d+?\]))",
    uR"((\\agrd\[\d+?\]))",
    uR"((\\arec\[\d+?\]))",
    uR"((\\apha\[\d+?\]))",
    uR"((\\amcr\[\d+?\]))",
    uR"((\\atcr\[\d+?\]))",
    uR"((\\apdr\[\d+?\]))",
    uR"((\\amdr\[\d+?\]))",
    uR"((\\afdr\[\d+?\]))",
    uR"((\\aexr\[\d+?\]))",
    uR"((\\emhp\[\d+?\]))",
    uR"((\\ehp\[\d+?\]))",
    uR"((\\ehp%\[\d+?\]))",
    uR"((\\emmp\[\d+?\]))",
    uR"((\\emp\[\d+?\]))",
    uR"((\\emp%\[\d+?\]))",
    uR"((\\emtp\[\d+?\]))",
    uR"((\\etp\[\d+?\]))",
    uR"((\\etp%\[\d+?\]))",
    uR"((\\eatk\[\d+?\]))",
    uR"((\\edef\[\d+?\]))",
    uR"((\\emat\[\d+?\]))",
    uR"((\\emdf\[\d+?\]))",
    uR"((\\eagi\[\d+?\]))",
    uR"((\\eluk\[\d+?\]))",
    uR"((\\eexp\[\d+?\]))",
    uR"((\\egold\[\d+?\]))",
    uR"((\\ehit\[\d+?\]))",
    uR"((\\eeva\[\d+?\]))",
    uR"((\\ecri\[\d+?\]))",
    uR"((\\ecev\[\d+?\]))",
    uR"((\\emev\[\d+?\]))",
    uR"((\\emrf\[\d+?\]))",
    uR"((\\ecnt\[\d+?\]))",
    uR"((\\ehrg\[\d+?\]))",
    uR"((\\emrg\[\d+?\]))",
    uR"((\\etrg\[\d+?\]))",
    uR"((\\etgr\[\d+?\]))",
    uR"((\\egrd\[\d+?\]))",
    uR"((\\erec\[\d+?\]))",
    uR"((\\epha\[\d+?\]))",
    uR"((\\emcr\[\d+?\]))",
    uR"((\\etcr\[\d+?\]))",
    uR"((\\epdr\[\d+?\]))",
    uR"((\\emdr\[\d+?\]))",
    uR"((\\efdr\[\d+?\]))",
    uR"((\\eexr\[\d+?\]))",

    // https://www.yanfly.moe/wiki/Message_Eval_Text_(YEP)
    uR"((\\evalText<<.+?>>))",

    // https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)
    uR"((\\th\[\d+?\]))",

    // https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)
    uR"((\\bpDamage\[\d+?\]))",
    uR"((\\bpTurn\[\d+?\]))",
    uR"((\\bpRepeat\[\d+?\]))",
    uR"((\\bpAnalyze\[\d+?\]))",
    uR"((\\bpEffect\[\d+?\]))",
    uR"((\\bp\[.+?\]))",
    uR"((\\bp0\[.+?\]))",
    uR"((\\bp>\d+?\[.+?\]))",
    uR"((\\bp>=\d+?\[.+?\]))",
    uR"((\\bp=\d+?\[.+?\]))",
    uR"((\\bp<=\d+?\[.+?\]))",
    uR"((\\bp<\d+?\[.+?\]))",

    // https://www.yanfly.moe/wiki/Real_Time_System_(Irina)
    uR"((\\Year))",
    uR"((\\YearA))",
    uR"((\\Month))",
    uR"((\\MonthID))",
    uR"((\\Date))",
    uR"((\\WeekDay))",
    uR"(((?:\\Hour|\\Hour24)))",
    uR"((\\Hour12))",
    uR"((\\Minute))",
    uR"((\\Second))",
    uR"((\\Meridiem))",

    // https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)
    uR"((\\bust\[\d+?\]))",
    uR"((\\bustClear\[\d+?(?:, \d+?)?\]))",
    uR"((\\(?:bustExp|bustExpression)\[\d+?, .+?\]))",
    uR"((\\(?:bustAni|bustAnimation)\[\d+?, .+?(?:, .+?(?:, \d+?)?)?\]))",
    uR"((\\(?:bustRepAni|bustRepeatAnimation)\[\d+?, .+?(?:, .+?(?:, \d+?)?)?\]))",
    uR"((\\(?:bustClearAni|bustClearAnimation)\[\d+?\]))",
    uR"((\\bustMoveTo\[\d+?, \d+?, \d+?(?:, \d+?)?\]))",
    uR"((\\bustMoveBy\[\d+?, (?:\+|-)\d+?, (?:\+|-)\d+?(?:, \d+?)?\]))",
    uR"((\\bustMoveHome\[\d+?(?:, \d+?)?\]))",
    uR"((\\bustMoveType\[\d+?, \d+?\]))",
    uR"((\\(?:bustFadeIn|bustFadeOut)\[\d+?(?:, \d+?)?\]))",
    uR"((\\bustOpacityTo\[\d+?, \d+?(?:, \d+?)?\]))",
    uR"((\\bustOpacityBy\[\d+?, (?:\+|-)\d+?(?:, \d+?)?\]))",
    uR"((\\(?:bustSlideIn|bustSlideInFromLeft|bustSlideInFromRight|bustSlideOut|bustSlideOutToLeft|bustSlideOutToRight)\[\d+?(?:, \d+?)?\]))",
    uR"((\\(?:bustMirror|bustUnmirror|bustMirrorToggle)\[\d+?\]))",
    uR"((\\(?:bustScaleTo|bustScaleToX|bustScaleToY)\[\d+?, [\d.]+?(?:, \d+?)?\]))",
    uR"((\\(?:bustScaleBy|bustScaleByX|bustScaleByY)\[\d+?, (?:\+|-)[\d.]+?(?:, \d+?)?\]))",
    uR"((\\bustTone\[\d+?, -?\d+?, -?\d+?, -?\d+?, -?\d+?(?:, \d+?)?\]))",
    uR"((\\(?:bustDim|bustLight)\[\d+?(?:, \d+?)?\]))",

    // RPG Maker 2000/2003-only codes. The rest (\V, \N, \C, \$, \., \|, \!, \>, \<, \^, \\) are shared with Message Core above.
    uR"((\\S\[\d+?\]))",
    uR"((\\_))",
};

// A number in a code can be a variable code, since the game expands every \V[n] first (`\N[\V[3]]` is valid). So each
// plain number matches as "a number, or a \V[n] group", which recurses to any depth.
static constexpr QStringView NUMBER = uR"(\d+?)";
static constexpr QStringView NUMBER_OR_VARIABLE = uR"((?:\d+?|(?28)))";
static_assert(
    matchesAt(PATTERN_PARTS[27], 0, uR"((\\V\[\d+?\]))") && PATTERN_PARTS[27].size() == 13,
    "Group 28 has to be the \\V[n] pattern, see NUMBER_OR_VARIABLE."
);

static constexpr usize PATTERN_SIZE = joinedPatternSize(PATTERN_PARTS, NUMBER, NUMBER_OR_VARIABLE);
static constexpr array<char16_t, PATTERN_SIZE> PATTERN_STORAGE =
    buildPattern<PATTERN_SIZE>(PATTERN_PARTS, NUMBER, NUMBER_OR_VARIABLE);
static constexpr QStringView PATTERN = QStringView(std::u16string_view(PATTERN_STORAGE.data(), PATTERN_STORAGE.size()));

static constexpr array<const char*, 237> TOOLTIPS = {
    //
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nb{text} - Shows the namebox with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nbl{text} - Shows the namebox left of the message with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nbr{text} - Shows the namebox right of the message with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nbt{text} - Shows the namebox above the message with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nbb{text} - Shows the namebox below the message with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nblt{text} - Shows the namebox left and above with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nblb{text} - Shows the namebox left and below with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nbrt{text} - Shows the namebox right and above with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\nbrb{text} - Shows the namebox right and below with text displayed.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\fit - Fits the window to this message (:paragraph_format must be false).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\e[n] - Sets the text box in reference to character n (0 = Player; >0 = Event).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\et[n] - Sets the text box above character n (0 = Player; >0 = Event).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\eb[n] - Sets the text box below character n (0 = Player; >0 = Event).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\el[n] - Sets the text box to the left of character n (0 = Player; >0 = Event).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\er[n] - Sets the text box to the right of character n (0 = Player; >0 = Event).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\lse - Turns the letter by letter SE on.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(/lse - Turns the letter by letter SE off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\pse - Turns the pause SE on.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(/pse - Turns the pause SE off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\fse - Turns the finish SE on.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(/fse - Turns the finish SE off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\tse - Turns the terminate SE on.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(/tse - Turns the terminate SE off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\pSE[file,x,y] - Play the "file" SE at volume x and pitch y.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\pME[file,x,y] - Play the "file" ME at volume x and pitch y.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\pANIM[x,n] - Play the animation with ID n over character x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[ATS Message Options](https://rmrk.net/index.php?topic=44525.0.html)\n\n"
        R"(\pBLN[x,n] - Play the balloon with ID n over character x.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\V[n] is replaced by the value of the nth variable.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\N[n] is replaced by the name of the nth actor.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\P[n] is replaced by the name of the nth party member.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\G is replaced by the currency unit.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        "\\C[n] draws the subsequent text in the nth color.\nThis tag should always be closed with \\C[0] tag to reset the color to default.\n\\C[n] uses the colors from img/system/Window.png file."
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        "\\I[n] is replaced by the nth icon.\n\\I[n] uses the icons from img/system/IconSet.rpgmvp file."
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\{ increases the text size by one step.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\} descreases the text size by one step.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\\ inserts a literal backslash (\).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\$ opens the gold window.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\. waits 1/4th seconds.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\| waits 1 second.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\! waits for button input.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\> displays the remaining text on same line all at once.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\< cancels the effect that displays text all at once.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(\^ does not wait for input after displaying text.)"
    ),

    // Wait
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Wait - \w[x] waits x frames (60 frames = 1 second). Message window only.)"
    ),

    // NameWindow
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Name Window - \n\<x\> creates a name box with x string. Left side. Works for message window only.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Name Window - \nc\<x\> creates a name box with x string. Centered. Works for message window only.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Name Window - \nr\<x\> creates a name box with x string. Right side. Works for message window only.)"
    ),

    // Line break
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Line Break - if using word wrap mode, <br> will cause a line break.)"
    ),

    // Position
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Position - \px[x] sets x position of text to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Position - \py[x] sets y position of text to x.)"
    ),

    // Outline
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Outline - \oc[x] sets outline color to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Outline - \ow[x] sets outline width to x.)"
    ),

    // Font
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Font - \fr resets all font changes.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Font - \fs[x] changes font size to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Font - \fn<x> changes font name to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Font - \fb toggles font boldness.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Font - \fi toggles font italic.)"
    ),

    // Actor
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Actor - \af[x] shows face of actor x. Works for message window only.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Actor - \ac[x] writes out actor x's class name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Actor - \an[x] writes out actor x's nickname.)"
    ),

    // Party
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Party - \pf[x] shows face of party member x. Works for message window only.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Party - \pc[x] writes out party member x's class name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Party - \pn[x] writes out party member x's nickname.)"
    ),

    // Names
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Names - \nc[x] writes out class x's name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Names - \ni[x] writes out item x's name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Names - \nw[x] writes out weapon x's name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Names - \na[x] writes out armor x's name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Names - \ns[x] writes out skill x's name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Names - \nt[x] writes out state x's name.)"
    ),

    // Icon Names
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Icon Names - \ii[x] writes out item x's name including icon.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Icon Names - \iw[x] writes out weapon x's name including icon.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Icon Names - \ia[x] writes out armor x's weapon including icon.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Icon Names - \is[x] writes out skill x's name including icon.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Yanfly Message Core (YEP)](https://www.yanfly.moe/wiki/Message_Core_(YEP)#Text_Codes)\n\n"
        R"(Icon Names - \it[x] writes out state x's name including icon.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lson turns letter sounds on.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lsoff turns letter sounds off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lsn[filename] changes the letter sound to play the specified filename (case sensitive, no extension).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lsv[x] sets the volume of the letter sound to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lspi[x] sets the pitch of the letter sound to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lspiv[x] sets the pitch variance of the letter sound to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lspa[x] sets the pan of the letter sound to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lspav[x] sets the pan variance of the letter sound to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\lsi[x] sets the interval at which letter sounds play to x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\en[x] displays the name of the enemy at database position x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\et[x] displays the name of the enemy at troop position x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\n<x> creates a name box with text x on the left side.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\nc<x> creates a name box with text x centered.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\nr<x> creates a name box with text x on the right side.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\nd<x> creates a dimmed name box with text x on the left side.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\ndc<x> creates a dimmed name box with text x centered.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\ndr<x> creates a dimmed name box with text x on the right side.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\nt<x> creates a transparent name box with text x on the left side.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\ntc<x> creates a transparent name box with text x centered.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\ntr<x> creates a transparent name box with text x on the right side.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgposx[x] sets the X position of the message window to x. Use 'auto' to reset to default.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgposy[x] sets the Y position of the message window to x. Use 'auto' to reset to default.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgevent[x] positions the message window relative to event x on the map (0 = player). Map only.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgactor[x] places the message over actor x's head in battle. Requires Battle Engine Core.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgparty[x] places the message over party member x's head in battle. Requires Battle Engine Core.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgenemy[x] places the message over enemy x's head in battle. Requires Battle Engine Core.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgwidth[x] sets the width of the message window to x. Use 'auto' to fit the text (not compatible with Word Wrap).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgrows[x] sets the number of rows in the message window to x. Use 'auto' to fit the text (not compatible with Word Wrap).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\autoevent[x] fits the message window to its content and positions it relative to event x (0 = player). Map only. Not compatible with Word Wrap.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\autoactor[x] fits the message window to its content and places it over actor x's head in battle.

**Requires Battle Engine Core.**

**Not compatible with Word Wrap.**)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\autoparty[x] fits the message window to its content and places it over party member x's head in battle. Requires Battle Engine Core. Not compatible with Word Wrap.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\autoenemy[x] fits the message window to its content and places it over enemy x's head in battle. Requires Battle Engine Core. Not compatible with Word Wrap.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\auto sets the message window width and rows to fit the current message. Not compatible with Word Wrap.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\msgreset resets all message window positioning settings to their defaults.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\faceindex[x] changes the face graphic index to x at the start of the message.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\dg[x] displays number x formatted with comma separators (e.g. 12345 becomes 12,345).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\hc[rrggbb] changes the text color to the specified hex color code (e.g. \hc[ff0000] for red).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\playtime displays the player's current playtime.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 1 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_1_(YEP)#Text_Codes)\n\n"
        R"(\map[x] displays the name of map ID x. Use 0 for the current map.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\qi[x] writes out the quantity of item x the party currently has.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\qw[x] writes out the quantity of weapon x the party currently has.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\qa[x] writes out the quantity of armor x the party currently has.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\compare<x:y> / compareN<x:y> changes text color based on comparison.

\compare: x >= y = Green, x < y = Red.

\compare1: x >= y = Red, x < y = White.

\compare2: x >= y = Yellow, x < y = White.

\compare3: x >= y = Green, x < y = White.

\compare4: x >= y = Blue, x < y = Purple.

\compare5: x >= y = White, x < y = Grey.

\compare6: x >= y = White, x < y = Red.

\compare7: x >= y = White, x < y = Purple.

\compare8: x >= y = White, x < y = Dark Blue.

\compare9: x >= y = White, x < y = Brown.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\caseSwitch{s?x:y} returns text x if switch s is on, or text y if switch s is off.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\caseEval{e?x:y} returns text x if eval code e is true, or text y if eval code e is false.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amhp[x] writes out the Max HP value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ahp[x] writes out the current HP value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ahp%[x] writes out the HP rate of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ammp[x] writes out the Max MP value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amp[x] writes out the current MP value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amp%[x] writes out the MP rate of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amtp[x] writes out the Max TP value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\atp[x] writes out the current TP value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\atp%[x] writes out the TP rate of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\aatk[x] writes out the ATK (Attack) value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\adef[x] writes out the DEF (Defense) value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amat[x] writes out the MAT (Magic Attack) value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amdf[x] writes out the MDF (Magic Defense) value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\aagi[x] writes out the AGI (Agility) value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\aluk[x] writes out the LUK (Luck) value of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ahit[x] writes out the HIT (Hit Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\aeva[x] writes out the EVA (Evasion Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\acri[x] writes out the CRI (Critical Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\acev[x] writes out the CEV (Critical Evasion Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amev[x] writes out the MEV (Magic Evasion Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amrf[x] writes out the MRF (Magic Reflection Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\acnt[x] writes out the CNT (Counter Attack Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ahrg[x] writes out the HRG (HP Regeneration Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amrg[x] writes out the MRG (MP Regeneration Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\atrg[x] writes out the TRG (TP Regeneration Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\atgr[x] writes out the TGR (Target Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\agrd[x] writes out the GRD (Guard Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\arec[x] writes out the REC (Recovery Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\apha[x] writes out the PHA (Pharmacology Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amcr[x] writes out the MCR (MP Cost Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\atcr[x] writes out the TCR (TP Charge Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\apdr[x] writes out the PDR (Physical Damage Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\amdr[x] writes out the MDR (Magical Damage Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\afdr[x] writes out the FDR (Floor Damage Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\aexr[x] writes out the EXR (Experience Rate) of actor x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emhp[x] writes out the Max HP value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ehp[x] writes out the current HP value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ehp%[x] writes out the HP rate of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emmp[x] writes out the Max MP value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emp[x] writes out the current MP value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emp%[x] writes out the MP rate of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emtp[x] writes out the Max TP value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\etp[x] writes out the current TP value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\etp%[x] writes out the TP rate of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\eatk[x] writes out the ATK (Attack) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\edef[x] writes out the DEF (Defense) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emat[x] writes out the MAT (Magic Attack) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emdf[x] writes out the MDF (Magic Defense) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\eagi[x] writes out the AGI (Agility) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\eluk[x] writes out the LUK (Luck) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\eexp[x] writes out the EXP (Experience) value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\egold[x] writes out the GOLD value of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ehit[x] writes out the HIT (Hit Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\eeva[x] writes out the EVA (Evasion Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ecri[x] writes out the CRI (Critical Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ecev[x] writes out the CEV (Critical Evasion Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emev[x] writes out the MEV (Magic Evasion Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emrf[x] writes out the MRF (Magic Reflection Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ecnt[x] writes out the CNT (Counter Attack Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\ehrg[x] writes out the HRG (HP Regeneration Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emrg[x] writes out the MRG (MP Regeneration Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\etrg[x] writes out the TRG (TP Regeneration Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\etgr[x] writes out the TGR (Target Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\egrd[x] writes out the GRD (Guard Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\erec[x] writes out the REC (Recovery Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\epha[x] writes out the PHA (Pharmacology Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emcr[x] writes out the MCR (MP Cost Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\etcr[x] writes out the TCR (TP Charge Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\epdr[x] writes out the PDR (Physical Damage Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\emdr[x] writes out the MDR (Magical Damage Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\efdr[x] writes out the FDR (Floor Damage Rate) of enemy x.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Extended Message Pack 2 (YEP)](https://www.yanfly.moe/wiki/Extended_Message_Pack_2_(YEP)#Text_Codes)\n\n"
        R"(\eexr[x] writes out the EXR (Experience Rate) of enemy x.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Message Eval Text (YEP)](https://www.yanfly.moe/wiki/Message_Eval_Text_(YEP)#Text_Codes)\n\n"
        R"(\evalText<<code>> replaces 'code' with JavaScript code. It will run the code inside, then return whatever is the last line of the code that's ran.

# Example

\evalText<<$gameActors.actor(1).atk + $gameActors.actor(2).atk>> - Displays the sum of actor 1's ATK and actor 2's ATK.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[In-Battle Status (YEP)](https://www.yanfly.moe/wiki/In-Battle_Status_(YEP)#Text_Codes)\n\n"
        R"(\th[x] is replaced by the text used in state x's help description.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bpDamage[x] applies BP damage multipliers to x based on actor's current BP.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bpTurn[x] applies BP turn multipliers to x based on actor's current BP.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bpRepeat[x] applies BP repeat multipliers to x based on actor's current BP.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bpAnalyze[x] applies BP analyze multipliers to x based on actor's current BP.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bpEffect[x] applies BP effect multipliers to x based on actor's current BP.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp[text] shows text only if at least 1 BP is used.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp0[text] shows text only if no BP is being used.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp>x[text] shows text only if more than x BP is being used.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp>=x[text] shows text only if x or more BP is being used.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp=x[text] shows text only if exactly x BP is being used.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp<=x[text] shows text only if x or fewer BP is being used.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Boost Point System (Olivia)](https://www.yanfly.moe/wiki/Boost_Point_System_(Olivia)#Text_Codes)\n\n"
        R"(\bp<x[text] shows text only if less than x BP is being used.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Year writes out the current year.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\YearA writes out the last two digits of the current year.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Month writes out the current month's name.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\MonthID writes out the current month as a number.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Date writes out the current date number.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\WeekDay writes out the current day of the week.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Hour or \Hour24 writes out the current hour in 24-hour format.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Hour12 writes out the current hour in 12-hour format.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Minute writes out the current minute, padded if under 10.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Second writes out the current second, padded if under 10.)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Real Time System (Irina)](https://www.yanfly.moe/wiki/Real_Time_System_(Irina)#Text_Codes)\n\n"
        R"(\Meridiem writes out AM or PM based on the current time.)"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bust[id] displays the selected face graphic as a bust.

# Arguments
- id: `0` for the message window bust, `1`-`10` for screen busts.

# Examples
- \bust[0]
- \bust[3])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustClear[id, *duration*] removes a bust from view and resets it to default settings.

# Arguments
- id: bust ID to clear.
- duration *(optional)*: fade-out length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustClear[0]
- \bustClear[3, 30])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustExp[id, expression] or \bustExpression[id, expression] changes the expression of a bust.

# Arguments
- id: bust ID you want to affect.
- expression: number index or the string associated with the expression.

# Examples
- \bustExp[0, 1]
- \bustExp[2, Angry])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustAni[id, ani, *mirror*, *delay*] or \bustAnimation[id, ani, *mirror*, *delay*] plays a battle animation on a bust once.

# Arguments
- id: bust ID to animate.
- ani: animation ID from the database.
- mirror *(optional)*: pass `mirror` to flip the animation horizontally.
- delay *(optional)*: delay in frames before playback starts.

# Examples
- \bustAni[0, 5]
- \bustAni[2, 5, mirror, 10])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustRepAni[id, ani, *mirror*, *delay*] or \bustRepeatAnimation[id, ani, *mirror*, *delay*] plays a battle animation on a bust repeatedly until the event ends or it is cleared.

# Arguments
- id: bust ID to animate.
- ani: animation ID from the database.
- mirror *(optional)*: pass `mirror` to flip the animation horizontally.
- delay *(optional)*: delay in frames between repeats.

# Examples
- \bustRepAni[0, 5]
- \bustRepAni[2, 5, mirror, 10])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustClearAni[id] / \bustClearAnimation[id] stops a repeating battle animation on the specified bust.

# Arguments
- id: bust ID to stop animating.

# Examples
- \bustClearAni[0])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustMoveTo[id, x, y, *duration*] moves a screen bust to specific coordinates. Does not work for the message bust (id 0).

# Arguments
- id: screen bust ID (`1`-`10`).
- x: target X coordinate, in pixels.
- y: target Y coordinate, in pixels.
- duration *(optional)*: movement length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustMoveTo[1, 400, 300]
- \bustMoveTo[1, 400, 300, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustMoveBy[id, x, y, *duration*] moves a screen bust relative to its current position. Does not work for the message bust (id 0).

# Arguments
- id: screen bust ID (`1`-`10`).
- x: horizontal offset in pixels; sign (`+`/`-`) sets direction.
- y: vertical offset in pixels; sign (`+`/`-`) sets direction.
- duration *(optional)*: movement length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustMoveBy[1, +50, -20]
- \bustMoveBy[1, +50, -20, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustMoveHome[id, *duration*] returns a screen bust to its original home position. Does not work for the message bust (id 0).

# Arguments
- id: screen bust ID (`1`-`10`).
- duration *(optional)*: movement length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustMoveHome[1]
- \bustMoveHome[1, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustMoveType[id, type] sets the easing curve used by subsequent bust movement (`\bustMoveTo`, `\bustMoveBy`, `\bustMoveHome`). Affects all busts, including the message bust.

# Arguments
- id: bust ID whose movement easing is being set.
- type: easing name, one of the values below.

# Easing types
| Family | In | Out | InOut |
|---|---|---|---|
| Linear | `Linear` (no acceleration) | - | - |
| Sine | `InSine` | `OutSine` | `InOutSine` |
| Quad (LV2) | `InQuad` | `OutQuad` | `InOutQuad` |
| Cubic (LV3) | `InCubic` | `OutCubic` | `InOutCubic` |
| Quart (LV4) | `InQuart` | `OutQuart` | `InOutQuart` |
| Quint (LV5) | `InQuint` | `OutQuint` | `InOutQuint` |
| Expo | `InExpo` | `OutExpo` | `InOutExpo` |
| Circ | `InCirc` | `OutCirc` | `InOutCirc` |
| Back | `InBack` | `OutBack` | `InOutBack` |
| Elastic | `InElastic` | `OutElastic` | `InOutElastic` |
| Bounce | `InBounce` | `OutBounce` | `InOutBounce` |

`In` accelerates from zero velocity, `Out` decelerates to zero velocity, `InOut` accelerates then decelerates.

# Examples
- \bustMoveType[1, OutQuad])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustFadeIn[id, *duration*] / \bustFadeOut[id, *duration*] fades a bust in to full opacity (255) or out to zero opacity (0).

# Arguments
- id: bust ID to fade.
- duration *(optional)*: fade length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustFadeIn[0]
- \bustFadeOut[0, 30])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustOpacityTo[id, opacity, *duration*] sets a bust's opacity to an exact value.

# Arguments
- id: bust ID to affect.
- opacity: target opacity, `0`-`255`.
- duration *(optional)*: transition length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustOpacityTo[0, 128]
- \bustOpacityTo[0, 128, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustOpacityBy[id, opacity, *duration*] adjusts a bust's opacity relative to its current value.

# Arguments
- id: bust ID to affect.
- opacity: amount to adjust by; sign (`+`/`-`) sets direction.
- duration *(optional)*: transition length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustOpacityBy[0, -50]
- \bustOpacityBy[0, -50, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustSlideIn[id, *duration*] / \bustSlideInFromLeft[id, *duration*] / \bustSlideInFromRight[id, *duration*] / \bustSlideOut[id, *duration*] / \bustSlideOutToLeft[id, *duration*] / \bustSlideOutToRight[id, *duration*] slides a bust into or out of the scene with a combined move and fade effect from its home position.

# Arguments
- id: bust ID to slide.
- duration *(optional)*: slide length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustSlideIn[1]
- \bustSlideOutToLeft[1, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustMirror[id] / \bustUnmirror[id] / \bustMirrorToggle[id] flips, unflips, or toggles a bust horizontally.

# Arguments
- id: bust ID to mirror.

# Examples
- \bustMirrorToggle[0])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustScaleTo[id, scale, *duration*] / \bustScaleToX[id, scale, *duration*] / \bustScaleToY[id, scale, *duration*] scales a bust as a whole, or only horizontally/vertically, to an exact size.

# Arguments
- id: bust ID to scale.
- scale: target scale factor, e.g. `0.5` for 50%, `1.0` for 100%.
- duration *(optional)*: transition length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustScaleTo[0, 1.2]
- \bustScaleToX[0, 0.8, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustScaleBy[id, scale, *duration*] / \bustScaleByX[id, scale, *duration*] / \bustScaleByY[id, scale, *duration*] adjusts a bust's whole, horizontal, or vertical scale relative to its current value.

# Arguments
- id: bust ID to scale.
- scale: amount to adjust the scale factor by; sign (`+`/`-`) sets direction.
- duration *(optional)*: transition length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustScaleBy[0, +0.1]
- \bustScaleByY[0, -0.2, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustTone[id, red, green, blue, gray, *duration*] applies a color tint to a bust.

# Arguments
- id: bust ID to tint.
- red, green, blue: color channel adjustments, `-255` to `255`.
- gray: grayscale amount, `0` to `255`.
- duration *(optional)*: transition length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustTone[0, -68, -68, 0, 0]
- \bustTone[0, -68, -68, 0, 0, 20])"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[Visual Novel Busts (Irina)](https://www.yanfly.moe/wiki/Visual_Novel_Busts_(Irina)#Text_Codes)\n\n"
        R"(\bustDim[id, *duration*] / \bustLight[id, *duration*] applies the preset dim or light tone to a bust, typically used to mark a speaker as inactive/active.

# Arguments
- id: bust ID to affect.
- duration *(optional)*: transition length in frames. Defaults to the plugin parameter value if omitted.

# Examples
- \bustDim[0]
- \bustLight[0, 20])"
    ),

    QT_TRANSLATE_NOOP(
        "Lint",
        "[RPG Maker 2000/2003](https://rpgmaker.net/tutorials/43/)\n\n"
        R"(\S[n] sets the text speed to n, from 1 to 20 (1 is the fastest).)"
    ),
    QT_TRANSLATE_NOOP(
        "Lint",
        "[RPG Maker 2000/2003](https://rpgmaker.net/tutorials/43/)\n\n"
        R"(\_ inserts a half-width space.)"
    )
};

const static QRegularExpression REGEXP =
    QRegularExpression(PATTERN.toString(), QRegularExpression::CaseInsensitiveOption);
}  // namespace TextCodes