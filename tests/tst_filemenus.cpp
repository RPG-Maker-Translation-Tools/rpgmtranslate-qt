#include "FileSelectMenu.hpp"
#include "MarkingTranslator.hpp"
#include "ModalCatcher.hpp"
#include "SerdeMenu.hpp"
#include "TabListModel.hpp"
#include "TestFiles.hpp"
#include "WriteMenu.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
auto files(const QStringList& names) -> vector<TabListItem> {
    vector<TabListItem> result;

    for (const QString& name : names) {
        result.push_back(TabListItem{ .name = name, .properties = {} });
    }

    return result;
}

auto checkboxOf(const FileSelectMenu& menu, const QString& name) -> QCheckBox* {
    for (auto* const checkbox : menu.findChildren<QCheckBox*>()) {
        if (checkbox->text() == name) {
            return checkbox;
        }
    }

    return nullptr;
}

void press(QWidget* const widget, const QPoint& pos, const Qt::MouseButton button = Qt::LeftButton) {
    QMouseEvent event(QEvent::MouseButtonPress, pos, widget->mapToGlobal(pos), button, button, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

void move(QWidget* const widget, const QPoint& pos, const Qt::MouseButtons buttons = Qt::LeftButton) {
    QMouseEvent event(QEvent::MouseMove, pos, widget->mapToGlobal(pos), Qt::NoButton, buttons, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

void release(QWidget* const widget, const QPoint& pos, const Qt::MouseButton button = Qt::LeftButton) {
    QMouseEvent event(QEvent::MouseButtonRelease, pos, widget->mapToGlobal(pos), button, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}
}  // namespace

class TestFileMenus final : public QObject {
    Q_OBJECT

   private slots:
    // FileSelectMenu
    void aNewMenuIsEmpty() {
        FileSelectMenu menu;

        QVERIFY(menu.empty());
        QCOMPARE(menu.selectedCount(), u16{ 0 });
        QVERIFY(menu.selected().empty());
    }

    void initAddsAnUncheckedBoxPerFile() {
        FileSelectMenu menu;
        menu.init(files({ u"actors"_s, u"items"_s, u"map001"_s, u"system"_s }));

        QVERIFY(!menu.empty());
        QCOMPARE(menu.findChildren<QCheckBox*>().size(), qsizetype{ 4 });
        QCOMPARE(menu.selectedCount(), u16{ 0 });
    }

    void addFileCanStartChecked() {
        FileSelectMenu menu;
        menu.addFile(u"actors"_s, true);
        menu.addFile(u"items"_s);

        QCOMPARE(menu.selectedCount(), u16{ 1 });
        QVERIFY(checkboxOf(menu, u"actors"_s)->isChecked());
    }

    void selectedMapsEachFileNameToItsFlag_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<u16>("flag");

        QTest::newRow("actors") << u"actors"_s << FileFlags_Actors;
        QTest::newRow("armors") << u"armors"_s << FileFlags_Armors;
        QTest::newRow("classes") << u"classes"_s << FileFlags_Classes;
        QTest::newRow("commonevents") << u"commonevents"_s << FileFlags_CommonEvents;
        QTest::newRow("enemies") << u"enemies"_s << FileFlags_Enemies;
        QTest::newRow("items") << u"items"_s << FileFlags_Items;
        QTest::newRow("skills") << u"skills"_s << FileFlags_Skills;
        QTest::newRow("states") << u"states"_s << FileFlags_States;
        QTest::newRow("troops") << u"troops"_s << FileFlags_Troops;
        QTest::newRow("weapons") << u"weapons"_s << FileFlags_Weapons;
        QTest::newRow("system") << u"system"_s << FileFlags_System;
        QTest::newRow("scripts") << u"scripts"_s << FileFlags_Scripts;
        // MV/MZ call the file of the scripts `plugins`
        QTest::newRow("plugins") << u"plugins"_s << FileFlags_Scripts;
        QTest::newRow("terms") << u"terms"_s << FileFlags_Database;
    }

    void selectedMapsEachFileNameToItsFlag() {
        QFETCH(QString, name);
        QFETCH(u16, flag);

        FileSelectMenu menu;
        menu.addFile(name, true);
        menu.addFile(u"unrelated"_s, true);

        const Selected selected = menu.selected();

        QCOMPARE(selected.flags, static_cast<FileFlags>(flag));
        QCOMPARE(selected.mapCount, u16{ 0 });
    }

    void selectedCollectsTheMaps() {
        FileSelectMenu menu;
        menu.addFile(u"map001"_s, true);
        menu.addFile(u"map012"_s, true);
        menu.addFile(u"map002"_s);

        const Selected selected = menu.selected();

        QCOMPARE(selected.mapCount, u16{ 2 });
        QVERIFY(selected.validIndices[1]);
        QVERIFY(selected.validIndices[12]);
        QVERIFY(!selected.validIndices[2]);
        QVERIFY(!selected.empty());
    }

    void aMapOutsideTheSetsIsLeftOut() {
        FileSelectMenu menu;
        menu.addFile(u"map9999"_s, true);

        QCOMPARE(menu.selected().mapCount, u16{ 0 });
    }

    void skippedInvertsTheSelection() {
        FileSelectMenu menu;
        menu.addFile(u"actors"_s, true);
        menu.addFile(u"items"_s);

        const Selected skipped = menu.selected(true);

        QCOMPARE(skipped.flags, static_cast<FileFlags>(FileFlags_Items));
    }

    void selectAllAndDeselectAllTouchEveryBox() {
        FileSelectMenu menu;
        menu.init(files({ u"actors"_s, u"items"_s, u"map001"_s }));

        menu.findChild<QPushButton*>(u"selectAllButton"_s)->click();
        QCOMPARE(menu.selectedCount(), u16{ 3 });

        menu.findChild<QPushButton*>(u"deselectAllButton"_s)->click();
        QCOMPARE(menu.selectedCount(), u16{ 0 });
    }

    void clearRemovesTheBoxes() {
        FileSelectMenu menu;
        menu.init(files({ u"actors"_s, u"items"_s }));

        menu.clear();

        QVERIFY(menu.empty());
        QVERIFY(menu.findChildren<QCheckBox*>().isEmpty());
    }

    void clickingABoxTogglesIt() {
        FileSelectMenu menu;
        menu.init(files({ u"actors"_s }));
        menu.show();

        QCheckBox* const box = checkboxOf(menu, u"actors"_s);
        const QPoint center = box->rect().center();

        press(box, center);
        release(box, center);
        QVERIFY(box->isChecked());

        press(box, center);
        release(box, center);
        QVERIFY(!box->isChecked());
    }

    void draggingOverBoxesTogglesEachOnce() {
        FileSelectMenu menu;
        menu.init(files({ u"actors"_s, u"items"_s, u"skills"_s }));
        menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&menu));

        QCheckBox* const first = checkboxOf(menu, u"actors"_s);
        QCheckBox* const second = checkboxOf(menu, u"items"_s);
        QCheckBox* const third = checkboxOf(menu, u"skills"_s);

        press(first, first->rect().center());
        QVERIFY(first->isChecked());

        const QPoint overSecond = second->mapTo(&menu, second->rect().center());
        move(&menu, overSecond);
        move(&menu, overSecond);
        QVERIFY(second->isChecked());

        move(&menu, third->mapTo(&menu, third->rect().center()));
        QVERIFY(third->isChecked());

        release(third, third->rect().center());

        // The drag is over: moving over the first box again changes nothing
        move(&menu, first->mapTo(&menu, first->rect().center()));
        QVERIFY(first->isChecked());
        QCOMPARE(menu.selectedCount(), u16{ 3 });
    }

    void anotherButtonDoesNotToggleABox() {
        FileSelectMenu menu;
        menu.init(files({ u"actors"_s }));
        menu.show();

        QCheckBox* const box = checkboxOf(menu, u"actors"_s);
        press(box, box->rect().center(), Qt::RightButton);

        QVERIFY(!box->isChecked());
    }

    // PersistentMenu
    void showingAndHidingAreAnnounced() {
        FileSelectMenu menu;
        QSignalSpy shown(&menu, &PersistentMenu::shown);
        QSignalSpy hidden(&menu, &PersistentMenu::hidden);

        menu.show();
        menu.hide();

        QCOMPARE(shown.count(), 1);
        QCOMPARE(hidden.count(), 1);
    }

    void draggingTheMenuMovesIt() {
        FileSelectMenu menu;
        menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&menu));

        QSignalSpy moved(&menu, &PersistentMenu::mouseMovedChanged);
        const QPoint start = menu.frameGeometry().topLeft();
        const QPoint grab(2, 2);
        // The menu moves under the mouse, so the positions on the screen are fixed here
        const QPoint origin = menu.mapToGlobal(grab);

        const auto send = [&](const QEvent::Type type, const QPoint& delta, const Qt::MouseButton button) -> void {
            QMouseEvent event(
                type,
                grab,
                origin + delta,
                type == QEvent::MouseMove ? Qt::NoButton : button,
                type == QEvent::MouseButtonRelease ? Qt::NoButton : button,
                Qt::NoModifier
            );
            QApplication::sendEvent(&menu, &event);
        };

        QVERIFY(!menu.mouseMoved());
        send(QEvent::MouseButtonPress, QPoint(), Qt::LeftButton);
        // A small jitter is not a drag yet
        send(QEvent::MouseMove, QPoint(1, 1), Qt::LeftButton);
        QVERIFY(!menu.mouseMoved());

        send(QEvent::MouseMove, QPoint(60, 40), Qt::LeftButton);
        send(QEvent::MouseButtonRelease, QPoint(60, 40), Qt::LeftButton);

        QVERIFY(menu.mouseMoved());
        QCOMPARE(moved.count(), 1);
        QCOMPARE(menu.frameGeometry().topLeft(), start + QPoint(60, 40));

        menu.resetMouseMovedFlag();
        QVERIFY(!menu.mouseMoved());
        QCOMPARE(moved.count(), 2);
        // Resetting a reset flag says nothing
        menu.resetMouseMovedFlag();
        QCOMPARE(moved.count(), 2);
    }

    void aDisabledDragLeavesTheMenuWhereItIs() {
        FileSelectMenu menu;
        menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&menu));

        QSignalSpy toggled(&menu, &PersistentMenu::dragMoveEnabledChanged);
        menu.setDragMoveEnabled(false);
        menu.setDragMoveEnabled(false);
        QCOMPARE(toggled.count(), 1);
        QVERIFY(!menu.dragMoveEnabled());

        const QPoint start = menu.frameGeometry().topLeft();
        press(&menu, QPoint(2, 2));
        move(&menu, QPoint(60, 40));
        release(&menu, QPoint(60, 40));

        QCOMPARE(menu.frameGeometry().topLeft(), start);
        QVERIFY(!menu.mouseMoved());
    }

    void onlyTheLeftButtonDragsTheMenu() {
        FileSelectMenu menu;
        menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&menu));

        const QPoint start = menu.frameGeometry().topLeft();
        press(&menu, QPoint(2, 2), Qt::RightButton);
        move(&menu, QPoint(60, 40), Qt::RightButton);
        release(&menu, QPoint(60, 40), Qt::RightButton);

        QCOMPARE(menu.frameGeometry().topLeft(), start);
    }

    // SerdeMenu
    void serdeMenuAsksForAFolderFirst() {
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        host.findChild<FileSelectMenu*>()->findChild<QPushButton*>(u"selectAllButton"_s)->click();

        QSignalSpy exporting(&menu, &SerdeMenu::exportRequested);
        QSignalSpy importing(&menu, &SerdeMenu::importRequested);

        {
            ModalCatcher modal;
            menu.findChild<QPushButton*>(u"exportButton"_s)->click();
            QVERIFY2(modal.text.contains(u"destination"_qsv), qPrintable(modal.text));
        }
        {
            ModalCatcher modal;
            menu.findChild<QPushButton*>(u"importButton"_s)->click();
            QVERIFY2(modal.text.contains(u"source"_qsv) && !modal.text.contains(u"destination"_qsv), qPrintable(modal.text));
        }

        QCOMPARE(exporting.count(), 0);
        QCOMPARE(importing.count(), 0);
    }

    void serdeMenuAsksForFilesNext() {
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        menu.findChild<QLineEdit*>(u"directoryInput"_s)->setText(u"/somewhere"_s);

        QSignalSpy exporting(&menu, &SerdeMenu::exportRequested);

        ModalCatcher modal;
        menu.findChild<QPushButton*>(u"exportButton"_s)->click();

        QVERIFY2(modal.text.contains(u"Select some files"_s), qPrintable(modal.text));
        QCOMPARE(exporting.count(), 0);
    }

    void serdeMenuRequestsAnExportAndHides() {
        QTemporaryDir dir;
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s, u"items"_s }));
        host.findChild<FileSelectMenu*>()->findChild<QCheckBox*>()->setChecked(true);
        menu.findChild<QComboBox*>(u"formatSelect"_s)->setCurrentIndex(static_cast<i32>(SerdeFormat::Json));
        // A folder that does not exist yet is made
        menu.findChild<QLineEdit*>(u"directoryInput"_s)->setText(dir.path() + u"/out/deeper"_s);
        menu.show();

        QSignalSpy exporting(&menu, &SerdeMenu::exportRequested);

        menu.findChild<QPushButton*>(u"exportButton"_s)->click();

        QCOMPARE(exporting.count(), 1);
        QVERIFY(QDir(dir.path() + u"/out/deeper"_s).exists());
        QVERIFY(!menu.isVisible());
        QCOMPARE(menu.format(), SerdeFormat::Json);
        QCOMPARE(menu.directory(), dir.path() + u"/out/deeper"_s);
        QVERIFY(!menu.selected().empty());
    }

    void serdeMenuRequestsAnImportWithoutMakingTheFolder() {
        QTemporaryDir dir;
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        host.findChild<FileSelectMenu*>()->findChild<QCheckBox*>()->setChecked(true);
        menu.findChild<QComboBox*>(u"formatSelect"_s)->setCurrentIndex(static_cast<i32>(SerdeFormat::Json));
        menu.findChild<QLineEdit*>(u"directoryInput"_s)->setText(dir.path() + u"/absent"_s);

        QSignalSpy importing(&menu, &SerdeMenu::importRequested);

        menu.findChild<QPushButton*>(u"importButton"_s)->click();

        QCOMPARE(importing.count(), 1);
        QVERIFY(!QDir(dir.path() + u"/absent"_s).exists());
    }

    void serdeMenuRefusesAFolderItCannotMake() {
        QTemporaryDir dir;
        writeFile(dir.path() + u"/file"_s, "x");

        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        host.findChild<FileSelectMenu*>()->findChild<QCheckBox*>()->setChecked(true);
        menu.findChild<QComboBox*>(u"formatSelect"_s)->setCurrentIndex(static_cast<i32>(SerdeFormat::Json));
        // Below a file
        menu.findChild<QLineEdit*>(u"directoryInput"_s)->setText(dir.path() + u"/file/sub"_s);

        QSignalSpy exporting(&menu, &SerdeMenu::exportRequested);

        ModalCatcher modal;
        menu.findChild<QPushButton*>(u"exportButton"_s)->click();

        QVERIFY2(modal.text.contains(u"Cannot create the folder"_s), qPrintable(modal.text));
        QCOMPARE(exporting.count(), 0);
    }

    void serdeMenuNamesAFormatThatWasNotCompiled() {
        // Which formats this build has, in the order of the format select
        const array<bool, 5> available = {
#ifdef ENABLE_SERDE_CSV
            true,
#else
            false,
#endif
#ifdef ENABLE_SERDE_XLSX
            true,
#else
            false,
#endif
#ifdef ENABLE_SERDE_XML
            true,
#else
            false,
#endif
            true,
#ifdef ENABLE_SERDE_YAML
            true,
#else
            false,
#endif
        };

        QTemporaryDir dir;

        for (i32 format = 0; format < 5; format++) {
            QWidget host;
            SerdeMenu menu(&host);
            menu.init(files({ u"actors"_s }));
            host.findChild<FileSelectMenu*>()->findChild<QCheckBox*>()->setChecked(true);
            menu.findChild<QComboBox*>(u"formatSelect"_s)->setCurrentIndex(format);
            menu.findChild<QLineEdit*>(u"directoryInput"_s)->setText(dir.path());

            QSignalSpy importing(&menu, &SerdeMenu::importRequested);

            ModalCatcher modal;
            menu.findChild<QPushButton*>(u"importButton"_s)->click();

            if (available[static_cast<usize>(format)]) {
                QCOMPARE(importing.count(), 1);
            } else {
                QVERIFY2(modal.text.contains(u"not compiled into this build"_s), qPrintable(modal.text));
                QCOMPARE(importing.count(), 0);
            }
        }
    }

    void serdeMenuBrowseFillsTheFolderAndCancelKeepsIt() {
        const QTemporaryDir dir;
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        auto* const input = menu.findChild<QLineEdit*>(u"directoryInput"_s);
        input->setText(u"/before"_s);

        {
            ModalCatcher modal;
            menu.findChild<QPushButton*>(u"browseButton"_s)->click();
            QCOMPARE(modal.dialogs.size(), qsizetype{ 1 });
            QCOMPARE(input->text(), u"/before"_s);
        }
        {
            ModalCatcher modal;
            modal.file = dir.path();
            menu.findChild<QPushButton*>(u"browseButton"_s)->click();
            QCOMPARE(QDir(input->text()).canonicalPath(), QDir(dir.path()).canonicalPath());
        }
    }

    void serdeMenuClearForgetsTheFolderAndTheFiles() {
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        menu.findChild<QLineEdit*>(u"directoryInput"_s)->setText(u"/somewhere"_s);

        menu.clear();

        QVERIFY(menu.directory().isEmpty());
        QVERIFY(host.findChild<FileSelectMenu*>()->empty());
    }

    void serdeMenuShowsItsFileMenuAndHidesItWithItself() {
        QWidget host;
        SerdeMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        menu.show();

        auto* const fileMenu = host.findChild<FileSelectMenu*>();
        menu.findChild<QPushButton*>(u"fileSelectButton"_s)->click();
        QVERIFY(fileMenu->isVisible());

        menu.findChild<QPushButton*>(u"fileSelectButton"_s)->click();
        QVERIFY(!fileMenu->isVisible());

        menu.findChild<QPushButton*>(u"fileSelectButton"_s)->click();
        QVERIFY(fileMenu->isVisible());

        menu.findChild<QPushButton*>(u"closeButton"_s)->click();
        QVERIFY(!menu.isVisible());
        QVERIFY(!fileMenu->isVisible());
    }

    void serdeMenuRetranslatesOnALanguageChange() {
        QWidget host;
        SerdeMenu menu(&host);
        const MarkingTranslator translator;
        translator.retranslate(&menu);

        QVERIFY(menu.findChild<QPushButton*>(u"exportButton"_s)->text().startsWith(MarkingTranslator::MARK));
    }

    // WriteMenu
    void writeMenuAcceptsOnApply() {
        QWidget host;
        WriteMenu menu(&host);
        QSignalSpy accepted(&menu, &QDialog::accepted);

        menu.findChild<QPushButton*>(u"applyButton"_s)->click();

        QCOMPARE(accepted.count(), 1);
    }

    void writeMenuHoldsAFileSelection() {
        QWidget host;
        WriteMenu menu(&host);
        menu.init(files({ u"actors"_s, u"items"_s }));
        host.findChild<FileSelectMenu*>()->findChild<QCheckBox*>()->setChecked(true);

        QVERIFY(!menu.selected().empty());
        QVERIFY(!menu.selected(true).empty());

        menu.clear();
        QVERIFY(menu.selected().empty());
    }

    void writeMenuTogglesItsFileMenu() {
        QWidget host;
        WriteMenu menu(&host);
        menu.init(files({ u"actors"_s }));
        menu.show();

        auto* const fileMenu = host.findChild<FileSelectMenu*>();
        menu.findChild<QPushButton*>(u"fileSelectButton"_s)->click();
        QVERIFY(fileMenu->isVisible());

        menu.hide();
        QVERIFY(!fileMenu->isVisible());
    }

    void writeMenuRetranslatesOnALanguageChange() {
        QWidget host;
        WriteMenu menu(&host);
        const MarkingTranslator translator;
        translator.retranslate(&menu);

        QVERIFY(menu.findChild<QPushButton*>(u"applyButton"_s)->text().startsWith(MarkingTranslator::MARK));
    }
};

QTEST_MAIN(TestFileMenus)
#include "tst_filemenus.moc"
