#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QWidget>

class LintTooltip final : public QWidget {
    Q_OBJECT

   public:
    explicit LintTooltip(QWidget* parent = nullptr);

    void showAt(
        const QPoint& globalPos,
        const QString& text,
        const QString& captured,
        const ProjectSettings* projectSettings
    );
    void hide();

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    QString tooltipText;
};