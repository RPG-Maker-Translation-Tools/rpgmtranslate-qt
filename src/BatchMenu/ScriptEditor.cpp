#include "ScriptEditor.hpp"

#include "CodeViewer.hpp"
#include "Utils.hpp"

#include <QDialogButtonBox>
#include <QVBoxLayout>

#ifdef ENABLE_JS_HIGHLIGHTING
#include "rpgmtranslate_rs.h"

#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTimer>
#endif

namespace {
constexpr i32 EDITOR_FONT_POINT_SIZE = 10;
constexpr QSize EDITOR_SIZE = { 900, 700 };

#ifdef ENABLE_JS_HIGHLIGHTING
constexpr i32 REHIGHLIGHT_DELAY_MS = 250;

class ScriptHighlighter final : public QSyntaxHighlighter {
   public:
    explicit ScriptHighlighter(QTextDocument* const document) : QSyntaxHighlighter(document) {}

    void refresh(const QString& source) {
        if (source == highlighted) {
            return;
        }

        const QByteArray utf8 = source.toUtf8();
        FFIString html;

        if (!rpgm_highlight_code(strtoffi(utf8), HighlightLanguage::JS, &html)) {
            return;
        }

        reference.setHtml(QString::fromUtf8(html.ptr, scast<isize>(html.len)));
        rpgm_string_free(html);

        highlighted = source;
        rehighlight();
    }

   protected:
    void highlightBlock(const QString& /* text */) override {
        const QTextBlock block = reference.findBlockByNumber(currentBlock().blockNumber());

        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat source = fragment.charFormat();

            auto format = QTextCharFormat();
            format.setForeground(source.foreground());
            format.setFontWeight(source.fontWeight());
            format.setFontItalic(source.fontItalic());

            setFormat(fragment.position() - block.position(), fragment.length(), format);
        }
    }

   private:
    QTextDocument reference;
    QString highlighted;
};
#endif
}  // namespace

ScriptEditor::ScriptEditor(QWidget* const parent, const QString& name, const QString& source, const bool readOnly) :
    QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint),
    editor(new CodeViewer(this)) {
    setWindowTitle(name);
    resize(EDITOR_SIZE);

    auto monoFont = QFont();
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(EDITOR_FONT_POINT_SIZE);

    editor->setFont(monoFont);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    editor->setPlainText(source);
    editor->setReadOnly(readOnly);

    auto* const buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(
        readOnly ? QDialogButtonBox::StandardButtons(QDialogButtonBox::Close)
                 : QDialogButtonBox::Save | QDialogButtonBox::Cancel
    );

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* const layout = new QVBoxLayout(this);
    layout->addWidget(editor);
    layout->addWidget(buttons);

#ifdef ENABLE_JS_HIGHLIGHTING
    auto* const highlighter = new ScriptHighlighter(editor->document());
    highlighter->refresh(source);

    // Highlighting rewrites the formats, which can raise `textChanged` again: `refresh` ignores an unchanged text
    auto* const timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(REHIGHLIGHT_DELAY_MS);

    connect(timer, &QTimer::timeout, this, [this, highlighter] -> void {
        highlighter->refresh(editor->toPlainText());
    });
    connect(editor, &QPlainTextEdit::textChanged, this, [timer] -> void { timer->start(); });
#endif
}

auto ScriptEditor::source() const -> QString {
    return editor->toPlainText();
}
