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

    // Replaces the whole text as one undoable edit, without triggering auto-replacements.
    void replaceText(const QString& text);

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

   signals:
    void contentHeightChanged(u32 height);
    void focusNextInput();

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

    // Created when the first tooltip is shown, most cells are edited without one
    unique_ptr<LintTooltip> tooltip;
    // The lints under the mouse, and those of the tooltip shown with the range of text they are of
    QList<LintEntry> pendingLints;
    QList<LintEntry> shownLints;

    vector<Replacement> lastReplacements;

    QTimer tooltipDelayTimer;
    QPoint pendingCursorPos;

    const ProjectSettings* const projectSettings;
    const Settings* const settings;

    u32 lastContentHeight = 0;

    i32 hoveredRangeStart = -1;
    i32 hoveredRangeEnd = -1;
    i32 shownRangeStart = -1;
    i32 shownRangeEnd = -1;

    // Characters added by the last change of the document, 0 for a deletion
    i32 lastCharsAdded = 0;

    bool blockTextChanged = false;
};