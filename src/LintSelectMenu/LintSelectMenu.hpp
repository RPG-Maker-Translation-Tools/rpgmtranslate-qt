#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"

class LintSelectMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit LintSelectMenu(QWidget* parent = nullptr);

    void init(Settings* settings, ProjectSettings* projectSettings, GlossaryMenu* glossaryMenu);

    void refresh();

    // Forgets the project: its settings are freed with it. Hides the menu and empties it.
    void clear();

    [[nodiscard]] auto sizeHint() const -> QSize override;

   protected:
    void changeEvent(QEvent* event) override;

   private:
    struct Toggle {
        QString label;
        QString tooltip;
        bool* disabled;
    };

    void addCategory(const QString& title, const vector<Toggle>& toggles);
    void buildSavePicker();
    void buildLints();
    void buildTerms();

    [[nodiscard]] static auto takeToggles(
        span<const QStringView> patterns,
        span<const char* const> tooltips,
        span<SequenceSettings> sequenceSettings
    ) -> vector<Toggle>;

    static void clearLayout(QVBoxLayout* layout);

    [[nodiscard]] static auto entryBoxes(const QGroupBox* group, const QCheckBox* toggleAll) -> QList<QCheckBox*>;

    static void bindToggleAll(QCheckBox* toggleAll, QGroupBox* group);

    Settings* settings = nullptr;
    ProjectSettings* projectSettings = nullptr;
    GlossaryMenu* glossaryMenu = nullptr;

    QTabWidget* const tabs;
    QVBoxLayout* const lintsLayout;
    QVBoxLayout* const termsLayout;
};
