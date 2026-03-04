#include "MatchMenu.hpp"

#include "rpgmtranslate.h"

#include <QPushButton>

MatchMenu::MatchMenu(QWidget* const parent) : QDockWidget(parent) {
    hide();
}

void MatchMenu::init(
    QPushButton* const clearButton,
    MatchTable* const matchTable
) {
    this->clearButton = clearButton;
    this->matchTable = matchTable;

    connect(clearButton, &QPushButton::pressed, this, [this] -> void {
        this->matchTable->model()->clear();
    });
}

void MatchMenu::appendMatch(
    const QString& filename,
    const u32 lineIndex,
    const QString& termSource,
    const QString& termTranslation,
    const QStringView source,
    const QStringView translation,
    const ByteBuffer matches
) {
    this->matchTable->appendMatch(
        filename,
        lineIndex,
        termSource,
        termTranslation,
        source,
        translation,
        matches
    );
}

void MatchMenu::clear() {
    matchTable->model()->clear();
}