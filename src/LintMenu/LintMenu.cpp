#include "LintMenu.hpp"

#include "Constants.hpp"
#include "FileSelectMenu.hpp"
#include "LintSelectMenu.hpp"
#include "LintTable.hpp"
#include "Settings.hpp"
#include "rpgmtranslate_rs.h"
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
    Settings* sets,
    const ProjectSettings* projS,
    GlossaryMenu* const glossaryMenu
) {
    this->clearButton = ui_->lintClearMenuButton;
    this->checkButton = ui_->lintCheckButton;
    this->fileSelectMenuButton = ui_->lintFileSelectMenuButton;
    this->lintSelectMenuButton = ui_->lintLintSelectMenuButton;
    this->lintTable = ui_->lintMenuTable;

    fileSelectMenu->init(files);
    lintSelectMenu->init(sets, glossaryMenu);

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

    connect(ui_->disableLintingCheckbox, &QCheckBox::checkStateChanged, this, [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::LintingDisabled;
        } else {
            sets->translation.lintFlags &= ~LintFlags::LintingDisabled;
        }
    });

    connect(
        ui_->leadingWhitespaceCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::LeadingWhitespace;
        } else {
            sets->translation.lintFlags &= ~LintFlags::LeadingWhitespace;
        }
    }
    );

    connect(
        ui_->trailingWhitespaceCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::TrailingWhitespace;
        } else {
            sets->translation.lintFlags &= ~LintFlags::TrailingWhitespace;
        }
    }
    );

    connect(
        ui_->contiguousWhitespaceCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::ContiguousWhitespace;
        } else {
            sets->translation.lintFlags &= ~LintFlags::ContiguousWhitespace;
        }
    }
    );

    connect(
        ui_->unclosedPunctuationCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::UnclosedPunctuation;
        } else {
            sets->translation.lintFlags &= ~LintFlags::UnclosedPunctuation;
        }
    }
    );

    connect(ui_->tagMismatchCheckbox, &QCheckBox::checkStateChanged, this, [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::TagMismatch;
        } else {
            sets->translation.lintFlags &= ~LintFlags::TagMismatch;
        }
    });

    connect(ui_->spellcheckCheckbox, &QCheckBox::checkStateChanged, this, [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::Spellcheck;
        } else {
            sets->translation.lintFlags &= ~LintFlags::Spellcheck;
        }
    });

    connect(ui_->ltCheckbox, &QCheckBox::checkStateChanged, this, [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::LanguageTool;
        } else {
            sets->translation.lintFlags &= ~LintFlags::LanguageTool;
        }
    });

    connect(ui_->termMismatchCheckbox, &QCheckBox::checkStateChanged, this, [=](const Qt::CheckState state) -> void {
        if (state == Qt::Checked) {
            sets->translation.lintFlags |= LintFlags::TermMismatch;
        } else {
            sets->translation.lintFlags &= ~LintFlags::TermMismatch;
        }
    });

    connect(fileSelectMenuButton, &QPushButton::pressed, this, [this] -> void {
        toggleAboveDock(fileSelectMenu, fileSelectMenuButton);
    });

    connect(lintSelectMenuButton, &QPushButton::pressed, this, [this] -> void {
        if (lintSelectMenu->isHidden()) {
            lintSelectMenu->refresh();
        }

        toggleAboveDock(lintSelectMenu, lintSelectMenuButton);
    });

    connect(checkButton, &QPushButton::pressed, this, [this] -> void {
        emit checkRequested(fileSelectMenu->selected());
    });
    connect(clearButton, &QPushButton::pressed, this, [this] -> void { this->lintTable->model()->clear(); });
}

void LintMenu::clear() {
    if (lintTable != nullptr) {
        lintTable->model()->clear();
    }
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