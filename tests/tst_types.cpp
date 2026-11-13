#include "Types.hpp"

#include <QtTest>

Q_DECLARE_METATYPE(EngineType)

namespace {
// The names joined with commas
auto namesOf(const vector<FilenameArray>& names) -> QByteArray {
    QByteArray out;

    for (const FilenameArray& name : names) {
        if (!out.isEmpty()) {
            out.append(',');
        }

        out.append(name.data());
    }

    return out;
}

auto asciiOf(const u16 number) -> QByteArray {
    const array<char, 4> digits = u16ToAscii(number);
    return QByteArray(digits.data(), static_cast<qsizetype>(strnlen(digits.data(), digits.size())));
}
}  // namespace

class TestTypes final : public QObject {
    Q_OBJECT

   private slots:
    void u16ToAsciiData_data() {
        QTest::addColumn<u16>("number");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("zero") << u16{ 0 } << QByteArray("0");
        QTest::newRow("one digit") << u16{ 9 } << QByteArray("9");
        QTest::newRow("two digits low") << u16{ 10 } << QByteArray("10");
        QTest::newRow("two digits high") << u16{ 99 } << QByteArray("99");
        QTest::newRow("three digits low") << u16{ 100 } << QByteArray("100");
        QTest::newRow("three digits inner zero") << u16{ 305 } << QByteArray("305");
        QTest::newRow("three digits high") << u16{ 999 } << QByteArray("999");
        QTest::newRow("four digits low") << u16{ 1000 } << QByteArray("1000");
        QTest::newRow("four digits inner zeros") << u16{ 1007 } << QByteArray("1007");
        QTest::newRow("four digits max map") << u16{ 2047 } << QByteArray("2047");
    }

    void u16ToAsciiData() {
        QFETCH(u16, number);
        QFETCH(QByteArray, expected);

        QCOMPARE(asciiOf(number), expected);
    }

    // Short numbers leave the rest of the 4 bytes zero: `filenames()` copies all 4 bytes after "map"
    void u16ToAsciiZeroFillsTheTail() {
        const array<char, 4> digits = u16ToAscii(42);
        QCOMPARE(digits[2], '\0');
        QCOMPARE(digits[3], '\0');
    }

    void mapNamesFollowActualIndexNotDenseOne() {
        Selected selected;
        // Maps 1, 9, 10, 99, 100, 999, 1000 and 2047 exist; dense positions 0..7
        for (const u32 actual : { 1U, 9U, 10U, 99U, 100U, 999U, 1000U, 2047U }) {
            selected.validIndices[actual] = true;
        }

        selected.mapCount = 8;
        // Dense positions 0, 2, 4 and 7 selected: actual maps 1, 10, 100 and 2047
        for (const u32 dense : { 0U, 2U, 4U, 7U }) {
            selected.mapIndices[dense] = true;
        }

        QCOMPARE(namesOf(selected.filenames(EngineType::MVMZ)), (QByteArray(QByteArray("map1")+","+QByteArray("map10")+","+QByteArray("map100")+","+QByteArray("map2047"))));
    }

    void mapsBeyondMapCountAreIgnored() {
        Selected selected;
        selected.validIndices[3] = true;
        selected.validIndices[4] = true;
        selected.mapCount = 1;
        selected.mapIndices[0] = true;
        selected.mapIndices[1] = true;

        QCOMPARE(namesOf(selected.filenames(EngineType::MVMZ)), (QByteArray(QByteArray("map3"))));
    }

    void flagNames_data() {
        QTest::addColumn<u16>("flag");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("actors") << FileFlags_Actors << QByteArray("actors");
        QTest::newRow("armors") << FileFlags_Armors << QByteArray("armors");
        QTest::newRow("classes") << FileFlags_Classes << QByteArray("classes");
        QTest::newRow("common events") << FileFlags_CommonEvents << QByteArray("commonevents");
        QTest::newRow("enemies") << FileFlags_Enemies << QByteArray("enemies");
        QTest::newRow("items") << FileFlags_Items << QByteArray("items");
        QTest::newRow("skills") << FileFlags_Skills << QByteArray("skills");
        QTest::newRow("states") << FileFlags_States << QByteArray("states");
        QTest::newRow("troops") << FileFlags_Troops << QByteArray("troops");
        QTest::newRow("weapons") << FileFlags_Weapons << QByteArray("weapons");
        QTest::newRow("system") << FileFlags_System << QByteArray("system");
        QTest::newRow("database") << FileFlags_Database << QByteArray("terms");
    }

