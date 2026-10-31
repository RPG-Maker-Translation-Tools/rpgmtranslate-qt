#include "glazemeta.hpp"

#include "Settings.hpp"
#include "TestFiles.hpp"

#include <QTemporaryDir>
#include <QtTest>

namespace {
struct Sample {
    i32 count = 5;
    string name = "default";
    vector<i32> list;
};

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
        QVERIFY(!readLenient<HashMap<FilenameArray, u64>>(R"({"a_much_too_long_filename.json":1})").has_value());
        QVERIFY(!readLenient<HashMap<FilenameArray, u64>>(R"({"ab":1})").has_value());
        QVERIFY(readLenient<HashMap<FilenameArray, u64>>("{}").has_value());
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
