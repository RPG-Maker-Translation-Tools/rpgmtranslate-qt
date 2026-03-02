#include "GlossaryMenu.hpp"

#include "Constants.hpp"
#include "FileSelectMenu.hpp"
#include "PersistentMenu.hpp"
#include "Types.hpp"
#include "ui_GlossaryMenu.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

constexpr u8 SOURCE_IDX = 0;
constexpr u8 TRANSLATION_IDX = 1;
constexpr u8 NOTE_IDX = 2;
constexpr u8 ACTIONS_IDX = 3;
constexpr u8 ACTIONS_COL_WIDTH = 120;

constexpr QMargins CELL_MARGINS = { 8, 8, 8, 8 };
constexpr u8 ROW_MARGIN = 4;
constexpr u8 ROW_SPACING = 8;
constexpr u8 BUTTON_SIZE = 32;

constexpr f32 FUZZY_MIN = 0.0F;
constexpr f32 FUZZY_MAX = 1.0F;
constexpr u8 FUZZY_DECIMALS = 3;

GlossaryMenu::GlossaryMenu(QWidget* const parent) :
    PersistentMenu(parent, Qt::FramelessWindowHint),
    ui(setupUi()),
    searchInput(ui->searchInput),
    searchButton(ui->searchButton),
    addTermButton(ui->addTermButton),
    fileSelectButton(ui->fileSelectButton),
    qcButton(ui->qcButton),
    glossaryTable(ui->glossaryTable),

    fileSelectMenu(new FileSelectMenu(parent)) {
    setDragMoveEnabled(true);

    glossaryTable->setUniformRowHeights(true);
    auto* const header = glossaryTable->header();
    header->setSectionResizeMode(SOURCE_IDX, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(
        TRANSLATION_IDX,
        QHeaderView::ResizeToContents
    );
    header->setSectionResizeMode(NOTE_IDX, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ACTIONS_IDX, QHeaderView::Fixed);
    header->resizeSection(ACTIONS_IDX, ACTIONS_COL_WIDTH);

    connect(addTermButton, &QPushButton::pressed, this, [this] -> void {
        addNewEntry();
    });

    connect(qcButton, &QPushButton::pressed, this, [this] -> void {
        if (fileSelectMenu->selectedCount() == 0) {
            return;
        }

        emit checkRequested(fileSelectMenu->selected(), glossary());
    });

    connect(fileSelectButton, &QPushButton::pressed, this, [this] -> void {
        fileSelectMenu->move(x() + width(), y());
        fileSelectMenu->show();
    });

    connect(searchButton, &QPushButton::pressed, this, [this] -> void {
        const QString searchText = searchInput->text();
        const QStringView trimmed = QStringView(searchText).trimmed();

        if (trimmed.isEmpty()) {
            return;
        }

        const i32 itemCount = glossaryTable->topLevelItemCount();
        if (itemCount == 0) {
            return;
        }

        i32 startIndex = lastSearchIndex + 1;

        if (startIndex >= itemCount) {
            startIndex = 0;
        }

        for (i32 i = 0; i < itemCount; i++) {
            const i32 currentIndex = (startIndex + i) % itemCount;
            auto* const item = glossaryTable->topLevelItem(currentIndex);

            const auto* const sourceCell =
                as<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
            const auto* const translationCell = as<TermInfoCell*>(
                glossaryTable->itemWidget(item, TRANSLATION_IDX)
            );

            const QString sourceText = sourceCell->text();
            const QString translationText = translationCell->text();

            if (sourceText.contains(searchText, Qt::CaseInsensitive) ||
                translationText.contains(searchText, Qt::CaseInsensitive)) {
                glossaryTable->setCurrentItem(item);
                glossaryTable->scrollToItem(item);
                lastSearchIndex = currentIndex;
                return;
            }
        }

        lastSearchIndex = 0;
        QMessageBox::information(
            nullptr,
            tr("Search"),
            tr("No match found for: %1").arg(searchText)
        );
    });

    connect(this, &PersistentMenu::hidden, this, [this] -> void {
        fileSelectMenu->hide();
        fileSelectButton->setChecked(false);
    });
}

GlossaryMenu::~GlossaryMenu() {
    delete ui;
}

auto GlossaryMenu::setupUi() -> Ui::GlossaryMenu* {
    auto* const ui_ = new Ui::GlossaryMenu();
    ui_->setupUi(this);
    return ui_;
}

void GlossaryMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    PersistentMenu::changeEvent(event);
}

