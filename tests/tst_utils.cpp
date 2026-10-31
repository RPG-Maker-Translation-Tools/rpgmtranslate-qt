#include "Utils.hpp"

#include "Constants.hpp"
#include "QtAliases.hpp"
#include "TestFiles.hpp"

#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

namespace {
auto toStrings(const QSVList& parts) -> QStringList {
    QStringList strings;
    strings.reserve(parts.size());

    for (const QStringView part : parts) {
        strings.append(part.toString());
    }

    return strings;
}
}  // namespace

class TestUtils final : public QObject {
    Q_OBJECT

   private slots:
    // A translation file line is `source<#>translation<#>...`: the source column first, then one per translation
    void lineParts_data() {
        QTest::addColumn<QString>("line");
        QTest::addColumn<QStringList>("parts");

        QTest::newRow("one translation") << u"a<#>b"_s << QStringList{ u"a"_s, u"b"_s };
        QTest::newRow("two translations") << u"a<#>b<#>c"_s << QStringList{ u"a"_s, u"b"_s, u"c"_s };
        QTest::newRow("empty translation") << u"a<#>"_s << QStringList{ u"a"_s, QString() };
        QTest::newRow("empty source") << u"<#>b"_s << QStringList{ QString(), u"b"_s };
        QTest::newRow("empty gap") << u"a<#><#>c"_s << QStringList{ u"a"_s, QString(), u"c"_s };
        QTest::newRow("escaped line break") << u"a\\#b<#>c"_s << QStringList{ u"a\\#b"_s, u"c"_s };
    }

    void lineParts() {
        QFETCH(QString, line);
        QFETCH(QStringList, parts);

        QCOMPARE(toStrings(::lineParts(line, 1, u"file.txt"_s)), parts);
    }

    // Without a separator the line is not a row: it gives no parts and a warning that names the line and the file
    void linePartsRejectsALineWithoutSeparator() {
        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(u"Couldn't split text at line 5 in file notes\\.txt"_s)
        );

