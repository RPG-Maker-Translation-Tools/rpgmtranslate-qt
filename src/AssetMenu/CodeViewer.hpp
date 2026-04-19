#pragma once

#include "FWD.hpp"
#include "rpgmtranslate.h"

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QWidget>

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || \
    defined(ENABLE_RUBY_HIGHLIGHTING)
class TreeSitterHighlighter final : public QSyntaxHighlighter {
   public:
    using QSyntaxHighlighter::QSyntaxHighlighter;
    ~TreeSitterHighlighter() override;

    void setHighlights(span<const HighlightToken> newHighlights);

   protected:
    void highlightBlock(const QString& text) override;

   private:
    [[nodiscard]] static auto highlightColor(HighlightType type) -> QColor;

    span<const HighlightToken> highlights;
};
#endif

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

    [[nodiscard]] auto lineNumberAreaWidth() const -> i32;
    void lineNumberAreaPaintEvent(QPaintEvent* event);

   protected:
    void resizeEvent(QResizeEvent* event) override;

   private:
    void updateLineNumberAreaWidth(i32 /* newBlockCount */);
    void updateLineNumberArea(const QRect& rect, i32 yPos);

    LineNumberArea* lineNumberArea;
};
