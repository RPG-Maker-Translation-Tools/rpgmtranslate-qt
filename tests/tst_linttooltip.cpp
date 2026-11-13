#include "LintTooltip.hpp"

#include "Constants.hpp"
#include "ProjectSettings.hpp"
#include "TestFiles.hpp"
#include "Types.hpp"

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QtTest>

Q_DECLARE_METATYPE(LintType)

namespace {
// The MV/MZ save of the party below, made by the Rust core: Harry (1, renamed by the game) and the unnamed actor 2 are in
// the party, map 2, an hour of playtime, 3 of item 1, 2 of weapon 1, switch 1 on and switch 2 off, variable 2 = 42
constexpr auto SAVE_FILE =
    "N4IgzgnmAuCmC2IBcoD6AzATgQ3rMA8gHYDK2AbrMgEwCMAbAAzMC+ANCPNgA7JpfcAkgBMa7EN2yZoEPiFQBzAPYAbUUmoBWRh1TYAxtCWYwyANq021ALq6AlnHimUIWsgDM41AHdYPJUTOoG4aLOIGRiZyqMLY0NjmRACuKipsaBHGIsiW8kS4VEggABJSmLK6RHb6ANb5eMggAMIAFkpgsEQguvoq2GBg2Rq6KrCUKsgALLotvEjuOvLwcwAcutBztOwZhlnq1JUFjd151XVHRSeovf2D6rmoo+M5M5uLqMvImutzjCzW4jA3gc+ha+GisXiiRSaWgmCSsDY6GwKg6AI45CkdmwACNRkF5JCEkgzMlUmwdJMbGEgA";

// The game data the tooltips look names up in
constexpr auto ACTORS =
    R"([null,{"id":1,"name":"Harold","nickname":"The Brave","classId":2},{"id":2,"name":"Mira","nickname":"Sage","classId":1}])";
constexpr auto CLASSES = R"([null,{"id":1,"name":"Fighter"},{"id":2,"name":"Mage"}])";
constexpr auto ITEMS = R"([null,{"id":1,"name":"Potion","iconIndex":3}])";
constexpr auto WEAPONS = R"([null,{"id":1,"name":"Sword","iconIndex":4}])";
constexpr auto ARMORS = R"([null,{"id":1,"name":"Shield","iconIndex":5}])";
constexpr auto SKILLS = R"([null,{"id":1,"name":"Fire","iconIndex":6}])";
constexpr auto STATES = R"([null,{"id":1,"name":"Poison","iconIndex":7},{"id":2,"name":"Sleep","iconIndex":0}])";
constexpr auto ENEMIES = R"([null,{"id":1,"name":"Slime"}])";
constexpr auto SYSTEM =
    R"({"currencyUnit":"Gold","variables":["","Steps","Kills"],"switches":["","Door","Lamp"],"gameTitle":"Test"})";
constexpr auto MAP_INFOS = R"([null,{"id":1,"name":"Town"},{"id":2,"name":"Cave"}])";
constexpr auto COMMON_EVENTS = R"([null,{"id":1,"name":"Heal"}])";
constexpr auto ANIMATIONS = R"([null,{"id":1,"name":"Blast"}])";

// 16 icons of 32 pixels per row, two rows
auto iconSheet() -> QImage {
    QImage image(512, 64, QImage::Format_RGB32);
    image.fill(Qt::blue);
    return image;
}

struct Fixture {
    explicit Fixture(const EngineType engine = EngineType::MVMZ) {
        if (!dir.isValid()) {
            QFAIL("no temporary directory");
        }

        sets.projectPath = dir.path();
        sets.engineType = engine;
        sets.sourceDirectory = engine == EngineType::RM2K ? SourceDirectory::Root : SourceDirectory::LowercaseData;

        if (engine == EngineType::MVMZ) {
            data("Actors", ACTORS);
            data("Classes", CLASSES);
            data("Items", ITEMS);
            data("Weapons", WEAPONS);
            data("Armors", ARMORS);
            data("Skills", SKILLS);
            data("States", STATES);
            data("Enemies", ENEMIES);
            data("System", SYSTEM);
            data("MapInfos", MAP_INFOS);
            data("CommonEvents", COMMON_EVENTS);
            data("Animations", ANIMATIONS);

            QVERIFY(QDir().mkpath(dir.path() + u"/img/system"_s));
            QVERIFY(iconSheet().save(dir.path() + u"/img/system/IconSet.png"_s, "PNG"));
        }

        tooltip.resize(10, 10);
    }

    void data(const char* const name, const char* const json) const {
        QVERIFY(QDir().mkpath(sets.sourcePath()));
        writeFile(u"%1/%2.json"_s.arg(sets.sourcePath(), QString::fromLatin1(name)), json);
    }

    void save() {
        QVERIFY(QDir().mkpath(dir.path() + u"/save"_s));
        writeFile(dir.path() + u"/save/file1.rpgsave"_s, SAVE_FILE);
        sets.activeSave = u"save/file1.rpgsave"_s;
    }

    // The text of the tooltip for `lints`; the images in it are the object replacement character
    auto show(const QList<LintEntry>& lints) -> QString {
        tooltip.showAt(QPoint(200, 300), lints, &sets);

        const auto* const document = tooltip.findChild<QTextDocument*>();
        return document != nullptr ? document->toPlainText() : QString();
    }

    auto show(const LintEntry& lint) -> QString { return show(QList<LintEntry>{ lint }); }