        QVERIFY(::lineParts(u"no separator here", 5, u"notes.txt"_s).isEmpty());
    }

    void translationIsTheLastNonEmptyColumn() {
        const QString line = u"src<#>first<#>second<#>"_s;
        const QSVList parts = ::lineParts(line, 1, u"f"_s);

        const Translation translation = getTranslation(parts);

        QCOMPARE(translation.getTranslation().toString(), u"second"_s);
        // Counted from the source column, which is 0
        QCOMPARE(translation.getIndex(), u8{ 2 });
    }

    void translationOfARowWithoutOneIsEmpty() {
        const QString line = u"src<#><#>"_s;
        const QSVList parts = ::lineParts(line, 1, u"f"_s);

        const Translation translation = getTranslation(parts);

        QVERIFY(translation.getTranslation().isEmpty());
        QCOMPARE(translation.getIndex(), u8{ 0 });
    }

    void sourceAndTranslationsSplitTheParts() {
        const QString line = u"src<#>one<#>two"_s;
        const QSVList parts = ::lineParts(line, 1, u"f"_s);

        QCOMPARE(getSource(parts).toString(), u"src"_s);
        QCOMPARE(toStrings(getTranslations(parts)), (QStringList{ u"one"_s, u"two"_s }));
    }

    void hasTranslation_data() {
        QTest::addColumn<QString>("line");
        QTest::addColumn<bool>("translated");

        QTest::newRow("empty") << u"src<#>"_s << false;
        // Whitespace alone is not a translation
        QTest::newRow("blank") << u"src<#>   "_s << false;
        QTest::newRow("text") << u"src<#>text"_s << true;
        QTest::newRow("second column only") << u"src<#><#>text"_s << true;
        QTest::newRow("padded text") << u"src<#> text "_s << true;
        // The source is not a translation
        QTest::newRow("source only") << u"src<#><#>"_s << false;
    }

    void hasTranslation() {
        QFETCH(QString, line);
        QFETCH(bool, translated);

        const QSVList parts = ::lineParts(line, 1, u"f"_s);

        QCOMPARE(::hasTranslation(parts), translated);
    }

    void isRowLine_data() {
        QTest::addColumn<QString>("line");
        QTest::addColumn<bool>("isRow");

        QTest::newRow("text") << u"Hello<#>Hola"_s << true;
        QTest::newRow("comment") << u"<!>NAME<#>Alice"_s << false;
        QTest::newRow("id comment") << u"<!>ID<#>3"_s << false;
        QTest::newRow("bookmark") << u"<!>BOOKMARK<#>note"_s << false;
        // A map's display name is a comment, but it is text to translate
        QTest::newRow("map display name") << u"<!>IN-GAME DISPLAYED NAME: Town<#>"_s << true;
    }

    void isRowLine() {
        QFETCH(QString, line);
        QFETCH(bool, isRow);

        QCOMPARE(::isRowLine(line), isRow);
    }

    void chopCarriageReturn_data() {
        QTest::addColumn<QString>("line");
        QTest::addColumn<QString>("chopped");

        QTest::newRow("crlf") << u"abc\r"_s << u"abc"_s;
        QTest::newRow("lf") << u"abc"_s << u"abc"_s;
        QTest::newRow("only cr") << u"\r"_s << QString();
        QTest::newRow("empty") << QString() << QString();
        // Only the one of the line ending
        QTest::newRow("two cr") << u"a\r\r"_s << u"a\r"_s;
        QTest::newRow("inner cr") << u"a\rb"_s << u"a\rb"_s;
    }

    void chopCarriageReturn() {
        QFETCH(QString, line);
        QFETCH(QString, chopped);

        QCOMPARE(::chopCarriageReturn(line).toString(), chopped);
    }

    void gameTitleLine_data() {
        QTest::addColumn<QString>("content");
        QTest::addColumn<bool>("found");
        QTest::addColumn<QString>("title");

        QTest::newRow("lf") << u"<!>NAME<#>Game Title\nMy Game<#>Mein Spiel\n"_s << true << u"My Game<#>Mein Spiel"_s;
        QTest::newRow("crlf") << u"<!>NAME<#>Game Title\r\nMy Game\r\nrest"_s << true << u"My Game"_s;
        QTest::newRow("no final line ending") << u"<!>NAME<#>Game Title\nMy Game"_s << true << u"My Game"_s;
        QTest::newRow("after other lines")
            << u"a<#>b\n<!>NAME<#>Game Title\nTitle<#>"_s << true << u"Title<#>"_s;
        QTest::newRow("marker is the last line") << u"<!>NAME<#>Game Title\n"_s << false << QString();
        QTest::newRow("marker without line ending") << u"<!>NAME<#>Game Title"_s << false << QString();
        QTest::newRow("no marker") << u"a<#>b\nc<#>d\n"_s << false << QString();
    }

    void gameTitleLine() {
        QFETCH(QString, content);
        QFETCH(bool, found);
        QFETCH(QString, title);

        const auto line = ::gameTitleLine(content, content.indexOf(GAME_TITLE_MARKER));

        QCOMPARE(line.has_value(), found);

        if (found) {
            QCOMPARE(line->toString(), title);
        }
    }

    void entryNameOfLine_data() {
        QTest::addColumn<QString>("line");
        QTest::addColumn<bool>("isName");
        QTest::addColumn<QString>("name");

        QTest::newRow("name") << u"<!>NAME<#>Alice"_s << true << u"Alice"_s;
        QTest::newRow("name with translation") << u"<!>NAME<#>Alice<#>Alicia"_s << true << u"Alice"_s;
        QTest::newRow("empty name") << u"<!>NAME<#>"_s << true << QString();
        QTest::newRow("bare marker") << u"<!>NAME"_s << true << QString();
        // `<!>NAMES` and the like are other comments
        QTest::newRow("longer comment") << u"<!>NAMES<#>x"_s << false << QString();
        QTest::newRow("id") << u"<!>ID<#>3"_s << false << QString();
        QTest::newRow("row") << u"Alice<#>Alicia"_s << false << QString();
    }

    void entryNameOfLine() {
        QFETCH(QString, line);
        QFETCH(bool, isName);
        QFETCH(QString, name);

        const auto entry = ::entryNameOfLine(line);

        QCOMPARE(entry.has_value(), isName);

        if (isName) {
            QCOMPARE(entry->toString(), name);
        }
    }

    // The name belongs to the rows after it, up to the `<!>ID` row that starts the next entry
    void entryNameForStopsAtTheEntrysIdRow() {
        const QStringList lines = {
            u"<!>ID<#>1"_s,
            u"<!>NAME<#>Alice"_s,
            u"description<#>text"_s,
            u"<!>ID<#>2"_s,
            u"other<#>text"_s,
        };

        QSVList views;

        for (const QString& line : lines) {
            views.append(line);
        }

        QCOMPARE(entryNameFor(views, 1).toString(), u"Alice"_s);
        QCOMPARE(entryNameFor(views, 2).toString(), u"Alice"_s);
        // Entry 2 has no name: the one of entry 1 is not its own
        QVERIFY(entryNameFor(views, 3).isEmpty());
        QVERIFY(entryNameFor(views, 4).isEmpty());
        QVERIFY(entryNameFor(views, 0).isEmpty());
        // A row past the end counts as the last one
        QVERIFY(entryNameFor(views, 100).isEmpty());
    }

    void translatableSource_data() {
        QTest::addColumn<QString>("source");
        QTest::addColumn<bool>("translatable");
        QTest::addColumn<QString>("text");

        QTest::newRow("text") << u"Hello"_s << true << u"Hello"_s;
        QTest::newRow("map display name") << u"<!>IN-GAME DISPLAYED NAME: Town"_s << true << u"Town"_s;
        QTest::newRow("comment") << u"<!>NAME"_s << false << QString();
        QTest::newRow("bookmark") << u"<!>BOOKMARK"_s << false << QString();
        QTest::newRow("bare prefix") << u"<!>"_s << false << QString();
    }

    void translatableSource() {
        QFETCH(QString, source);
        QFETCH(bool, translatable);
        QFETCH(QString, text);

        const auto result = ::translatableSource(source);

        QCOMPARE(result.has_value(), translatable);

        if (translatable) {
            QCOMPARE(result->toString(), text);
        }
    }

    void countTextIgnoresCodes() {
        QCOMPARE(countText(u"Hello world"_s, 0).words, 2U);
        QCOMPARE(countText(u"\\C[2]Hello\\C[0] world"_s, 0).words, 2U);
    }

    // The line feeds that join the lines are whitespace to the counter, but not characters of the text
    void countTextDropsTheLineFeedsFromTheCharacters() {
        const QString text = u"Hello world"_s;
        const u32 withoutLines = countText(text, 0).characters;

        QCOMPARE(countText(text, 1).characters, withoutLines - 1);
        QCOMPARE(countText(text, 1).characters_no_whitespace, countText(text, 0).characters_no_whitespace);
    }

    void countTextNeverGoesBelowZero() {
        QCOMPARE(countText(QString(), 5).characters, 0U);
        QCOMPARE(countText(u"ab"_s, 100).characters, 0U);
    }

    void filenameArraySizes() {
        QVERIFY(!fitsFilenameArray(0));
        QVERIFY(!fitsFilenameArray(3));
        QVERIFY(fitsFilenameArray(4));
        QVERIFY(fitsFilenameArray(15));
        // The array is 16 bytes: a 16 character name leaves no room
        QVERIFY(!fitsFilenameArray(16));
        QVERIFY(!fitsFilenameArray(40));
    }

    void copyFilenameArray_data() {
        QTest::addColumn<int>("size");

        for (int size = 4; size < 16; size++) {
            QTest::newRow(qPrintable(QString::number(size))) << size;
        }
    }

    // Two overlapping loads and stores must cover every byte of the name, and none past it
    void copyFilenameArray() {
        QFETCH(int, size);

        static constexpr char NAME[] = "abcdefghijklmno";
        FilenameArray destination{};

        ::copyFilenameArray(NAME, static_cast<usize>(size), destination.data());

        QCOMPARE(QByteArray(destination.data(), size), QByteArray(NAME, size));

        for (usize idx = static_cast<usize>(size); idx < destination.size(); idx++) {
            QCOMPARE(destination[idx], '\0');
        }
    }

    void taggedTranslationKeepsTheViewAndTheIndex() {
        const QString text = u"translated text"_s;
        Translation translation(text, 3);

        QCOMPARE(translation.getIndex(), u8{ 3 });
        QCOMPARE(translation.getTranslation().data(), text.constData());
        QCOMPARE(translation.getTranslation().size(), text.size());

        // The index lives in the pointer's top byte: changing it must not move the view
        translation.setIndex(200);

        QCOMPARE(translation.getIndex(), u8{ 200 });
        QCOMPARE(translation.getTranslation().toString(), text);

        translation.setIndex(255);

        QCOMPARE(translation.getIndex(), u8{ 255 });
        QCOMPARE(translation.getTranslation().toString(), text);

        // And pointing at other text must not change the index
        const QString other = u"other"_s;
        translation.setTranslation(other);

        QCOMPARE(translation.getIndex(), u8{ 255 });
        QCOMPARE(translation.getTranslation().toString(), other);
    }

    void ffiStringsRoundTripUtf8() {
        const QString text = u"日本語 😀 text"_s;
        const QByteArray utf8 = text.toUtf8();

        const FFIString ffi = strtoffi(utf8);

        QCOMPARE(static_cast<qsizetype>(ffi.len), utf8.size());
        QCOMPARE(ffi.cap, 0U);
        QCOMPARE(QString::fromUtf8(ffitostr(ffi)), text);
    }

    void keepBackupCopiesTheFileAndReplacesAnOlderBackup() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"settings.json"_s);
        const QString backup = path + BACKUP_SUFFIX.toString();

        writeFile(path, "first");
        keepBackup(path);

        QCOMPARE(readFile(backup), QByteArray("first"));

        writeFile(path, "second");
        keepBackup(path);

        QCOMPARE(readFile(backup), QByteArray("second"));
        // The copy is made beside the backup first: nothing of it may be left behind
        QVERIFY(!QFile::exists(backup + u".tmp"_qsv.toString()));
    }

    // An empty file is a failed save, not something worth keeping: it must not replace a good backup
    void keepBackupSkipsAnEmptyOrMissingFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString empty = dir.filePath(u"empty.json"_s);
        const QString emptyBackup = empty + BACKUP_SUFFIX.toString();
        writeFile(empty, QByteArray());
        writeFile(emptyBackup, "good");

        keepBackup(empty);

        QCOMPARE(readFile(emptyBackup), QByteArray("good"));

        const QString missing = dir.filePath(u"missing.json"_s);
        keepBackup(missing);

        QVERIFY(!QFile::exists(missing + BACKUP_SUFFIX.toString()));
    }
};

QTEST_MAIN(TestUtils)

#include "tst_utils.moc"
