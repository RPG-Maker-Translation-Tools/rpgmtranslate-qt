#include "LintMenu.hpp"

#include "FileSelectMenu.hpp"
#include "GlossaryMenu.hpp"
#include "LintSelectMenu.hpp"
#include "LintTable.hpp"
#include "Settings.hpp"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QScreen>

namespace {
constexpr i32 MENU_GAP = 4;
}  // namespace

LintMenu::LintMenu(QWidget* const parent) :
    QDockWidget(parent),
    fileSelectMenu(new FileSelectMenu(this)),
    lintSelectMenu(new LintSelectMenu(this)) {
    hide();
}

void LintMenu::init(
    Ui::MainWindow* const ui_,
    const vector<TabListItem>& files,
    Settings* const sets,
    ProjectSettings* const projS,
    GlossaryMenu* const glossaryMenu
) {
    this->clearButton = ui_->lintClearMenuButton;
    this->checkButton = ui_->lintCheckButton;
    this->fileSelectMenuButton = ui_->lintFileSelectMenuButton;
    this->lintSelectMenuButton = ui_->lintLintSelectMenuButton;
    this->lintTable = ui_->lintMenuTable;

    fileSelectMenu->init(files);
    lintSelectMenu->init(sets, projS, glossaryMenu);

    this->sets = sets;
    this->projS = projS;

    ui_->disableLintingCheckbox->setChecked((sets->translation.lintFlags & LintFlags::LintingDisabled) != 0);
    ui_->leadingWhitespaceCheckbox->setChecked((sets->translation.lintFlags & LintFlags::LeadingWhitespace) != 0);
    ui_->trailingWhitespaceCheckbox->setChecked((sets->translation.lintFlags & LintFlags::TrailingWhitespace) != 0);
    ui_->contiguousWhitespaceCheckbox->setChecked((sets->translation.lintFlags & LintFlags::ContiguousWhitespace) != 0);
    ui_->unclosedPunctuationCheckbox->setChecked((sets->translation.lintFlags & LintFlags::UnclosedPunctuation) != 0);
    ui_->tagMismatchCheckbox->setChecked((sets->translation.lintFlags & LintFlags::TagMismatch) != 0);
    ui_->spellcheckCheckbox->setChecked((sets->translation.lintFlags & LintFlags::Spellcheck) != 0);
    ui_->ltCheckbox->setChecked((sets->translation.lintFlags & LintFlags::LanguageTool) != 0);
    ui_->termMismatchCheckbox->setChecked((sets->translation.lintFlags & LintFlags::TermMismatch) != 0);

    if (connected) {
        return;
    }

    connected = true;

    // The menu lists the terms by position
    connect(glossaryMenu, &GlossaryMenu::termRemoved, lintSelectMenu, &QWidget::hide);

    const auto bindFlag = [this](const QCheckBox* const checkbox, const LintFlags flag) -> void {
        connect(checkbox, &QCheckBox::checkStateChanged, this, [this, flag](const Qt::CheckState state) -> void {
            if (state == Qt::Checked) {
                this->sets->translation.lintFlags |= flag;
            } else {
                this->sets->translation.lintFlags &= ~flag;
            }
        });
    };

    bindFlag(ui_->disableLintingCheckbox, LintFlags::LintingDisabled);
    bindFlag(ui_->leadingWhitespaceCheckbox, LintFlags::LeadingWhitespace);
    bindFlag(ui_->trailingWhitespaceCheckbox, LintFlags::TrailingWhitespace);
    bindFlag(ui_->contiguousWhitespaceCheckbox, LintFlags::ContiguousWhitespace);
    bindFlag(ui_->unclosedPunctuationCheckbox, LintFlags::UnclosedPunctuation);
    bindFlag(ui_->tagMismatchCheckbox, LintFlags::TagMismatch);
    bindFlag(ui_->spellcheckCheckbox, LintFlags::Spellcheck);
    bindFlag(ui_->ltCheckbox, LintFlags::LanguageTool);
    bindFlag(ui_->termMismatchCheckbox, LintFlags::TermMismatch);

    connect(fileSelectMenuButton, &QPushButton::clicked, this, [this] -> void {
        toggleAboveDock(fileSelectMenu, fileSelectMenuButton);
    });

    connect(lintSelectMenuButton, &QPushButton::clicked, this, [this] -> void {
        if (lintSelectMenu->isHidden()) {
            lintSelectMenu->refresh();
        }

        toggleAboveDock(lintSelectMenu, lintSelectMenuButton);
    });

    connect(checkButton, &QPushButton::clicked, this, [this] -> void {
        emit checkRequested(fileSelectMenu->selected());
    });
    connect(clearButton, &QPushButton::clicked, this, [this] -> void { this->lintTable->model()->clear(); });
}

void LintMenu::clearResults() {
    if (lintTable != nullptr) {
        lintTable->model()->clear();
    }
}

void LintMenu::clear() {
    clearResults();

    // What they list is that of the project that closes
    fileSelectMenu->clear();
    fileSelectMenu->hide();
    lintSelectMenu->clear();
}

void LintMenu::settingsChanged() {
    // The toggles of the menu point into the settings, and the settings window has replaced what they point to
    lintSelectMenu->hide();
}

void LintMenu::toggleAboveDock(QWidget* const menu, const QWidget* const anchor) {
    if (!menu->isHidden()) {
        menu->hide();
        return;
    }

    menu->adjustSize();

    const QPoint dockTopLeft = mapToGlobal(QPoint(0, 0));
    const QPoint anchorTopLeft = anchor->mapToGlobal(QPoint(0, 0));

    auto pos = QPoint(anchorTopLeft.x(), dockTopLeft.y() - menu->height() - MENU_GAP);

    if (const QScreen* const screen = qApp->screenAt(anchorTopLeft)) {
        const QRect available = screen->availableGeometry();

        pos.setX(clamp(pos.x(), available.left(), max(available.left(), available.right() - menu->width())));
        pos.setY(max(pos.y(), available.top()));
    }

    menu->move(pos);
    menu->show();
}
