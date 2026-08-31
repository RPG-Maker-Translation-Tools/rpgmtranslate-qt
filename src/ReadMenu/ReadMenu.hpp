#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "rpgmtranslate_rs.h"

#include <QDialog>
#include <QEventLoop>
#include <QScreen>
#include <QWidget>

QT_BEGIN_NAMESPACE

namespace Ui {
class ReadMenu;
}  // namespace Ui

QT_END_NAMESPACE

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
    [[nodiscard]] auto flags() const -> BaseFlags;
    [[nodiscard]] auto selected(bool skipped = false) const -> Selected;
    [[nodiscard]] auto title() -> QString;

    void init(const shared_ptr<ProjectSettings>& settings);

    auto exec(const QString& projectPath, EngineType engineType) -> QDialog::DialogCode;

   signals:
    void accepted();
    void rejected();

   protected:
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::ReadMenu*;

    QByteArray projectPath;
    QString decodedTitle;
    ByteBuffer title_{};
    QStringView iniFileName;

    Ui::ReadMenu* const ui;

    FileSelectMenu* const fileSelectMenu;
    shared_ptr<ProjectSettings> projectSettings;

    EngineType engineType;
};
