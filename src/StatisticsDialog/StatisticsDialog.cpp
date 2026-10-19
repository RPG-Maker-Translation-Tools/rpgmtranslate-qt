#include "StatisticsDialog.hpp"

#include "Types.hpp"
#include "Utils.hpp"

#include <QEvent>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
constexpr i32 COLUMN_COUNT = 5;
constexpr i32 DIALOG_WIDTH = 720;
constexpr i32 DIALOG_HEIGHT = 480;

[[nodiscard]] auto numberItem(const u64 number, const bool bold) -> QTableWidgetItem* {
    auto* const item = new QTableWidgetItem(QString(qsv(qitos(number))));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    if (bold) {
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
    }

    return item;
}
}  // namespace

StatisticsDialog::StatisticsDialog(QWidget* const parent) :
    QDialog(parent),
    table(new QTableWidget(this)) {
    resize(DIALOG_WIDTH, DIALOG_HEIGHT);

    table->setColumnCount(COLUMN_COUNT);
    retranslate();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    auto* const layout = new QVBoxLayout(this);
    layout->addWidget(table);
}

void StatisticsDialog::retranslate() {
    setWindowTitle(tr("Project Statistics"));
    table->setHorizontalHeaderLabels(
        { tr("File"), tr("Source words"), tr("Source characters"), tr("Translation words"), tr("Translation characters") }
    );

    // The totals are the last row, once the dialog is filled
    if (table->rowCount() > 0) {
        table->item(table->rowCount() - 1, 0)->setText(tr("Total"));
    }
}

void StatisticsDialog::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }

    QDialog::changeEvent(event);
}

void StatisticsDialog::fill(const vector<FileStatistics>& files) const {
    table->setRowCount(scast<i32>(files.size()) + 1);

    u64 sourceWords = 0;
    u64 sourceCharacters = 0;
    u64 translationWords = 0;
    u64 translationCharacters = 0;

    for (const auto [row, file] : views::enumerate(files)) {
        table->setItem(scast<i32>(row), 0, new QTableWidgetItem(QString::fromLatin1(file.filename.data())));
        table->setItem(scast<i32>(row), 1, numberItem(file.source.words, false));
        table->setItem(scast<i32>(row), 2, numberItem(file.source.characters, false));
        table->setItem(scast<i32>(row), 3, numberItem(file.translation.words, false));
        table->setItem(scast<i32>(row), 4, numberItem(file.translation.characters, false));

        sourceWords += file.source.words;
        sourceCharacters += file.source.characters;
        translationWords += file.translation.words;
        translationCharacters += file.translation.characters;
    }

    const i32 totalRow = scast<i32>(files.size());
    auto* const totalLabel = new QTableWidgetItem(tr("Total"));
    QFont font = totalLabel->font();
    font.setBold(true);
    totalLabel->setFont(font);

    table->setItem(totalRow, 0, totalLabel);
    table->setItem(totalRow, 1, numberItem(sourceWords, true));
    table->setItem(totalRow, 2, numberItem(sourceCharacters, true));
    table->setItem(totalRow, 3, numberItem(translationWords, true));
    table->setItem(totalRow, 4, numberItem(translationCharacters, true));
}
