#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QPlainTextEdit>
#include <QWidget>

class LineNumberArea final : public QWidget {
   public:
    explicit LineNumberArea(CodeViewer* editor);

    [[nodiscard]] auto sizeHint() const -> QSize override;

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    CodeViewer* editor;
};

class CodeViewer final : public QPlainTextEdit {
    Q_OBJECT

   public:
    explicit CodeViewer(QWidget* parent = nullptr);

    // Replaces the document with `html` (from `rpgm_highlight_code`, already fully colored) -
    // there's no separate highlighter to manage, lumis does the highlighting on the Rust side.
    void setHighlightedHtml(const QString& html);

    [[nodiscard]] auto lineNumberAreaWidth() const -> i32;
    void lineNumberAreaPaintEvent(QPaintEvent* event);

   protected:
    void resizeEvent(QResizeEvent* event) override;

   private:
    void updateLineNumberAreaWidth(i32 /* newBlockCount */);
    void updateLineNumberArea(const QRect& rect, i32 yPos);

    LineNumberArea* lineNumberArea;
};
