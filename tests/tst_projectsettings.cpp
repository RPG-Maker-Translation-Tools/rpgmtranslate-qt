#include "ProjectSettings.hpp"

#include "TestFiles.hpp"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

namespace {
auto settingsAt(const QString& projectPath, const SourceDirectory directory) -> ProjectSettings {
    ProjectSettings settings;
    settings.projectPath = projectPath;
    settings.sourceDirectory = directory;
    return settings;
}
}  // namespace

class TestProjectSettings final : public QObject {
    Q_OBJECT

   private slots:
    void engineExtension_data() {
        QTest::addColumn<EngineType>("engine");
        QTest::addColumn<QString>("extension");

        QTest::newRow("MV/MZ") << EngineType::MVMZ << u"json"_s;
        QTest::newRow("VX Ace") << EngineType::VXAce << u"rvdata2"_s;
        QTest::newRow("VX") << EngineType::VX << u"rvdata"_s;
        QTest::newRow("XP") << EngineType::XP << u"rxdata"_s;
    }

    void engineExtension() {
        QFETCH(const EngineType, engine);
        QFETCH(const QString, extension);

        ProjectSettings settings;
        settings.engineType = engine;

        QCOMPARE(settings.engineExtension().toString(), extension);
    }

    void programDataLivesInTheProject() {
        const auto settings = settingsAt(u"/games/demo"_s, SourceDirectory::LowercaseData);

        QCOMPARE(settings.programDataPath(), u"/games/demo/.rpgmtranslate"_s);
        QCOMPARE(settings.translationPath(), u"/games/demo/.rpgmtranslate/translation"_s);
        QCOMPARE(settings.backupPath(), u"/games/demo/.rpgmtranslate/backups"_s);
        QCOMPARE(settings.outputPath(), u"/games/demo/.rpgmtranslate/output"_s);
        QCOMPARE(settings.glossaryPath(), u"/games/demo/.rpgmtranslate/glossary.json"_s);
        QCOMPARE(settings.projectSettingsPath(), u"/games/demo/.rpgmtranslate/project-settings.json"_s);
        QCOMPARE(settings.baselinePath(), u"/games/demo/.rpgmtranslate/baseline-data"_s);
    }

    void sourcePathFollowsTheDirectoryKind() {
        QCOMPARE(settingsAt(u"/g"_s, SourceDirectory::UppercaseData).sourcePath(), u"/g/Data"_s);
        QCOMPARE(settingsAt(u"/g"_s, SourceDirectory::LowercaseData).sourcePath(), u"/g/data"_s);
        QCOMPARE(settingsAt(u"/g"_s, SourceDirectory::Root).sourcePath(), u"/g"_s);
    }

    // A baseline copy of the data, when one exists, is what reads and diffs run against
    void actualSourcePathPrefersTheBaseline() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto settings = settingsAt(dir.path(), SourceDirectory::UppercaseData);
        QCOMPARE(settings.actualSourcePath(), dir.path() + u"/Data"_s);

        QVERIFY(QDir().mkpath(settings.baselinePath()));
        QCOMPARE(settings.actualSourcePath(), settings.baselinePath() + u"/data"_s);
    }

    void rm2kDatabaseFollowsTheActualSource() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto settings = settingsAt(dir.path(), SourceDirectory::Root);
        QCOMPARE(settings.rm2kDatabasePath(), dir.path() + u"/RPG_RT.ldb"_s);

        QVERIFY(QDir().mkpath(settings.baselinePath()));
        QCOMPARE(settings.rm2kDatabasePath(), settings.baselinePath() + u"/data/RPG_RT.ldb"_s);
    }

    void defaults() {
        const ProjectSettings settings;

        QCOMPARE(settings.sourceDirectory, SourceDirectory::None);
        QCOMPARE(settings.engineType, EngineType::MVMZ);
        QCOMPARE(settings.duplicateMode, DuplicateMode::Allow);
        QCOMPARE(settings.lineLengthHint, u16{ 0 });
        QVERIFY(settings.readEncoding.isEmpty());
        QVERIFY(settings.writeEncoding.isEmpty());
        QVERIFY(settings.columns.empty());
    }

    // The settings file is read back by a possibly newer or older version
    void jsonRoundTrip() {
        ProjectSettings settings;
        settings.projectPath = u"/games/demo"_s;
        settings.sourceLang = u"ja"_s;
        settings.translationLang = u"en"_s;
        settings.readEncoding = u"Shift_JIS"_s;
        settings.lineLengthHint = 60;
        settings.hintFontSize = 22;
        settings.engineType = EngineType::VXAce;
        settings.sourceDirectory = SourceDirectory::UppercaseData;
        settings.duplicateMode = DuplicateMode::Remove;
        settings.completedFiles = QStringList{ u"Map001"_s, u"Items"_s };
        settings.columns.push_back(ColumnInfo{ u"Source"_s, 300 });
        settings.columns.push_back(ColumnInfo{ u"Translation"_s, 450 });

        const auto json = glz::write_json(settings);
        QVERIFY(json.has_value());

        const auto read = readLenient<ProjectSettings>(*json);
        QVERIFY(read.has_value());

        QCOMPARE(read->projectPath, settings.projectPath);
        QCOMPARE(read->sourceLang, settings.sourceLang);
        QCOMPARE(read->translationLang, settings.translationLang);
        QCOMPARE(read->readEncoding, settings.readEncoding);
        QCOMPARE(read->lineLengthHint, settings.lineLengthHint);
        QCOMPARE(read->hintFontSize, settings.hintFontSize);
        QCOMPARE(read->engineType, settings.engineType);
        QCOMPARE(read->sourceDirectory, settings.sourceDirectory);
        QCOMPARE(read->duplicateMode, settings.duplicateMode);
        QCOMPARE(read->completedFiles, settings.completedFiles);
        QCOMPARE(read->columns.size(), usize{ 2 });
        QCOMPARE(read->columns[1].name, u"Translation"_s);
        QCOMPARE(read->columns[1].width, u16{ 450 });
    }
};

QTEST_MAIN(TestProjectSettings)
#include "tst_projectsettings.moc"
