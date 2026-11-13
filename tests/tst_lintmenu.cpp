#include "FileSelectMenu.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "LintSelectMenu.hpp"
#include "LintTable.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "TabListModel.hpp"
#include "ui_MainWindow.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QScreen>
#include <QtTest>

namespace {
auto rowOf(const u32 line) -> LintRow {
    return LintRow{ .filename = u"map001"_s,
                    .sourceText = u"source"_s,
                    .translationText = u"translation"_s,
                    .sourceHighlights = {},
                    .translationHighlights = {},
                    .info = u"info"_s,
                    .lineNumber = line };
}

constexpr bool hasFlag(const LintFlags flags, const LintFlags flag) {
    return (flags & flag) != 0;
}
}  // namespace

// The menu is bound to widgets of the main window's form; the test stands in for the form with plain widgets
struct Fixture {
    Fixture() {
        projectSettings.projectPath = dir.path();
        projectSettings.engineType = EngineType::MVMZ;
        projectSettings.sourceDirectory = SourceDirectory::LowercaseData;

        ui.disableLintingCheckbox = &disableLinting;
        ui.leadingWhitespaceCheckbox = &leadingWhitespace;
        ui.trailingWhitespaceCheckbox = &trailingWhitespace;
        ui.contiguousWhitespaceCheckbox = &contiguousWhitespace;
        ui.unclosedPunctuationCheckbox = &unclosedPunctuation;
        ui.tagMismatchCheckbox = &tagMismatch;
        ui.spellcheckCheckbox = &spellcheck;
        ui.ltCheckbox = &languageTool;
        ui.termMismatchCheckbox = &termMismatch;

        ui.lintClearMenuButton = &clearButton;
        ui.lintCheckButton = &checkButton;
        ui.lintFileSelectMenuButton = &fileSelectButton;
        ui.lintLintSelectMenuButton = &lintSelectButton;
        ui.lintMenuTable = &table;

        menu.move(100, 450);
        menu.resize(300, 100);
        host.move(200, 480);
    }

    void init() { menu.init(&ui, files, &settings, &projectSettings, &glossary); }

    [[nodiscard]] auto fileSelect() const -> FileSelectMenu* { return menu.findChild<FileSelectMenu*>(); }

    [[nodiscard]] auto lintSelect() const -> LintSelectMenu* { return menu.findChild<LintSelectMenu*>(); }

    QTemporaryDir dir;
    QWidget host;
    Ui::MainWindow ui{};

    QCheckBox disableLinting{ &host };
    QCheckBox leadingWhitespace{ &host };
    QCheckBox trailingWhitespace{ &host };
    QCheckBox contiguousWhitespace{ &host };
    QCheckBox unclosedPunctuation{ &host };
    QCheckBox tagMismatch{ &host };
    QCheckBox spellcheck{ &host };
    QCheckBox languageTool{ &host };
    QCheckBox termMismatch{ &host };

    QPushButton clearButton{ &host };
    QPushButton checkButton{ &host };
    QPushButton fileSelectButton{ &host };
    QPushButton lintSelectButton{ &host };
    LintTable table{ &host };

    Settings settings;
    ProjectSettings projectSettings;
    GlossaryMenu glossary{ &host };
    LintMenu menu;
    vector<TabListItem> files{ { .name = u"actors"_s, .properties = {} }, { .name = u"map001"_s, .properties = {} } };
};

class TestLintMenu final : public QObject {
    Q_OBJECT

   private slots:
    void startsHiddenWithBothSelectionMenus() {
        Fixture fixture;

        QVERIFY(fixture.menu.isHidden());
        QVERIFY(fixture.fileSelect() != nullptr);
        QVERIFY(fixture.lintSelect() != nullptr);
    }

    void clearingBeforeInitTouchesNothing() {
        Fixture fixture;

        fixture.menu.clearResults();
        fixture.menu.clear();
        fixture.menu.settingsChanged();

        QCOMPARE(fixture.table.model()->rowCount(), 0);
    }

