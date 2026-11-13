#include "Utils.hpp"

#include "ProjectSettings.hpp"
#include "TestFiles.hpp"
#include "Types.hpp"

#include <QBuffer>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QTemporaryDir>
#include <QtTest>

namespace {
// The system graphics of an engine other than MV/MZ
constexpr auto OLDER_SYSTEM = "/Graphics/System";
constexpr auto NEWER_SYSTEM = "/img/system";

auto makeSettings(const QString& root, const EngineType engine) -> ProjectSettings {
    ProjectSettings settings;
    settings.projectPath = root;
    settings.engineType = engine;
    settings.sourceDirectory = SourceDirectory::Root;
    return settings;
}

auto savePng(const QImage& image, const QString& path) -> bool {
    return QDir().mkpath(QFileInfo(path).path()) && image.save(path, "PNG");
}

// The picture as the encrypted `.rpgmvp` keeps it: the fake header, then the picture with its first 16 bytes masked
auto encryptedPng(const QImage& image) -> QByteArray {
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    for (i32 idx = 0; idx < 16; idx++) {
        png[idx] = static_cast<char>(png[idx] ^ static_cast<char>(0x5A + idx));
    }

    return QByteArray("RPGMV\0\0\0\0\x03\x01\0\0\0\0\0", 16) + png;
}

// Sets the modification time apart from what the previous write left, so that the caches see an edit even when the
// file system's clock is coarse
void touch(const QString& path, const qint64 secondsFromNow) {
    QFile file(path);
    QVERIFY(file.open(QFile::ReadWrite));
    QVERIFY(file.setFileTime(QDateTime::currentDateTime().addSecs(secondsFromNow), QFileDevice::FileModificationTime));
}

// A Window.png whose color squares each hold a distinct color, laid out as the engine's sheet is
auto windowSheet(const i32 offsetX, const i32 offsetY, const i32 square, const i32 color) -> QImage {
    QImage image(256, 256, QImage::Format_RGB32);
    image.fill(Qt::black);

    for (i32 row = 0; row < 4; row++) {
        for (i32 col = 0; col < 8; col++) {
            image.setPixel(offsetX + (col * square), offsetY + (row * square), qRgb((row * 8) + col + 1, color, 0));
        }
    }

    return image;
}

auto iconSheet(const i32 iconSize, const i32 columns, const i32 rows) -> QImage {
    QImage image(iconSize * columns, iconSize * rows, QImage::Format_RGB32);
    QPainter painter(&image);

    // Each icon is a flat square of its own color, 1-based
    for (i32 row = 0; row < rows; row++) {
        for (i32 col = 0; col < columns; col++) {
            painter.fillRect(
                QRect(col * iconSize, row * iconSize, iconSize, iconSize),
                QColor(qRgb((row * columns) + col + 1, 0, 0))
            );
        }
    }

    return image;
}

// The sheet of RPG Maker 2000/2003: two rows of ten color cells, each holding a color of its own
auto rm2kSheet(const i32 tag) -> QImage {
    QImage image(160, 96, QImage::Format_RGB32);
    image.fill(Qt::black);

    for (i32 color = 0; color < 20; color++) {
        image.setPixel((color % 10 * 16) + 8, 48 + (color / 10 * 16) + 8, qRgb(color + 1, tag, 0));
    }

    return image;
}

// A database that names `graphic` as the system graphic of the game: the header, then the chunk of the system data
// (0x16) that holds the chunk of the graphic name (0x13)
auto rm2kDatabase(const QByteArray& graphic) -> QByteArray {
    QByteArray database;
    database.append(char{ 11 });
    database.append("LcfDataBase");
    database.append(char{ 0x16 });
    database.append(static_cast<char>(2 + graphic.size()));
    database.append(char{ 0x13 });
    database.append(static_cast<char>(graphic.size()));
    database.append(graphic);
    return database;
}

void setModified(const QString& path, const QDateTime& time) {
    QFile file(path);
    QVERIFY(file.open(QFile::ReadWrite));
    QVERIFY(file.setFileTime(time, QFileDevice::FileModificationTime));
}
}  // namespace

class TestUtilsImages final : public QObject {
    Q_OBJECT

   private slots:
    void olderEnginesReadWindowColorsFromTheirSheet_data() {
        QTest::addColumn<EngineType>("engine");

        QTest::newRow("VX Ace") << EngineType::VXAce;
        QTest::newRow("VX") << EngineType::VX;
        QTest::newRow("XP") << EngineType::XP;
    }

    void olderEnginesReadWindowColorsFromTheirSheet() {
        QFETCH(const EngineType, engine);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(windowSheet(64, 96, 8, 7), dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/Window.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), engine);
        const auto colors = getWindowColors(&settings);

