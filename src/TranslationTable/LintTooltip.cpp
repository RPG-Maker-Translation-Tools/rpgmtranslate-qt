#include "LintTooltip.hpp"

#include "PluginSequences.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"

#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QPainter>
#include <QScreen>
#include <QTextDocument>
#include <QWheelEvent>

namespace {
constexpr i32 Y_MARGIN = 8;
constexpr i32 H_PADDING = 8;
constexpr i32 V_PADDING = 4;
constexpr i32 MAX_HEIGHT = 160;
constexpr i32 MAX_WIDTH = 360;
constexpr i32 SCROLLBAR_WIDTH = 4;
constexpr i32 COLOR_SWATCH_SIZE = 12;

// Appends a key segment to `buf` (`PATH_BUFFER_SIZE` bytes, one for the terminator). An `Index` comes from typed text,
// so nothing is written unless the whole segment fits. False then, or for a non-ASCII index.
[[nodiscard]] constexpr auto appendSegment(char* const buf, u32& pos, const Segment seg) -> bool {
    usize needed = (pos != 0 ? 1 : 0) + 1;

    if (seg.type == SegmentType::String) {
        needed += seg.l1sv.size();
    } else if (seg.type == SegmentType::Index) {
        needed += seg.qsv.size();

        for (const QChar chr : seg.qsv) {
            if (chr.unicode() > 0x7F) {
                return false;
            }
        }
    } else {
        for (const auto name : seg.keys) {
            needed += name.size() + 1;
        }
    }

    if (pos + needed >= PATH_BUFFER_SIZE) {
        return false;
    }

    if (pos != 0) {
        buf[pos++] = '\x04';
    }

    if (seg.type == SegmentType::String) {
        buf[pos++] = '\x01';
        memcpy(buf + pos, seg.l1sv.data(), seg.l1sv.size());
        pos += scast<u32>(seg.l1sv.size());
    } else if (seg.type == SegmentType::Index) {
        buf[pos++] = '\x03';
        for (const QChar chr : seg.qsv) {
            buf[pos++] = chr.toLatin1();
        }
    } else {
        buf[pos++] = '\x01';

        for (const auto name : seg.keys) {
            memcpy(buf + pos, name.data(), name.size());
            pos += scast<u32>(name.size());
            buf[pos++] = '\x02';
        }

        buf[pos - 1] = '\0';
    }

    return true;
}

[[nodiscard]] inline auto parseCaptured(const QString& str, const i32 start, const i32 num) -> Segment {
    return QStringView(str).sliced(start).chopped(num);
}

constexpr QStringView SUGGESTION_ANCHOR_PREFIX = u"lint:";

// Text shown as it is in a Markdown link, so that a suggestion with `*` or `]` is not rendered as markup.
[[nodiscard]] inline auto escapeMarkdown(const QStringView text) -> QString {
    static constexpr QStringView SPECIAL = u"\\`*_[]<>&~";

    const auto isSpecial = [](const QChar chr) -> bool { return SPECIAL.contains(chr); };
    const isize specialCount = ranges::count_if(text, isSpecial);

    QString escaped;
    escaped.reserve(text.size() + specialCount);

    for (const QChar chr : text) {
        if (isSpecial(chr)) {
            escaped += u'\\';
        }

        escaped += chr;
    }

    return escaped;
}

[[nodiscard]] inline auto
suggestionLinksMarkdown(const QStringList& suggestions, const LintType type, const i32 lintIndex) -> QString {
    QStringList links;
    links.reserve(suggestions.size());

    for (i32 suggIdx = 0; suggIdx < suggestions.size(); suggIdx++) {
        links.append(u"[%1](%2%3,%4,%5)"_qsv.arg(
            escapeMarkdown(suggestions[suggIdx]),
            SUGGESTION_ANCHOR_PREFIX,
            qsv(qitos(scast<u8>(type))),
            qsv(qitos(lintIndex)),
            qsv(qitos(suggIdx))
        ));
    }

    return links.join(u", ");
}

[[nodiscard]] inline auto getPath(const QStringView filename, const ProjectSettings* sets) -> QByteArray {
    return (sets->actualSourcePath() % u'/' % filename % u'.' % sets->engineExtension()).toUtf8();
}

// A code whose first number is given by a variable code - `\N[\V[3]]`, `\C[\V[\V[1]]]`. The group holds the variable
// when it is a plain number; deeper nesting has none.
const QRegularExpression NESTED_NUMBER_REGEXP =
    QRegularExpression(uR"(^\\[A-Za-z]+\[\\V\[(?:(\d+)\](?=[\],]))?)"_s, QRegularExpression::CaseInsensitiveOption);

// The tooltips below are keyed by the position of the code in `TextCodes::PATTERN_PARTS`, so a pattern that is added or
// moved has to fail the build instead of showing the tooltip of another code.
constexpr i32 ACTOR_STATS_FIRST = 119;
constexpr i32 ACTOR_STATS_LAST = 153;
constexpr i32 ENEMY_STATS_FIRST = 154;
constexpr i32 ENEMY_STATS_LAST = 190;
constexpr i32 REAL_TIME_FIRST = 205;
constexpr i32 REAL_TIME_LAST = 215;

static_assert(matchesAt(TextCodes::PATTERN_PARTS[29], 0, uR"((\\P\[)"), "\\P moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[60], 0, uR"((\\pf\[)"), "\\pf moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[61], 0, uR"((\\pc\[)"), "\\pc moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[62], 0, uR"((\\pn\[)"), "\\pn moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[98], 0, uR"((\\msgparty\[)"), "\\msgparty moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[104], 0, uR"((\\autoparty\[)"), "\\autoparty moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[111], 0, uR"((\\playtime)"), "\\playtime moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[112], 0, uR"((\\map\[)"), "\\map moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[113], 0, uR"((\\qi\[)"), "\\qi moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[115], 0, uR"((\\qa\[)"), "\\qa moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[117], 0, uR"((\\caseSwitch\{)"), "\\caseSwitch moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[ACTOR_STATS_FIRST], 0, uR"((\\amhp\[)"), "\\amhp moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[120], 0, uR"((\\ahp\[)"), "\\ahp moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[123], 0, uR"((\\amp\[)"), "\\amp moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[126], 0, uR"((\\atp\[)"), "\\atp moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[ACTOR_STATS_LAST], 0, uR"((\\aexr\[)"), "\\aexr moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[ENEMY_STATS_FIRST], 0, uR"((\\emhp\[)"), "\\emhp moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[ENEMY_STATS_LAST], 0, uR"((\\eexr\[)"), "\\eexr moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[REAL_TIME_FIRST], 0, uR"((\\Year)"), "\\Year moved");
static_assert(matchesAt(TextCodes::PATTERN_PARTS[REAL_TIME_LAST], 0, uR"((\\Meridiem)"), "\\Meridiem moved");

// The first number of a code: `2` of `\P[2]`, `3` of `\caseSwitch{3?a:b}`. Empty when there is none.
[[nodiscard]] constexpr auto codeArgument(const QStringView captured) -> QStringView {
    isize start = captured.indexOf(u'[');

    if (start == -1) {
        start = captured.indexOf(u'{');
    }

    start++;
    isize end = start;

    while (end < captured.size() && captured[end].isDigit()) {
        end++;
    }

    return captured.sliced(start, end - start);
}

// `3661` frames -> `0:01:01`.
[[nodiscard]] inline auto playtimeText(const u32 frames) -> QString {
    const u32 seconds = frames / 60;
    const auto pad = [](const u32 value) -> QString {
        return value < 10 ? u"0%1"_qsv.arg(qsv(qitos(value))) : QString(qsv(qitos(value)));
    };

    return u"%1:%2:%3"_qsv.arg(qsv(qitos(seconds / 3600)), pad((seconds / 60) % 60), pad(seconds % 60));
}

struct EntityKind {
    const QStringView file;
    const RPGMFileType type;
};

[[nodiscard]] constexpr auto entityKindFor(const i32 lintIndex) -> EntityKind {
    // NOLINTBEGIN(readability-magic-numbers)
    switch (lintIndex) {
        case 63:
            return { .file = u"Classes", .type = RPGMFileType::Classes };
        case 64:
        case 69:
        case 113:
            return { .file = u"Items", .type = RPGMFileType::Items };
        case 65:
        case 70:
        case 114:
            return { .file = u"Weapons", .type = RPGMFileType::Weapons };
        case 66:
        case 71:
        case 115:
            return { .file = u"Armors", .type = RPGMFileType::Armors };
        case 67:
        case 72:
            return { .file = u"Skills", .type = RPGMFileType::Skills };
        case 68:
        case 73:
            return { .file = u"States", .type = RPGMFileType::States };
        case 83:
            return { .file = u"Enemies", .type = RPGMFileType::Enemies };
        default:
            std::unreachable();
    }
    // NOLINTEND(readability-magic-numbers)
}
}  // namespace

// What a number of a note tag, plugin command or comment tag refers to.
enum class Ref : u8 {
    None,
    Actor,
    Class,
    Skill,
    Item,
    Weapon,
    Armor,
    Enemy,
    State,
    CommonEvent,
    Animation,
    Icon,
    Switch,
    Variable,
    // An item, a weapon, an armor or a skill, depending on which of the words the tag has.
    ItemKind,
};

// The numbers of one tag, in written order. A list tag (`<Passive State: 1, 2, 3>`) has a kind per number; otherwise
// `kinds` covers the first numbers and the rest are plain values. `index` and `prefix` tie the tag to its pattern
// in `PATTERN_PARTS`, so a moved pattern fails the build instead of showing another tag's tooltip.
struct TagReference {
    QStringView prefix;
    array<Ref, 2> kinds;
    u16 index;
    bool list;
};

namespace {
constexpr usize MAX_REFERENCES = 8;

struct EntityFile {
    const QStringView folder;
    const RPGMFileType type;
    const bool icon;
};

[[nodiscard]] constexpr auto entityFileFor(const Ref kind) -> EntityFile {
    switch (kind) {
        case Ref::Actor:
            return { .folder = u"Actors", .type = RPGMFileType::Actors, .icon = false };
        case Ref::Class:
            return { .folder = u"Classes", .type = RPGMFileType::Classes, .icon = false };
        case Ref::Skill:
            return { .folder = u"Skills", .type = RPGMFileType::Skills, .icon = true };
        case Ref::Item:
            return { .folder = u"Items", .type = RPGMFileType::Items, .icon = true };
        case Ref::Weapon:
            return { .folder = u"Weapons", .type = RPGMFileType::Weapons, .icon = true };
        case Ref::Armor:
            return { .folder = u"Armors", .type = RPGMFileType::Armors, .icon = true };
        case Ref::Enemy:
            return { .folder = u"Enemies", .type = RPGMFileType::Enemies, .icon = false };
        case Ref::State:
            return { .folder = u"States", .type = RPGMFileType::States, .icon = true };
        case Ref::CommonEvent:
            return { .folder = u"CommonEvents", .type = RPGMFileType::Events, .icon = false };
        case Ref::Animation:
            return { .folder = u"Animations", .type = RPGMFileType::Invalid, .icon = false };
        default:
            std::unreachable();
    }
}

[[nodiscard]] consteval auto
referencesMatch(const span<const QStringView> patterns, const span<const TagReference> table) -> bool {
    for (const auto& reference : table) {
        if (reference.index >= patterns.size() || !matchesAt(patterns[reference.index], 1, reference.prefix)) {
            return false;
        }
    }

    return true;
}

// https://www.yanfly.moe/wiki/Category:Notetags_(MV)
constexpr auto NOTE_TAG_REFERENCES = std::to_array<TagReference>({
    TagReference{ .prefix = uR"(<Passive State: )", .kinds = { Ref::State }, .index = 47, .list = true },
    { .prefix = uR"(<Passive State: )", .kinds = { Ref::State, Ref::State }, .index = 48, .list = false },
    { .prefix = uR"(<Passive Condition: Switch )", .kinds = { Ref::Switch }, .index = 51, .list = false },
    { .prefix = uR"(<Passive Condition: Variable )", .kinds = { Ref::Variable }, .index = 52, .list = false },
    { .prefix = uR"(<Extra Skill List: )", .kinds = { Ref::Skill }, .index = 71, .list = true },
    { .prefix = uR"(<(?:Physical |Magical |Certain )?Follow Up Skill: )",
      .kinds = { Ref::Skill },
      .index = 80,
      .list = false },
    { .prefix = uR"(<State Immunity: )", .kinds = { Ref::State }, .index = 81, .list = true },
    { .prefix = uR"(<Dissolve State: )", .kinds = { Ref::State }, .index = 87, .list = true },
    { .prefix = uR"(<Cast Animation: )", .kinds = { Ref::Animation }, .index = 91, .list = false },
    { .prefix = uR"(<Reflect Animation ID: )", .kinds = { Ref::Animation }, .index = 92, .list = false },
    { .prefix = uR"(<Attack Animation: )", .kinds = { Ref::Animation }, .index = 95, .list = false },
    { .prefix = uR"(<CTB Icon: )", .kinds = { Ref::Icon }, .index = 135, .list = false },
    { .prefix = uR"(<Class \d+? CTB Icon: )", .kinds = { Ref::Class, Ref::Icon }, .index = 137, .list = false },
    { .prefix = uR"(<Counter Icon: )", .kinds = { Ref::Icon }, .index = 173, .list = false },
    { .prefix = uR"(<(?:Begin|End) Action: Animation )", .kinds = { Ref::Animation }, .index = 180, .list = false },
    { .prefix = uR"(<(?:Begin|End) Action: (?:Add|Remove) State )",
      .kinds = { Ref::State },
      .index = 187,
      .list = false },
    { .prefix = uR"(<Turn Order Icon: )", .kinds = { Ref::Icon }, .index = 188, .list = false },
    { .prefix = uR"(<Class \d+? Turn Order Icon: )", .kinds = { Ref::Class, Ref::Icon }, .index = 190, .list = false },
    { .prefix = uR"(<(?:Regen|DoT) Animation: )", .kinds = { Ref::Animation }, .index = 237, .list = false },
    { .prefix = uR"(<Protection Animation: )", .kinds = { Ref::Animation }, .index = 242, .list = false },
    { .prefix = uR"(<(?:HP|MP|Both) Protection: Trigger Add State )",
      .kinds = { Ref::State },
      .index = 251,
      .list = false },
    { .prefix = uR"(<(?:HP|MP|Both) Protection: Trigger Remove State )",
      .kinds = { Ref::State },
      .index = 252,
      .list = false },
    { .prefix = uR"(<State Animation: )", .kinds = { Ref::Animation }, .index = 258, .list = false },
    { .prefix = uR"(<Card (?:Hand|Discard|Exhaust|Library) (?:Start|Upkeep|Draw|End) Common Event: )",
      .kinds = { Ref::CommonEvent },
      .index = 289,
      .list = false },
    { .prefix = uR"(<Card Spell \d+? Common Event: )",
      .kinds = { Ref::None, Ref::CommonEvent },
      .index = 293,
      .list = false },
    { .prefix = uR"(<Card Spell \d+? Icon: )", .kinds = { Ref::None, Ref::Icon }, .index = 296, .list = false },
    { .prefix = uR"(<Card Spell \d+? Animation: )",
      .kinds = { Ref::None, Ref::Animation },
      .index = 298,
      .list = false },
    { .prefix = uR"(<Card (?:Pre-Battle|Tie Battle|Lose Battle|Win Battle|Post-Battle|Resolve Battle) Common Event: )",
      .kinds = { Ref::CommonEvent },
      .index = 305,
      .list = false },
    { .prefix = uR"(<Unlock Class: )", .kinds = { Ref::Class }, .index = 306, .list = true },
    { .prefix = uR"(<Icon: )", .kinds = { Ref::Icon }, .index = 311, .list = false },
    { .prefix = uR"(<Subclass: )", .kinds = { Ref::Class }, .index = 317, .list = false },
    { .prefix = uR"(<Restrict (?:Class|Subclass): )", .kinds = { Ref::Class }, .index = 319, .list = true },
    { .prefix = uR"(<Require (?:Class|Subclass): )", .kinds = { Ref::Class }, .index = 323, .list = true },
    { .prefix = uR"(<(?:Physical |Magical |Certain Hit )?Counter State )",
      .kinds = { Ref::State },
      .index = 324,
      .list = false },
    { .prefix = uR"(<Base Parameters on Class: )", .kinds = { Ref::Class }, .index = 411, .list = false },
    { .prefix = uR"(<Base (?:maxhp|maxmp|atk|def|mat|mdf|agi|luk) Parameter on Class: )",
      .kinds = { Ref::Class },
      .index = 412,
      .list = false },
    { .prefix = uR"(<Equip State: )", .kinds = { Ref::State }, .index = 427, .list = true },
    { .prefix = uR"(<(?:Item|Weapon|Armor) )", .kinds = { Ref::ItemKind }, .index = 455, .list = false },
    { .prefix = uR"(<(?:Instant|Cancel Instant) (?:Skill|Item): )",
      .kinds = { Ref::ItemKind },
      .index = 472,
      .list = true },
    { .prefix = uR"(<(?:Instant|Cancel Instant) (?:Skill|Item): )",
      .kinds = { Ref::ItemKind },
      .index = 473,
      .list = true },
    { .prefix = uR"(<(?:Item|Weapon|Armor) Recipe: )", .kinds = { Ref::ItemKind }, .index = 537, .list = true },
    { .prefix = uR"(<Class \d+? Starting JP: )", .kinds = { Ref::Class }, .index = 544, .list = false },
    { .prefix = uR"(<Level Up Learn Skill: )", .kinds = { Ref::Skill }, .index = 550, .list = false },
    { .prefix = uR"(<Level Up Switch (?:On|Off): )", .kinds = { Ref::Switch }, .index = 551, .list = true },
});

// https://www.yanfly.moe/wiki/Category:Plugin_Commands_(MV)
constexpr auto PLUGIN_COMMAND_REFERENCES = std::to_array<TagReference>({
    TagReference{ .prefix = uR"(UnlockClass )", .kinds = { Ref::Actor, Ref::Class }, .index = 19, .list = false },
    { .prefix = uR"(RemoveClass )", .kinds = { Ref::Actor, Ref::Class }, .index = 20, .list = false },
    { .prefix = uR"(UnlockClassAll )", .kinds = { Ref::Actor }, .index = 21, .list = false },
    { .prefix = uR"(RemoveClassAll )", .kinds = { Ref::Actor }, .index = 22, .list = false },
    { .prefix = uR"((?:Enable|Disable)PrimaryClassChange )", .kinds = { Ref::Actor }, .index = 23, .list = false },
    { .prefix = uR"(SetCommonEventMenuCancel )", .kinds = { Ref::CommonEvent }, .index = 26, .list = false },
    { .prefix = uR"((?:Increase|Decrease)ActorBattleSlots )", .kinds = { Ref::Actor }, .index = 65, .list = false },
    { .prefix = uR"(OpenSynthesis (?:Item|Weapon|Armor) )", .kinds = { Ref::ItemKind }, .index = 101, .list = false },
    { .prefix = uR"(PlayerCompassIcon )", .kinds = { Ref::Icon }, .index = 116, .list = false },
    { .prefix = uR"(ChangeSubclass )", .kinds = { Ref::Actor, Ref::Class }, .index = 119, .list = false },
    { .prefix = uR"((?:EnableSubclassChange|DisableSubclassChange) )",
      .kinds = { Ref::Actor },
      .index = 120,
      .list = false },
});

// https://www.yanfly.moe/wiki/Category:Comment_Tags_(MV)
constexpr auto COMMENT_TAG_REFERENCES = std::to_array<TagReference>({
    TagReference{ .prefix = uR"(<Icon: )", .kinds = { Ref::Icon }, .index = 5, .list = false },
    { .prefix = uR"(<Icon on Event: )", .kinds = { Ref::Icon }, .index = 33, .list = false },
    { .prefix = uR"(<Compass Icon: )", .kinds = { Ref::Icon }, .index = 40, .list = false },
});

static_assert(referencesMatch(Notetags::PATTERN_PARTS, NOTE_TAG_REFERENCES), "A note tag moved.");
static_assert(referencesMatch(PluginCommands::PATTERN_PARTS, PLUGIN_COMMAND_REFERENCES), "A plugin command moved.");
static_assert(referencesMatch(CommentTags::PATTERN_PARTS, COMMENT_TAG_REFERENCES), "A comment tag moved.");
}  // namespace

template <typename... Segments>
    requires(std::same_as<Segments, Segment> && ...)
constexpr auto LintTooltip::buildPath(Segments&&... segments) -> FFIString {
    u32 pos = 0;

    // A key that doesn't fit is an empty one, which the lookup answers with an error
    if (!(appendSegment(buf.data(), pos, std::forward<Segments>(segments)) && ...)) {
        pos = 0;
    }

    buf[pos] = '\0';
    return strtoffi(buf.data());
}

LintTooltip::LintTooltip(QWidget* const parent) : QWidget(parent) {
    timer.setSingleShot(true);
    timer.setInterval(TOOLTIP_DELAY_MS);
    connect(&timer, &QTimer::timeout, this, &LintTooltip::hide_);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setMouseTracking(true);
}

LintTooltip::~LintTooltip() {
    releaseOut();
}

void LintTooltip::releaseOut() {
    for (auto& entry : out) {
        rpgm_string_free(entry);
        entry = FFIString{};
    }
}

auto LintTooltip::fetchEntityData(
    QString& markdownSource,
    const ProjectSettings* const sets,
    const QStringView folder,
    const RPGMFileType type,
    const FFIString& key
) -> bool {
    // `key` points into buf, never into out, so releasing first is safe, and needed since a case may fetch twice.
    releaseOut();

    // Per-entity files exist only in the other engines; RPG Maker 2000/2003 is served by `fetchRm2kName`.
    if (sets->engineType == EngineType::RM2K) {
        markdownSource += tr("This data is not available in RPG Maker 2000/2003 projects.");
        return false;
    }

    const QByteArray path = getPath(folder, sets);
    const QByteArray encoding = sets->readEncoding.toUtf8();

    if (!rpgm_get_entity_data(
            strtoffi(path),
            sets->engineType,
            type,
            key,
            strtoffi(encoding),
            out.data(),
            scast<u32>(out.size())
        )) {
        markdownSource += tr("Failed to fetch data: %1").arg(svtostr(ffitostr(rpgm_error())));
        return false;
    }
    return true;
}

void LintTooltip::processTextCode(QString& markdownSource, const LintEntry& lint, const ProjectSettings* const sets) {
    const auto fetchEntity = [&](const QStringView folder, const RPGMFileType type, const FFIString& key) -> bool {
        return fetchEntityData(markdownSource, sets, folder, type, key);
    };

    const auto fetchRm2kName = [&](const Rm2kEntity entity, const i32 id) -> bool {
        releaseOut();

        const QByteArray path = sets->rm2kDatabasePath().toUtf8();
        const QByteArray encoding = sets->readEncoding.toUtf8();

        if (!rpgm_get_rm2k_entity_name(strtoffi(path), entity, id, strtoffi(encoding), out.data())) {
            markdownSource += tr("Failed to fetch data: %1").arg(svtostr(ffitostr(rpgm_error())));
            return false;
        }

        return true;
    };

    const auto variableName = [&](const QStringView id) -> optional<QString> {
        if (sets->engineType == EngineType::RM2K) {
            if (!fetchRm2kName(Rm2kEntity::Variables, stoa<i32>(id).value_or(0))) {
                return nullopt;
            }
        } else if (!fetchEntity(u"System", RPGMFileType::System, buildPath(Segment("variables"_L1), Segment(id)))) {
            return nullopt;
        }

        return ffitostr(out[0]).toString();
    };

    const auto fetchIcon = [&](const i32 iconIndex) -> optional<QString> {
        const auto result = getIcon(iconIndex, sets);

        if (!result) {
            markdownSource += tr("Failed to fetch icon: %1").arg(result.error().translated());
            return std::nullopt;
        }

        return insertPixmap(result.value());
    };

    const auto colorSwatch = [&](const QRgb color) -> QString {
        auto pixmap = QPixmap(COLOR_SWATCH_SIZE, COLOR_SWATCH_SIZE);
        pixmap.fill(color);
        return insertPixmap(pixmap);
    };

    // The active save, or nothing, after the tooltip said why. `announce` is false for the codes that make sense
    // without a save and only show more with one.
    shared_ptr<const SaveSummary> save;
    const auto activeSave = [&](const bool announce) -> const SaveSummary* {
        if (save != nullptr) {
            return save.get();
        }

        if (sets->activeSave.isEmpty()) {
            if (announce) {
                markdownSource += tr("Pick an active save in the lint selection menu to see the value in it.");
            }

            return nullptr;
        }

        auto summary = getSaveSummary(sets);

        if (!summary) {
            markdownSource += tr("Failed to read the save: %1").arg(summary.error().translated());
            return nullptr;
        }

        save = std::move(summary.value());
        return save.get();
    };

    const auto savedActor = [&](const SaveSummary& summary, const i32 actorId) -> const SaveActor* {
        const auto found = ranges::find(summary.actors, actorId, &SaveActor::actorId);
        return found == summary.actors.end() ? nullptr : &*found;
    };

    const auto databaseActorName = [&](const i32 actorId) -> optional<QString> {
        if (sets->engineType == EngineType::RM2K) {
            if (!fetchRm2kName(Rm2kEntity::Actors, actorId)) {
                return nullopt;
            }
        } else if (!fetchEntity(
                       u"Actors",
                       RPGMFileType::Actors,
                       buildPath(Segment(qsv(qitos(actorId))), Segment("name"_L1))
                   )) {
            return nullopt;
        }

        return ffitostr(out[0]).toString();
    };

    // What the game calls the actor: the name the save holds, since the game can rename an actor, else the database's.
    const auto actorName = [&](const SaveActor* const saved, const i32 actorId) -> optional<QString> {
        if (saved != nullptr && !saved->name.isEmpty()) {
            return saved->name;
        }

        return databaseActorName(actorId);
    };

    // The member `position` (1-based) of the party of the active save, with `member` pointing at its actor.
    const auto partyMember = [&](const QStringView position, const SaveActor*& member) -> optional<i32> {
        const SaveSummary* const summary = activeSave(true);

        if (summary == nullptr) {
            return nullopt;
        }

        const i32 index = stoa<i32>(position).value_or(0) - 1;

        if (index < 0 || index >= scast<i32>(summary->party.size())) {
            markdownSource += tr("The party of the active save has no member %1.").arg(position);
            return nullopt;
        }

        const i32 actorId = summary->party[index];
        member = savedActor(*summary, actorId);
        return actorId;
    };

    // RPG Maker 2000/2003 has no plugins to add codes: of the ones here, `\V`, `\N` and `\C` are the only ones that
    // need the game's data there.
    if (sets->engineType == EngineType::RM2K && lint.index != 27 && lint.index != 28 && lint.index != 31) {
        return;
    }

    // The game replaces `\V[n]` with its value first, so any number of any code may be one - and only the game knows
    // what it is. What is known is which variable it comes from.
    if (const auto nested = NESTED_NUMBER_REGEXP.match(lint.captured); nested.hasMatch()) {
        markdownSource += tr("A number of this code is given by a variable, so it is only known while the game runs.");

        if (nested.hasCaptured(1)) {
            if (const auto name = variableName(nested.capturedView(1))) {
                markdownSource += tr("\n\nVariable %1: %2").arg(nested.capturedView(1), *name);
            }
        }

        releaseOut();
        return;
    }

    // NOLINTBEGIN(readability-magic-numbers)
    switch (lint.index) {
        case 27: {
            const auto name = variableName(parseCaptured(lint.captured, 3, 1).qsv);

            if (!name) {
                break;
            }

            markdownSource += tr("The value of this variable will be displayed: %1").arg(*name);

            if (const SaveSummary* const summary = activeSave(false)) {
                const usize variableId = stoa<usize>(parseCaptured(lint.captured, 3, 1).qsv).value_or(0);
                const i32 value = variableId < summary->variables.size() ? summary->variables[variableId] : 0;
                markdownSource += tr("\n\nValue in the active save: %1").arg(qsv(qitos(value)));
            }

            break;
        }
        case 28:
        case 57:
        case 97:
        case 103: {
            i32 actorId = stoa<i32>(codeArgument(lint.captured)).value_or(0);

            // `\N` prints the name the actor has now, which the game can change; the others only point at an actor.
            const SaveSummary* const summary = lint.index == 28 ? activeSave(false) : nullptr;

            // Actor 0 is the leader of the party, not an entry of the database.
            if (sets->engineType == EngineType::RM2K && actorId == 0) {
                if (summary == nullptr || summary->party.empty()) {
                    markdownSource += tr("The name of the party leader will be displayed.");
                    break;
                }

                actorId = summary->party.front();
            }

            const auto name = actorName(summary != nullptr ? savedActor(*summary, actorId) : nullptr, actorId);

            if (!name) {
                break;
            }

            markdownSource +=
                (lint.index == 28 ? tr("This pattern will be replaced with: %1") : tr("Actor's name: %1")).arg(*name);
            break;
        }
        case 29:
        case 60:
        case 61:
        case 62:
        case 98:
        case 104: {
            const SaveActor* member = nullptr;
            const auto actorId = partyMember(codeArgument(lint.captured), member);

            if (!actorId) {
                break;
            }

            switch (lint.index) {
                case 61: {
                    // The class the actor has now, which the game can change.
                    const i32 classId = member != nullptr ? member->classId : 0;

                    if (!fetchEntity(
                            u"Classes",
                            RPGMFileType::Classes,
                            buildPath(Segment(qsv(qitos(classId))), Segment("name"_L1))
                        )) {
                        break;
                    }

                    markdownSource += tr("This pattern will be replaced with: %1").arg(svtostr(ffitostr(out[0])));
                    break;
                }
                case 62:
                    markdownSource += tr("This pattern will be replaced with: %1")
                                          .arg(member != nullptr ? member->nickname : QString());
                    break;
                default: {
                    const auto name = actorName(member, *actorId);

                    if (name) {
                        markdownSource +=
                            (lint.index == 29 ? tr("This pattern will be replaced with: %1") : tr("Party member: %1"))
                                .arg(*name);
                    }

                    break;
                }
            }

            break;
        }
        case 30: {
            const FFIString keyUtf8 = buildPath(Segment("currency_unit"_L1));
            if (!fetchEntity(u"System", RPGMFileType::System, keyUtf8)) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 31: {
            bool okay = true;
            const u64 color = lint.extra.toUInt(&okay);

            if (okay) {
                markdownSource += tr("The following color will take effect: %1").arg(colorSwatch(color));
            } else {
                markdownSource += tr("Failed to fetch data: %1").arg(lint.extra.toString());
            }

            break;
        }
        case 32: {
            const auto icon = fetchIcon(stoa<i32>(parseCaptured(lint.captured, 3, 1).qsv).value_or(0));

            if (!icon) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(insertPixmap(icon.value()));
            break;
        }
        case 43: {
            const u32 frameNum = stoa<u32>(parseCaptured(lint.captured, 3, 1).qsv).value_or(0);
            markdownSource += tr("Here, the game will wait %n frame(s) (%1 sec.)", nullptr, scast<i32>(frameNum))
                                  .arg(qsv(qftos(scast<f32>(frameNum) / 60, 3)));
            break;
        }
        case 58: {
            const FFIString keyUtf8 =
                buildPath(parseCaptured(lint.captured, 4, 1), Segment("name"_L1), Segment("class_id"_L1));

            if (!fetchEntity(u"Actors", RPGMFileType::Actors, keyUtf8)) {
                break;
            }

            if (!fetchEntity(
                    u"Classes",
                    RPGMFileType::Classes,
                    buildPath(Segment(QL1SV(out[1].ptr, out[1].len)), Segment("name"_L1), Segment("1"_L1))
                )) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 59: {
            static constexpr array<QL1SV, 2> keys = { "name"_L1, "nickname"_L1 };
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 4, 1), Segment(keys));
            if (!fetchEntity(u"Actors", RPGMFileType::Actors, keyUtf8)) {
                break;
            }

            markdownSource +=
                tr("This pattern will be replaced with: %2").arg(svtostr(ffitostr(out[0])), svtostr(ffitostr(out[1])));
            break;
        }
        case 63:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 83: {
            const auto [file, type] = entityKindFor(lint.index);
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 4, 1), Segment("name"_L1));
            if (!fetchEntity(file, type, keyUtf8)) {
                break;
            }

            markdownSource += (lint.index == 83 ? tr("Enemy name: %1") : tr("This pattern will be replaced with: %1"))
                                  .arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 69:
        case 70:
        case 71:
        case 72:
        case 73: {
            const auto [file, type] = entityKindFor(lint.index);
            static constexpr array<QL1SV, 2> keys = { "name"_L1, "icon_index"_L1 };
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 4, 1), Segment(keys));

            if (!fetchEntity(file, type, keyUtf8)) {
                break;
            }

            const auto icon = fetchIcon(stoa<i32>(ffitostr(out[1])).value_or(0));
            if (!icon) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1%2").arg(svtostr(ffitostr(out[0])), *icon);
            break;
        }
        case 109: {
            const QStringView num = QStringView(lint.captured).sliced(4).chopped(1);
            static const QLocale usLocale(QLocale::English, QLocale::UnitedStates);
            markdownSource +=
                tr("This pattern will be replaced with: %1").arg(usLocale.toString(stoa<u16>(num).value_or(0)));
            break;
        }
        case 110: {
            const QStringView rgbHex = QStringView(lint.captured).sliced(4).chopped(1);
            markdownSource +=
                tr("The following color will take effect: %1").arg(colorSwatch(stoa<u32>(rgbHex, 16).value_or(0)));
            break;
        }
        case 84:
        case 99:
        case 105:
            markdownSource += tr("The enemy is given by its position in the troop, so it is only known in a battle.");
            break;
        case 111: {
            const SaveSummary* const summary = activeSave(true);

            if (summary == nullptr) {
                break;
            }

            markdownSource += tr("Playtime in the active save: %1").arg(playtimeText(summary->frames));
            break;
        }
        case 112: {
            // The code comes from an MV plugin; the other engines have no MapInfos to look the name up in.
            if (sets->engineType != EngineType::MVMZ) {
                break;
            }

            i32 mapId = stoa<i32>(codeArgument(lint.captured)).value_or(0);

            // Map 0 is the map the player is on.
            if (mapId == 0) {
                const SaveSummary* const summary = activeSave(true);

                if (summary == nullptr) {
                    break;
                }

                mapId = summary->mapId;
            }

            if (!fetchEntity(
                    u"MapInfos",
                    RPGMFileType::Map,
                    buildPath(Segment(qsv(qitos(mapId))), Segment("name"_L1))
                )) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 113:
        case 114:
        case 115: {
            const SaveSummary* const summary = activeSave(true);

            if (summary == nullptr) {
                break;
            }

            const i32 id = stoa<i32>(codeArgument(lint.captured)).value_or(0);
            const auto& stacks = lint.index == 113   ? summary->items
                                 : lint.index == 114 ? summary->weapons
                                                     : summary->armors;
            const auto stack = ranges::find(stacks, id, &SaveStack::id);
            const auto [file, type] = entityKindFor(lint.index);

            QString name = u"#%1"_qsv.arg(qsv(qitos(id)));

            if (fetchEntity(file, type, buildPath(Segment(qsv(qitos(id))), Segment("name"_L1)))) {
                name = ffitostr(out[0]).toString();
            }

            markdownSource +=
                tr("The party of the active save has %n of %1.", nullptr, stack == stacks.end() ? 0 : stack->count)
                    .arg(name);
            break;
        }
        case 117: {
            const SaveSummary* const summary = activeSave(true);

            if (summary == nullptr) {
                break;
            }

            const QStringView switchId = codeArgument(lint.captured);
            const usize index = stoa<usize>(switchId).value_or(0);
            const bool on = index < summary->switches.size() && summary->switches[index];

            markdownSource += (on ? tr("Switch %1 is ON in the active save, so the first text is displayed.")
                                  : tr("Switch %1 is OFF in the active save, so the second text is displayed."))
                                  .arg(switchId);
            break;
        }
        default: {
            if (lint.index >= ACTOR_STATS_FIRST && lint.index <= ACTOR_STATS_LAST) {
                const i32 actorId = stoa<i32>(codeArgument(lint.captured)).value_or(0);
                const SaveSummary* const summary = activeSave(false);
                const SaveActor* const saved = summary != nullptr ? savedActor(*summary, actorId) : nullptr;

                if (const auto name = actorName(saved, actorId)) {
                    markdownSource += tr("Actor: %1").arg(*name);
                }

                // The save holds the current HP, MP and TP; every other value is computed by the game from the class,
                // the level and the equipment of the actor.
                switch (lint.index) {
                    case 120:
                    case 123:
                    case 126: {
                        if (saved == nullptr) {
                            break;
                        }

                        const i32 value = lint.index == 120 ? saved->hp : lint.index == 123 ? saved->mp : saved->tp;

                        // RPG Maker 2000/2003 stores -1 for the maximum.
                        if (value >= 0) {
                            markdownSource += tr("\n\nValue in the active save: %1").arg(qsv(qitos(value)));
                        }

                        break;
                    }
                    default:
                        markdownSource += tr(
                            "\n\nThe game computes this value from the class, the level and the equipment of the actor."
                        );
                        break;
                }
            } else if (lint.index >= ENEMY_STATS_FIRST && lint.index <= ENEMY_STATS_LAST) {
                markdownSource +=
                    tr("The enemy is given by its position in the troop, so it is only known in a battle.");
            } else if (lint.index >= REAL_TIME_FIRST && lint.index <= REAL_TIME_LAST) {
                static constexpr array<QStringView, 11> FORMATS = { u"yyyy", u"yy", u"MMMM", u"M",  u"d", u"dddd",
                                                                    u"H",    u"h",  u"mm",   u"ss", u"AP" };
                static const QLocale usLocale(QLocale::English, QLocale::UnitedStates);

                markdownSource +=
                    tr("At this moment, it would be replaced with: %1")
                        .arg(usLocale.toString(QDateTime::currentDateTime(), FORMATS[lint.index - REAL_TIME_FIRST]));
            }

            break;
        }
    }
    // NOLINTEND(readability-magic-numbers)

    releaseOut();
}

[[nodiscard]] auto LintTooltip::insertPixmap(const QPixmap& pixmap) -> QString {
    // A tooltip with more images than that just goes without the rest.
    if (resourceCounter == resources.size()) {
        return {};
    }

    auto url = QUrl(u"icon:%1"_qsv.arg(qsv(qitos(resourceCounter))));

    document->addResource(QTextDocument::ImageResource, url, pixmap);
    QString img = u"<img src=\"%1\" />"_qsv.arg(url.toString());

    resources[resourceCounter++] = { std::move(url), pixmap };

    return img;
};

void LintTooltip::processReferences(
    QString& markdownSource,
    const LintEntry& lint,
    const ProjectSettings* const sets,
    const span<const TagReference> table
) {
    // The plugins the tags are of are made for MV/MZ, and so are the files the names are read from.
    if (sets->engineType != EngineType::MVMZ) {
        return;
    }

    const auto reference = ranges::find(table, lint.index, &TagReference::index);

    if (reference == table.end()) {
        return;
    }

    const QStringView text = lint.captured;

    array<i32, MAX_REFERENCES> ids;
    usize count = 0;

    for (isize pos = 0; pos < text.size() && count < ids.size();) {
        if (!text[pos].isDigit()) {
            pos++;
            continue;
        }

        isize end = pos + 1;

        while (end < text.size() && text[end].isDigit()) {
            end++;
        }

        ids[count++] = stoa<i32>(text.sliced(pos, end - pos)).value_or(0);
        pos = end;
    }

    Ref itemKind = Ref::Item;

    if (text.contains(u"weapon", Qt::CaseInsensitive)) {
        itemKind = Ref::Weapon;
    } else if (text.contains(u"armor", Qt::CaseInsensitive)) {
        itemKind = Ref::Armor;
    } else if (text.contains(u"skill", Qt::CaseInsensitive)) {
        itemKind = Ref::Skill;
    }

    for (usize pos = 0; pos < count; pos++) {
        Ref kind = Ref::None;

        if (reference->list) {
            kind = reference->kinds[0];
        } else if (pos < reference->kinds.size()) {
            kind = reference->kinds[pos];
        }

        if (kind == Ref::ItemKind) {
            kind = itemKind;
        }

        if (kind == Ref::None) {
            continue;
        }

        const i32 id = ids[pos];
        QString label;

        if (kind == Ref::Icon) {
            const auto icon = getIcon(id, sets);

            if (!icon) {
                markdownSource += tr("Failed to fetch icon: %1").arg(icon.error().translated());
                break;
            }

            label = insertPixmap(icon.value());
        } else if (kind == Ref::Switch || kind == Ref::Variable) {
            const QL1SV list = kind == Ref::Switch ? "switches"_L1 : "variables"_L1;

            if (!fetchEntityData(
                    markdownSource,
                    sets,
                    u"System",
                    RPGMFileType::System,
                    buildPath(Segment(list), Segment(qsv(qitos(id))))
                )) {
                break;
            }

            label = ffitostr(out[0]).toString();
        } else {
            const auto [folder, type, hasIcon] = entityFileFor(kind);
            static constexpr array<QL1SV, 2> NAME_AND_ICON = { "name"_L1, "icon_index"_L1 };

            const FFIString key = hasIcon ? buildPath(Segment(qsv(qitos(id))), Segment(NAME_AND_ICON))
                                          : buildPath(Segment(qsv(qitos(id))), Segment("name"_L1));

            if (!fetchEntityData(markdownSource, sets, folder, type, key)) {
                break;
            }

            label = ffitostr(out[0]).toString();

            // The first icon is "no icon".
            if (hasIcon) {
                if (const i32 iconIndex = stoa<i32>(ffitostr(out[1])).value_or(0); iconIndex != 0) {
                    if (const auto icon = getIcon(iconIndex, sets)) {
                        label = insertPixmap(icon.value()) % u' ' % label;
                    }
                }
            }
        }

        markdownSource += u"- %1: %2\n"_qsv.arg(qsv(qitos(id)), label);
    }

    releaseOut();
}

void LintTooltip::showAt(const QPoint& cursorPos, const QList<LintEntry>& lints, const ProjectSettings* const projS) {
    document = make_unique<QTextDocument>(this);
    resourceCounter = 0;

    QString markdownSource;

    for (const auto& lint : lints) {
        const char* const msg = lint.translationId;

        if (lint.type == LintType::Custom || lint.type == LintType::LanguageToolCheck) {
            // Custom and LanguageTool lints carry a pre-formatted tooltip rather than a translation id.
            markdownSource += lint.extra.toString();
        } else if (msg != nullptr && *msg != 0) {
            markdownSource += QCoreApplication::translate("Lint", msg);
        } else {
            markdownSource += tr("Not yet implemented - work in progress.");
        }

        markdownSource += u"\n\n";

        switch (lint.type) {
            case TextCode: {
                processTextCode(markdownSource, lint, projS);
                break;
            }
            case NoteTag: {
                processReferences(markdownSource, lint, projS, NOTE_TAG_REFERENCES);
                break;
            }
            case PluginCommand: {
                processReferences(markdownSource, lint, projS, PLUGIN_COMMAND_REFERENCES);
                break;
            }
            case CommentTag: {
                processReferences(markdownSource, lint, projS, COMMENT_TAG_REFERENCES);
                break;
            }
            case Misc: {
                // Only the lint 4 has suggestions
                if (lint.index == 4) {
                    const QStringList suggestions = lint.suggestions.toStringList();
                    markdownSource +=
                        tr("Suggestions: %1").arg(suggestionLinksMarkdown(suggestions, lint.type, lint.index));
                    markdownSource += u"\n\n";
                }

                break;
            }
            case Custom: {
                // The whole tooltip is the user-written text emitted above.
                break;
            }
            case LanguageToolCheck: {
                const QStringList suggestions = lint.suggestions.toStringList();

                if (!suggestions.isEmpty()) {
                    markdownSource +=
                        tr("Suggestions: %1").arg(suggestionLinksMarkdown(suggestions, lint.type, lint.index));
                    markdownSource += u"\n\n";
                }

                break;
            }
        }

        markdownSource += u"\n\n";
    }

    document->setDocumentMargin(0);
    document->setMarkdown(markdownSource, QTextDocument::MarkdownDialectCommonMark);

    for (const auto& [url, resource] : span(resources.data(), resourceCounter)) {
        document->addResource(QTextDocument::ImageResource, url, resource);
    }

    document->setTextWidth(-1);  // measure unwrapped first
    const i32 naturalWidth = scast<i32>(document->idealWidth());
    const i32 textWidth = min(naturalWidth, MAX_WIDTH);
    document->setTextWidth(textWidth);

    contentHeight = scast<i32>(document->size().height());
    scrollOffset = 0;

    const i32 width = textWidth + (H_PADDING * 2);
    const i32 height = min(contentHeight + (V_PADDING * 2), MAX_HEIGHT);
    resize(width, height);

    QPoint pos = cursorPos;
    pos.setY(pos.y() - Y_MARGIN - height);

    if (const QScreen* const screen = qApp->screenAt(cursorPos)) {
        const QRect screenRect = screen->availableGeometry();

        if (pos.x() + width > screenRect.right()) {
            pos.setX(screenRect.right() - width);
        }

        if (pos.x() < screenRect.left()) {
            pos.setX(screenRect.left());
        }

        if (pos.y() < screenRect.top()) {
            pos.setY(screenRect.top());
        }

        if (pos.y() + height > screenRect.bottom()) {
            pos.setY(screenRect.bottom() - height);
        }
    }

    move(pos);
    show();
    update();
}

void LintTooltip::hide() {
    if (timer.isActive()) {
        hide_();
    }
}

void LintTooltip::hide_() {
    QWidget::hide();
    scrollOffset = 0;
}

void LintTooltip::waitForEnter() {
    timer.start();
}

void LintTooltip::enterEvent(QEnterEvent* const /* event */) {
    timer.stop();
}

void LintTooltip::leaveEvent(QEvent* const /* event */) {
    timer.start();
}

void LintTooltip::wheelEvent(QWheelEvent* const event) {
    const i32 viewportHeight = height() - (V_PADDING * 2);
    const i32 maxScroll = qMax(0, contentHeight - viewportHeight);

    scrollOffset = qBound(0, scrollOffset - (event->angleDelta().y() / 4), maxScroll);
    update();
    event->accept();
}

auto LintTooltip::textAreaRect() const -> QRect {
    return QRect(0, 0, width(), height()).adjusted(H_PADDING, V_PADDING, -H_PADDING, -V_PADDING);
}

auto LintTooltip::toDocumentPos(const QPoint& widgetPos) const -> QPointF {
    const QRect area = textAreaRect();
    return { widgetPos - area.topLeft() + QPoint(0, scrollOffset) };
}

void LintTooltip::mouseMoveEvent(QMouseEvent* const event) {
    const QString anchor = document->documentLayout()->anchorAt(toDocumentPos(event->pos()));
    setCursor(anchor.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
    QWidget::mouseMoveEvent(event);
}

void LintTooltip::mouseReleaseEvent(QMouseEvent* const event) {
    if (event->button() == Qt::LeftButton) {
        const QString anchor = document->documentLayout()->anchorAt(toDocumentPos(event->pos()));

        if (anchor.startsWith(SUGGESTION_ANCHOR_PREFIX)) {
            const QSVList parts = QStringView(anchor).sliced(SUGGESTION_ANCHOR_PREFIX.size()).split(u',');

            // Text from the game or a custom tooltip can make a `lint:` link of its own, with fewer parts
            if (parts.size() == 3) {
                emit suggestionActivated(
                    scast<LintType>(stoa<u8>(parts[0]).value_or(0)),
                    stoa<i32>(parts[1]).value_or(0),
                    stoa<i32>(parts[2]).value_or(0)
                );
            }
        } else if (!anchor.isEmpty()) {
            emit linkActivated(QUrl(anchor));
        }
    }

    QWidget::mouseReleaseEvent(event);
}

void LintTooltip::paintEvent(QPaintEvent* const /* event */) {
    if (document->isEmpty()) {
        return;
    }

    const auto bodyRect = QRect(0, 0, width(), height());
    const QPalette& pal = palette();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(pal.color(QPalette::Highlight));
    painter.setBrush(pal.color(QPalette::Base));
    painter.drawRect(bodyRect.adjusted(0, 0, -1, -1));

    const QRect textArea = textAreaRect();
    painter.setClipRect(textArea);
    painter.save();
    painter.translate(textArea.topLeft() - QPoint(0, scrollOffset));

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette = pal;
    document->documentLayout()->draw(&painter, ctx);

    painter.restore();
    painter.setClipping(false);

    const i32 maxScroll = contentHeight - textArea.height();

    if (maxScroll > 0) {
        const i32 trackHeight = textArea.height();
        const i32 thumbHeight = qMax(20, trackHeight * textArea.height() / contentHeight);
        const i32 thumbY = textArea.top() + ((scrollOffset * (trackHeight - thumbHeight)) / maxScroll);

        painter.setPen(Qt::NoPen);
        painter.setBrush(pal.color(QPalette::Highlight));
        painter
            .drawRoundedRect(QRect(bodyRect.right() - SCROLLBAR_WIDTH - 2, thumbY, SCROLLBAR_WIDTH, thumbHeight), 2, 2);
    }
}
