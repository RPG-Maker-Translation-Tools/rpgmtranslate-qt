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

    explicit SettingsWindow(Settings* settings, ProjectSettings* projectSettings, QWidget* parent);
    ~SettingsWindow() override;

   protected:
    void changeEvent(QEvent* event) override;
    void done(i32 result) override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::SettingsWindow*;

    void retranslateDynamic();

    void syncAll(Direction direction);
    void syncEndpoint(Direction direction, EndpointSettings& endpoint);
    void syncSequenceTable(Direction direction, QTableWidget* table, span<SequenceSettings> sequences);
    void syncCustomLints(Direction direction);
    void syncReplacements(Direction direction);

    [[nodiscard]] auto validate() -> bool;

    void setEndpointFormEnabled(bool enabled);
    void buildSequenceTables();
    void addCustomLintRow(const CustomLint& lint);
    void exportCustomLints();
    void importCustomLints();

    inline void refreshSpellcheckDictionarySelect();
    inline void checkKey();
    inline void checkLanguageToolConnection();

    inline void onTypeChange(TranslationEndpoint endpoint);

    Settings* settings;
    ProjectSettings* projectSettings;

    // What Cancel restores: the dialog edits `settings` directly (endpoints are added and removed in place).
    Settings initialSettings;
    // Their credentials are deleted only when the changes are saved.
    vector<QString> removedEndpointIds;

    // The family the hint font select showed when it was loaded with no hint font set: while it stays selected, the
    // setting stays empty ("the editor font") instead of becoming that family.
    QString defaultHintFont;

    Ui::SettingsWindow* const ui;
    QNetworkAccessManager languageToolNetworkManager;
};