        QVERIFY(colors.has_value());
        QCOMPARE((*colors)[0], qRgb(1, 7, 0));
        QCOMPARE((*colors)[9], qRgb(10, 7, 0));
        QCOMPARE((*colors)[31], qRgb(32, 7, 0));
    }

    void mvmzReadsWindowColorsFromItsLargerSquares() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(windowSheet(96, 144, 12, 3), dir.path() + QString::fromLatin1(NEWER_SYSTEM) + u"/Window.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);
        const auto colors = getWindowColors(&settings);

        QVERIFY(colors.has_value());
        QCOMPARE((*colors)[0], qRgb(1, 3, 0));
        QCOMPARE((*colors)[8], qRgb(9, 3, 0));
        QCOMPARE((*colors)[31], qRgb(32, 3, 0));
    }

    void mvmzWebDeploymentKeepsItsImagesUnderWww() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(windowSheet(96, 144, 12, 5), dir.path() + u"/www"_s + QString::fromLatin1(NEWER_SYSTEM) + u"/Window.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);

        QVERIFY(getWindowColors(&settings).has_value());
    }

    void windowColorsAreReadAgainWhenTheImageIsEdited() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/Window.png"_s;
        QVERIFY(savePng(windowSheet(64, 96, 8, 1), path));
        touch(path, -60);

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);
        QCOMPARE((*getWindowColors(&settings))[0], qRgb(1, 1, 0));

        // Unchanged: the cached colors answer
        QCOMPARE((*getWindowColors(&settings))[0], qRgb(1, 1, 0));

        QVERIFY(savePng(windowSheet(64, 96, 8, 2), path));
        touch(path, 0);
        QCOMPARE((*getWindowColors(&settings))[0], qRgb(1, 2, 0));
    }

    void aMissingWindowImageFails() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    // A lint asks for the colors for each color code, so a failure is remembered rather than retried
    void aWindowFailureIsRememberedForAWhile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);
        QVERIFY(!getWindowColors(&settings).has_value());

        QVERIFY(savePng(windowSheet(64, 96, 8, 1), dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/Window.png"_s));
        QVERIFY(!getWindowColors(&settings).has_value());
    }

    void anUndecodableWindowImageFails() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir().mkpath(dir.path() + QString::fromLatin1(OLDER_SYSTEM)));
        writeFile(dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/Window.png"_s, "not a picture");

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    void anEncryptedWindowImageThatYieldsNoPictureFails_data() {
        QTest::addColumn<QByteArray>("content");

        // Decryption fails
        QTest::newRow("too short to decrypt") << "tiny"_ba;
        // The header that every encrypted file starts with, and nothing behind it
        QTest::newRow("only the header") << QByteArray("RPGMV\0\0\0\0\x03\x01\0\0\0\0\0", 16);
        // Twice the header: there is no picture behind it
        QTest::newRow("a zeroed header and no picture") << QByteArray(32, '\0');
    }

    void anEncryptedWindowImageThatYieldsNoPictureFails() {
        QFETCH(const QByteArray, content);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir().mkpath(dir.path() + QString::fromLatin1(NEWER_SYSTEM)));
        writeFile(dir.path() + QString::fromLatin1(NEWER_SYSTEM) + u"/Window.rpgmvp"_s, content);

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    void aWindowImageThatCannotBeOpenedFails() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // The path exists but is no file that can be read
        QVERIFY(QDir().mkpath(dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/Window.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    // RPG Maker 2000/2003 names its system graphic in the database
    void rm2kWithoutADatabaseFails() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::RM2K);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    void rm2kReadsColorsFromTheSheetTheDatabaseNames() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeFile(dir.path() + u"/RPG_RT.ldb"_s, rm2kDatabase("System2"));
        QVERIFY(savePng(rm2kSheet(4), dir.path() + u"/System/System2.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::RM2K);
        const auto colors = getWindowColors(&settings);

        QVERIFY(colors.has_value());
        QCOMPARE((*colors)[0], qRgb(1, 4, 0));
        QCOMPARE((*colors)[9], qRgb(10, 4, 0));
        QCOMPARE((*colors)[10], qRgb(11, 4, 0));
        QCOMPARE((*colors)[19], qRgb(20, 4, 0));
        // The game has no more colors than that
        QCOMPARE((*colors)[20], QRgb{ 0 });
        QCOMPARE((*colors)[31], QRgb{ 0 });
    }

    // The database is big, so the name is read again only when the file is edited
    void rm2kReadsTheDatabaseAgainWhenItIsEdited() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString database = dir.path() + u"/RPG_RT.ldb"_s;
        writeFile(database, rm2kDatabase("First"));
        touch(database, -60);
        QVERIFY(savePng(rm2kSheet(1), dir.path() + u"/System/First.png"_s));
        QVERIFY(savePng(rm2kSheet(2), dir.path() + u"/System/Second.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::RM2K);
        QCOMPARE((*getWindowColors(&settings))[0], qRgb(1, 1, 0));
        QCOMPARE((*getWindowColors(&settings))[0], qRgb(1, 1, 0));

        writeFile(database, rm2kDatabase("Second"));
        touch(database, 0);
        QCOMPARE((*getWindowColors(&settings))[0], qRgb(1, 2, 0));
    }

    void rm2kDatabaseThatCannotBeParsedFails() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeFile(dir.path() + u"/RPG_RT.ldb"_s, "not a database");

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::RM2K);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    void rm2kNamedGraphicThatIsMissingFails() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeFile(dir.path() + u"/RPG_RT.ldb"_s, rm2kDatabase("Nowhere"));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::RM2K);

        QVERIFY(!getWindowColors(&settings).has_value());
    }

    void anEncryptedWindowImageIsReadLikeAPlainOne() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir().mkpath(dir.path() + QString::fromLatin1(NEWER_SYSTEM)));
        writeFile(dir.path() + QString::fromLatin1(NEWER_SYSTEM) + u"/Window.rpgmvp"_s, encryptedPng(windowSheet(96, 144, 12, 3)));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);
        const auto colors = getWindowColors(&settings);

        QVERIFY(colors.has_value());
        QCOMPARE((*colors)[0], qRgb(1, 3, 0));
        QCOMPARE((*colors)[31], qRgb(32, 3, 0));
    }

    void iconsAreCutFromTheSheetLeftToRight() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(iconSheet(24, 16, 2), dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/IconSet.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        const auto first = getIcon(0, &settings);
        QVERIFY(first.has_value());
        QCOMPARE(first->size(), QSize(24, 24));
        QCOMPARE(first->toImage().pixel(0, 0), qRgb(1, 0, 0));

        const auto fifth = getIcon(4, &settings);
        QVERIFY(fifth.has_value());
        QCOMPARE(fifth->toImage().pixel(12, 12), qRgb(5, 0, 0));
    }

    // The second row of the sheet starts after the first row's icons, not one pixel below the top
    void iconsOfTheSecondRowComeFromTheSecondRow() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(iconSheet(24, 16, 2), dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/IconSet.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        const auto icon = getIcon(17, &settings);
        QVERIFY(icon.has_value());
        QCOMPARE(icon->toImage().pixel(0, 0), qRgb(18, 0, 0));
        QCOMPARE(icon->toImage().pixel(23, 23), qRgb(18, 0, 0));
    }

    void mvmzIconsAre32Pixels() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(iconSheet(32, 16, 2), dir.path() + QString::fromLatin1(NEWER_SYSTEM) + u"/IconSet.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);

        const auto icon = getIcon(1, &settings);
        QVERIFY(icon.has_value());
        QCOMPARE(icon->size(), QSize(32, 32));
        QCOMPARE(icon->toImage().pixel(0, 0), qRgb(2, 0, 0));
    }

    void iconsOutsideTheSheetFail_data() {
        QTest::addColumn<i32>("index");

        QTest::newRow("negative") << -1;
        QTest::newRow("first past the end") << 32;
        QTest::newRow("far past the end") << 1000000;
        // 64-bit math keeps a huge index from wrapping back into the sheet
        QTest::newRow("overflowing") << 2000000000;
    }

    void iconsOutsideTheSheetFail() {
        QFETCH(const i32, index);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(savePng(iconSheet(24, 16, 2), dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/IconSet.png"_s));

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        QVERIFY(!getIcon(index, &settings).has_value());
    }

    void iconsFailWithoutAnIconSet() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        QVERIFY(!getIcon(0, &settings).has_value());
    }

    void iconsFailWhenTheIconSetIsNotAPicture() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir().mkpath(dir.path() + QString::fromLatin1(OLDER_SYSTEM)));
        writeFile(dir.path() + QString::fromLatin1(OLDER_SYSTEM) + u"/IconSet.png"_s, "not a picture");

        const ProjectSettings settings = makeSettings(dir.path(), EngineType::VXAce);

        QVERIFY(!getIcon(0, &settings).has_value());
    }

    void aMissingSaveHasNoSummary() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);
        settings.activeSave = u"save/file1.rpgsave"_s;

        QVERIFY(!getSaveSummary(&settings).has_value());
    }

    void aSaveThatIsNotASaveHasNoSummary() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir().mkpath(dir.path() + u"/save"_s));
        writeFile(dir.path() + u"/save/file1.rpgsave"_s, "garbage");

        ProjectSettings settings = makeSettings(dir.path(), EngineType::MVMZ);
        settings.activeSave = u"save/file1.rpgsave"_s;

        QVERIFY(!getSaveSummary(&settings).has_value());
    }
};

QTEST_MAIN(TestUtilsImages)
#include "tst_utilsimages.moc"