    void theCheckboxesShowTheFlagsOfTheSettings() {
        Fixture fixture;
        fixture.settings.translation.lintFlags = LintFlags::LeadingWhitespace | LintFlags::Spellcheck;

        fixture.init();

        QVERIFY(fixture.leadingWhitespace.isChecked());
        QVERIFY(fixture.spellcheck.isChecked());
        QVERIFY(!fixture.disableLinting.isChecked());
        QVERIFY(!fixture.trailingWhitespace.isChecked());
        QVERIFY(!fixture.contiguousWhitespace.isChecked());
        QVERIFY(!fixture.unclosedPunctuation.isChecked());
        QVERIFY(!fixture.tagMismatch.isChecked());
        QVERIFY(!fixture.languageTool.isChecked());
        QVERIFY(!fixture.termMismatch.isChecked());
    }

    void everyDefaultLintIsCheckedExceptDisablingThem() {
        Fixture fixture;

        fixture.init();

        QVERIFY(!fixture.disableLinting.isChecked());
        QVERIFY(fixture.leadingWhitespace.isChecked());
        QVERIFY(fixture.trailingWhitespace.isChecked());
        QVERIFY(fixture.contiguousWhitespace.isChecked());
        QVERIFY(fixture.unclosedPunctuation.isChecked());
        QVERIFY(fixture.tagMismatch.isChecked());
        QVERIFY(fixture.spellcheck.isChecked());
        QVERIFY(fixture.languageTool.isChecked());
        QVERIFY(fixture.termMismatch.isChecked());
    }

    void eachCheckboxFlipsItsOwnFlag() {
        Fixture fixture;
        fixture.init();

        const array<pair<QCheckBox*, LintFlags>, 9> bindings{ {
            { &fixture.disableLinting, LintFlags::LintingDisabled },
            { &fixture.leadingWhitespace, LintFlags::LeadingWhitespace },
            { &fixture.trailingWhitespace, LintFlags::TrailingWhitespace },
            { &fixture.contiguousWhitespace, LintFlags::ContiguousWhitespace },
            { &fixture.unclosedPunctuation, LintFlags::UnclosedPunctuation },
            { &fixture.tagMismatch, LintFlags::TagMismatch },
            { &fixture.spellcheck, LintFlags::Spellcheck },
            { &fixture.languageTool, LintFlags::LanguageTool },
            { &fixture.termMismatch, LintFlags::TermMismatch },
        } };

        // A lint added to the enum has to get its checkbox, and its row here
        QCOMPARE(bindings.size(), static_cast<usize>(enchantum::count<LintFlags>));

        for (const auto& [checkbox, flag] : bindings) {
            const LintFlags before = fixture.settings.translation.lintFlags;
            const bool wasSet = hasFlag(before, flag);

            checkbox->setChecked(!wasSet);
            QCOMPARE(hasFlag(fixture.settings.translation.lintFlags, flag), !wasSet);
            // Only that one flag changed
            QCOMPARE(fixture.settings.translation.lintFlags ^ before, flag);

            checkbox->setChecked(wasSet);
            QCOMPARE(fixture.settings.translation.lintFlags, before);
        }
    }

    void initializingAgainShowsNewSettingsWithoutBindingTwice() {
        Fixture fixture;
        fixture.init();

        Settings other;
        other.translation.lintFlags = LintFlags::TagMismatch;
        fixture.menu.init(&fixture.ui, fixture.files, &other, &fixture.projectSettings, &fixture.glossary);

        QVERIFY(fixture.tagMismatch.isChecked());
        QVERIFY(!fixture.leadingWhitespace.isChecked());

        // The checkboxes now write to the new settings
        fixture.leadingWhitespace.setChecked(true);
        QVERIFY(hasFlag(other.translation.lintFlags, LintFlags::LeadingWhitespace));

        usize requests = 0;
        connect(&fixture.menu, &LintMenu::checkRequested, &fixture.menu, [&requests](const Selected&) -> void {
            requests++;
        });
        fixture.checkButton.click();
        QCOMPARE(requests, usize{ 1 });
    }

    void checkingAsksForTheSelectedFiles() {
        Fixture fixture;
        fixture.init();

        for (QCheckBox* const box : fixture.fileSelect()->findChildren<QCheckBox*>()) {
            if (box->text() == u"actors"_qsv) {
                box->setChecked(true);
            }
        }

        Selected received;
        usize requests = 0;
        connect(
            &fixture.menu,
            &LintMenu::checkRequested,
            &fixture.menu,
            [&received, &requests](const Selected& selected) -> void {
                received = selected;
                requests++;
            }
        );

        fixture.checkButton.click();

        QCOMPARE(requests, usize{ 1 });
        QVERIFY((received.flags & FileFlags_Actors) != 0);
        QVERIFY(!received.empty());
    }

