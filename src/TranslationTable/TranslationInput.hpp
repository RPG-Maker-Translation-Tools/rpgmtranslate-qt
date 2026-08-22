#pragma once

#include "Aliases.hpp"
#include "LintTooltip.hpp"
#include "TranslationHighlighter.hpp"
#include "Types.hpp"

#include <QPlainTextEdit>
#include <QTimer>

class TranslationInput final : public QPlainTextEdit {
    Q_OBJECT

   public:
    explicit TranslationInput(
        const ProjectSettings* projectSettings,
        const Settings* settings,
        QWidget* parent = nullptr
    );

    TranslationHighlighter* highlighter;

    void applyLintFormats(vector<LintCharState> charStates, i32 size);

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
        QString original;
        QString replacement;
        i32 position;
    };

    void onTextChanged();
    void updateContentHeight();
    void performAutoReplacements();
    void showPendingTooltip();
    void applySuggestion(LintType type, i32 lintIndex, i32 suggestionIndex);

    LintTooltip tooltip;
    QList<LintEntry> pendingLints;

    vector<Replacement> lastReplacements;

    QTimer tooltipDelayTimer;
    QPoint pendingCursorPos;

    const ProjectSettings* const projectSettings;
    const Settings* const settings;

    u32 lastContentHeight = 0;

    i32 hoveredRangeStart = -1;
    i32 hoveredRangeEnd = -1;

    bool blockTextChanged = false;
};