    QTemporaryDir dir;
    ProjectSettings sets;
    LintTooltip tooltip;
};

auto entry(const LintType type, const i32 index, const QString& captured, const QVariant& extra = {}) -> LintEntry {
    return { .extra = extra, .suggestions = {}, .captured = captured, .translationId = "Head", .index = index, .type = type };
}

auto code(const i32 index, const QString& captured, const QVariant& extra = {}) -> LintEntry {
    return entry(TextCode, index, captured, extra);
}

// Finds a point of the tooltip that is on a link
auto linkPoint(const LintTooltip& tooltip) -> QPoint {
    const auto* const document = tooltip.findChild<QTextDocument*>();

    for (i32 yPos = 0; yPos < tooltip.height(); yPos++) {
        for (i32 xPos = 0; xPos < tooltip.width(); xPos++) {
            // The text starts 8 pixels right and 4 down of the corner
            if (!document->documentLayout()->anchorAt(QPointF(xPos - 8, yPos - 4)).isEmpty()) {
                return { xPos, yPos };
            }
        }
    }

    return { -1, -1 };
}

auto imageCount(const QString& text) -> qsizetype {
    return text.count(QChar::ObjectReplacementCharacter);
}
}  // namespace

class TestLintTooltip final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() { qRegisterMetaType<LintType>(); }

    // What a lint says in general
    void aLintWithoutATranslationShowsAPlaceholder() {
        Fixture fixture;
        LintEntry lint = code(30, uR"(\G)"_s);
        lint.translationId = nullptr;

        QVERIFY(fixture.show(lint).contains(u"Not yet implemented"_s));

        lint.translationId = "";
        QVERIFY(fixture.show(lint).contains(u"Not yet implemented"_s));
    }

    void customAndLanguageToolLintsShowTheirOwnText() {
        Fixture fixture;

        QVERIFY(fixture.show(entry(Custom, 0, {}, u"A custom note"_s)).contains(u"A custom note"_s));
        QVERIFY(fixture.show(entry(LanguageToolCheck, 0, {}, u"A grammar note"_s)).contains(u"A grammar note"_s));
    }

    void everyLintAddsItsParagraph() {
        Fixture fixture;

        const QString text = fixture.show(QList<LintEntry>{ code(84, uR"(\et[1])"_s), code(84, uR"(\et[2])"_s) });

        QCOMPARE(text.count(u"Head"_s), 2);
    }

    void aMiscLintFourOffersSuggestions() {
        Fixture fixture;
        LintEntry lint = entry(Misc, 4, {});
        lint.suggestions = QStringList{ u"alpha"_s, u"be*ta"_s };

        const QString text = fixture.show(lint);

        QVERIFY(text.contains(u"alpha"_s));
        // The markup in a suggestion is shown as it is
        QVERIFY(text.contains(u"be*ta"_s));
    }

    void otherMiscLintsHaveNoSuggestions() {
        Fixture fixture;
        LintEntry lint = entry(Misc, 5, {});
        lint.suggestions = QStringList{ u"alpha"_s };

        QVERIFY(!fixture.show(lint).contains(u"alpha"_s));
    }

    void languageToolSuggestionsAreShownOnlyWhenThereAreSome() {
        Fixture fixture;
        LintEntry lint = entry(LanguageToolCheck, 1, {}, u"Note"_s);

        QVERIFY(!fixture.show(lint).contains(u"Suggestions"_s));

        lint.suggestions = QStringList{ u"fix"_s };
        QVERIFY(fixture.show(lint).contains(u"Suggestions"_s));
    }

    // Text codes of an MV/MZ project
    void aMissingDataFileIsReported() {
        Fixture fixture;
        QVERIFY(QFile::remove(fixture.sets.sourcePath() + u"/System.json"_s));

        const QString text = fixture.show(code(27, uR"(\V[2])"_s));

        QVERIFY2(text.contains(u"Failed to fetch data"_s), qPrintable(text));
    }

    void aColorCodeShowsASwatch() {
        Fixture fixture;

        const QString text = fixture.show(code(31, uR"(\C[2])"_s, QVariant::fromValue<u32>(0xFF00FF00)));

        QVERIFY(text.contains(u"The following color will take effect"_s));
        QCOMPARE(imageCount(text), 1);
    }

    void aColorCodeThatCouldNotBeReadSaysWhy() {
        Fixture fixture;

        const QString text = fixture.show(code(31, uR"(\C[2])"_s, u"No window sheet"_s));

        QVERIFY(text.contains(u"Failed to fetch data: No window sheet"_s));
        QCOMPARE(imageCount(text), 0);
    }

    void anIconCodeShowsTheIcon() {
        Fixture fixture;

        const QString text = fixture.show(code(32, uR"(\I[3])"_s));

        QVERIFY(text.contains(u"This pattern will be replaced with"_s));
        QCOMPARE(imageCount(text), 1);
    }

    void anIconOutsideTheSheetIsReported() {
        Fixture fixture;

        const QString text = fixture.show(code(32, uR"(\I[999])"_s));

        QVERIFY(text.contains(u"Failed to fetch icon"_s));
        QCOMPARE(imageCount(text), 0);
    }

    void aMissingIconSheetIsReported() {
        Fixture fixture;
        QVERIFY(QFile::remove(fixture.dir.path() + u"/img/system/IconSet.png"_s));

        QVERIFY(fixture.show(code(32, uR"(\I[3])"_s)).contains(u"Failed to fetch icon"_s));
    }

    void aWaitCodeCountsFrames() {
        Fixture fixture;

        const QString text = fixture.show(code(43, uR"(\w[30])"_s));

        QVERIFY2(text.contains(u"30 frame"_s), qPrintable(text));
        QVERIFY2(text.contains(u"0.5"_s), qPrintable(text));
    }

    void actorClassAndNameCodes() {
        Fixture fixture;

        const QString classText = fixture.show(code(58, uR"(\ac[1])"_s));
        QVERIFY2(classText.contains(u"This pattern will be replaced with: Mage"_s), qPrintable(classText));

        const QString nameText = fixture.show(code(59, uR"(\an[1])"_s));
        QVERIFY2(nameText.contains(u"The Brave"_s) && !nameText.contains(u"Harold"_s), qPrintable(nameText));
    }

    void entityNameCodes_data() {
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");
        QTest::addColumn<QString>("expected");

        QTest::newRow("class") << 63 << uR"(\nc[2])"_s << u"This pattern will be replaced with: Mage"_s;
        QTest::newRow("item") << 64 << uR"(\ni[1])"_s << u"This pattern will be replaced with: Potion"_s;
        QTest::newRow("weapon") << 65 << uR"(\nw[1])"_s << u"This pattern will be replaced with: Sword"_s;
        QTest::newRow("armor") << 66 << uR"(\na[1])"_s << u"This pattern will be replaced with: Shield"_s;
        QTest::newRow("skill") << 67 << uR"(\ns[1])"_s << u"This pattern will be replaced with: Fire"_s;
        QTest::newRow("state") << 68 << uR"(\nt[1])"_s << u"This pattern will be replaced with: Poison"_s;
        QTest::newRow("enemy") << 83 << uR"(\en[1])"_s << u"Enemy name: Slime"_s;
        QTest::newRow("a variable") << 27 << uR"(\V[2])"_s << u"The value of this variable will be displayed: Kills"_s;
        QTest::newRow("actor name") << 28 << uR"(\N[1])"_s << u"This pattern will be replaced with: Harold"_s;
        QTest::newRow("actor name, face") << 57 << uR"(\af[2])"_s << u"Actor's name: Mira"_s;
        QTest::newRow("actor name, message") << 97 << uR"(\msgactor[1])"_s << u"Actor's name: Harold"_s;
        QTest::newRow("actor name, auto") << 103 << uR"(\autoactor[2])"_s << u"Actor's name: Mira"_s;
        QTest::newRow("the currency") << 30 << uR"(\G)"_s << u"This pattern will be replaced with: Gold"_s;
        QTest::newRow("the map in MV") << 112 << uR"(\map[1])"_s << u"This pattern will be replaced with: Town"_s;
    }

    void entityNameCodes() {
        QFETCH(i32, index);
        QFETCH(QString, captured);
        QFETCH(QString, expected);

        Fixture fixture;

        const QString text = fixture.show(code(index, captured));

        QVERIFY2(text.contains(expected), qPrintable(text));
    }

    void entityNameCodesWithAnIcon_data() {
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");
        QTest::addColumn<QString>("expected");

        QTest::newRow("item") << 69 << uR"(\ii[1])"_s << u"Potion"_s;
        QTest::newRow("weapon") << 70 << uR"(\iw[1])"_s << u"Sword"_s;
        QTest::newRow("armor") << 71 << uR"(\ia[1])"_s << u"Shield"_s;
        QTest::newRow("skill") << 72 << uR"(\is[1])"_s << u"Fire"_s;
        QTest::newRow("state") << 73 << uR"(\it[1])"_s << u"Poison"_s;
    }

    void entityNameCodesWithAnIcon() {
        QFETCH(i32, index);
        QFETCH(QString, captured);
        QFETCH(QString, expected);

        Fixture fixture;

        const QString text = fixture.show(code(index, captured));

        QVERIFY2(text.contains(expected), qPrintable(text));
        QCOMPARE(imageCount(text), 1);
    }

    void groupedNumbersAndColorCodes() {
        Fixture fixture;

        QVERIFY(fixture.show(code(109, uR"(\dg[1234])"_s)).contains(u"1,234"_s));

        const QString color = fixture.show(code(110, uR"(\hc[ff0000])"_s));
        QVERIFY(color.contains(u"The following color will take effect"_s));
        QCOMPARE(imageCount(color), 1);
    }

    void battleOnlyCodesSayTheyNeedABattle_data() {
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");

        QTest::newRow("enemy in a message") << 84 << uR"(\et[1])"_s;
        QTest::newRow("enemy in a message box") << 99 << uR"(\msgenemy[1])"_s;
        QTest::newRow("enemy in auto") << 105 << uR"(\autoenemy[1])"_s;
        QTest::newRow("first enemy stat") << 154 << uR"(\emhp[1])"_s;
        QTest::newRow("last enemy stat") << 190 << uR"(\eexr[1])"_s;
    }

    void battleOnlyCodesSayTheyNeedABattle() {
        QFETCH(i32, index);
        QFETCH(QString, captured);

        Fixture fixture;

        QVERIFY(fixture.show(code(index, captured)).contains(u"only known in a battle"_s));
    }

    void realTimeCodesShowTheCurrentTime() {
        Fixture fixture;

        // The year may turn between the tooltip and the check at midnight of the new year
        const i32 yearBefore = QDate::currentDate().year();
        const QString year = fixture.show(code(205, uR"(\Year)"_s));
        QVERIFY2(
            year.contains(QString::number(yearBefore)) || year.contains(QString::number(QDate::currentDate().year())),
            qPrintable(year)
        );

        // The whole range has a format
        for (i32 index = 205; index <= 215; index++) {
            QVERIFY(fixture.show(code(index, uR"(\Year)"_s)).contains(u"At this moment"_s));
        }
    }

    void aCodeNumberGivenByAVariableIsOnlyKnownWhileTheGameRuns() {
        Fixture fixture;

        const QString text = fixture.show(code(28, uR"(\N[\V[2]])"_s));

        QVERIFY(text.contains(u"given by a variable"_s));
        QVERIFY2(text.contains(u"Variable 2: Kills"_s), qPrintable(text));
    }

    void aDeeperNestingNamesNoVariable() {
        Fixture fixture;

        const QString text = fixture.show(code(28, uR"(\N[\V[\V[2]]])"_s));

        QVERIFY(text.contains(u"given by a variable"_s));
        QVERIFY(!text.contains(u"Variable 2"_s));
    }

    // Whatever the code or the tag points at, a thing the data does not have is reported, not shown as a name
    void aReferenceToWhatTheDataLacksIsReported_data() {
        QTest::addColumn<LintType>("type");
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");

        QTest::newRow("variable") << TextCode << 27 << uR"(\V[9])"_s;
        QTest::newRow("actor name") << TextCode << 28 << uR"(\N[9])"_s;
        QTest::newRow("actor class") << TextCode << 58 << uR"(\ac[9])"_s;
        QTest::newRow("actor nickname") << TextCode << 59 << uR"(\an[9])"_s;
        QTest::newRow("class") << TextCode << 63 << uR"(\nc[9])"_s;
        QTest::newRow("item") << TextCode << 64 << uR"(\ni[9])"_s;
        QTest::newRow("weapon") << TextCode << 65 << uR"(\nw[9])"_s;
        QTest::newRow("armor") << TextCode << 66 << uR"(\na[9])"_s;
        QTest::newRow("skill") << TextCode << 67 << uR"(\ns[9])"_s;
        QTest::newRow("state") << TextCode << 68 << uR"(\nt[9])"_s;
        QTest::newRow("enemy") << TextCode << 83 << uR"(\en[9])"_s;
        QTest::newRow("item with icon") << TextCode << 69 << uR"(\ii[9])"_s;
        QTest::newRow("weapon with icon") << TextCode << 70 << uR"(\iw[9])"_s;
        QTest::newRow("armor with icon") << TextCode << 71 << uR"(\ia[9])"_s;
        QTest::newRow("skill with icon") << TextCode << 72 << uR"(\is[9])"_s;
        QTest::newRow("state with icon") << TextCode << 73 << uR"(\it[9])"_s;
        QTest::newRow("map") << TextCode << 112 << uR"(\map[9])"_s;
        QTest::newRow("tag item") << NoteTag << 455 << u"<Item 9>"_s;
        QTest::newRow("tag weapon") << NoteTag << 455 << u"<Weapon 9>"_s;
        QTest::newRow("tag armor") << NoteTag << 455 << u"<Armor 9>"_s;
        QTest::newRow("tag skill") << NoteTag << 472 << u"<Instant Skill: 9>"_s;
        QTest::newRow("tag state") << NoteTag << 47 << u"<Passive State: 9>"_s;
        QTest::newRow("tag switch") << NoteTag << 51 << u"<Passive Condition: Switch 9>"_s;
        QTest::newRow("tag variable") << NoteTag << 52 << u"<Passive Condition: Variable 9>"_s;
        QTest::newRow("tag animation") << NoteTag << 91 << u"<Cast Animation: 9>"_s;
        QTest::newRow("tag common event") << NoteTag << 289 << u"<Card Hand Start Common Event: 9>"_s;
        QTest::newRow("plugin command") << PluginCommand << 19 << u"UnlockClass 9 9"_s;
        // 70 digits do not fit the 64 bytes of a key
        QTest::newRow("a key that does not fit") << TextCode << 63 << uR"(\nc[)"_s + QString(70, u'1') + u"]"_s;
        QTest::newRow("an index of non-ASCII characters") << TextCode << 63 << uR"(\nc[ä])"_s;
    }

    void aReferenceToWhatTheDataLacksIsReported() {
        QFETCH(LintType, type);
        QFETCH(i32, index);
        QFETCH(QString, captured);

        Fixture fixture;
        const QString text = fixture.show(entry(type, index, captured));

        QVERIFY2(text.contains(u"Failed to fetch data"_s), qPrintable(text));
        QVERIFY2(!text.contains(u"will be replaced with"_s), qPrintable(text));
    }

    void anActorOfAClassTheDataLacksIsReported() {
        Fixture fixture;
        fixture.data("Actors", R"([null,{"id":1,"name":"Harold","nickname":"The Brave","classId":9}])");

        const QString text = fixture.show(code(58, uR"(\ac[1])"_s));

        QVERIFY2(text.contains(u"Failed to fetch data"_s), qPrintable(text));
        QVERIFY2(!text.contains(u"will be replaced with"_s), qPrintable(text));
    }

    void aCurrencyTheSystemFileLacksIsReported() {
        Fixture fixture;
        QVERIFY(QFile::remove(fixture.sets.sourcePath() + u"/System.json"_s));

        QVERIFY(fixture.show(code(30, uR"(\G)"_s)).contains(u"Failed to fetch data"_s));
    }

    void anItemWhoseIconIsOutsideTheSheetIsReported() {
        Fixture fixture;
        fixture.data("Items", R"([null,{"id":1,"name":"Potion","iconIndex":999}])");

        const QString text = fixture.show(code(69, uR"(\ii[1])"_s));

        QVERIFY2(text.contains(u"Failed to fetch icon"_s), qPrintable(text));
        QVERIFY2(!text.contains(u"will be replaced with"_s), qPrintable(text));
    }

    void aPartyMemberOfAClassTheDataLacksIsReported() {
        Fixture fixture;
        fixture.save();
        QVERIFY(QFile::remove(fixture.sets.sourcePath() + u"/Classes.json"_s));

        QVERIFY(fixture.show(code(61, uR"(\pc[1])"_s)).contains(u"Failed to fetch data"_s));
    }

    void aMapCodeOfOtherEnginesShowsNothing() {
        Fixture fixture(EngineType::VXAce);
        fixture.sets.sourceDirectory = SourceDirectory::Root;

        QCOMPARE(fixture.show(code(112, uR"(\map[1])"_s)).trimmed(), u"Head"_s);
    }

    void codesThatNeedASaveAskForOne_data() {
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");

        QTest::newRow("playtime") << 111 << uR"(\playtime)"_s;
        QTest::newRow("party member") << 29 << uR"(\P[1])"_s;
        QTest::newRow("party member class") << 61 << uR"(\pc[1])"_s;
        QTest::newRow("map of the player") << 112 << uR"(\map[0])"_s;
        QTest::newRow("items") << 113 << uR"(\qi[1])"_s;
        QTest::newRow("weapons") << 114 << uR"(\qw[1])"_s;
        QTest::newRow("armors") << 115 << uR"(\qa[1])"_s;
        QTest::newRow("switch") << 117 << uR"(\caseSwitch{1?a:b})"_s;
    }

    void codesThatNeedASaveAskForOne() {
        QFETCH(i32, index);
        QFETCH(QString, captured);

        Fixture fixture;

        QVERIFY(fixture.show(code(index, captured)).contains(u"Pick an active save"_s));
    }

    void codesThatOnlyShowMoreWithASaveDoNotAskForOne() {
        Fixture fixture;

        const QString variable = fixture.show(code(27, uR"(\V[1])"_s));
        QVERIFY(!variable.contains(u"Pick an active save"_s));
        QVERIFY(!variable.contains(u"Value in the active save"_s));

        const QString actor = fixture.show(code(120, uR"(\ahp[1])"_s));
        QVERIFY(actor.contains(u"Actor: Harold"_s));
        QVERIFY(!actor.contains(u"Pick an active save"_s));
        QVERIFY(!actor.contains(u"Value in the active save"_s));
    }

    void actorStatsExplainHowTheyAreComputed() {
        Fixture fixture;

        const QString first = fixture.show(code(119, uR"(\amhp[2])"_s));
        QVERIFY(first.contains(u"Actor: Mira"_s));
        QVERIFY(first.contains(u"computes this value"_s));

        QVERIFY(fixture.show(code(153, uR"(\aexr[1])"_s)).contains(u"computes this value"_s));
    }

    void anUnknownCodeShowsOnlyItsHeading() {
        Fixture fixture;

        QCOMPARE(fixture.show(code(10, uR"(\x)"_s)).trimmed(), u"Head"_s);
    }

    // With a save
    void aSaveGivesTheValuesOfTheRunningGame() {
        Fixture fixture;
        fixture.save();

        QVERIFY(fixture.show(code(27, uR"(\V[2])"_s)).contains(u"Value in the active save: 42"_s));
        QVERIFY(fixture.show(code(27, uR"(\V[1])"_s)).contains(u"Value in the active save: 0"_s));
        QVERIFY(fixture.show(code(27, uR"(\V[2])"_s)).contains(u"Kills"_s));
        QVERIFY(fixture.show(code(111, uR"(\playtime)"_s)).contains(u"Playtime in the active save: 1:00:00"_s));
    }

    // Codes that read the active save; the save renames actors, so `\N[1]` is Harry and not Harold
    void theSaveGivesTheValuesOfTheRunningGame_data() {
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<bool>("present");

        // The game renamed actor 1; actor 2 has no name in the save, so the database's is used
        QTest::newRow("renamed actor") << 28 << uR"(\N[1])"_s << u"This pattern will be replaced with: Harry"_s << true;
        QTest::newRow("actor not renamed") << 28 << uR"(\N[2])"_s << u"This pattern will be replaced with: Mira"_s << true;
        // Codes that only point at an actor give the database's name
        QTest::newRow("actor by face") << 57 << uR"(\af[1])"_s << u"Actor's name: Harold"_s << true;
        QTest::newRow("party member 1") << 29 << uR"(\P[1])"_s << u"This pattern will be replaced with: Harry"_s << true;
        QTest::newRow("party member 2") << 29 << uR"(\P[2])"_s << u"This pattern will be replaced with: Mira"_s << true;
        QTest::newRow("party member, message") << 98 << uR"(\msgparty[1])"_s << u"Party member: Harry"_s << true;
        QTest::newRow("party member, auto") << 104 << uR"(\autoparty[2])"_s << u"Party member: Mira"_s << true;
        QTest::newRow("party member, face") << 60 << uR"(\pf[1])"_s << u"Party member: Harry"_s << true;
        QTest::newRow("party leader name") << 62 << uR"(\pn[1])"_s << u"This pattern will be replaced with: Chosen"_s << true;
        // Actor 1 is a mage in the save, and actor 2 a fighter
        QTest::newRow("party class 1") << 61 << uR"(\pc[1])"_s << u"This pattern will be replaced with: Mage"_s << true;
        QTest::newRow("party class 2") << 61 << uR"(\pc[2])"_s << u"This pattern will be replaced with: Fighter"_s << true;
        QTest::newRow("no party member 3") << 29 << uR"(\P[3])"_s << u"has no member 3"_s << true;
        QTest::newRow("no party member 0") << 29 << uR"(\P[0])"_s << u"has no member 0"_s << true;
        QTest::newRow("items") << 113 << uR"(\qi[1])"_s << u"has 3 of Potion"_s << true;
        QTest::newRow("weapons") << 114 << uR"(\qw[1])"_s << u"has 2 of Sword"_s << true;
        QTest::newRow("armors") << 115 << uR"(\qa[1])"_s << u"has 0 of Shield"_s << true;
        // An item the database has no name for goes by its number
        QTest::newRow("item without a name") << 113 << uR"(\qi[7])"_s << u"has 0 of #7"_s << true;
        QTest::newRow("map of the player 0") << 112 << uR"(\map[0])"_s << u"This pattern will be replaced with: Cave"_s << true;
        QTest::newRow("map of the player 1") << 112 << uR"(\map[1])"_s << u"This pattern will be replaced with: Town"_s << true;
        QTest::newRow("switch on") << 117 << uR"(\caseSwitch{1?a:b})"_s << u"Switch 1 is ON"_s << true;
        QTest::newRow("switch off") << 117 << uR"(\caseSwitch{2?a:b})"_s << u"Switch 2 is OFF"_s << true;
        QTest::newRow("switch not in the save") << 117 << uR"(\caseSwitch{9?a:b})"_s << u"Switch 9 is OFF"_s << true;
        QTest::newRow("actor hp") << 120 << uR"(\ahp[1])"_s << u"Value in the active save: 30"_s << true;
        QTest::newRow("actor mp") << 123 << uR"(\amp[1])"_s << u"Value in the active save: 8"_s << true;
        QTest::newRow("actor tp") << 126 << uR"(\atp[1])"_s << u"Value in the active save: 1"_s << true;
        QTest::newRow("actor 2 hp") << 120 << uR"(\ahp[2])"_s << u"Value in the active save: 10"_s << true;
        // Not an actor of the save: nothing to show
        QTest::newRow("actor not in the save") << 120 << uR"(\ahp[9])"_s << u"Value in the active save"_s << false;
    }

    void theSaveGivesTheValuesOfTheRunningGame() {
        QFETCH(const i32, index);
        QFETCH(const QString, captured);
        QFETCH(const QString, expected);
        QFETCH(const bool, present);

        Fixture fixture;
        fixture.save();

        const QString text = fixture.show(code(index, captured));

        QVERIFY2(text.contains(expected) == present, qPrintable(text));
    }

    void aBrokenSaveIsReported() {
        Fixture fixture;
        fixture.save();
        writeFile(fixture.dir.path() + u"/save/file1.rpgsave"_s, "not a save");
        fixture.sets.activeSave = u"save/file1.rpgsave"_s;

        // The summary of the earlier file may be cached by its time; touch the file
        QFile file(fixture.dir.path() + u"/save/file1.rpgsave"_s);
        QVERIFY(file.open(QFile::ReadWrite));
        QVERIFY(file.setFileTime(QDateTime::currentDateTime().addSecs(120), QFileDevice::FileModificationTime));
        file.close();

        QVERIFY(fixture.show(code(111, uR"(\playtime)"_s)).contains(u"Failed to read the save"_s));
    }

    // RPG Maker 2000/2003
    void rm2kShowsOnlyTheCodesItsDataExplains() {
        Fixture fixture(EngineType::RM2K);

        QCOMPARE(fixture.show(code(30, uR"(\G)"_s)).trimmed(), u"Head"_s);
        QCOMPARE(fixture.show(code(32, uR"(\I[3])"_s)).trimmed(), u"Head"_s);
        QCOMPARE(fixture.show(code(63, uR"(\nc[1])"_s)).trimmed(), u"Head"_s);
        QVERIFY(fixture.show(code(31, uR"(\C[2])"_s, QVariant::fromValue<u32>(0xFF0000FF))).contains(u"color will take effect"_s));
    }

    void rm2kNamesComeFromItsDatabase() {
        Fixture fixture(EngineType::RM2K);

        // There is no database in the fixture
        QVERIFY(fixture.show(code(27, uR"(\V[1])"_s)).contains(u"Failed to fetch data"_s));
        QVERIFY(fixture.show(code(28, uR"(\N[1])"_s)).contains(u"Failed to fetch data"_s));
    }

    void rm2kActorZeroIsTheLeaderOfTheParty() {
        Fixture fixture(EngineType::RM2K);

        QVERIFY(fixture.show(code(28, uR"(\N[0])"_s)).contains(u"name of the party leader"_s));
    }

    void rm2kTagsAreNotShown() {
        Fixture fixture(EngineType::RM2K);

        QCOMPARE(fixture.show(entry(NoteTag, 47, u"<Passive State: 1>"_s)).trimmed(), u"Head"_s);
    }

    // Tags
    void noteTagListsNameTheirEntities() {
        Fixture fixture;

        const QString text = fixture.show(entry(NoteTag, 47, u"<Passive State: 1, 2>"_s));

        QVERIFY2(text.contains(u"Poison"_s), qPrintable(text));
        QVERIFY2(text.contains(u"2: Sleep"_s), qPrintable(text));
        // State 1 has an icon, state 2 has none
        QCOMPARE(imageCount(text), 1);
    }

    void noteTagsOfSeveralKindsNameTheirEntities() {
        Fixture fixture;

        const QString text = fixture.show(entry(NoteTag, 137, u"<Class 2 CTB Icon: 3>"_s));

        QVERIFY2(text.contains(u"2: Mage"_s), qPrintable(text));
        QCOMPARE(imageCount(text), 1);
    }

    void aNumberWithoutAKindIsLeftOut() {
        Fixture fixture;

        const QString text = fixture.show(entry(NoteTag, 293, u"<Card Spell 1 Common Event: 1>"_s));

        QVERIFY2(text.contains(u"1: Heal"_s), qPrintable(text));
        QCOMPARE(text.count(u"Heal"_s), 1);
    }

    void iconsInTagsAreShown() {
        Fixture fixture;

        QCOMPARE(imageCount(fixture.show(entry(NoteTag, 311, u"<Icon: 3>"_s))), 1);
        QCOMPARE(imageCount(fixture.show(entry(CommentTag, 5, u"<Icon: 3>"_s))), 1);
        QVERIFY(fixture.show(entry(NoteTag, 311, u"<Icon: 999>"_s)).contains(u"Failed to fetch icon"_s));
    }

    void tagsNameTheirEntities_data() {
        QTest::addColumn<LintType>("type");
        QTest::addColumn<i32>("index");
        QTest::addColumn<QString>("captured");
        QTest::addColumn<QString>("expected");

        QTest::newRow("switch") << NoteTag << 51 << u"<Passive Condition: Switch 2>"_s << u"2: Lamp"_s;
        QTest::newRow("variable") << NoteTag << 52 << u"<Passive Condition: Variable 1>"_s << u"1: Steps"_s;
        QTest::newRow("animation") << NoteTag << 91 << u"<Cast Animation: 1>"_s << u"1: Blast"_s;
        QTest::newRow("common event") << NoteTag << 289 << u"<Card Hand Start Common Event: 1>"_s << u"1: Heal"_s;
        QTest::newRow("weapon") << NoteTag << 455 << u"<Weapon 1>"_s << u"Sword"_s;
        QTest::newRow("armor") << NoteTag << 455 << u"<Armor 1>"_s << u"Shield"_s;
        QTest::newRow("item") << NoteTag << 455 << u"<Item 1>"_s << u"Potion"_s;
        QTest::newRow("skill") << NoteTag << 472 << u"<Instant Skill: 1>"_s << u"Fire"_s;
        QTest::newRow("plugin command, actor") << PluginCommand << 19 << u"UnlockClass 1 2"_s << u"1: Harold"_s;
        QTest::newRow("plugin command, class") << PluginCommand << 19 << u"UnlockClass 1 2"_s << u"2: Mage"_s;
    }

    void tagsNameTheirEntities() {
        QFETCH(const LintType, type);
        QFETCH(const i32, index);
        QFETCH(const QString, captured);
        QFETCH(const QString, expected);

        Fixture fixture;

        const QString text = fixture.show(entry(type, index, captured));

        QVERIFY2(text.contains(expected), qPrintable(text));
    }

    void aMissingEntityStopsTheList() {
        Fixture fixture;

        const QString text = fixture.show(entry(NoteTag, 47, u"<Passive State: 1, 9, 2>"_s));

        QVERIFY2(text.contains(u"Poison"_s), qPrintable(text));
        QVERIFY(text.contains(u"Failed to fetch data"_s));
        QVERIFY(!text.contains(u"2: Sleep"_s));
    }

    void aListShowsNoMoreThanEightEntries() {
        Fixture fixture;

        const QString text = fixture.show(entry(NoteTag, 47, u"<Passive State: 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>"_s));

        QCOMPARE(text.count(u"Poison"_s), 8);
    }

    void tagsThatTheTableDoesNotKnowShowNothing() {
        Fixture fixture;

        QCOMPARE(fixture.show(entry(NoteTag, 9999, u"<Whatever: 1>"_s)).trimmed(), u"Head"_s);
    }

    void tagsOfOtherEnginesShowNothing() {
        Fixture fixture(EngineType::VXAce);
        fixture.sets.sourceDirectory = SourceDirectory::Root;

        QCOMPARE(fixture.show(entry(NoteTag, 47, u"<Passive State: 1>"_s)).trimmed(), u"Head"_s);
    }

    // The widget
    void aTooltipHoldsNoMoreThanSixteenImages() {
        Fixture fixture;
        QList<LintEntry> lints;

        for (i32 count = 0; count < 20; count++) {
            lints.append(code(110, uR"(\hc[ff0000])"_s));
        }

        QCOMPARE(imageCount(fixture.show(lints)), 16);
    }

    void aTallTooltipIsCappedAndScrolls() {
        Fixture fixture;
        QList<LintEntry> lints;

        for (i32 count = 0; count < 30; count++) {
            lints.append(code(84, uR"(\et[1])"_s));
        }

        fixture.show(lints);
        QVERIFY(fixture.tooltip.isVisible());
        QCOMPARE(fixture.tooltip.height(), 160);

        const QImage before = fixture.tooltip.grab().toImage();

        QWheelEvent wheel(
            QPointF(20, 20),
            QPointF(20, 20),
            QPoint(),
            QPoint(0, -240),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        QVERIFY(QApplication::sendEvent(&fixture.tooltip, &wheel));
        QVERIFY(wheel.isAccepted());
        QVERIFY(fixture.tooltip.grab().toImage() != before);

        // Scrolling back up returns to the start, and never past it
        QWheelEvent up(
            QPointF(20, 20),
            QPointF(20, 20),
            QPoint(),
            QPoint(0, 2400),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        QVERIFY(QApplication::sendEvent(&fixture.tooltip, &up));
        QCOMPARE(fixture.tooltip.grab().toImage(), before);
    }

    void aShortTooltipHasNothingToScroll() {
        Fixture fixture;

        fixture.show(code(84, uR"(\et[1])"_s));
        QVERIFY(fixture.tooltip.height() < 160);

        const QImage before = fixture.tooltip.grab().toImage();
        QWheelEvent wheel(
            QPointF(20, 20),
            QPointF(20, 20),
            QPoint(),
            QPoint(0, -240),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        QVERIFY(QApplication::sendEvent(&fixture.tooltip, &wheel));
        QCOMPARE(fixture.tooltip.grab().toImage(), before);
    }

    void theTooltipStaysOnTheScreen() {
        Fixture fixture;
        const QScreen* const screen = QApplication::primaryScreen();
        const QRect area = screen->availableGeometry();

        fixture.tooltip.showAt(area.bottomRight(), { code(84, uR"(\et[1])"_s) }, &fixture.sets);
        QVERIFY(area.contains(fixture.tooltip.geometry()));

        fixture.tooltip.showAt(area.topLeft(), { code(84, uR"(\et[1])"_s) }, &fixture.sets);
        QVERIFY(area.contains(fixture.tooltip.geometry()));
    }

    void dismissHidesAtOnce() {
        Fixture fixture;

        fixture.show(code(84, uR"(\et[1])"_s));
        QVERIFY(fixture.tooltip.isVisible());

        fixture.tooltip.dismiss();
        QVERIFY(!fixture.tooltip.isVisible());
    }

    void hideWaitsForTheMouseToLeave() {
        Fixture fixture;

        fixture.show(code(84, uR"(\et[1])"_s));

        // The mouse never left: nothing is waited for
        fixture.tooltip.hide();
        QVERIFY(fixture.tooltip.isVisible());

        fixture.tooltip.waitForEnter();
        fixture.tooltip.hide();
        QVERIFY(!fixture.tooltip.isVisible());
    }

    void enteringKeepsTheTooltipAndLeavingHidesItLater() {
        Fixture fixture;

        fixture.show(code(84, uR"(\et[1])"_s));
        fixture.tooltip.waitForEnter();

        QEnterEvent enter(QPointF(5, 5), QPointF(5, 5), QPointF(5, 5));
        QApplication::sendEvent(&fixture.tooltip, &enter);
        QTest::qWait(TOOLTIP_DELAY_MS + 300);
        QVERIFY(fixture.tooltip.isVisible());

        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(&fixture.tooltip, &leave);
        QTRY_VERIFY_WITH_TIMEOUT(!fixture.tooltip.isVisible(), TOOLTIP_DELAY_MS + 2000);
    }

    void clickingASuggestionTellsWhichOne() {
        Fixture fixture;
        LintEntry lint = entry(Misc, 4, {});
        lint.suggestions = QStringList{ u"alpha"_s, u"beta"_s };
        fixture.show(lint);

        QSignalSpy activated(&fixture.tooltip, &LintTooltip::suggestionActivated);
        QSignalSpy linked(&fixture.tooltip, &LintTooltip::linkActivated);

        const QPoint point = linkPoint(fixture.tooltip);
        QVERIFY(point.x() >= 0);
        QTest::mouseClick(&fixture.tooltip, Qt::LeftButton, Qt::NoModifier, point);

        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.at(0).at(0).value<LintType>(), Misc);
        QCOMPARE(activated.at(0).at(1).toInt(), 4);
        QCOMPARE(activated.at(0).at(2).toInt(), 0);
        QCOMPARE(linked.count(), 0);
    }

    void clickingALinkOfACustomTooltipOpensIt() {
        Fixture fixture;
        fixture.show(entry(Custom, 0, {}, u"[docs](https://example.com/page)"_s));

        QSignalSpy linked(&fixture.tooltip, &LintTooltip::linkActivated);
        QSignalSpy activated(&fixture.tooltip, &LintTooltip::suggestionActivated);

        const QPoint point = linkPoint(fixture.tooltip);
        QVERIFY(point.x() >= 0);
        QTest::mouseClick(&fixture.tooltip, Qt::LeftButton, Qt::NoModifier, point);

        QCOMPARE(linked.count(), 1);
        QCOMPARE(linked.at(0).at(0).toUrl(), QUrl(u"https://example.com/page"_s));
        QCOMPARE(activated.count(), 0);
    }

    void aForeignSuggestionLinkIsIgnored() {
        Fixture fixture;
        fixture.show(entry(Custom, 0, {}, u"[bad](lint:1,2)"_s));

        QSignalSpy linked(&fixture.tooltip, &LintTooltip::linkActivated);
        QSignalSpy activated(&fixture.tooltip, &LintTooltip::suggestionActivated);

        const QPoint point = linkPoint(fixture.tooltip);
        QVERIFY(point.x() >= 0);
        QTest::mouseClick(&fixture.tooltip, Qt::LeftButton, Qt::NoModifier, point);

        QCOMPARE(linked.count(), 0);
        QCOMPARE(activated.count(), 0);
    }

    void onlyTheLeftButtonActivatesALink() {
        Fixture fixture;
        fixture.show(entry(Custom, 0, {}, u"[docs](https://example.com/page)"_s));

        QSignalSpy linked(&fixture.tooltip, &LintTooltip::linkActivated);

        const QPoint point = linkPoint(fixture.tooltip);
        QVERIFY(point.x() >= 0);

        QTest::mouseClick(&fixture.tooltip, Qt::RightButton, Qt::NoModifier, point);
        QCOMPARE(linked.count(), 0);

        // The same point does open the link for the left button
        QTest::mouseClick(&fixture.tooltip, Qt::LeftButton, Qt::NoModifier, point);
        QCOMPARE(linked.count(), 1);
    }

    void clickingPlainTextActivatesNothing() {
        Fixture fixture;
        fixture.show(entry(Custom, 0, {}, u"Just text"_s));

        QSignalSpy linked(&fixture.tooltip, &LintTooltip::linkActivated);
        QTest::mouseClick(&fixture.tooltip, Qt::LeftButton, Qt::NoModifier, QPoint(10, 8));

        QCOMPARE(linked.count(), 0);
    }

    void theCursorIsAHandOverALink() {
        Fixture fixture;
        fixture.show(entry(Custom, 0, {}, u"[docs](https://example.com/page)"_s));

        const QPoint point = linkPoint(fixture.tooltip);
        QMouseEvent over(QEvent::MouseMove, point, fixture.tooltip.mapToGlobal(point), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&fixture.tooltip, &over);
        QCOMPARE(fixture.tooltip.cursor().shape(), Qt::PointingHandCursor);

        QMouseEvent away(QEvent::MouseMove, QPoint(fixture.tooltip.width() - 1, fixture.tooltip.height() - 1), fixture.tooltip.mapToGlobal(QPoint(1, 1)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&fixture.tooltip, &away);
        QCOMPARE(fixture.tooltip.cursor().shape(), Qt::ArrowCursor);
    }
};

QTEST_MAIN(TestLintTooltip)
#include "tst_linttooltip.moc"
