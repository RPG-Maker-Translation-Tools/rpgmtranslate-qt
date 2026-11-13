#include "glazemeta.hpp"

#include "Settings.hpp"
#include "TestFiles.hpp"
#include "Types.hpp"

#include <QTemporaryDir>
#include <QtTest>

// Not in the anonymous namespace: glaze's reflection can't name a type without external linkage
struct Sample {
    i32 count = 5;
    string name = "default";
    vector<i32> list;
};

namespace {
auto filenameOf(const string_view name) -> FilenameArray {
    FilenameArray filename{};
    std::copy_n(name.data(), name.size(), filename.data());
    return filename;
}
}  // namespace

class TestJson final : public QObject {
    Q_OBJECT

   private slots:
    // Files outlive versions: a key a newer version added must not make an older one drop the file
    void lenientReadSkipsUnknownKeys() {
        const auto sample = readLenient<Sample>(R"({"count":7,"future":{"a":[1,2],"b":null},"name":"set"})");

        QVERIFY(sample.has_value());
        QCOMPARE(sample->count, 7);
        QCOMPARE(QString::fromStdString(sample->name), u"set"_s);
    }

    void lenientReadKeepsDefaultsForMissingKeys() {
        const auto sample = readLenient<Sample>(R"({"list":[1,2,3]})");

        QVERIFY(sample.has_value());
        QCOMPARE(sample->count, 5);
        QCOMPARE(QString::fromStdString(sample->name), u"default"_s);
        QCOMPARE(sample->list.size(), usize{ 3 });

        QVERIFY(readLenient<Sample>("{}").has_value());
    }

    void lenientReadRejectsMalformedJson() {
        QVERIFY(!readLenient<Sample>(R"({"count":)").has_value());
        QVERIFY(!readLenient<Sample>("").has_value());
        QVERIFY(!readLenient<Sample>(R"({"count":"not a number"})").has_value());
    }

    void settingsReadFromAnEmptyOrFutureFile() {
        QVERIFY(readLenient<Settings>("{}").has_value());
        QVERIFY(readLenient<Settings>(R"({"added_in_a_later_version":{"x":1}})").has_value());
    }

    // Saving and loading must not change the settings: written again, they come out the same
    void settingsSurviveASaveAndLoad() {
        const auto defaults = Settings{};
        const auto written = glz::write_json(defaults);

        QVERIFY(written.has_value());

        const auto loaded = readLenient<Settings>(*written);

        QVERIFY(loaded.has_value());
        QCOMPARE(QString::fromStdString(*glz::write_json(*loaded)), QString::fromStdString(*written));
    }

    void hashesReadAndWriteFilenames() {
        const auto hashes =
            readLenient<HashMap<FilenameArray, u64>>(R"({"Actors.json":42,"Map001.json":7,"System.json":9})");

        QVERIFY(hashes.has_value());
        QCOMPARE(hashes->size(), usize{ 3 });
        QCOMPARE(hashes->find(filenameOf("Actors.json"))->second, u64{ 42 });
        QCOMPARE(hashes->find(filenameOf("Map001.json"))->second, u64{ 7 });

        const auto written = glz::write_json(*hashes);
        QVERIFY(written.has_value());

        const auto again = readLenient<HashMap<FilenameArray, u64>>(*written);
        QVERIFY(again.has_value());
        QCOMPARE(again->size(), usize{ 3 });
        QCOMPARE(again->find(filenameOf("System.json"))->second, u64{ 9 });
    }

    // A name that does not fit a `FilenameArray` would be copied past its end: the file is refused instead
    void hashesRefuseNamesThatDoNotFit() {
        QVERIFY(!(readLenient<HashMap<FilenameArray, u64>>(R"({"a_much_too_long_filename.json":1})").has_value()));
        QVERIFY(!(readLenient<HashMap<FilenameArray, u64>>(R"({"ab":1})").has_value()));
        QVERIFY((readLenient<HashMap<FilenameArray, u64>>("{}").has_value()));
    }

    // The metadata of rvpacker keeps hashes as signed numbers: the same 64 bits as the unsigned ones
    void metadataReadsHashesOfEitherSignAndTheOptionsAroundThem() {
        const auto metadata = readLenient<Metadata>(
            R"({"hashes":{"Actors.json":42,"System.json":-1},"lineSeparator":"|","duplicateMode":1})"
        );

        QVERIFY(metadata.has_value());
        QCOMPARE(metadata->hashes.find(filenameOf("Actors.json"))->second, u64{ 42 });
        QCOMPARE(metadata->hashes.find(filenameOf("System.json"))->second, std::numeric_limits<u64>::max());
        QCOMPARE(metadata->lineSeparator.value_or(string()), string("|"));
        QCOMPARE(metadata->duplicateMode.value_or(0), u8{ 1 });
        QVERIFY(!metadata->readEncoding.has_value());

        // Written back, the hash is the signed number again
        const auto written = glz::write_json(*metadata);
        QVERIFY(written.has_value());
        QVERIFY(QString::fromStdString(*written).contains(u"\"System.json\":-1"_qsv));
    }

