#include "LintSelectMenu.hpp"

#include "GlossaryMenu.hpp"
#include "PluginSequences.hpp"
#include "Settings.hpp"
#include "Types.hpp"

#include <QCheckBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
constexpr i32 MARGIN = 8;
constexpr i32 SPACING = 4;
constexpr i32 DEFAULT_WIDTH = 520;
constexpr i32 DEFAULT_HEIGHT = 480;

[[nodiscard]] auto makeScrollArea(QWidget* const content, QWidget* const parent) -> QScrollArea* {
    auto* const area = new QScrollArea(parent);
    area->setWidgetResizable(true);
    area->setWidget(content);
    return area;
}
}  // namespace

LintSelectMenu::LintSelectMenu(QWidget* const parent) :
    PersistentMenu(parent),
    tabs(new QTabWidget(this)),
    lintsLayout(new QVBoxLayout()),
    termsLayout(new QVBoxLayout()) {
    setDragMoveEnabled(true);

    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    layout->addWidget(tabs);

    auto* const lintsContent = new QWidget(tabs);
    lintsContent->setLayout(lintsLayout);
    lintsLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    lintsLayout->setSpacing(SPACING);
    lintsLayout->addStretch(1);

    auto* const termsContent = new QWidget(tabs);
    termsContent->setLayout(termsLayout);
    termsLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    termsLayout->setSpacing(SPACING);
    termsLayout->addStretch(1);

    tabs->addTab(makeScrollArea(lintsContent, tabs), tr("Lints"));
    tabs->addTab(makeScrollArea(termsContent, tabs), tr("Terms"));

    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

auto LintSelectMenu::sizeHint() const -> QSize {
    return { DEFAULT_WIDTH, DEFAULT_HEIGHT };
}

void LintSelectMenu::init(Settings* const settings, GlossaryMenu* const glossaryMenu) {
    this->settings = settings;
    this->glossaryMenu = glossaryMenu;
}

void LintSelectMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        tabs->setTabText(0, tr("Lints"));
        tabs->setTabText(1, tr("Terms"));
        refresh();
    }

    PersistentMenu::changeEvent(event);
}

void LintSelectMenu::clearLayout(QVBoxLayout* const layout) {
    while (layout->count() > 1) {
        const auto* const item = layout->takeAt(0);
        delete item->widget();
        delete item;
    }
}

auto LintSelectMenu::entryBoxes(const QGroupBox* const group, const QCheckBox* const toggleAll) -> QList<QCheckBox*> {
    QList<QCheckBox*> boxes = group->findChildren<QCheckBox*>();
    boxes.removeOne(ccast<QCheckBox*>(toggleAll));
    return boxes;
}

void LintSelectMenu::bindToggleAll(QCheckBox* const toggleAll, QGroupBox* const group) {
    const auto syncToggleAll = [toggleAll, group] -> void {
        const QList<QCheckBox*> boxes = entryBoxes(group, toggleAll);

        const auto checked =
            ranges::count_if(boxes, [](const QCheckBox* const box) -> bool { return box->isChecked(); });

        Qt::CheckState state = Qt::PartiallyChecked;

        if (checked == 0) {
            state = Qt::Unchecked;
        } else if (checked == boxes.size()) {
            state = Qt::Checked;
        }

        const QSignalBlocker blocker(toggleAll);
        toggleAll->setCheckState(state);
    };

    connect(toggleAll, &QCheckBox::clicked, group, [toggleAll, group](const bool checked) -> void {
        const bool enable = toggleAll->checkState() == Qt::PartiallyChecked || checked;

        for (QCheckBox* const box : entryBoxes(group, toggleAll)) {
            box->setChecked(enable);
        }

        const QSignalBlocker blocker(toggleAll);
        toggleAll->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
    });

    for (const QCheckBox* const box : entryBoxes(group, toggleAll)) {
        connect(box, &QCheckBox::toggled, group, syncToggleAll);
    }

    syncToggleAll();
}

