#include "PluginSequences.hpp"

#include "QtAliases.hpp"

#include <QRegularExpression>
#include <QSet>
#include <QtTest>

namespace {
// The pattern builders are `consteval`: what they produce for the real tables is what these small ones show
constexpr array<QStringView, 3> PARTS = { u"a(\\d)", u"b", u"c(\\d)(\\d)" };
constexpr array<QStringView, 1> SINGLE_PART = { u"only" };

constexpr usize JOINED_SIZE = joinedPatternSize(PARTS);
constexpr auto JOINED = buildPattern<JOINED_SIZE>(PARTS);

constexpr usize REPLACED_SIZE = joinedPatternSize(PARTS, u"\\d", u"[0-9]");
constexpr auto REPLACED = buildPattern<REPLACED_SIZE>(PARTS, u"\\d", u"[0-9]");

constexpr usize SINGLE_SIZE = joinedPatternSize(SINGLE_PART);
constexpr auto SINGLE = buildPattern<SINGLE_SIZE>(SINGLE_PART);

constexpr auto asView(const auto& storage) -> std::u16string_view {
    return { storage.data(), storage.size() };
}

static_assert(asView(JOINED) == u"a(\\d)|b|c(\\d)(\\d)");
static_assert(asView(REPLACED) == u"a([0-9])|b|c([0-9])([0-9])");
// An empty needle replaces nothing, and a lone part gets no separator
static_assert(joinedPatternSize(PARTS, {}, u"x") == JOINED_SIZE);
static_assert(asView(SINGLE) == u"only");

static_assert(matchesAt(u"hello", 1, u"ell"));
static_assert(!matchesAt(u"hello", 3, u"llo"));
// A needle running past the end, and an empty one, never match
static_assert(!matchesAt(u"hello", 4, u"ox"));
static_assert(!matchesAt(u"hello", 0, u""));

// The tags a text holds, as the lint reads them: the first group of each match that took part in it
auto tagsIn(const QRegularExpression& regexp, const QStringView text) -> QStringList {
    QStringList tags;

    for (const auto& match : regexp.globalMatchView(text)) {
        for (i32 idx = 1; idx < match.lastCapturedIndex() + 1; idx++) {
            if (match.capturedLength(idx) > 0) {
                tags.append(match.captured(idx));
                break;
            }
        }
    }

    return tags;
}

struct Table {
    const QRegularExpression& regexp;
    span<const QStringView> parts;
    QStringView (*category)(u32);
    usize tooltips;
};

const array<Table, 4> TABLES = { {
    { TextCodes::REGEXP, TextCodes::PATTERN_PARTS, TextCodes::category, TextCodes::TOOLTIPS.size() },
    { Notetags::REGEXP, Notetags::PATTERN_PARTS, Notetags::category, Notetags::TOOLTIPS.size() },
    { PluginCommands::REGEXP, PluginCommands::PATTERN_PARTS, PluginCommands::category, PluginCommands::TOOLTIPS.size() },
    { CommentTags::REGEXP, CommentTags::PATTERN_PARTS, CommentTags::category, CommentTags::TOOLTIPS.size() },
} };

auto tableNamed(const QString& name) -> const Table& {
    return TABLES[name == u"text codes" ? 0 : name == u"notetags" ? 1 : name == u"plugin commands" ? 2 : 3];
}

// The index of the part that took the first match in `text`, -1 when nothing matched. The lint tells the parts apart by
// the number of the group that took part.
auto partOf(const QRegularExpression& regexp, const QString& text, QString* captured = nullptr) -> i32 {
    const auto match = regexp.match(text);

    for (i32 idx = 1; idx < match.lastCapturedIndex() + 1; idx++) {
        if (match.capturedLength(idx) > 0) {
            if (captured != nullptr) {
                *captured = match.captured(idx);
            }

            return idx - 1;
        }
    }

    return -1;
}
}  // namespace

class TestLints final : public QObject {
    Q_OBJECT

   private slots:
    void everyTableCompiles_data() {
        QTest::addColumn<QRegularExpression>("regexp");

        QTest::newRow("text codes") << TextCodes::REGEXP;
        QTest::newRow("notetags") << Notetags::REGEXP;
        QTest::newRow("plugin commands") << PluginCommands::REGEXP;
        QTest::newRow("comment tags") << CommentTags::REGEXP;
    }

    // A typo in one of the hundreds of patterns would make the whole alternation invalid, and the lint would find no
    // tags at all.
    void everyTableCompiles() {
        QFETCH(QRegularExpression, regexp);

        QVERIFY2(regexp.isValid(), qPrintable(regexp.errorString()));
    }

    void everyTableNamesItsPartsOneToOne_data() {
        QTest::addColumn<QString>("table");

        QTest::newRow("text codes") << u"text codes"_s;
        QTest::newRow("notetags") << u"notetags"_s;
        QTest::newRow("plugin commands") << u"plugin commands"_s;
        QTest::newRow("comment tags") << u"comment tags"_s;
    }

    // The lint finds the part of a match by the number of the group that took part, so one stray capturing group
    // shifts the tooltip, the category and the setting of every later part.
    void everyTableNamesItsPartsOneToOne() {
        QFETCH(QString, table);
        const Table& tab = tableNamed(table);

        QCOMPARE(tab.tooltips, tab.parts.size());
        QCOMPARE(tab.regexp.captureCount(), static_cast<i32>(tab.parts.size()));

        for (usize idx = 0; idx < tab.parts.size(); idx++) {
            const QRegularExpression part(tab.parts[idx].toString(), QRegularExpression::CaseInsensitiveOption);

            QVERIFY2(part.isValid(), qPrintable(u"part %1: %2"_s.arg(idx).arg(part.errorString())));
            QVERIFY2(part.captureCount() == 1, qPrintable(u"part %1 has %2 groups"_s.arg(idx).arg(part.captureCount())));
        }
    }

