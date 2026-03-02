#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "FileSelectMenu.hpp"
#include "PersistentMenu.hpp"
#include "Types.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
    class GlossaryMenu;
}  // namespace Ui

QT_END_NAMESPACE

class GlossaryMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit GlossaryMenu(QWidget* parent = nullptr);
    ~GlossaryMenu() override;

    void fill(const Glossary& glossary);
    void addFile(const QString& file);
    [[nodiscard]] auto glossary() const -> Glossary;
    void clear();

   signals:
    void checkRequested(
        Selected selected,
        const std::variant<Glossary, Term>& variant
    );

   protected:
    void changeEvent(QEvent* event) override;

   private:
    inline auto setupUi() -> Ui::GlossaryMenu*;

    auto makeTermInfoCell(const QString& text, const MatchModeInfo& info)
        -> QWidget*;
    auto makeNoteCell(const QString& note) -> QWidget*;
    auto makeActionCell(QTreeWidgetItem* item, u16 row) -> QWidget*;
    void addNewEntry(
        const QString& source = QString(),
        const QString& translation = QString(),
        const QString& note = QString(),
        MatchModeInfo sourceMatchMode =
            MatchModeInfo{
                .fuzzyThreshold = DEFAULT_FUZZY_THRESHOLD,
                .mode = MatchMode::Exact,
                .caseSensitive = false,
                .permissive = false,
            },
        MatchModeInfo translationMatchMode =
            MatchModeInfo{
                .fuzzyThreshold = DEFAULT_FUZZY_THRESHOLD,
                .mode = MatchMode::Exact,
                .caseSensitive = false,
                .permissive = false,
            },
        bool editable = true
    );
    void setRowEditable(QTreeWidgetItem* item, bool editable);

    Ui::GlossaryMenu* const ui;

    QLineEdit* searchInput;
    QPushButton* searchButton;

    QPushButton* addTermButton;
    QPushButton* fileSelectButton;
    QPushButton* qcButton;

    QTreeWidget* glossaryTable;

    FileSelectMenu* fileSelectMenu;

    i32 lastSearchIndex = 0;
};

class TermInfoCell final : public QWidget {
    Q_OBJECT

   public:
    explicit TermInfoCell(QWidget* parent = nullptr);
    void setFrom(const QString& text, const MatchModeInfo& info);
    [[nodiscard]] auto toMatchModeInfo() const -> MatchModeInfo;
    [[nodiscard]] auto text() const -> QString;
    void setEditable(bool editable);

   private:
    QLineEdit* textInput;
    QComboBox* modeSelect;
    QCheckBox* caseSensitive;
    QCheckBox* permissive;
    QLineEdit* fuzzyThresholdInput;
};

class ActionButtonsCell final : public QWidget {
    Q_OBJECT

   public:
    explicit ActionButtonsCell(QWidget* parent = nullptr);
    void setEditable(bool editable);

    QPushButton* editButton;

   signals:
    void editToggled(bool editable);
    void deleteRequested();
    void checkRequested();

   private:
    QPushButton* deleteButton;
    QPushButton* checkButton;
};