auto GlossaryMenu::makeTermInfoCell(
    const QString& text,
    const MatchModeInfo& info
) -> QWidget* {
    auto* const cell = new TermInfoCell(glossaryTable);
    cell->setFrom(text, info);
    return cell;
}

auto GlossaryMenu::makeNoteCell(const QString& note) -> QWidget* {
    auto* const noteContainer = new QWidget(glossaryTable);
    auto* const layout = new QHBoxLayout(noteContainer);

    // FIXME: No frame
    auto* const noteInput = new QPlainTextEdit(noteContainer);
    noteInput->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    noteInput->setWordWrapMode(QTextOption::WordWrap);
    noteInput->setPlainText(note);
    noteInput->setLineWidth(2);

    layout->addWidget(noteInput);
    return noteContainer;
}

auto GlossaryMenu::makeActionCell(QTreeWidgetItem* const item, const u16 index)
    -> QWidget* {
    auto* const actionsCell = new ActionButtonsCell(glossaryTable);

    connect(
        actionsCell,
        &ActionButtonsCell::editToggled,
        this,
        [this, actionsCell, item, index](const bool editable) -> void {
        if (!editable) {
            auto* const rowItem = glossaryTable->itemFromIndex(
                glossaryTable->model()->index(index, 0)
            );

            auto* const sourceInput =
                glossaryTable->itemWidget(rowItem, 0)->findChild<QLineEdit*>();
            auto* const translationInput =
                glossaryTable->itemWidget(rowItem, 1)->findChild<QLineEdit*>();

            if (sourceInput->text().isEmpty()) {
                QMessageBox::warning(
                    this,
                    tr("Term is empty"),
                    tr("Empty term is not allowed.")
                );
                actionsCell->editButton->setChecked(true);
                return;
            }

            if (translationInput->text().isEmpty()) {
                QMessageBox::warning(
                    this,
                    tr("Translation is empty"),
                    tr("Empty term translation is not allowed.")
                );
                actionsCell->editButton->setChecked(true);
                return;
            }
        }

        actionsCell->setEditable(editable);
        setRowEditable(item, editable);
    }
    );

    connect(
        actionsCell,
        &ActionButtonsCell::deleteRequested,
        this,
        [this, item] -> void {
        const auto pressed = QMessageBox::question(
            nullptr,
            tr("Confirm Delete"),
            tr("Are you sure you want to delete this entry?")
        );

        if (pressed == QMessageBox::Yes) {
            const i32 index = glossaryTable->indexOfTopLevelItem(item);
            delete glossaryTable->takeTopLevelItem(index);
            adjustSize();
        }
    }
    );

    connect(
        actionsCell,
        &ActionButtonsCell::checkRequested,
        this,
        [this, item] -> void {
        if (fileSelectMenu->selectedCount() == 0) {
            return;
        }

        const u32 row = glossaryTable->indexOfTopLevelItem(item);

        const auto* const sourceCell =
            as<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
        const auto* const translationCell =
            as<TermInfoCell*>(glossaryTable->itemWidget(item, TRANSLATION_IDX));
        const auto* const noteInput = glossaryTable->itemWidget(item, NOTE_IDX)
                                          ->findChild<QPlainTextEdit*>();

        Term term(
            sourceCell->text(),
            translationCell->text(),
            noteInput->toPlainText(),
            sourceCell->toMatchModeInfo(),
            translationCell->toMatchModeInfo()
        );

        emit checkRequested(fileSelectMenu->selected(), term);
    }
    );

    return actionsCell;
}

void GlossaryMenu::setRowEditable(QTreeWidgetItem* const item, bool editable) {
    auto* const sourceCell =
        as<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
    auto* const translationCell =
        as<TermInfoCell*>(glossaryTable->itemWidget(item, TRANSLATION_IDX));
    auto* const noteInput =
        glossaryTable->itemWidget(item, NOTE_IDX)->findChild<QPlainTextEdit*>();

    sourceCell->setEditable(editable);
    translationCell->setEditable(editable);
    noteInput->setReadOnly(!editable);
}

