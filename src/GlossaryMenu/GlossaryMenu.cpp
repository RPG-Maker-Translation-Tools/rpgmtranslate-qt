#include "GlossaryMenu.hpp"

#include "Constants.hpp"
#include "Notice.hpp"
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

namespace {
constexpr i32 SOURCE_IDX = 0;
constexpr i32 TRANSLATION_IDX = 1;
constexpr i32 NOTE_IDX = 2;
constexpr i32 ACTIONS_IDX = 3;
constexpr i32 ACTIONS_COL_WIDTH = 120;

constexpr i32 ENABLED_ROLE = Qt::UserRole;

constexpr QMargins CELL_MARGINS = { 8, 8, 8, 8 };
constexpr i32 ROW_MARGIN = 4;
constexpr i32 ROW_SPACING = 8;
constexpr i32 BUTTON_SIZE = 32;

constexpr f32 FUZZY_MIN = 0.0F;
constexpr f32 FUZZY_MAX = 1.0F;
constexpr i32 FUZZY_DECIMALS = 3;
}  // namespace

GlossaryMenu::GlossaryMenu(QWidget* const parent) :
    PersistentMenu(parent),
    ui(setupUi()),
    searchInput(ui->searchInput),
    searchButton(ui->searchButton),
    addTermButton(ui->addTermButton),
    glossaryTable(ui->glossaryTable) {
    setDragMoveEnabled(true);

    glossaryTable->setUniformRowHeights(true);
    auto* const header = glossaryTable->header();
    header->setSectionResizeMode(SOURCE_IDX, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(TRANSLATION_IDX, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(NOTE_IDX, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ACTIONS_IDX, QHeaderView::Fixed);
    header->resizeSection(ACTIONS_IDX, ACTIONS_COL_WIDTH);

    connect(addTermButton, &QPushButton::pressed, this, [this] -> void { addNewEntry(); });

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

        for (i32 idx = 0; idx < itemCount; idx++) {
            const i32 currentIndex = (startIndex + idx) % itemCount;
            auto* const item = glossaryTable->topLevelItem(currentIndex);

            const auto* const sourceCell = scast<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
            const auto* const translationCell = scast<TermInfoCell*>(glossaryTable->itemWidget(item, TRANSLATION_IDX));

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
        present(this, NOTICE("No match found for: %1", Info, Status, searchText));
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

auto GlossaryMenu::makeTermInfoCell(const QString& text, const MatchModeInfo& info) -> QWidget* {
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

auto GlossaryMenu::makeActionCell(QTreeWidgetItem* const item, const i32 index) -> QWidget* {
    auto* const actionsCell = new ActionButtonsCell(glossaryTable);

    connect(
        actionsCell,
        &ActionButtonsCell::editToggled,
        this,
        [this, actionsCell, item, index](const bool editable) -> void {
        if (!editable) {
            auto* const rowItem = glossaryTable->itemFromIndex(glossaryTable->model()->index(index, 0));

            auto* const sourceInput = glossaryTable->itemWidget(rowItem, 0)->findChild<QLineEdit*>();
            auto* const translationInput = glossaryTable->itemWidget(rowItem, 1)->findChild<QLineEdit*>();

            if (sourceInput->text().isEmpty()) {
                present(this, NOTICE("Empty term is not allowed.", Warning, Modal));
                actionsCell->editButton->setChecked(true);
                return;
            }

            if (translationInput->text().isEmpty()) {
                present(this, NOTICE("Empty term translation is not allowed.", Warning, Modal));
                actionsCell->editButton->setChecked(true);
                return;
            }
        }

        actionsCell->setEditable(editable);
        setRowEditable(item, editable);
    }
    );

    connect(actionsCell, &ActionButtonsCell::deleteRequested, this, [this, item] -> void {
        const auto pressed =
            QMessageBox::question(nullptr, tr("Confirm Delete"), tr("Are you sure you want to delete this entry?"));

        if (pressed == QMessageBox::Yes) {
            qInfo().noquote() << u"Glossary entry deletion was confirmed by user."_qsv;

            const i32 index = glossaryTable->indexOfTopLevelItem(item);
            delete glossaryTable->takeTopLevelItem(index);
            adjustSize();
        }
    });

    return actionsCell;
}

void GlossaryMenu::setRowEditable(QTreeWidgetItem* const item, bool editable) {
    auto* const sourceCell = scast<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
    auto* const translationCell = scast<TermInfoCell*>(glossaryTable->itemWidget(item, TRANSLATION_IDX));
    auto* const noteInput = glossaryTable->itemWidget(item, NOTE_IDX)->findChild<QPlainTextEdit*>();

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
    const bool editable,
    const bool enabled
) {
    const u16 row = glossaryTable->model()->rowCount();

    auto* const item = new QTreeWidgetItem(glossaryTable);
    item->setData(SOURCE_IDX, ENABLED_ROLE, enabled);

    auto* const sourceCell = makeTermInfoCell(source, sourceMatchMode);
    glossaryTable->setItemWidget(item, SOURCE_IDX, sourceCell);
    glossaryTable->setItemWidget(item, TRANSLATION_IDX, makeTermInfoCell(translation, translationMatchMode));
    auto* const noteCell = makeNoteCell(note);
    glossaryTable->setItemWidget(item, NOTE_IDX, noteCell);
    glossaryTable->setItemWidget(item, ACTIONS_IDX, makeActionCell(item, row));

    item->setTextAlignment(SOURCE_IDX, Qt::AlignTop | Qt::AlignHCenter);
    item->setTextAlignment(TRANSLATION_IDX, Qt::AlignTop | Qt::AlignHCenter);
    item->setTextAlignment(NOTE_IDX, Qt::AlignTop | Qt::AlignHCenter);
    item->setTextAlignment(ACTIONS_IDX, Qt::AlignCenter);

    setRowEditable(item, editable);
    scast<ActionButtonsCell*>(glossaryTable->itemWidget(item, ACTIONS_IDX))->setEditable(editable);

    noteCell->setFixedHeight(sourceCell->sizeHint().height());
    glossaryTable->addTopLevelItem(item);

    resize(sizeHint());
}

void GlossaryMenu::fill(const Glossary& glossary) {
    for (const auto& term : glossary) {
        addNewEntry(
            term.term,
            term.translation,
            term.note,
            term.sourceMatchMode,
            term.translationMatchMode,
            false,
            term.enabled
        );
    }
}

void GlossaryMenu::setTermEnabled(const u32 index, const bool enabled) {
    if (index >= scast<u32>(glossaryTable->topLevelItemCount())) {
        return;
    }

    glossaryTable->topLevelItem(scast<i32>(index))->setData(SOURCE_IDX, ENABLED_ROLE, enabled);
}

auto GlossaryMenu::glossary() const -> Glossary {
    Glossary out;

    const i32 entryCount = glossaryTable->topLevelItemCount();
    out.reserve(entryCount);

    for (i32 idx = 0; idx < entryCount; idx++) {
        auto* const item = glossaryTable->topLevelItem(idx);

        const auto* const sourceCell = scast<TermInfoCell*>(glossaryTable->itemWidget(item, SOURCE_IDX));
        const auto* const translationCell = scast<TermInfoCell*>(glossaryTable->itemWidget(item, TRANSLATION_IDX));
        const auto* const noteInput = glossaryTable->itemWidget(item, NOTE_IDX)->findChild<QPlainTextEdit*>();

        out.emplace_back(
            sourceCell->text(),
            translationCell->text(),
            noteInput->toPlainText(),
            sourceCell->toMatchModeInfo(),
            translationCell->toMatchModeInfo(),
            item->data(SOURCE_IDX, ENABLED_ROLE).toBool()
        );
    }

    return out;
}

void GlossaryMenu::clear() {
    glossaryTable->clear();
    searchInput->clear();
    lastSearchIndex = 0;
}

[[nodiscard]] auto TermInfoCell::toMatchModeInfo() const -> MatchModeInfo {
    const QString fuzzyText = fuzzyThresholdInput->text().trimmed();
    const f32 fuzzyThreshold = fuzzyText.isEmpty() ? DEFAULT_FUZZY_THRESHOLD : stoa<f32>(fuzzyText);

    const auto tagMode = MatchMode::Tag(modeSelect->currentIndex());

    MatchMode mode{ .tag = MatchMode::Tag::Exact };

    if (tagMode == MatchMode::Tag::Fuzzy) {
        mode = MatchMode{ .fuzzy = { .tag = MatchMode::Tag::Exact, .threshold = fuzzyThreshold } };
    } else if (tagMode != MatchMode::Tag::Exact) {
        mode = MatchMode{ .both = { .tag = MatchMode::Tag::Exact, .threshold = fuzzyThreshold } };
    }

    return MatchModeInfo{
        .mode = mode,
        .case_sensitive = caseSensitive->isChecked(),
        .permissive = permissive->isChecked(),
    };
}

[[nodiscard]] auto TermInfoCell::text() const -> QString {
    return textInput->text();
}

void TermInfoCell::setFrom(const QString& text, const MatchModeInfo& info) {
    textInput->setText(text);

    modeSelect->setCurrentIndex(scast<u8>(info.mode.tag));
    caseSensitive->setChecked(info.case_sensitive);
    permissive->setChecked(info.permissive);

    const f64 threshold =
        (info.mode.tag == MatchMode::Tag::Exact) ? DEFAULT_FUZZY_THRESHOLD : info.mode.fuzzy.threshold;
    fuzzyThresholdInput->setText(QString(ftos(threshold).qsv()));

    const bool show = info.mode.tag != MatchMode::Tag::Exact;
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
    rowLayout->setContentsMargins(ROW_MARGIN, ROW_MARGIN, ROW_MARGIN, ROW_MARGIN);
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
    fuzzyThresholdInput->setValidator(new QDoubleValidator(FUZZY_MIN, FUZZY_MAX, FUZZY_DECIMALS, fuzzyThresholdInput));
    rowLayout->addWidget(fuzzyThresholdInput);

    rowLayout->addStretch(1);
    layout->addWidget(row);

    const auto updateThresholdVisibility = [this] -> void {
        const auto mode = MatchMode::Tag(modeSelect->currentIndex());
        const bool show = (mode == MatchMode::Tag::Fuzzy || mode == MatchMode::Tag::Both);
        fuzzyThresholdInput->setVisible(show);

        if (show && fuzzyThresholdInput->text().trimmed().isEmpty()) {
            // DEFAULT_FUZZY_THRESHOLD
            fuzzyThresholdInput->setText(u"0.8"_s);
        }
    };

    connect(modeSelect, &QComboBox::currentIndexChanged, this, updateThresholdVisibility);

    updateThresholdVisibility();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

ActionButtonsCell::ActionButtonsCell(QWidget* const parent) : QWidget(parent) {
    auto* const layout = new QHBoxLayout(this);
    layout->setContentsMargins(CELL_MARGINS);
    layout->setSpacing(ROW_SPACING);
    layout->setAlignment(Qt::AlignCenter);

    editButton = new QPushButton(QIcon(u":/icons/edit_arrow_down.svg"_s), QString(), this);
    editButton->setCheckable(true);
    editButton->setChecked(false);
    editButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    layout->addWidget(editButton);

    deleteButton = new QPushButton(QIcon(u":/icons/close.svg"_s), QString(), this);
    deleteButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    layout->addWidget(deleteButton);

    connect(editButton, &QPushButton::toggled, this, [this](bool checked) -> void { emit editToggled(checked); });

    connect(deleteButton, &QPushButton::clicked, this, [this] -> void { emit deleteRequested(); });

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void ActionButtonsCell::setEditable(bool editable) const {
    editButton->setChecked(editable);
    editButton->setIcon(QIcon(editable ? u":/icons/edit_arrow_up.svg"_s : u":/icons/edit_arrow_down.svg"_s));
}
