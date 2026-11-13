#include "Pointer.hpp"
#include "TranslationTableHeader.hpp"

#include <QLineEdit>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTableView>
#include <QtTest>

namespace {
// A table over three columns, source first, with the header under test. Every section is 100 pixels wide, so the add
// button is right after the third.
struct Fixture {
    Fixture() : header(new TranslationTableHeader(&table)) {
        model.setHorizontalHeaderLabels({ u"Source"_s, u"English"_s, u"German"_s });
        table.setModel(&model);
        table.setHorizontalHeader(header);
        header->setDefaultSectionSize(100);
        header->setStretchLastSection(false);

        table.resize(600, 200);
        table.show();
        QVERIFY(QTest::qWaitForWindowExposed(&table));
    }

    [[nodiscard]] auto buttonCenter() const -> QPoint { return { 300 + 16, header->height() / 2 }; }

    [[nodiscard]] auto sectionCenter(const i32 section) const -> QPoint {
        return { section * 100 + 50, header->height() / 2 };
    }

    [[nodiscard]] auto label(const i32 section) const -> QString {
        return model.headerData(section, Qt::Horizontal).toString();
    }

    [[nodiscard]] auto editor() const -> QLineEdit* {
        for (QLineEdit* const edit : header->viewport()->findChildren<QLineEdit*>()) {
            if (edit->isVisible()) {
                return edit;
            }
        }

        return nullptr;
    }

    QStandardItemModel model{ 1, 3 };
    QTableView table;
    TranslationTableHeader* header;
};
}  // namespace

class TestTranslationHeader final : public QObject {
    Q_OBJECT

   private slots:
    void makesRoomForTheAddButton() {
        Fixture fixture;
        const QWidget* const widget = fixture.header;

        // `sizeHint` is protected in the header, so it is asked through the base class
        QVERIFY(widget->sizeHint().width() >= 32);
    }

    void clickingTheAddButtonReportsIt() {
        Fixture fixture;
        QSignalSpy clicked(fixture.header, &TranslationTableHeader::addButtonClicked);

        QTest::mouseClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.buttonCenter());

        QCOMPARE(clicked.size(), qsizetype{ 1 });
    }

    void releasingOutsideTheAddButtonDoesNotClickIt() {
        Fixture fixture;
        QSignalSpy clicked(fixture.header, &TranslationTableHeader::addButtonClicked);

        QTest::mousePress(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.buttonCenter());
        QTest::mouseRelease(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(0));

        QCOMPARE(clicked.size(), qsizetype{ 0 });
    }

    void theAddButtonShowsAHandUntilThePointerLeaves() {
        Fixture fixture;
        QWidget* const viewport = fixture.header->viewport();

        hoverAt(viewport, fixture.buttonCenter());
        QCOMPARE(viewport->cursor().shape(), Qt::PointingHandCursor);

        hoverAt(viewport, fixture.buttonCenter() + QPoint(1, 0));
        QCOMPARE(viewport->cursor().shape(), Qt::PointingHandCursor);

        hoverAt(viewport, fixture.sectionCenter(1));
        QVERIFY(viewport->cursor().shape() != Qt::PointingHandCursor);
    }

    void clickingASectionDoesNotClickTheButton() {
        Fixture fixture;
        QSignalSpy clicked(fixture.header, &TranslationTableHeader::addButtonClicked);

        QTest::mouseClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(1));

        QCOMPARE(clicked.size(), qsizetype{ 0 });
    }

    void paintingDrawsTheButtonWhenHoveredAndPressed() {
        Fixture fixture;

        const QImage idle = fixture.header->grab().toImage();

        hoverAt(fixture.header->viewport(), fixture.buttonCenter());
        const QImage hovered = fixture.header->grab().toImage();
        QVERIFY(hovered != idle);
        QTest::mousePress(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.buttonCenter());
        QVERIFY(fixture.header->grab().toImage() != hovered);
        QTest::mouseRelease(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.buttonCenter());
    }

    void doubleClickingATranslationColumnRenamesIt() {
        Fixture fixture;

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(1));

        QLineEdit* const edit = fixture.editor();
        QVERIFY(edit != nullptr);
        QCOMPARE(edit->text(), u"English"_s);

        edit->setText(u"  French  "_s);
        QTest::keyClick(edit, Qt::Key_Return);

        QCOMPARE(fixture.label(1), u"French"_s);
        QVERIFY(fixture.editor() == nullptr);
        QCOMPARE(fixture.label(0), u"Source"_s);
        QCOMPARE(fixture.label(2), u"German"_s);
    }

    // The source column is the key of the file, not a language
    void theSourceColumnCannotBeRenamed() {
        Fixture fixture;

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(0));

        QVERIFY(fixture.editor() == nullptr);
    }

    void leavingTheHeaderOnTheButtonPutsTheCursorBack() {
        Fixture fixture;
        QWidget* const viewport = fixture.header->viewport();

        hoverAt(viewport, fixture.buttonCenter());
        QCOMPARE(viewport->cursor().shape(), Qt::PointingHandCursor);

        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(fixture.header, &leave);

        QVERIFY(viewport->cursor().shape() != Qt::PointingHandCursor);
    }

    void doubleClickingWhereThereIsNothingToRenameRenamesNothing_data() {
        QTest::addColumn<i32>("where");

        QTest::newRow("beyond the columns") << 0;
        QTest::newRow("the button") << 1;
    }

    void doubleClickingWhereThereIsNothingToRenameRenamesNothing() {
        QFETCH(const i32, where);

        Fixture fixture;
        const QPoint point = where == 0 ? QPoint(590, fixture.header->height() / 2) : fixture.buttonCenter();

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, point);

        QVERIFY(fixture.editor() == nullptr);
    }

    void aColumnCannotBeLeftWithoutAName() {
        Fixture fixture;

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(2));
        QLineEdit* const edit = fixture.editor();
        QVERIFY(edit != nullptr);

        edit->setText(u"   "_s);
        QTest::keyClick(edit, Qt::Key_Return);

        QCOMPARE(fixture.label(2), u"German"_s);
        QVERIFY(fixture.editor() == nullptr);
    }

    void escapeKeepsTheOldName() {
        Fixture fixture;

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(1));
        QLineEdit* const edit = fixture.editor();
        QVERIFY(edit != nullptr);

        edit->setText(u"Typo"_s);
        QTest::keyClick(edit, Qt::Key_Escape);

        QCOMPARE(fixture.label(1), u"English"_s);
    }

    void clickingElsewhereFinishesTheEdit() {
        Fixture fixture;

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(1));
        QLineEdit* const edit = fixture.editor();
        QVERIFY(edit != nullptr);
        edit->setText(u"Spanish"_s);

        QTest::mouseClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(2));

        QCOMPARE(fixture.label(1), u"Spanish"_s);
        QVERIFY(fixture.editor() == nullptr);
    }

    void startingASecondEditFinishesTheFirst() {
        Fixture fixture;

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(1));
        fixture.editor()->setText(u"Dutch"_s);

        QTest::mouseDClick(fixture.header->viewport(), Qt::LeftButton, Qt::NoModifier, fixture.sectionCenter(2));

        QCOMPARE(fixture.label(1), u"Dutch"_s);
        QVERIFY(fixture.editor() != nullptr);
        QCOMPARE(fixture.editor()->text(), u"German"_s);
    }
};

QTEST_MAIN(TestTranslationHeader)
#include "tst_translationheader.moc"
