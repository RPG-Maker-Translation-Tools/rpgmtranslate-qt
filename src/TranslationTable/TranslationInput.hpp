#pragma once

#include "Aliases.hpp"
#include "LintTooltip.hpp"

#include <QPlainTextEdit>
#include <QTimer>

class TranslationInput final : public QPlainTextEdit {
    Q_OBJECT

   public:
    explicit TranslationInput(
        const ProjectSettings* projectSettings,
        QWidget* parent = nullptr
    );

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

   signals:
    void contentHeightChanged(u32 height);
    void editingFinished();

   private:
    struct Replacement {
        QL1SV original;
        QStringView replacement;
        i32 position;
    };

    void onTextChanged();
    void updateContentHeight();
    void performAutoReplacements();
    void showPendingTooltip();

    LintTooltip tooltip;

    QString pendingTooltipText;
    QString pendingTooltipCaptured;

    vector<Replacement> lastReplacements;

    QTimer tooltipDelayTimer;
    QPoint pendingTooltipPos;

    const ProjectSettings* projectSettings;

    u32 lastContentHeight = 0;

    i32 hoveredRangeStart = -1;
    i32 hoveredRangeEnd = -1;

    bool blockTextChanged = false;
};