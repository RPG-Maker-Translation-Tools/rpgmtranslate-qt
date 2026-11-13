#pragma once

#include <QCoreApplication>
#include <QEvent>
#include <QString>
#include <QTranslator>
#include <QWidget>

// Puts `MARK` before every text that is translated while it lives, and is installed on construction. Installing sends
// `LanguageChange` to the visible windows only, so `retranslate` tells the widget under test, and one that handles the
// event shows the mark where one that does not keeps its text.
class MarkingTranslator final : public QTranslator {
   public:
    static constexpr QChar MARK = u'~';

    MarkingTranslator() { QCoreApplication::installTranslator(this); }
    ~MarkingTranslator() override { QCoreApplication::removeTranslator(this); }

    MarkingTranslator(const MarkingTranslator&) = delete;
    auto operator=(const MarkingTranslator&) -> MarkingTranslator& = delete;

    [[nodiscard]] auto translate(const char*, const char* source, const char*, int) const -> QString override {
        return MARK + QString::fromUtf8(source);
    }

    static void retranslate(QWidget* const widget) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(widget, &event);
    }

    [[nodiscard]] auto isEmpty() const -> bool override { return false; }
};
