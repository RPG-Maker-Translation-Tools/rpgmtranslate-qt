// The preview window of an asset: the page it picks for each kind of file, what it says when the file can't be shown,
// the search of a text and the zoom of a picture.
#include "AssetPreviewWidget.hpp"
#include "Constants.hpp"
#include "GraphicsAssetViewer.hpp"
#include "TestFiles.hpp"

#include <QBuffer>
#include <QDirIterator>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QImage>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QWheelEvent>
#include <QtTest>

namespace {
// The order of the pages of the stack, as the widget adds them
constexpr i32 ERROR_PAGE = 0;
constexpr i32 GRAPHICS_PAGE = 1;
constexpr i32 TEXT_PAGE = 3;

auto pngBytes() -> QByteArray {
    QImage image(8, 6, QImage::Format_RGB32);
    image.fill(Qt::red);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

// What the engine makes of a picture: the fake header, then the picture with its first 16 bytes XORed with the key
auto encrypted(QByteArray picture) -> QByteArray {
    static constexpr char HEADER[HEADER_LENGTH] = { 'R', 'P', 'G', 'M', 'V', 0, 0, 0, 0, 3, 1, 0, 0, 0, 0, 0 };

    for (i32 idx = 0; idx < HEADER_LENGTH; idx++) {
        picture[idx] = static_cast<char>(picture[idx] ^ static_cast<char>(0x35 + idx));
    }

    return QByteArray(HEADER, HEADER_LENGTH) + picture;
}

struct Preview {
    [[nodiscard]] auto stack() const -> QStackedWidget* { return widget.findChild<QStackedWidget*>(); }
    [[nodiscard]] auto page() const -> i32 { return stack()->currentIndex(); }
    [[nodiscard]] auto error() const -> QString { return qobject_cast<QLabel*>(stack()->widget(ERROR_PAGE))->text(); }
    [[nodiscard]] auto viewer() const -> GraphicsAssetViewer* { return widget.findChild<GraphicsAssetViewer*>(); }

    // Of the two line edits, the search is the one without the placeholder of the font sample
    [[nodiscard]] auto searchInput() const -> QLineEdit* {
        for (QLineEdit* const edit : widget.findChildren<QLineEdit*>()) {
            if (edit->placeholderText().isEmpty()) {
                return edit;
            }
        }

        return nullptr;
    }

    [[nodiscard]] auto fontSampleInput() const -> QLineEdit* {
        for (QLineEdit* const edit : widget.findChildren<QLineEdit*>()) {
            if (!edit->placeholderText().isEmpty()) {
                return edit;
            }
        }

        return nullptr;
    }

    // Created in this order: regular expression, whole word, case, search, previous, next
    [[nodiscard]] auto buttons() const -> QList<QPushButton*> {
        return searchInput()->parentWidget()->findChildren<QPushButton*>();
    }

    [[nodiscard]] auto resultsLabel() const -> QString {
        return searchInput()->parentWidget()->findChild<QLabel*>()->text();
    }

    void search(const QString& query) const {
        searchInput()->setText(query);
        emit searchInput()->returnPressed();
    }

    AssetPreviewWidget widget;
};

// The wheel is turned over the picture, which is the viewport of the view
void turnWheel(GraphicsAssetViewer& viewer, const i32 delta) {
    QWheelEvent event(QPointF(1, 1), QPointF(1, 1), QPoint(), QPoint(0, delta), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(viewer.viewport(), &event);
}

auto plainTextOf(const Preview& preview) -> QString {
    return preview.widget.findChild<QPlainTextEdit*>()->toPlainText();
}
}  // namespace

class TestAssetPreview final : public QObject {
    Q_OBJECT

    QTemporaryDir dir;

    [[nodiscard]] auto make(const QString& name, const QByteArray& content) const -> QString {
        const QString path = dir.path() + u'/' + name;
        writeFile(path, content);
        return path;
    }

   private slots:
    void aPictureIsShownInTheGraphicsPage() {
        Preview preview;
        preview.widget.showAsset(make(u"picture.png"_s, pngBytes()));

        QCOMPARE(preview.page(), GRAPHICS_PAGE);
        QCOMPARE(preview.viewer()->scene()->items().size(), 1);
    }

    void anEncryptedPictureIsDecryptedBeforeItIsShown() {
        Preview preview;
        preview.widget.showAsset(make(u"picture.rpgmvp"_s, encrypted(pngBytes())));

        QCOMPARE(preview.page(), GRAPHICS_PAGE);
        QCOMPARE(preview.viewer()->scene()->items().size(), 1);
    }

    void aPictureThatCannotBeShownIsToldAboutOnTheErrorPage_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QByteArray>("content");

        QTest::newRow("not a picture") << u"broken.png"_s << QByteArray("text");
        QTest::newRow("encrypted without a picture") << u"broken.rpgmvp"_s << QByteArray("tiny");
        QTest::newRow("encrypted, only the header") << u"empty.png_"_s << encrypted(pngBytes()).first(HEADER_LENGTH);
    }

    void aPictureThatCannotBeShownIsToldAboutOnTheErrorPage() {
        QFETCH(QString, name);
        QFETCH(QByteArray, content);

        Preview preview;
        // A picture is shown first, to tell its page from the error that replaces it
        preview.widget.showAsset(make(u"before.png"_s, pngBytes()));
        QCOMPARE(preview.page(), GRAPHICS_PAGE);

        preview.widget.showAsset(make(name, content));

        QCOMPARE(preview.page(), ERROR_PAGE);
        QVERIFY(!preview.error().isEmpty());
    }

    void aMissingFileIsToldAboutOnTheErrorPage_data() {
        QTest::addColumn<QString>("name");

        QTest::newRow("picture") << u"missing.png"_s;
        QTest::newRow("text") << u"missing.json"_s;
        QTest::newRow("encrypted picture") << u"missing.rpgmvp"_s;
    }

    void aMissingFileIsToldAboutOnTheErrorPage() {
        QFETCH(QString, name);

        Preview preview;
        preview.widget.showAsset(dir.path() + u'/' + name);

        QCOMPARE(preview.page(), ERROR_PAGE);
        QVERIFY(preview.error().contains(name));
    }

    void anExtensionThatIsNotKnownIsToldAboutOnTheErrorPage() {
        Preview preview;
        preview.widget.showAsset(make(u"file.xyz"_s, "x"));

        QCOMPARE(preview.page(), ERROR_PAGE);
        QVERIFY(!preview.error().isEmpty());
    }

    void aBinaryGameFileThatCannotBeConvertedIsToldAboutOnTheErrorPage() {
        Preview preview;
        preview.widget.showAsset(make(u"Actors.rvdata2"_s, "not a game file"));

        QCOMPARE(preview.page(), ERROR_PAGE);
    }

    void aTextFileIsShownWithoutTheBomAndItsPictureIsNotKept() {
        Preview preview;
        preview.widget.showAsset(make(u"before.png"_s, pngBytes()));
        preview.widget.showAsset(make(u"data.json"_s, "\xEF\xBB\xBF{\"a\":1}"));

        QCOMPARE(preview.page(), TEXT_PAGE);
        QCOMPARE(plainTextOf(preview), u"{\"a\":1}"_s);
    }

    void beautifyingAJsonFileIndentsItAndKeepsItsData() {
        Preview preview;
        preview.widget.showAsset(make(u"data.json"_s, R"({"a":[1,2],"b":"text"})"));

        QPushButton* beautify = nullptr;
        for (QPushButton* const button : preview.widget.findChildren<QPushButton*>()) {
            if (button->text() == u"Beautify"_s) {
                beautify = button;
            }
        }

        QVERIFY(beautify != nullptr);
        QVERIFY(!beautify->isHidden());
        beautify->click();

        const QString text = plainTextOf(preview);
        QVERIFY2(text.count(u'\n') >= 4, qPrintable(text));
        QCOMPARE(QJsonDocument::fromJson(text.toUtf8()), QJsonDocument::fromJson(R"({"a":[1,2],"b":"text"})"));

        // The next pass starts from what is shown
        beautify->click();
        QCOMPARE(plainTextOf(preview), text);
    }

    void searchingCountsTheMatchesAndTheButtonsWalkThemInACircle() {
        Preview preview;
        preview.widget.showAsset(make(u"data.json"_s, R"({"a":"foo","b":"Foo","c":"foo"})"));

        preview.search(u"foo"_s);
        QCOMPARE(preview.resultsLabel(), u"1/3"_s);

        const QList<QPushButton*> buttons = preview.buttons();
        QPushButton* const previous = buttons.at(buttons.size() - 2);
        QPushButton* const next = buttons.last();

        next->click();
        QCOMPARE(preview.resultsLabel(), u"2/3"_s);
        next->click();
        next->click();
        QCOMPARE(preview.resultsLabel(), u"1/3"_s);

        previous->click();
        QCOMPARE(preview.resultsLabel(), u"3/3"_s);
        // The text cursor follows the current match
        QCOMPARE(preview.widget.findChild<QPlainTextEdit*>()->textCursor().selectedText(), u"foo"_s);
    }

    void theOptionsOfTheSearchNarrowTheMatches() {
        Preview preview;
        preview.widget.showAsset(make(u"data.json"_s, R"({"a":"foo","b":"Foo","c":"foo","d":"foobar"})"));

        QList<QPushButton*> checkable;
        for (QPushButton* const button : preview.buttons()) {
            if (button->isCheckable()) {
                checkable.append(button);
            }
        }
        QCOMPARE(checkable.size(), 3);
        QPushButton* const regex = checkable[0];
        QPushButton* const whole = checkable[1];
        QPushButton* const matchCase = checkable[2];

        preview.search(u"foo"_s);
        QCOMPARE(preview.resultsLabel(), u"1/4"_s);

        matchCase->setChecked(true);
        preview.search(u"foo"_s);
        QCOMPARE(preview.resultsLabel(), u"1/3"_s);

        whole->setChecked(true);
        preview.search(u"foo"_s);
        QCOMPARE(preview.resultsLabel(), u"1/2"_s);

        whole->setChecked(false);
        regex->setChecked(true);
        preview.search(u"fo+b"_s);
        QCOMPARE(preview.resultsLabel(), u"1/1"_s);

        // Without the case option, the pattern ignores the case
        matchCase->setChecked(false);
        preview.search(u"F(O)+$"_s);
        QCOMPARE(preview.resultsLabel(), u"No results"_s);
        preview.search(u"F(O)+\""_s);
        QCOMPARE(preview.resultsLabel(), u"1/3"_s);
    }

    void aSearchThatCannotMatchSaysSo() {
        Preview preview;
        preview.widget.showAsset(make(u"data.json"_s, "bbb"));

        preview.search(u"zzz"_s);
        QCOMPARE(preview.resultsLabel(), u"No results"_s);

        QList<QPushButton*> buttons = preview.buttons();
        buttons[0]->setCheckable(true);
        buttons[0]->setChecked(true);

        preview.search(u"("_s);
        QCOMPARE(preview.resultsLabel(), u"Invalid regex"_s);

        // A pattern that matches nothing but the empty text finds no results rather than looping
        preview.search(u"a*"_s);
        QCOMPARE(preview.resultsLabel(), u"No results"_s);

        // An empty query clears the count; Next/Previous with nothing found change nothing
        preview.search(QString());
        QVERIFY(preview.resultsLabel().isEmpty());
        buttons.last()->click();
        QVERIFY(preview.resultsLabel().isEmpty());
    }

    void aSearchStopsAtTheLimitOfMatches() {
        Preview preview;
        preview.widget.showAsset(make(u"data.json"_s, QByteArray(6000, 'a')));

        preview.search(u"a"_s);

        QCOMPARE(preview.resultsLabel(), u"1/5000+"_s);
    }

    void showingAnotherAssetForgetsTheSearchOfTheLastOne() {
        Preview preview;
        preview.widget.showAsset(make(u"data.json"_s, "foo foo"));
        preview.search(u"foo"_s);
        QCOMPARE(preview.resultsLabel(), u"1/2"_s);

        preview.widget.showAsset(make(u"other.json"_s, "bar"));

        QVERIFY(preview.resultsLabel().isEmpty());
        preview.buttons().last()->click();
        QVERIFY(preview.resultsLabel().isEmpty());
    }

    void aFontIsShownWithASampleAndReplacedByTheNextOne() {
        const QStringList fontDirs = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
        QString font;

        for (const QString& fontDir : fontDirs) {
            QDirIterator files(fontDir, { u"*.ttf"_s }, QDir::Files, QDirIterator::Subdirectories);

            if (files.hasNext()) {
                font = files.next();
                break;
            }
        }

        if (font.isEmpty()) {
            QSKIP("There is no TrueType font on the system to show");
        }

        Preview preview;
        preview.widget.showAsset(font);

        QCOMPARE(preview.page(), GRAPHICS_PAGE);
        QCOMPARE(preview.viewer()->scene()->items().size(), 1);
        QVERIFY(!preview.widget.findChildren<QLabel*>().isEmpty());

        // Typing a sample replaces the text that is drawn
        preview.fontSampleInput()->setText(u"Sample"_s);
        auto* const text = qgraphicsitem_cast<QGraphicsTextItem*>(preview.viewer()->scene()->items().first());
        QVERIFY(text != nullptr);
        QCOMPARE(text->toPlainText(), u"Sample"_s);

        // The same font again: the first registration is dropped, the second shows
        preview.widget.showAsset(font);
        QCOMPARE(preview.page(), GRAPHICS_PAGE);
        QCOMPARE(preview.viewer()->scene()->items().size(), 1);

        preview.widget.clear();
        QCOMPARE(preview.viewer()->scene()->items().size(), 0);
    }

    void aFileThatIsNoFontIsToldAboutOnTheErrorPage() {
        Preview preview;
        preview.widget.showAsset(make(u"broken.ttf"_s, "not a font"));

        QCOMPARE(preview.page(), ERROR_PAGE);
        QVERIFY(!preview.error().isEmpty());
    }

    void theWheelZoomsInAndOutAndReportsIt() {
        QGraphicsScene scene;
        GraphicsAssetViewer viewer(&scene);
        QSignalSpy rescaled(&viewer, &GraphicsAssetViewer::rescaled);

        const auto wheel = [&viewer](const i32 delta) -> void { turnWheel(viewer, delta); };

        viewer.setScaleFactor(1.0F);
        wheel(120);
        QCOMPARE(rescaled.count(), 1);
        QVERIFY(qFuzzyCompare(viewer.currentScale(), 1.15F));

        wheel(-120);
        QCOMPARE(rescaled.count(), 2);
        QVERIFY(qAbs(viewer.currentScale() - 1.0F) < 0.001F);
    }

    void theWheelStopsAtTheLimitsOfTheScale() {
        QGraphicsScene scene;
        GraphicsAssetViewer viewer(&scene);
        QSignalSpy rescaled(&viewer, &GraphicsAssetViewer::rescaled);

        const auto wheel = [&viewer](const i32 delta) -> void { turnWheel(viewer, delta); };

        // The step that crosses the maximum lands exactly on it, and a step past it changes nothing
        viewer.setScaleFactor(9.0F);
        wheel(120);
        QCOMPARE(rescaled.count(), 1);
        QCOMPARE(viewer.currentScale(), 10.0F);
        wheel(120);
        QCOMPARE(rescaled.count(), 1);
        QCOMPARE(viewer.currentScale(), 10.0F);

        viewer.setScaleFactor(0.016F);
        wheel(-120);
        QCOMPARE(rescaled.count(), 2);
        QCOMPARE(viewer.currentScale(), 0.01F);
        wheel(-120);
        QCOMPARE(rescaled.count(), 2);
        QCOMPARE(viewer.currentScale(), 0.01F);
    }
};

QTEST_MAIN(TestAssetPreview)
#include "tst_assetpreview.moc"