    void clearingTheMenuButtonEmptiesTheResults() {
        Fixture fixture;
        fixture.init();
        fixture.table.appendRow(rowOf(1));
        fixture.table.appendRow(rowOf(2));
        QCOMPARE(fixture.table.model()->rowCount(), 2);

        fixture.clearButton.click();

        QCOMPARE(fixture.table.model()->rowCount(), 0);
    }

    void clearResultsKeepsTheSelectionMenus() {
        Fixture fixture;
        fixture.init();
        fixture.table.appendRow(rowOf(1));
        fixture.fileSelectButton.click();
        QVERIFY(!fixture.fileSelect()->isHidden());

        fixture.menu.clearResults();

        QCOMPARE(fixture.table.model()->rowCount(), 0);
        QVERIFY(!fixture.fileSelect()->isHidden());
        QVERIFY(!fixture.fileSelect()->findChildren<QCheckBox*>().isEmpty());
    }

    void closingTheProjectEmptiesEverything() {
        Fixture fixture;
        fixture.init();
        fixture.table.appendRow(rowOf(1));
        fixture.fileSelectButton.click();
        fixture.lintSelectButton.click();
        QVERIFY(!fixture.fileSelect()->isHidden());
        QVERIFY(!fixture.lintSelect()->isHidden());

        fixture.menu.clear();

        QCOMPARE(fixture.table.model()->rowCount(), 0);
        QVERIFY(fixture.fileSelect()->isHidden());
        QVERIFY(fixture.fileSelect()->findChildren<QCheckBox*>().isEmpty());
        QVERIFY(fixture.lintSelect()->findChildren<QGroupBox*>().isEmpty());
    }

    void theFileButtonTogglesTheFileMenu() {
        Fixture fixture;
        fixture.init();
        QVERIFY(fixture.fileSelect()->isHidden());

        fixture.fileSelectButton.click();
        QVERIFY(!fixture.fileSelect()->isHidden());

        fixture.fileSelectButton.click();
        QVERIFY(fixture.fileSelect()->isHidden());
    }

    void theLintButtonFillsAndTogglesTheLintMenu() {
        Fixture fixture;
        fixture.init();
        QVERIFY(fixture.lintSelect()->findChildren<QGroupBox*>().isEmpty());

        fixture.lintSelectButton.click();
        QVERIFY(!fixture.lintSelect()->isHidden());
        QVERIFY(!fixture.lintSelect()->findChildren<QGroupBox*>().isEmpty());

        fixture.lintSelectButton.click();
        QVERIFY(fixture.lintSelect()->isHidden());
    }

    void aMenuOpensAboveTheDockInsideTheScreen() {
        Fixture fixture;
        fixture.host.show();
        fixture.menu.show();
        fixture.init();

        fixture.fileSelectButton.click();

        const QWidget* const popup = fixture.fileSelect();
        const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        const QPoint dockTop = fixture.menu.mapToGlobal(QPoint(0, 0));

        QVERIFY(popup->x() >= available.left());
        QVERIFY(popup->x() + popup->width() <= available.right() + 1);
        QVERIFY(popup->y() >= available.top());
        // Above the dock, as the screen has the room: the test would check nothing if it had not
        QVERIFY(dockTop.y() - popup->height() - 4 >= available.top());
        QCOMPARE(popup->y() + popup->height() + 4, dockTop.y());
    }

    void aTermRemovedFromTheGlossaryHidesTheLintMenu() {
        Fixture fixture;
        fixture.init();
        fixture.lintSelectButton.click();
        QVERIFY(!fixture.lintSelect()->isHidden());

        emit fixture.glossary.termRemoved();

        QVERIFY(fixture.lintSelect()->isHidden());
    }

    void changedSettingsHideTheLintMenu() {
        Fixture fixture;
        fixture.init();
        fixture.lintSelectButton.click();
        QVERIFY(!fixture.lintSelect()->isHidden());

        fixture.menu.settingsChanged();

        QVERIFY(fixture.lintSelect()->isHidden());
    }
};

QTEST_MAIN(TestLintMenu)
#include "tst_lintmenu.moc"
