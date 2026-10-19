#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Types.hpp"

namespace Ui {
class GlossaryMenu;
}  // namespace Ui

class GlossaryMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit GlossaryMenu(QWidget* parent = nullptr);
    ~GlossaryMenu() override;

    void fill(const Glossary& glossary);
    [[nodiscard]] auto glossary() const -> Glossary;
    void clear();

    void setTermEnabled(u32 index, bool enabled);

   signals:
    // The terms after it moved up: whoever refers to a term by its position must look again
    void termRemoved();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::GlossaryMenu*;

    [[nodiscard]] auto makeTermInfoCell(const QString& text, const MatchModeInfo& info) -> QWidget*;
    [[nodiscard]] auto makeNoteCell(const QString& note) -> QWidget*;
    [[nodiscard]] auto makeActionCell(QTreeWidgetItem* item) -> QWidget*;
    void addNewEntry(
        const QString& source = QString(),
        const QString& translation = QString(),
        const QString& note = QString(),
        MatchModeInfo sourceMatchMode =
            MatchModeInfo{
                .mode = { 
                          .tag = MatchMode::Tag::Exact,
                         },
                .case_sensitive = false,
                .permissive = false,
            },
        MatchModeInfo translationMatchMode =
            MatchModeInfo{
                .mode = {
                    .tag = MatchMode::Tag::Exact},
                .case_sensitive = false,
                .permissive = false,
            },
        bool editable = true,
        bool enabled = true
    );
    void setRowEditable(QTreeWidgetItem* item, bool editable);

    Ui::GlossaryMenu* const ui;

    QLineEdit* searchInput;
    QPushButton* searchButton;

    QPushButton* addTermButton;

    QTreeWidget* glossaryTable;

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

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();

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
    void setEditable(bool editable) const;

    QPushButton* editButton;

   signals:
    void editToggled(bool editable);
    void deleteRequested();

   private:
    QPushButton* deleteButton;
};
