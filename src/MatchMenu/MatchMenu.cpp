#include "MatchMenu.hpp"

#include "ColoredTextLabel.hpp"
#include "rpgmtranslate.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

struct SearchMatch {
    const u32 start;
    const u32 len;
    const f32 score;
};

MatchMenu::MatchMenu(QWidget* const parent) : QDockWidget(parent) {
    hide();
}

void MatchMenu::init(
    QPushButton* const clearButton,
    QTableWidget* const matchTable
) {
    this->clearButton = clearButton;
    this->matchTable = matchTable;

    connect(clearButton, &QPushButton::pressed, this, [this] -> void {
        this->matchTable->model()->removeRows(
            0,
            this->matchTable->model()->rowCount()
        );
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
    if (matches.len == 0) {
        return;
    }

    const i32 row = matchTable->rowCount();
    matchTable->insertRow(row);

    auto* const filenameItem = new QTableWidgetItem(filename);
    matchTable->setItem(row, 0, filenameItem);

    auto* const rowItem = new QTableWidgetItem(QString::number(lineIndex + 1));
    matchTable->setItem(row, 1, rowItem);

    const u8* ptr = matches.ptr;

    u32 sourceCount = *ras<const u32*>(ptr);
    ptr += sizeof(u32);

    auto sourceMatches = span(ras<const SearchMatch*>(ptr), sourceCount);
    ptr += sourceCount * sizeof(SearchMatch);

    u32 translationCount = *ras<const u32*>(ptr);
    ptr += sizeof(u32);

    auto translationMatches =
        span(ras<const SearchMatch*>(ptr), translationCount);
    ptr += translationCount * sizeof(SearchMatch);

    QString sourceMatchDescs;
    vector<Span> sourceSpans;

    sourceMatchDescs.reserve(sourceCount * 13);
    sourceSpans.reserve(sourceCount);

    if (sourceCount != 0) {
        sourceMatchDescs += ": "_L1;
    }

    for (const auto match : sourceMatches) {
        if (match.score == 0.0F) {
            sourceMatchDescs += tr("Exact");
        } else {
            sourceMatchDescs +=
                tr("Fuzzy (%1)").arg(QString::number(match.score, 10, 3));
        }

        sourceSpans.emplace_back(match.start, match.len);
    }

    QString translationMatchDescs;
    vector<Span> translationSpans;

    translationMatchDescs.reserve(translationCount * 13);
    translationSpans.reserve(translationCount);

    if (sourceCount != 0) {
        translationMatchDescs += ": "_L1;
    }

    for (const auto match : translationMatches) {
        if (match.score == 0.0F) {
            translationMatchDescs +=
                tr("Fuzzy (%1)").arg(QString::number(match.score, 10, 3));
        } else {
            translationMatchDescs += tr("Exact");
        }

        translationSpans.emplace_back(match.start, match.len);
    }

    auto* const termItem = new QTableWidgetItem(
        tr("%1 [%2 occurrences%3] -> %4 [%5 occurrences%6]")
            .arg(termSource)
            .arg(sourceCount)
            .arg(sourceMatchDescs)
            .arg(termTranslation)
            .arg(translationCount)
            .arg(translationMatchDescs)
    );
    matchTable->setItem(row, 2, termItem);

    auto* const resultWidget = new QWidget();
    auto* const resultLayout = new QHBoxLayout(resultWidget);

    const QPalette palette = qApp->palette();
    const QColor highlightedText =
        palette.color(QPalette::Active, QPalette::HighlightedText);
    const QColor text = palette.color(QPalette::Active, QPalette::Text);
    QColor highlightedColor =
        palette.color(QPalette::Active, QPalette::Highlight);

    if (highlightedText != text) {
        highlightedColor = highlightedText;
    }

    auto* const sourceMatchWidget = new ColoredTextLabel(resultWidget);
    sourceMatchWidget->setData(source, sourceSpans, highlightedColor);

    auto* const translationMatchWidget = new ColoredTextLabel(resultWidget);
    translationMatchWidget
        ->setData(translation, translationSpans, highlightedColor);

    resultLayout->addWidget(sourceMatchWidget);
    resultLayout->addWidget(new QLabel(u"->"_s, resultWidget));
    resultLayout->addWidget(translationMatchWidget);

    matchTable->setCellWidget(row, 3, resultWidget);

    QString info;
    const bool match = sourceCount <= translationCount;

    if (translation.isEmpty()) {
        info = tr("Translation is empty.");
    } else if (translationCount == 0) {
        info = tr("Term translation is not present.");
    } else if (!match) {
        info = tr(
            "Number of term occurrences doesn't match the number of translation occurrences."
        );
    } else {
        info = tr("Match.");
    }

    auto* const infoItem = new QTableWidgetItem(info);
    matchTable->setItem(row, 4, infoItem);
}

void MatchMenu::clear() {
    matchTable->model()->removeRows(0, matchTable->model()->rowCount());
}