// The small widgets the window is built from: the list of tabs with its progress, the number input, the input that
// lets go on Escape, and the menu of translations.
#include "BackupSelectDialog.hpp"
#include "CBSLIWidget.hpp"
#include "ClickableLabel.hpp"
#include "PasswordLineEdit.hpp"
#include "PopupInput.hpp"
#include "Settings.hpp"
#include "TabList.hpp"
#include "TabListModel.hpp"
#include "TabPanel.hpp"
#include "TranslationsMenu.hpp"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
[[nodiscard]] auto item(const QString& name, const i32 total, const i32 translated, const bool completed = false)
    -> TabListItem {
    TabListItem tab{ .name = name, .properties = {} };
    tab.properties.set<"total">(total);
    tab.properties.set<"translated">(translated);
    tab.properties.set<"completed">(completed);
    return tab;
}
}  // namespace

class TestSmallWidgets final : public QObject {
    Q_OBJECT

   private slots:
    // TabList and its delegate
    void tabListPaintsEveryDisplayWithoutLosingItsTabs() {
        TabList list;
        list.resize(300, 120);
        list.setTabs({ item(u"actors"_s, 10, 4), item(u"map001"_s, 0, 0), item(u"system"_s, 3, 3, true) });

        QCOMPARE(list.model()->rowCount(), 3);

        // Counts, then percents (a tab without text counts as done), and the completed row painted over
        const QPixmap counts = list.grab();
        list.setProgressDisplay(true);
        QVERIFY(list.progressDisplay());
        const QPixmap percents = list.grab();

        QVERIFY(!counts.isNull());
        QVERIFY(counts.toImage() != percents.toImage());
    }

    void toggleCompletedFlipsTheFlagOfThatTab() {
        TabList list;
        list.setTabs({ item(u"actors"_s, 10, 4), item(u"items"_s, 5, 0) });

        list.toggleCompleted(list.model()->index(1, 0));
        QVERIFY(list.tab(1).properties.get<"completed">());
        QVERIFY(!list.tab(0).properties.get<"completed">());

        list.toggleCompleted(list.model()->index(1, 0));
        QVERIFY(!list.tab(1).properties.get<"completed">());
    }

    void onlyARightClickIsReportedWithTheTabUnderIt() {
        TabList list;
        list.resize(300, 120);
        list.setTabs({ item(u"actors"_s, 10, 4) });
        list.show();
        QVERIFY(QTest::qWaitForWindowExposed(&list));

        QSignalSpy clicked(&list, &TabList::rightClicked);
        const QPoint onTab = list.visualRect(list.model()->index(0, 0)).center();

        QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier, onTab);
        QTest::mouseClick(list.viewport(), Qt::MiddleButton, Qt::NoModifier, onTab);
        QCOMPARE(clicked.count(), 0);

        QTest::mouseClick(list.viewport(), Qt::RightButton, Qt::NoModifier, onTab);
        QCOMPARE(clicked.count(), 1);
        QCOMPARE(clicked.first().at(0).toModelIndex().row(), 0);