    void everyPartHasACategory_data() {
        everyTableNamesItsPartsOneToOne_data();
    }

    // category() ends in std::unreachable for an index past the table
    void everyPartHasACategory() {
        QFETCH(QString, table);
        const Table& tab = tableNamed(table);

        for (u32 idx = 0; idx < tab.parts.size(); idx++) {
            QVERIFY2(!tab.category(idx).isEmpty(), qPrintable(u"part %1"_s.arg(idx)));
        }
    }

    void theSamePartIsNeverListedTwice_data() {
        QTest::addColumn<QString>("table");
        QTest::addColumn<i32>("known");

        // Copies that already are in the tables, belonging to other plugins that share a tag. The second one never
        // gets a match: the first one takes the text. A new copy is a mistake of the same kind.
        QTest::newRow("text codes") << u"text codes"_s << 4;
        QTest::newRow("notetags") << u"notetags"_s << 6;
        QTest::newRow("plugin commands") << u"plugin commands"_s << 2;
        QTest::newRow("comment tags") << u"comment tags"_s << 0;
    }

    void theSamePartIsNeverListedTwice() {
        QFETCH(QString, table);
        QFETCH(i32, known);
        const Table& tab = tableNamed(table);
        QSet<QStringView> seen;
        i32 copies = 0;

        for (const QStringView part : tab.parts) {
            copies += seen.contains(part);
            seen.insert(part);
        }

        QCOMPARE(copies, known);
    }

    void tagsAreFoundByTheirOwnPart_data() {
        QTest::addColumn<QString>("table");
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("tag");

        QTest::newRow("notetag") << u"notetags"_s << u"Intro <Bypass Barrier> outro"_s << u"<Bypass Barrier>"_s;
        QTest::newRow("notetag with sign") << u"notetags"_s << u"<Barrier Points: -5>"_s << u"<Barrier Points: -5>"_s;
        QTest::newRow("notetag either user") << u"notetags"_s << u"<Target Barrier: +3>"_s << u"<Target Barrier: +3>"_s;
        QTest::newRow("notetag case") << u"notetags"_s << u"<bypass barrier>"_s << u"<bypass barrier>"_s;
        QTest::newRow("plugin command") << u"plugin commands"_s << u"EnableAugments"_s << u"EnableAugments"_s;
        QTest::newRow("plugin command argument") << u"plugin commands"_s << u"setBattleSys ATB"_s << u"setBattleSys ATB"_s;
        QTest::newRow("plugin command number")
            << u"plugin commands"_s << u"BrightEffects Bloom Scale 1.5 3"_s << u"BrightEffects Bloom Scale 1.5 3"_s;
        QTest::newRow("comment tag") << u"comment tags"_s << u"<Battle BGM Name: Boss>"_s << u"<Battle BGM Name: Boss>"_s;
        QTest::newRow("text code") << u"text codes"_s << u"a \\C[3] b"_s << u"\\C[3]"_s;
    }

    void tagsAreFoundByTheirOwnPart() {
        QFETCH(QString, table);
        QFETCH(QString, text);
        QFETCH(QString, tag);
        const Table& tab = tableNamed(table);

        QString captured;
        const i32 idx = partOf(tab.regexp, text, &captured);

        QVERIFY(idx >= 0);
        QCOMPARE(captured, tag);

        // Alone, the part the lint reports takes the same text
        const QRegularExpression part(tab.parts[static_cast<usize>(idx)].toString(), QRegularExpression::CaseInsensitiveOption);
        QCOMPARE(part.match(tag).captured(1), tag);
    }

    void textWithoutTagsMatchesNothing_data() {
        everyTableNamesItsPartsOneToOne_data();
    }

    void textWithoutTagsMatchesNothing() {
        QFETCH(QString, table);

        QCOMPARE(partOf(tableNamed(table).regexp, u"Just a sentence, <not a tag> and 100% plain."_s), -1);
    }

    void textCodes_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QStringList>("tags");

        QTest::newRow("plain text") << u"Plain words, nothing special."_s << QStringList();
        QTest::newRow("empty") << QString() << QStringList();
        QTest::newRow("color") << u"\\C[2]"_s << QStringList{ u"\\C[2]"_s };
        QTest::newRow("variable") << u"You have \\V[12] gold"_s << QStringList{ u"\\V[12]"_s };
        QTest::newRow("color pair") << u"\\C[2]Red\\C[0]"_s << QStringList{ u"\\C[2]"_s, u"\\C[0]"_s };
        QTest::newRow("case insensitive") << u"\\c[2]"_s << QStringList{ u"\\c[2]"_s };
    }

    void textCodes() {
        QFETCH(QString, text);
        QFETCH(QStringList, tags);

        QCOMPARE(tagsIn(TextCodes::REGEXP, text), tags);
    }


    // The categories change where the RPG Maker 2000/2003 codes start, and that one runs to the end of the table
    void categoryChangesWhereTheRpgMaker2000CodesStart() {
        const u32 last = static_cast<u32>(TextCodes::PATTERN_PARTS.size()) - 1;

        QVERIFY(TextCodes::category(234) != TextCodes::category(235));
        QCOMPARE(TextCodes::category(235), TextCodes::category(last));
        QCOMPARE(TextCodes::category(235), u"RPG Maker 2000/2003"_qsv);
    }
};

QTEST_APPLESS_MAIN(TestLints)

#include "tst_lints.moc"
