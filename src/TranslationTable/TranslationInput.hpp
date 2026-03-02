#pragma once

#include "Aliases.hpp"

#include <QPlainTextEdit>

class TranslationInput final : public QPlainTextEdit {
    Q_OBJECT

   public:
    explicit TranslationInput(
        const u16* const* hint,
        QWidget* parent = nullptr
    );

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

   signals:
    void contentHeightChanged(i32 height);
    void editingFinished();

   private:
    struct Replacement {
        QLatin1StringView original;
        QStringView replacement;
        i32 position;
    };

    void onTextChanged();
    void updateContentHeight();
    void performAutoReplacements();

    vector<Replacement> lastReplacements;

    const u16* const* const lengthHint;

    i32 lastContentHeight = 0;
    bool blockTextChanged = false;
};