        // Empty space under the tabs is no tab
        QTest::mouseClick(list.viewport(), Qt::RightButton, Qt::NoModifier, QPoint(10, 100));
        QCOMPARE(clicked.count(), 1);
    }

    void clearingTheListEmptiesIt() {
        TabList list;
        list.setTabs({ item(u"actors"_s, 10, 4) });
        list.grab();
        list.clear();

        QCOMPARE(list.model()->rowCount(), 0);
        // And a font change makes the delegate measure again: shrinking the font paints the list as one that never had
        // the larger one (a column that kept its old width would differ)
        const QFont small = list.font();
        QFont large = small;
        large.setPointSize(small.pointSize() + 8);
        list.setFont(large);
        list.setTabs({ item(u"a much longer name"_s, 10, 4) });
        list.grab();
        list.setFont(small);

        TabList fresh;
        fresh.resize(list.size());
        fresh.setFont(small);
        fresh.setTabs({ item(u"a much longer name"_s, 10, 4) });

        QCOMPARE(list.grab().toImage(), fresh.grab().toImage());
    }

    // TabPanel
    void tabPanelFollowsTheTabItIsOn() {
        TabPanel panel;
        panel.setTabs({ item(u"actors"_s, 10, 4), item(u"items"_s, 5, 1) });

        QCOMPARE(panel.tabCount(), 2);
        QCOMPARE(panel.tabName(1), u"items"_s);
        QCOMPARE(panel.tabTotal(0), 10);
        QCOMPARE(panel.tabTranslated(0), 4);

        QSignalSpy changed(&panel, &TabPanel::tabChanged);
        panel.changeTab(u"items"_s);

        QCOMPARE(panel.currentTabName(), u"items"_s);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().at(0).toString(), u"items"_s);
        QCOMPARE(panel.currentTotal(), 5);
        QCOMPARE(panel.currentTranslated(), 1);

        // These take what changed, not the new count
        panel.setCurrentTranslated(3);
        panel.setCurrentTotal(-2);
        QCOMPARE(panel.tabTranslated(1), 4);
        QCOMPARE(panel.tabTotal(1), 3);

        panel.setTabTranslated(0, 9);
        QCOMPARE(panel.tabTranslated(0), 9);

        // Leaving every tab
        panel.changeTab(QString());
        QCOMPARE(panel.currentTabName(), QString());
        // With no tab to change they change nothing
        panel.setCurrentTranslated(5);
        panel.setCurrentTotal(5);
        QCOMPARE(panel.tabTranslated(1), 4);
        QCOMPARE(panel.tabTotal(1), 3);
        QCOMPARE(panel.currentTranslated(), 0);
        QCOMPARE(panel.currentTotal(), 0);

        panel.clear();
        QCOMPARE(panel.tabCount(), 0);
    }

    // CBSLIWidget
    void numberInputKeepsSliderAndTextTogether() {
        CBSLIWidget widget;
        widget.setRange(0, 100);
        QSignalSpy changed(&widget, &CBSLIWidget::valueChanged);

        widget.slider()->setValue(40);
        QCOMPARE(widget.lineEdit()->text(), u"40"_s);
        QCOMPARE(changed.count(), 1);

        widget.lineEdit()->setText(u"75"_s);
        emit widget.lineEdit()->editingFinished();
        QCOMPARE(widget.value(), 75);
        QCOMPARE(widget.slider()->value(), 75);
        QCOMPARE(changed.last().at(0).toInt(), 75);
    }

    void numberInputPullsValuesIntoItsRange() {
        CBSLIWidget widget;
        widget.setRange(10, 20);

        widget.setValue(500);
        QCOMPARE(widget.value(), 20);
        widget.setValue(-5);
        QCOMPARE(widget.value(), 10);

        // A smaller range pulls the current value in
        widget.setValue(18);
        widget.setRange(0, 15);
        QCOMPARE(widget.value(), 15);
        QCOMPARE(widget.lineEdit()->text(), u"15"_s);
    }

    void numberInputRestoresTextThatIsNotANumber() {
        CBSLIWidget widget;
        widget.setRange(0, 100);
        widget.setValue(33);
        QSignalSpy changed(&widget, &CBSLIWidget::valueChanged);

        widget.lineEdit()->setText(u"x"_s);
        emit widget.lineEdit()->editingFinished();

        QCOMPARE(widget.lineEdit()->text(), u"33"_s);
        QCOMPARE(changed.count(), 0);

        widget.setLabel(u"Volume"_s);
        QCOMPARE(widget.label()->text(), u"Volume"_s);
    }

    // PopupInput
    void popupInputReportsEscapeAndKeepsTyping() {
        PopupInput input;
        QSignalSpy escape(&input, &PopupInput::escapePressed);

        QTest::keyClicks(&input, u"abc"_s);
        QCOMPARE(input.text(), u"abc"_s);
        QCOMPARE(escape.count(), 0);

        QTest::keyClick(&input, Qt::Key_Escape);
        QCOMPARE(escape.count(), 1);
        QCOMPARE(input.text(), u"abc"_s);
    }

    // TranslationsMenu
    void translationsMenuOffersWhatEndpointsAnswered() {
        Settings settings;
        settings.translation.endpoints = { EndpointSettings{ .name = u"First"_s },
                                           EndpointSettings{ .name = u"Second"_s } };

        TranslationsMenu menu;
        QSignalSpy clicked(&menu, &TranslationsMenu::translationClicked);

        menu.showTranslations(
            { EndpointAnswer{ .text = u"Привет"_s, .failed = false },
              EndpointAnswer{ .text = u"timed out"_s, .failed = true } },
            &settings
        );

        const auto labels = menu.findChildren<ClickableLabel*>();
        QCOMPARE(labels.size(), 2);

        QTest::mouseClick(labels[0], Qt::LeftButton);
        QCOMPARE(clicked.count(), 1);
        QCOMPARE(clicked.first().at(0).toString(), u"Привет"_s);

        // The reason of a failure can be read but is not a translation
        QVERIFY(!labels[1]->isEnabled());
    }

    void translationsMenuIgnoresAnswersOfRemovedEndpoints() {
        Settings settings;
        settings.translation.endpoints = { EndpointSettings{ .name = u"Only"_s } };

        TranslationsMenu menu;
        menu.showTranslations(
            { EndpointAnswer{ .text = u"one"_s, .failed = false }, EndpointAnswer{ .text = u"two"_s, .failed = false } },
            &settings
        );

        QCOMPARE(menu.findChildren<ClickableLabel*>().size(), 1);
    }

    void translationsMenuShowsAnErrorInPlaceOfTheAnswers() {
        Settings settings;
        settings.translation.endpoints = { EndpointSettings{ .name = u"First"_s } };

        TranslationsMenu menu;
        menu.showTranslations({ EndpointAnswer{ .text = u"one"_s, .failed = false } }, &settings);
        menu.showError(u"No endpoints"_s);

        QCOMPARE(menu.findChildren<ClickableLabel*>().size(), 0);
        bool shown = false;

        for (const QLabel* const label : menu.findChildren<QLabel*>()) {
            shown |= label->text() == u"No endpoints"_s;
        }

        QVERIFY(shown);
    }
    void aPasswordIsHiddenUntilTheEyeIsToggled() {
        PasswordLineEdit edit;
        edit.setText(u"secret"_s);
        QCOMPARE(edit.echoMode(), QLineEdit::Password);

        auto* const toggle = edit.findChild<QAction*>();
        QVERIFY(toggle != nullptr);

        toggle->setChecked(true);
        QCOMPARE(edit.echoMode(), QLineEdit::Normal);

        toggle->setChecked(false);
        QCOMPARE(edit.echoMode(), QLineEdit::Password);
    }

    void aLineEditReportsLosingFocusOnlyWhenToldTo() {
        UnfocusLineEdit edit;
        QSignalSpy finished(&edit, &QLineEdit::editingFinished);
        QVERIFY(!edit.isUnfocusSignalEnabled());

        QFocusEvent out(QEvent::FocusOut, Qt::TabFocusReason);
        QCoreApplication::sendEvent(&edit, &out);
        QCOMPARE(finished.size(), qsizetype{ 0 });

        edit.setUnfocusSignal(true);
        QVERIFY(edit.isUnfocusSignalEnabled());
        QCoreApplication::sendEvent(&edit, &out);

        QCOMPARE(finished.size(), qsizetype{ 1 });
    }

    void backupsAreListedNewestFirstAndThoseWithoutADateAfterThem() {
        const QTemporaryDir dir;

        for (const QString& name : { u"01-01-2020_00-00-00.tar.xz"_s, u"02-01-2020_00-00-00.tar.xz"_s, u"zeta.tar.xz"_s, u"alpha.tar.xz"_s }) {
            QFile file(dir.path() + u'/' + name);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }

        auto* const dialog = new BackupSelectDialog(dir.path());
        auto* const list = dialog->findChild<QListWidget*>();
        auto* const load = dialog->findChild<QPushButton*>();

        QStringList names;

        for (i32 row = 0; row < list->count(); row++) {
            names.append(list->item(row)->text());
        }

        // The names start with the day, so 02-01 (the 2nd) is newer than 01-01 although it sorts after it by name
        QCOMPARE(names, (QStringList{ u"02-01-2020_00-00-00.tar.xz"_s, u"01-01-2020_00-00-00.tar.xz"_s, u"alpha.tar.xz"_s, u"zeta.tar.xz"_s }));

        QVERIFY(!load->isEnabled());
        list->setCurrentRow(2);
        QVERIFY(load->isEnabled());
        QCOMPARE(dialog->backupPath(), dir.path() + u"/alpha.tar.xz"_s);

        list->setCurrentRow(-1);
        QVERIFY(!load->isEnabled());

        delete dialog;
    }
};

QTEST_MAIN(TestSmallWidgets)
#include "tst_smallwidgets.moc"
