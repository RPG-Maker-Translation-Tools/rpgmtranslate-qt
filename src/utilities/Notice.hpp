#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QAnyStringView>
#include <QStringList>
#include <initializer_list>
#include <source_location>

enum class Severity : u8 {
    Debug,
    Info,
    Warning,
    Critical
};

enum class Delivery : u8 {
    // Transient status bar message.
    Status,
    // The call site renders it in its own surface (e.g. AssetPreviewWidget's error page); present() ignores these.
    Inline,
    // Modal message box.
    Modal,
};

// A user-facing message that logs itself, in English, at construction. Presentation is a separate, optional step.
// Diagnostics the user never sees use qWarning()/qCritical() instead.
//
// `source` is an untranslated English literal with %1..%99 placeholders, filled by `args`. It is also the translation
// catalog key.
class Notice final {
   public:
    // Args are copied, so callers may pass temporaries.
    Notice(
        const char* context,
        const char* source,
        Severity severity,
        Delivery delivery,
        std::initializer_list<QAnyStringView> args = {},
        // -1: no numerus. Otherwise substituted for %n and used for Qt's plural lookup.
        i32 n = -1,
        std::source_location location = std::source_location::current()
    );

    [[nodiscard]] auto translated() const -> QString;
    [[nodiscard]] auto english() const -> QString;

    [[nodiscard]] auto severity() const -> Severity { return severity_; }

    [[nodiscard]] auto delivery() const -> Delivery { return delivery_; }

   private:
    const char* context_;
    const char* source_;
    QStringList args_;
    Severity severity_;
    Delivery delivery_;
    i32 n_;
};

// Shows `notice` according to its delivery. Inline notices are a no-op here - they are rendered by whoever created
// them. A Status notice falls back to a modal when `parent` has no status bar to put it in.
void present(QWidget* parent, const Notice& notice);

// For use inside a Q_OBJECT class. The literal must come first, since lupdate reads it via the `QT_TR_NOOP+=NOTICE`
// alias in CMakeLists.txt.
#define NOTICE(text, severity, delivery, ...) \
    Notice(staticMetaObject.className(), text, Severity::severity, Delivery::delivery, { __VA_ARGS__ })

// Count-dependent variant: `text` holds one %n, which selects the singular/plural translation and is replaced by `n`.
// lupdate can't expand macros, so the call must keep the literal tr(text, nullptr, n) shape matched by the `tr+=NOTICE_N`
// alias in CMakeLists.txt.
#define NOTICE_N(text, disambiguation, n, severity, delivery, ...) \
    Notice(staticMetaObject.className(), text, Severity::severity, Delivery::delivery, { __VA_ARGS__ }, n)

// For free functions and non-QObject types: the context is passed explicitly, as `NOTICE_IN(context, text)` expands to
// the `context, source` pair the `QT_TRANSLATE_NOOP+=` alias needs:
//
//     Notice(NOTICE_IN("Utils", "Failed to open %1"), Severity::Warning, Delivery::Modal, { path })
#define NOTICE_IN(context, text) context, text