void GlossaryMenu::addNewEntry(
    const QString& source,
    const QString& translation,
    const QString& note,
    const MatchModeInfo sourceMatchMode,
    const MatchModeInfo translationMatchMode,
    const bool editable
) {
    const u16 row = glossaryTable->model()->rowCount();

    auto* const item = new QTreeWidgetItem(glossaryTable);

    auto* const sourceCell = makeTermInfoCell(source, sourceMatchMode);
    glossaryTable->setItemWidget(item, SOURCE_IDX, sourceCell);
    glossaryTable->setItemWidget(
        item,
        TRANSLATION_IDX,
        makeTermInfoCell(translation, translationMatchMode)
    );
    auto* const noteCell = makeNoteCell(note);
    glossaryTable->setItemWidget(item, NOTE_IDX, noteCell);
    glossaryTable->setItemWidget(item, ACTIONS_IDX, makeActionCell(item, row));

    item->setTextAlignment(SOURCE_IDX, Qt::AlignTop | Qt::AlignHCenter);
    item->setTextAlignment(TRANSLATION_IDX, Qt::AlignTop | Qt::AlignHCenter);
    item->setTextAlignment(NOTE_IDX, Qt::AlignTop | Qt::AlignHCenter);
    item->setTextAlignment(ACTIONS_IDX, Qt::AlignCenter);

    setRowEditable(item, editable);
    as<ActionButtonsCell*>(glossaryTable->itemWidget(item, ACTIONS_IDX))
        ->setEditable(editable);

    noteCell->setFixedHeight(sourceCell->sizeHint().height());
    glossaryTable->addTopLevelItem(item);

    resize(sizeHint());
}

void GlossaryMenu::fill(const Glossary& glossary) {
    for (const auto& term : glossary.terms) {
        addNewEntry(
            term.term,
            term.translation,
            term.note,
            term.sourceMatchMode,
            term.translationMatchMode,
            false
        );
    }
}

auto GlossaryMenu::glossary() const -> Glossary {
    Glossary out;

    const u32 entryCount = glossaryTable->topLevelItemCount();
    out.terms.reserve(entryCount);

    for (i32 idx = 0; idx < entryCount; idx++) {
        auto* const item = glossaryTable->topLevelItem(idx);

        const auto* const sourceCell =
            as<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
        const auto* const translationCell =
            as<TermInfoCell*>(glossaryTable->itemWidget(item, TRANSLATION_IDX));
        const auto* const noteInput = glossaryTable->itemWidget(item, NOTE_IDX)
                                          ->findChild<QPlainTextEdit*>();

        out.terms.emplace_back(
            sourceCell->text(),
            translationCell->text(),
            noteInput->toPlainText(),
            sourceCell->toMatchModeInfo(),
            translationCell->toMatchModeInfo()
        );
    }

    return out;
}

void GlossaryMenu::addFile(const QString& file) {
    fileSelectMenu->addFile(file);
}

void GlossaryMenu::clear() {
    glossaryTable->clear();
    fileSelectMenu->clear();
    searchInput->clear();
    lastSearchIndex = 0;
}

[[nodiscard]] auto TermInfoCell::toMatchModeInfo() const -> MatchModeInfo {
    const QString fuzzyText = fuzzyThresholdInput->text().trimmed();
    const f32 fuzzyThreshold = fuzzyText.isEmpty() ? DEFAULT_FUZZY_THRESHOLD
                                                   : f32(fuzzyText.toDouble());

    return MatchModeInfo{
        .fuzzyThreshold = fuzzyThreshold,
        .mode = MatchMode(modeSelect->currentIndex()),
        .caseSensitive = caseSensitive->isChecked(),
        .permissive = permissive->isChecked(),
    };
}

[[nodiscard]] auto TermInfoCell::text() const -> QString {
    return textInput->text();
}

void TermInfoCell::setFrom(const QString& text, const MatchModeInfo& info) {
    textInput->setText(text);

    modeSelect->setCurrentIndex(u8(info.mode));
    caseSensitive->setChecked(info.caseSensitive);
    permissive->setChecked(info.permissive);

    const f32 threshold = (info.fuzzyThreshold > 0.0F)
                              ? info.fuzzyThreshold
                              : DEFAULT_FUZZY_THRESHOLD;
    fuzzyThresholdInput->setText(QString::number(threshold));

    const bool show =
        (info.mode == MatchMode::Fuzzy || info.mode == MatchMode::Both);
    fuzzyThresholdInput->setVisible(show);
}

