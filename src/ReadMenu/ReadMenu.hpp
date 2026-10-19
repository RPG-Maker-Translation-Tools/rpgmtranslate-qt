#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "rpgmtranslate_rs.h"

#include <QDialog>

namespace Ui {
class ReadMenu;
}  // namespace Ui

class ReadMenu final : public QWidget {
    Q_OBJECT

   public:
    explicit ReadMenu(QWidget* parent = nullptr);
    ~ReadMenu() override;

    void clear();
    void init(const vector<TabListItem>& files);

    [[nodiscard]] auto parseMapEvents() const -> bool;
    [[nodiscard]] auto readMode() const -> ReadMode;
    [[nodiscard]] auto duplicateMode() const -> DuplicateMode;
    [[nodiscard]] auto readEncoding() const -> QString;
    [[nodiscard]] auto archiveReadEncoding() const -> QString;
    [[nodiscard]] auto flags() const -> BaseFlags;
    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;
    [[nodiscard]] auto title() const -> QString;

    void init(const ProjectSettings* settings);

    auto exec(const QString& projectPath, EngineType engineType, bool hasArchive) -> QDialog::DialogCode;

   signals:
    void accepted();
    void rejected();

   protected:
    auto event(QEvent* event) -> bool override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::ReadMenu*;
    void fitHeight();
    void updateModeLabels();
    void updateEngineWidgets();

    QByteArray projectPath;
    ByteBuffer title_{};
    QStringView iniFileName;

    const ProjectSettings* projectSettings = nullptr;

    Ui::ReadMenu* const ui;

    FileSelectMenu* const fileSelectMenu;

    EngineType engineType;
};
