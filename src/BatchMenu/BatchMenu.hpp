#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Types.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
    class BatchMenu;
}  // namespace Ui

QT_END_NAMESPACE

class BatchMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    enum TrimFlags : u8 {
        Leading = 1,
        Trailing = 2
    };

    explicit BatchMenu(QWidget* parent = nullptr);
    ~BatchMenu() override;

    void clear();
    void addFile(const QString& file);
    void addColumn(const QString& name);
    void renameColumn(u8 index, const QString& name);

   signals:
    void actionRequested(
        Selected selected,
        BatchAction action,
        u8 columnIndex,
        std::variant<TrimFlags, std::tuple<TranslationEndpoint, QString>, u8>
    );

   protected:
    void changeEvent(QEvent* event) override;

   private:
    inline auto setupUi() -> Ui::BatchMenu*;

    Ui::BatchMenu* const ui;

    FileSelectMenu* fileSelectMenu;

    bool dragging = false;
    bool dragSetValue = false;
    QSet<QCheckBox*> dragTouched;
};