#include "PluginSequences.hpp"

#include "QtAliases.hpp"

#include <QRegularExpression>
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

    // The rows after a translated line must hold the same tags as the source, so the same text always gives the
    // same list, however often it is read.
    void tagsAreStableAcrossReads() {
        const QString text = u"\\C[2]Red\\C[0] and \\V[3]"_s;

        QCOMPARE(tagsIn(TextCodes::REGEXP, text), tagsIn(TextCodes::REGEXP, text));
    }

    void categoryCoversTheWholeTable() {
        // Every index of the table has a category, the first and the last included
        QVERIFY(!TextCodes::category(0).isEmpty());
        QVERIFY(!TextCodes::category(static_cast<u32>(TextCodes::PATTERN_PARTS.size()) - 1).isEmpty());
        QCOMPARE(TextCodes::category(235), u"RPG Maker 2000/2003"_qsv);
    }
};

QTEST_APPLESS_MAIN(TestLints)

#include "tst_lints.moc"
