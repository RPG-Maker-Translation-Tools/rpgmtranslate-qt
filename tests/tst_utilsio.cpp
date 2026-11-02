#include "Utils.hpp"

#include "ProjectSettings.hpp"
#include "TestFiles.hpp"
#include "Types.hpp"

#include <QDir>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest>

class TestUtilsIo final : public QObject {
    Q_OBJECT

   private slots:
    void serdeFormatExtension_data() {
        QTest::addColumn<SerdeFormat>("format");
        QTest::addColumn<QString>("extension");

        QTest::newRow("csv") << SerdeFormat::Csv << u".csv"_s;
        QTest::newRow("xlsx") << SerdeFormat::Xlsx << u".xlsx"_s;
        QTest::newRow("xml") << SerdeFormat::Xml << u".xml"_s;
        QTest::newRow("json") << SerdeFormat::Json << u".json"_s;
        QTest::newRow("yaml") << SerdeFormat::Yaml << u".yaml"_s;
    }

    void serdeFormatExtension() {
        QFETCH(const SerdeFormat, format);
        QFETCH(const QString, extension);

        QCOMPARE(::serdeFormatExtension(format).toString(), extension);
    }

    void mvmzSavesAreInTheSaveDirectory() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ProjectSettings settings;
        settings.projectPath = dir.path();
        settings.engineType = EngineType::MVMZ;

        const SaveLocation plain = saveLocation(&settings);
        QCOMPARE(plain.directory, dir.path() + u"/save"_s);
        QCOMPARE(plain.filter, (QStringList{ u"*.rpgsave"_s, u"*.rmmzsave"_s }));
        QVERIFY(plain.recursive);

        // A web deployment keeps them under `www`
        QVERIFY(QDir().mkpath(dir.path() + u"/www"_s));
        QCOMPARE(saveLocation(&settings).directory, dir.path() + u"/www/save"_s);
    }

    void otherEnginesKeepSavesBesideTheGame_data() {
        QTest::addColumn<EngineType>("engine");
        QTest::addColumn<QString>("filter");

        QTest::newRow("VX Ace") << EngineType::VXAce << u"Save*.rvdata2"_s;
        QTest::newRow("VX") << EngineType::VX << u"Save*.rvdata"_s;
        QTest::newRow("XP") << EngineType::XP << u"Save*.rxdata"_s;
        QTest::newRow("2000/2003") << EngineType::RM2K << u"Save*.lsd"_s;
    }

    void otherEnginesKeepSavesBesideTheGame() {
        QFETCH(const EngineType, engine);
        QFETCH(const QString, filter);

        ProjectSettings settings;
        settings.projectPath = u"/games/demo"_s;
        settings.engineType = engine;

        const SaveLocation location = saveLocation(&settings);

        QCOMPARE(location.directory, u"/games/demo"_s);
        QCOMPARE(location.filter, QStringList{ filter });
        QVERIFY(!location.recursive);
    }

    void openForWritingCreatesTheFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QString path = dir.filePath(u"out.txt"_s);

        auto opened = openForWriting(nullptr, path, u"/ignored"_qsv);

        QVERIFY(opened.has_value());
        QVERIFY(*opened != nullptr);
        QCOMPARE((*opened)->fileName(), path);
        QCOMPARE(path, dir.filePath(u"out.txt"_s));

        QCOMPARE((*opened)->write("hello"), qint64{ 5 });
        QVERIFY(commitFile(nullptr, **opened));
        QCOMPARE(readFile(path), QByteArray("hello"));
    }

    // The target only changes when the write is committed, so a failure half way keeps the old file
    void writingIsAtomic() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"keep.txt"_s);
        writeFile(path, "original");

        QString opened = path;
        {
            auto file = openForWriting(nullptr, opened, u"/ignored"_qsv);
            QVERIFY(file.has_value());
            QVERIFY(*file != nullptr);

            QCOMPARE((*file)->write("replacement"), qint64{ 11 });
            QCOMPARE(readFile(path), QByteArray("original"));
        }

        // Dropped without a commit
        QCOMPARE(readFile(path), QByteArray("original"));

        auto file = openForWriting(nullptr, opened, u"/ignored"_qsv);
        QVERIFY(file.has_value());
        QCOMPARE((*file)->write("replacement"), qint64{ 11 });
        QVERIFY(commitFile(nullptr, **file));
        QCOMPARE(readFile(path), QByteArray("replacement"));
    }

    void commitFileCanBeFollowedByAnotherWrite() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QString path = dir.filePath(u"twice.txt"_s);

        for (const QByteArray content : { QByteArray("one"), QByteArray("second") }) {
            auto file = openForWriting(nullptr, path, u"/ignored"_qsv);
            QVERIFY(file.has_value());
            QCOMPARE((*file)->write(content), static_cast<qint64>(content.size()));
            QVERIFY(commitFile(nullptr, **file));
            QCOMPARE(readFile(path), content);
        }
    }
};

QTEST_MAIN(TestUtilsIo)
#include "tst_utilsio.moc"