void TermInfoCell::setEditable(const bool editable) {
    textInput->setReadOnly(!editable);
    modeSelect->setEnabled(editable);
    caseSensitive->setEnabled(editable);
    permissive->setEnabled(editable);
    fuzzyThresholdInput->setReadOnly(!editable);
}

TermInfoCell::TermInfoCell(QWidget* const parent) : QWidget(parent) {
    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(CELL_MARGINS);
    layout->setSpacing(ROW_SPACING);
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    textInput = new QLineEdit(this);
    layout->addWidget(textInput);

    auto* const row = new QWidget(this);
    auto* const rowLayout = new QHBoxLayout(row);
    rowLayout
        ->setContentsMargins(ROW_MARGIN, ROW_MARGIN, ROW_MARGIN, ROW_MARGIN);
    rowLayout->setSpacing(ROW_SPACING);

    modeSelect = new QComboBox(row);
    modeSelect->addItem(tr("Exact"));
    modeSelect->addItem(tr("Fuzzy"));
    modeSelect->addItem(tr("Both"));
    rowLayout->addWidget(modeSelect);

    caseSensitive = new QCheckBox(tr("Case Sensitive"), row);
    permissive = new QCheckBox(tr("Permissive"), row);
    rowLayout->addWidget(caseSensitive);
    rowLayout->addWidget(permissive);

    fuzzyThresholdInput = new QLineEdit(row);
    fuzzyThresholdInput->setValidator(new QDoubleValidator(
        FUZZY_MIN,
        FUZZY_MAX,
        FUZZY_DECIMALS,
        fuzzyThresholdInput
    ));
    rowLayout->addWidget(fuzzyThresholdInput);

    rowLayout->addStretch(1);
    layout->addWidget(row);

    const auto updateThresholdVisibility = [this] -> void {
        const auto mode = MatchMode(modeSelect->currentIndex());
        const bool show = (mode == MatchMode::Fuzzy || mode == MatchMode::Both);
        fuzzyThresholdInput->setVisible(show);

        if (show && fuzzyThresholdInput->text().trimmed().isEmpty()) {
            fuzzyThresholdInput->setText(
                QString::number(DEFAULT_FUZZY_THRESHOLD)
            );
        }
    };

    connect(
        modeSelect,
        &QComboBox::currentIndexChanged,
        this,
        updateThresholdVisibility
    );

    updateThresholdVisibility();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

ActionButtonsCell::ActionButtonsCell(QWidget* const parent) : QWidget(parent) {
    auto* const layout = new QHBoxLayout(this);
    layout->setContentsMargins(CELL_MARGINS);
    layout->setSpacing(ROW_SPACING);
    layout->setAlignment(Qt::AlignCenter);

    editButton = new QPushButton(
        QIcon(u":/icons/edit_arrow_down.svg"_s),
        QString(),
        this
    );
    editButton->setCheckable(true);
    editButton->setChecked(false);
    editButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    layout->addWidget(editButton);

    deleteButton =
        new QPushButton(QIcon(u":/icons/close.svg"_s), QString(), this);
    deleteButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    layout->addWidget(deleteButton);

    checkButton = new QPushButton(
        QIcon(u":/icons/document_search.svg"_s),
        QString(),
        this
    );
    checkButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    layout->addWidget(checkButton);

    connect(
        editButton,
        &QPushButton::toggled,
        this,
        [this](bool checked) -> void { emit editToggled(checked); }
    );

    connect(deleteButton, &QPushButton::clicked, this, [this] -> void {
        emit deleteRequested();
    });

    connect(checkButton, &QPushButton::clicked, this, [this] -> void {
        emit checkRequested();
    });

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void ActionButtonsCell::setEditable(bool editable) {
    editButton->setChecked(editable);
    editButton->setIcon(QIcon(
        editable ? u":/icons/edit_arrow_up.svg"_s
                 : u":/icons/edit_arrow_down.svg"_s
    ));
}