    void flagNames() {
        QFETCH(u16, flag);
        QFETCH(QByteArray, expected);

        Selected selected;
        selected.flags = flag;

        QCOMPARE(namesOf(selected.filenames(EngineType::MVMZ)), (QByteArray{ expected }));
    }

    void scriptsAreNamedByEngine_data() {
        QTest::addColumn<EngineType>("engine");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("MV/MZ plugins") << EngineType::MVMZ << QByteArray("plugins");
        QTest::newRow("RM2K scripts") << EngineType::RM2K << QByteArray("scripts");
    }

    void scriptsAreNamedByEngine() {
        QFETCH(EngineType, engine);
        QFETCH(QByteArray, expected);

        Selected selected;
        selected.flags = FileFlags_Scripts;

        QCOMPARE(namesOf(selected.filenames(engine)), (QByteArray{ expected }));
    }

    void mapsComeBeforeFlagsAndFlagsKeepBitOrder() {
        Selected selected;
        selected.validIndices[5] = true;
        selected.mapCount = 1;
        selected.mapIndices[0] = true;
        selected.flags = FileFlags_System | FileFlags_Actors | FileFlags_Items;

        QCOMPARE(namesOf(selected.filenames(EngineType::MVMZ)), (QByteArray(QByteArray("map5")+","+QByteArray("actors")+","+QByteArray("items")+","+QByteArray("system"))));
    }

    void mapFlagAloneProducesNoFile() {
        Selected selected;
        selected.flags = FileFlags_Map;

        QVERIFY(selected.filenames(EngineType::MVMZ).empty());
    }

    void emptyReflectsMapsAndFlags() {
        Selected selected;
        QVERIFY(selected.empty());

        // A map bit past mapCount is not selected
        selected.mapIndices[3] = true;
        selected.mapCount = 3;
        QVERIFY(selected.empty());

        selected.mapCount = 4;
        QVERIFY(!selected.empty());

        selected.mapIndices.reset();
        QVERIFY(selected.empty());

        selected.flags = FileFlags_Items;
        QVERIFY(!selected.empty());
    }

    void forEngineAddsDatabaseOnlyForRM2KWithDatabaseFamilyFlags_data() {
        QTest::addColumn<EngineType>("engine");
        QTest::addColumn<u16>("flags");
        QTest::addColumn<bool>("promoted");

        QTest::newRow("RM2K items") << EngineType::RM2K << FileFlags_Items << true;
        QTest::newRow("RM2K system") << EngineType::RM2K << FileFlags_System << true;
        QTest::newRow("RM2K scripts only") << EngineType::RM2K << FileFlags_Scripts << false;
        QTest::newRow("RM2K nothing") << EngineType::RM2K << u16{ 0 } << false;
        QTest::newRow("MV/MZ items") << EngineType::MVMZ << FileFlags_Items << false;
    }

    void forEngineAddsDatabaseOnlyForRM2KWithDatabaseFamilyFlags() {
        QFETCH(EngineType, engine);
        QFETCH(u16, flags);
        QFETCH(bool, promoted);

        Selected selected;
        selected.flags = flags;
        selected.mapCount = 2;

        const Selected result = selected.forEngine(engine);

        QCOMPARE((result.flags & FileFlags_Database) != 0, promoted);
        QCOMPARE(static_cast<u16>(result.flags & ~FileFlags_Database), flags);
        QCOMPARE(selected.flags, flags);  // the original is untouched
        QCOMPARE(result.mapCount, selected.mapCount);
    }
};

QTEST_APPLESS_MAIN(TestTypes)
#include "tst_types.moc"
