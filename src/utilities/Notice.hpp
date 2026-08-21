#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QAnyStringView>
#include <QStringList>
#include <source_location>
#include <initializer_list>

enum class Severity : u8 { Debug, Info, Warning, Critical };

enum class Delivery : u8 {
    // Transient status bar message.
    Status,
    // The call site renders it in its own surface (e.g. AssetPreviewWidget's error page); present() ignores these.
    Inline,
    // Modal message box.
    Modal,
};

// A user-facing message that logs itself, in English, at construction. Constructing one is what puts it in the log,
// so a Notice cannot be lost by being propagated, swallowed or dropped. Presentation is a separate, optional step.
//
// Purely diagnostic logging that never reaches the user stays on the plain qWarning()/qCritical() macros - a Notice
// would put a string no user ever sees in front of the translators.
//
// `source` is an untranslated English literal with %1..%99 placeholders; `args` fill them. The literal doubles as the
// translation catalog key, so the log gets English and the user gets their own language from the same string.
class Notice final {
   public:
    // Arguments are taken as views so call sites can pass whatever they already hold - QString, QStringView, the
    // QUtf8SV that ffitostr() hands back, itos(...).qsv() - and are copied into the Notice, which outlives them.
    Notice(
        const char* context,
        const char* source,
        Severity severity,
        Delivery delivery,
        std::initializer_list<QAnyStringView> args = {},
        // Drives %n (see NOTICE_N below): -1 means "no numerus", any other value is both substituted for a literal
        // %n in the source string and passed to Qt's plural-form lookup when translating.
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

// Construct a Notice inside a Q_OBJECT class. The message literal must come first: lupdate collects it through the
// `QT_TR_NOOP+=NOTICE` alias in CMakeLists.txt, which reads argument one and files it under the enclosing class's
// context - the same context staticMetaObject.className() yields at runtime.
#define NOTICE(text, severity, delivery, ...) \
    Notice(staticMetaObject.className(), text, Severity::severity, Delivery::delivery, { __VA_ARGS__ })

// Same, for a message whose text depends on a count - `text` should contain a single %n, which both selects the
// singular/plural translated form and is substituted with `n`'s value (Qt handles %n automatically once `n` reaches
// QCoreApplication::translate(); Notice does the same substitution itself for the untranslated/logged form). Extra
// %1.. placeholders alongside %n are filled from the trailing argument list as usual.
//
// lupdate never expands macros, so it can only recognize a numerus message by parsing a literal tr()/translate()
// call shape at the call site - aliasing to a NOOP-family macro (as NOTICE does) can't work here, since none of
// those macros have a numerus argument for the alias to inherit. So unlike NOTICE, this macro's *call-site* argument
// order is deliberately written to match tr(sourceText, disambiguation, n) positionally - text, then nullptr, then
// n - token-for-token the same shape as a plain member tr("%n foo(s)", nullptr, n) call. That's what the `tr+=
// NOTICE_N` alias in CMakeLists.txt keys off, and it's why lupdate generates separate singular/plural entries for
// this, instead of the one flat entry a NOOP-style alias would produce.
#define NOTICE_N(text, disambiguation, n, severity, delivery, ...) \
    Notice(staticMetaObject.className(), text, Severity::severity, Delivery::delivery, { __VA_ARGS__ }, n)

// Same, for free functions and non-QObject types, where the context has to be named explicitly. The aliased
// QT_TRANSLATE_NOOP form only fires on a two-argument call, so this expands to just the `context, source` pair and the
// rest of the Notice arguments are spelled out at the call site:
//
//     Notice(NOTICE_IN("Utils", "Failed to open %1"), Severity::Warning, Delivery::Modal, { path })
#define NOTICE_IN(context, text) context, text