void LintSelectMenu::addCategory(const QString& title, const vector<Toggle>& toggles) {
    auto* const group = new QGroupBox(title, this);
    auto* const groupLayout = new QVBoxLayout(group);
    groupLayout->setSpacing(SPACING);

    lintsLayout->insertWidget(lintsLayout->count() - 1, group);

    if (toggles.empty()) {
        auto* const placeholder = new QLabel(tr("No lints in this category."), group);
        placeholder->setEnabled(false);
        groupLayout->addWidget(placeholder);
        return;
    }

    auto* const toggleAll = new QCheckBox(tr("All"), group);
    toggleAll->setTristate(true);
    groupLayout->addWidget(toggleAll);

    for (const auto& [label, tooltip, disabled] : toggles) {
        auto* const box = new QCheckBox(label, group);
        box->setChecked(!*disabled);
        box->setToolTip(tooltip);

        connect(box, &QCheckBox::toggled, this, [this, disabled](const bool checked) -> void {
            *disabled = !checked;
            emit lintsToggled();
        });

        groupLayout->addWidget(box);
    }

    bindToggleAll(toggleAll, group);
}

auto LintSelectMenu::takeToggles(
    const span<const QStringView> patterns,
    const span<const char* const> tooltips,
    const span<SequenceSettings> sequenceSettings
) -> vector<Toggle> {
    vector<Toggle> toggles;
    toggles.reserve(patterns.size());

    for (const auto [idx, pattern] : views::enumerate(patterns)) {
        toggles.emplace_back(pattern.toString(), tr(tooltips[idx]), &sequenceSettings[idx].disabled);
    }

    return toggles;
}

void LintSelectMenu::buildLints() {
    clearLayout(lintsLayout);

    auto& translation = settings->translation;

    addCategory(tr("Text Codes"), takeToggles(TextCodes::PATTERN_PARTS, TextCodes::TOOLTIPS, translation.textCodes));
    addCategory(tr("Note Tags"), takeToggles(Notetags::PATTERN_PARTS, Notetags::TOOLTIPS, translation.notetags));
    addCategory(
        tr("Plugin Commands"),
        takeToggles(PluginCommands::PATTERN_PARTS, PluginCommands::TOOLTIPS, translation.pluginCommands)
    );
    addCategory(
        tr("Comment Tags"),
        takeToggles(CommentTags::PATTERN_PARTS, CommentTags::TOOLTIPS, translation.commentTags)
    );

    vector<Toggle> customToggles;
    customToggles.reserve(translation.customLints.size());

    for (auto& customLint : translation.customLints) {
        customToggles.emplace_back(customLint.sequence, customLint.tooltip, &customLint.disabled);
    }

    addCategory(tr("Custom Lints"), customToggles);
}

void LintSelectMenu::buildTerms() {
    clearLayout(termsLayout);

    const Glossary glossary = glossaryMenu->glossary();

    if (glossary.empty()) {
        auto* const placeholder = new QLabel(tr("The glossary is empty."), this);
        placeholder->setEnabled(false);
        termsLayout->insertWidget(0, placeholder);
        return;
    }

    auto* const group = new QGroupBox(tr("Terms"), this);
    auto* const groupLayout = new QVBoxLayout(group);
    groupLayout->setSpacing(SPACING);

    termsLayout->insertWidget(termsLayout->count() - 1, group);

    auto* const toggleAll = new QCheckBox(tr("All"), group);
    toggleAll->setTristate(true);
    groupLayout->addWidget(toggleAll);

    for (const auto& [idx, term] : views::enumerate(glossary)) {
        auto* const box = new QCheckBox(u"%1 - %2"_qsv.arg(term.term, term.translation), group);
        box->setChecked(term.enabled);
        box->setToolTip(term.note);

        connect(box, &QCheckBox::toggled, this, [this, idx](const bool checked) -> void {
            glossaryMenu->setTermEnabled(scast<u32>(idx), checked);
            emit lintsToggled();
        });

        groupLayout->addWidget(box);
    }

    bindToggleAll(toggleAll, group);
}

void LintSelectMenu::refresh() {
    if (settings == nullptr || glossaryMenu == nullptr) {
        return;
    }

    buildLints();
    buildTerms();
}