    void metadataRefusesWhatItCannotHold_data() {
        QTest::addColumn<QByteArray>("json");

        QTest::newRow("a name that does not fit") << QByteArray(R"({"hashes":{"a_much_too_long_filename.json":1}})");
        QTest::newRow("a hash that is no number") << QByteArray(R"({"hashes":{"Actors.json":"text"}})");
        QTest::newRow("a hash that is a fraction") << QByteArray(R"({"hashes":{"Actors.json":1.5}})");
    }

    void metadataRefusesWhatItCannotHold() {
        QFETCH(QByteArray, json);

        QVERIFY(!readLenient<Metadata>(string_view(json.constData(), static_cast<usize>(json.size()))).has_value());
    }

    // The threshold belongs to the fuzzy and the combined modes only
    void matchModeRoundTripsWithItsThreshold_data() {
        QTest::addColumn<u8>("tag");
        QTest::addColumn<f64>("threshold");

        QTest::newRow("exact") << static_cast<u8>(MatchMode::Tag::Exact) << 0.0;
        QTest::newRow("fuzzy") << static_cast<u8>(MatchMode::Tag::Fuzzy) << 0.75;
        QTest::newRow("both") << static_cast<u8>(MatchMode::Tag::Both) << 0.5;
    }

    void matchModeRoundTripsWithItsThreshold() {
        QFETCH(u8, tag);
        QFETCH(f64, threshold);

        // The threshold comes after the mode in the file, as it is written
        const string json = std::format(R"({{"mode":{},"fuzzyThreshold":{},"caseSensitive":true}})", tag, threshold);
        const auto info = readLenient<MatchModeInfo>(json);

        QVERIFY(info.has_value());
        QCOMPARE(static_cast<u8>(info->mode.tag), tag);
        QVERIFY(info->case_sensitive);

        const auto written = glz::write_json(*info);
        QVERIFY(written.has_value());
        const auto again = readLenient<MatchModeInfo>(*written);

        QVERIFY(again.has_value());
        QCOMPARE(static_cast<u8>(again->mode.tag), tag);

        if (tag != static_cast<u8>(MatchMode::Tag::Exact)) {
            QCOMPARE(again->mode.fuzzy.threshold, threshold);
        }
    }

    void jsonFileIsReadFromItsPath() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"settings.json"_s);
        writeFile(path, R"({"count":9})");

        const auto sample = readJsonFile<Sample>(path);

        QVERIFY(sample.has_value());
        QCOMPARE(sample->count, 9);
        // A good file is left as it is
        QVERIFY(!QFile::exists(path + CORRUPT_SUFFIX.toString()));
    }

    // The backup is what `keepBackup` made before the last save: a corrupt file must fall back to it
    void corruptFileFallsBackToTheBackupAndIsKept() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"settings.json"_s);
        const QString corrupt = path + CORRUPT_SUFFIX.toString();
        writeFile(path, "{broken");
        writeFile(path + BACKUP_SUFFIX.toString(), R"({"count":3})");

        const auto sample = readJsonFile<Sample>(path);

        QVERIFY(sample.has_value());
        QCOMPARE(sample->count, 3);
        QCOMPARE(readFile(corrupt), QByteArray("{broken"));
        // The restored file is the backup, so the next start reads it directly
        QCOMPARE(readFile(path), QByteArray(R"({"count":3})"));
    }

    void missingFileFallsBackToTheBackup() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"settings.json"_s);
        writeFile(path + BACKUP_SUFFIX.toString(), R"({"count":4})");

        const auto sample = readJsonFile<Sample>(path);

        QVERIFY(sample.has_value());
        QCOMPARE(sample->count, 4);
        QCOMPARE(readFile(path), QByteArray(R"({"count":4})"));
    }

    // `accept` is for values that parse but this version can't use: the backup gets its chance, then nothing is left
    void rejectedValuesTryTheBackup() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"settings.json"_s);
        writeFile(path, R"({"count":-1})");
        writeFile(path + BACKUP_SUFFIX.toString(), R"({"count":2})");

        const auto positive = [](const Sample& sample) -> bool { return sample.count > 0; };

        const auto sample = readJsonFile<Sample>(path, positive);

        QVERIFY(sample.has_value());
        QCOMPARE(sample->count, 2);
    }

    void nothingUsableGivesNothing() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"settings.json"_s);

        QVERIFY(!readJsonFile<Sample>(path).has_value());

        writeFile(path, "{broken");
        writeFile(path + BACKUP_SUFFIX.toString(), "also {broken");

        QVERIFY(!readJsonFile<Sample>(path).has_value());
        // Nothing was restored over the file
        QCOMPARE(readFile(path), QByteArray("{broken"));
    }
};

QTEST_MAIN(TestJson)

#include "tst_json.moc"
