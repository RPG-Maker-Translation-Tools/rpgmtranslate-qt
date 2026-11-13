#include "Utils.hpp"

#include "ModalCatcher.hpp"
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

    // A directory that is not there fails the open, and the user is asked what to do about it
    void aFileThatCannotBeOpenedCanBeLeftUnwritten() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath(u"missing/out.txt"_s);
        ModalCatcher catcher;
        catcher.press = u"Continue anyway"_s;

        const auto opened = openForWriting(nullptr, path, u"/out.txt"_qsv);

        QVERIFY(opened.has_value());
        QVERIFY(*opened == nullptr);
        QVERIFY(catcher.text.isEmpty() == false);
        QCOMPARE(path, dir.filePath(u"missing/out.txt"_s));
    }

    void aFileThatCannotBeOpenedCanBeAbandoned() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath(u"missing/out.txt"_s);
        ModalCatcher catcher;
        catcher.press = u"Abort"_s;

        const auto opened = openForWriting(nullptr, path, u"/out.txt"_qsv);

        QVERIFY(!opened.has_value());
    }

    void aFileThatCannotBeOpenedIsOpenedAgainOnRetry() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString folder = dir.filePath(u"missing"_s);
        QString path = folder + u"/out.txt"_s;
        ModalCatcher catcher;
        catcher.press = u"Retry"_s;
        // The cause is gone by the time the user presses the button
        catcher.beforeAnswer = [&folder] -> void { QVERIFY(QDir().mkpath(folder)); };

        const auto opened = openForWriting(nullptr, path, u"/out.txt"_qsv);

        QVERIFY(opened.has_value());
        QVERIFY(*opened != nullptr);
        QCOMPARE((*opened)->fileName(), path);
        QCOMPARE(catcher.seen.size(), qsizetype{ 1 });
    }

    void aFileThatCannotBeOpenedCanGoToAnotherFolder() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QTemporaryDir elsewhere;
        QVERIFY(elsewhere.isValid());
        QString path = dir.filePath(u"missing/out.txt"_s);
        ModalCatcher catcher;
        catcher.press = u"Save to custom location"_s;
        catcher.file = elsewhere.path();

        const auto opened = openForWriting(nullptr, path, u"/out.txt"_qsv);

        QVERIFY(opened.has_value());
        QVERIFY(*opened != nullptr);
        QCOMPARE(path, elsewhere.path() + u"/out.txt"_s);
        QCOMPARE((*opened)->fileName(), path);
        QCOMPARE(catcher.dialogs.size(), qsizetype{ 1 });
    }

    void aCancelledFolderPickerAsksAgain() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath(u"missing/out.txt"_s);
        ModalCatcher catcher;
        catcher.press = u"Save to custom location"_s;
        // Nothing is picked the first time. The next question is answered by giving up.
        catcher.beforeAnswer = [&catcher] -> void {
            if (!catcher.dialogs.isEmpty()) {
                catcher.press = u"Abort"_s;
            }
        };

        const auto opened = openForWriting(nullptr, path, u"/out.txt"_qsv);

        QVERIFY(!opened.has_value());
        QCOMPARE(catcher.dialogs.size(), qsizetype{ 1 });
        QCOMPARE(catcher.seen.size(), qsizetype{ 2 });
        QCOMPARE(path, dir.filePath(u"missing/out.txt"_s));
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

    void commitFileReportsAWriteThatCannotBeCommitted() {
        ModalCatcher catcher;
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"blocked.txt"_s);
        QSaveFile file(path);
        QVERIFY(file.open(QFile::WriteOnly));
        QCOMPARE(file.write("content"), qint64{ 7 });

        // A folder where the file goes: the write is complete, but cannot replace it
        QVERIFY(QDir().mkdir(path));

        QVERIFY(!commitFile(nullptr, file));
        QVERIFY2(catcher.text.contains(u"Failed to save file"_qsv), qPrintable(catcher.text));
        QVERIFY(QFileInfo(path).isDir());
    }
};

QTEST_MAIN(TestUtilsIo)
#include "tst_utilsio.moc"
