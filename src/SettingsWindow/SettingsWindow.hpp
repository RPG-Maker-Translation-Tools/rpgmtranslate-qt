#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Settings.hpp"
#include "rpgmtranslate_rs.h"

#include <QDialog>
#include <QNetworkAccessManager>

namespace Ui {
class SettingsWindow;
}  // namespace Ui

class SettingsWindow final : public QDialog {
    Q_OBJECT

   public:
    // Every setting is transferred by one `sync` call that handles both directions, so a setting can't be
    // half-wired: reading it without writing it back is not expressible.
    enum class Direction : u8 {
        Load,
        Save
    };

    explicit SettingsWindow(
        const shared_ptr<Settings>& settings,
        const shared_ptr<ProjectSettings>& projectSettings,
        const QStringList& tabs,
        QWidget* parent
    );
    ~SettingsWindow() override;

   protected:
    void changeEvent(QEvent* event) override;
    void done(i32 result) override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::SettingsWindow*;

    void syncAll(Direction direction);
    void syncEndpoint(Direction direction, EndpointSettings& endpoint);
    void syncSequenceTable(Direction direction, QTableWidget* table, span<SequenceSettings> sequences);
    void syncCustomLints(Direction direction);
    void syncReplacements(Direction direction);
    void syncFileContext(Direction direction, const QModelIndex& index);

    [[nodiscard]] auto validate() -> bool;

    void buildSequenceTables();
    void addCustomLintRow(const CustomLint& lint);
    void exportCustomLints();
    void importCustomLints();

    inline void refreshSpellcheckDictionarySelect();
    inline void setDefaultBaseURL(TranslationEndpoint endpoint);
    inline void checkKey();
    inline void checkLanguageToolConnection();

    inline void onTypeChange(TranslationEndpoint endpoint);

    shared_ptr<Settings> settings;
    shared_ptr<ProjectSettings> projectSettings;

    Ui::SettingsWindow* const ui;
    QNetworkAccessManager languageToolNetworkManager;
};
