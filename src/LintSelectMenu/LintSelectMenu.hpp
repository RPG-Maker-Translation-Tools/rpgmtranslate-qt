#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"

class LintSelectMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit LintSelectMenu(QWidget* parent = nullptr);

    void init(Settings* settings, GlossaryMenu* glossaryMenu);

    void refresh();

    [[nodiscard]] auto sizeHint() const -> QSize override;

   signals:
    void lintsToggled();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    struct Toggle {
        QString label;
        QString tooltip;
        bool* disabled;
    };

    void addCategory(const QString& title, const vector<Toggle>& toggles);
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
    GlossaryMenu* glossaryMenu = nullptr;

    QTabWidget* const tabs;
    QVBoxLayout* const lintsLayout;
    QVBoxLayout* const termsLayout;
};
