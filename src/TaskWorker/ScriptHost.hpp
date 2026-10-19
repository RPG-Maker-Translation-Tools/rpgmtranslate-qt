#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Settings.hpp"
#include "Task.hpp"
#include "Types.hpp"

#include <QDeadlineTimer>
#include <quickjs.h>

constexpr i32 SCRIPT_TIMEOUT = 15000;

auto qstringToJS(JSContext* ctx, QStringView qsv) -> JSValue;
auto qstringToJS(JSContext* ctx, QLatin1StringView qsv) -> JSValue;
auto jsToQString(JSContext* ctx, JSValueConst val) -> QString;
// The enabled terms of `glossary` as an array of `{ term, translation, note }`.
auto glossaryToJS(JSContext* ctx, const Glossary& glossary) -> JSValue;
// The `options` object every batch callback receives. `variant` holds the action's own setting: `LintFlags` for Trim,
// the wrap length (`u8`) for Wrap, nothing for the rest.
auto makeScriptOptions(
    JSContext* ctx,
    BatchAction action,
    u8 columnIndex,
    const BatchVariant& variant,
    const ProjectSettings* projectSettings,
    const Glossary& glossary
) -> JSValue;
auto jsArrayLength(JSContext* ctx, JSValueConst val) -> u32;
auto jsGetExceptionText(JSContext* ctx) -> QString;

// `rpgm.log` is always available; `hostApi` also installs `rpgm.readFile/writeFile/exec`, so pass it only for approved
// scripts (see MainWindow::confirmScriptAccess). `task` (optional) lets Cancel interrupt a running process.
struct ScriptRuntime {
    struct HostState {
        const Task* task;
        QDeadlineTimer* deadline;
        QStringList* log = nullptr;
    };

    explicit ScriptRuntime(QDeadlineTimer deadline, const Task* task = nullptr, bool hostApi = false);
    ~ScriptRuntime();

    ScriptRuntime(const ScriptRuntime&) = delete;
    auto operator=(const ScriptRuntime&) -> ScriptRuntime& = delete;

    QDeadlineTimer deadline_;
    HostState host;
    JSRuntime* rt_;
    JSContext* ctx;
};

struct JSValueGuard {
    JSValueGuard(JSContext* ctx, JSValue value) : ctx_(ctx), value_(value) {}

    ~JSValueGuard() { JS_FreeValue(ctx_, value_); }

    JSValueGuard(const JSValueGuard&) = delete;
    auto operator=(const JSValueGuard&) -> JSValueGuard& = delete;

    operator JSValue() const { return value_; }

    JSContext* ctx_;
    JSValue value_;
};

// Hex SHA-256 of a script's source; the key under which approval is remembered.
[[nodiscard]] auto scriptHash(QStringView source) -> QString;

// Asks the user whether `source` may run processes and touch files, unless its hash is already in `approved`; a
// positive answer is remembered there. GUI thread only.
auto confirmScriptAccess(QWidget* parent, QStringList& approved, const QString& name, const QString& source) -> bool;

// Calls `rpgmTranslateCallback(text, sourceLanguage, translationLanguage, glossary, filename)` of a user script and
// returns its string or the error text. `glossary` holds `{ term, translation, note }` entries. The caller must check
// that the script is approved.
[[nodiscard]] auto runTranslateScript(
    const QString& source,
    const QString& text,
    const QString& sourceLanguage,
    const QString& translationLanguage,
    const Glossary& glossary,
    const QString& filename
) -> Result<QString, QString>;

// Runs a built-in script (Trim, Wrap or Latinize) on one cell: its `rpgmLineCallback` for `columnIndex` of `text`.
// Returns the new text, or `text` itself when the script leaves the cell alone. `text` is in file form (line breaks
// are the `\#` marker). Built-in scripts are trusted and get no host API.
[[nodiscard]] auto runCellScript(
    QStringView script,
    BatchAction action,
    const QString& text,
    u8 columnIndex,
    const BatchVariant& variant,
    const ProjectSettings* projectSettings,
    const Glossary& glossary
) -> Result<QString, QString>;
