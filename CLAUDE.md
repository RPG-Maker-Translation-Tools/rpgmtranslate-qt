# rpgmtranslate-qt

Qt 6 desktop app (C++) with a Rust core linked in over FFI (`rpgmtranslate.h`, `rust/src/ffi.rs`). CMake + Ninja build, Corrosion for the Rust half.

## Commits

Never sign commits, and never add Claude as a co-author. Ever.

## Engineering philosophy

Core stance: write code for its actual call site, not for a hypothetical future one. YAGNI is the default assumption, not an exception - deviate only for genuine utilities (aliases, hash maps, general conversions) that cost nothing to keep around. This codebase optimizes for **performance and readability together**, not for adherence to "clean code" rules.

- **No OOP for OOP's sake.** Free functions and `final` classes beat class hierarchies, singletons, and Factory/Strategy/Observer boilerplate whenever they do the job as clearly and faster. Runtime polymorphism is justified only by a genuine runtime-variant requirement (Qt's own `QObject` hierarchy) - never as a design nicety to "decouple" code. Don't build a strategy pattern where a `switch` or `std::variant` + `std::visit` will do.
- **No indirection or lifetime machinery without a reason.** Don't reach for `unique_ptr`/`shared_ptr` when a value's lifetime is trivially scoped to one function with no early-return complications - plain `new`/`delete`, stack allocation, a scoped buffer, or Qt parent-ownership wins there. Reserve smart pointers/RAII wrappers for genuinely ambiguous ownership, shared lifetime, or multiple exit paths where manual management would be error-prone.
- **Provably-safe unsafe/unchecked operations are preferred, not just tolerated.** Manual bit-packing, `ras<>`/`reinterpret_cast`, raw pointer ownership with manual `new[]`/`delete[]`, unions, placement tricks, and unchecked accessors are expected wherever the precondition is provably (or very confidently, from calling context) upheld or the FFI ABI requires it. Don't "clean them up" into safer-looking but slower or more indirect code. This is the one place comments are mandatory rather than banned: state *why* it's safe.
- **Actively look for micro-optimizations.** On every piece of code touched, consider whether it does unnecessary allocations, copies, or repeated passes over data:
    - Pre-size/reserve a buffer and write into it once rather than concatenating or reallocating repeatedly.
    - Building a string/list from a collection: size it in one pass, reserve once, fill in a second pass - never grow incrementally.
    - Avoid redundant copies - pass by reference/move where correctness allows; don't copy unless ownership is genuinely required.
    - Prefer stack allocation and value types over heap allocation when size and lifetime are known and bounded.
    - Combine passes over data, drop redundant traversals, eliminate avoidable heap churn.
    - Don't stop at the obvious cases above - scan for analogous patterns throughout the code.
- **Terseness over ceremony.** A tight, well-named loop beats a dozen tiny helper functions that exist only to make call sites read like prose. Don't split a function purely for length or "single responsibility" - split only when a piece is independently reusable or independently testable.
- **Balance performance against readability, situationally.** Changes that improve performance without adding meaningful complexity or code volume should basically always be applied. For larger or more convoluted hacks (unsafe tricks, manual memory tricks, bit manipulation, cache-friendliness rewrites), judge by how hot the path actually is: on a hot path they're justified even at real complexity cost - explain the tradeoff briefly; off a hot path, plain obviously-correct code wins. When unsure which bucket applies, ask or state the assumption explicitly.
- **Restraint on unsolicited rewrites.** Flag code that's both slower and more convoluted than necessary, but don't rewrite working, non-bottleneck code into "cleaner" form unless asked.

## Language standard & idioms

Target is **C++23**, set via `CMAKE_CXX_STANDARD 23`. Prefer the modern idiom over the older one it replaces, unconditionally:

- `std::expected<T,E>` (aliased `result<T,E>`) for fallible operations - never exceptions, error out-params, or `optional` plus a side channel. Reserve `std::variant` for genuine sum types (e.g. `BatchResult = variant<ScriptResult, TranslateResult>`), not as an error channel.
- `std::ranges`/`std::views` (aliased `ranges`/`views`) over hand-rolled index loops wherever a view composes cleanly (`views::enumerate`, `views::filter`, `views::drop`, `views::take`, `ranges::find_if`); fall back to a manual loop only when `break`/early-return mid-iteration would need extra bookkeeping.
- Designated initializers (`Struct{.field = value}`) as the default way to construct aggregates - not positional construction or a hand-written constructor.
- Concepts/constraints over SFINAE, deducing `this`, `if consteval`, multidimensional `operator[]`, `std::unreachable()` in switches that already cover every enumerator, `std::to_underlying` over manual casts.
- Exceptions are acceptable only at genuine external boundaries (`fs::filesystem_error`, nuspell) - catch narrowly there, never introduce new exception-based control flow elsewhere.

Don't reach for an older-standard idiom (macro-based type traits, C-style casts, manual RAII where a standard wrapper exists) when a C++23-appropriate one is available.

## Declaration syntax

Which spelling to use depends on how the variable is initialized:

- Plain initialization - a constructor call or explicit member init - uses `auto var = Type{...}` / `auto var = Type(...)`. Writing `Type var{...}` or `Type var(...)` there is prohibited.
- Initialization from a static/factory function, or no initializer at all, uses the concrete type: `Type var = Type::create(...);`, `Type var;`. Don't write `auto` there.
- Integer types are the exception to both rules: always spell the alias, never `auto` - `u32 count = parse(s);`, `usize len;` - regardless of where the value comes from.

## No zero-initialization

Do not zero-initialize (or otherwise default-fill) a variable or field whose value is written before any read. Leave it uninitialized - `Type var;` for scalars, `array<u8, N> buf;` for buffers - and let the first real write be the initialization. The only case for initializing at declaration is when something genuinely depends on the pre-write value: a flag/sentinel that gets *checked* before it is set, a member read on a path that may not have written it yet, or a type whose constructor must run. Never add a `= 0` / `{}` "for safety" - if a read can reach an unwritten value, that's a control-flow bug to fix, not to paper over with a zero.

## Const and compile-time correctness

`constexpr` beats `const` beats mutable, always, wherever the value's lifetime/computability allows it: if a value can be known at compile time, it is `constexpr`. Every `constexpr` variable, namespace-scope or local, is `static constexpr` (Clang optimizes it better than plain `constexpr`; GCC is never a target) - except give internal-linkage constants an anonymous namespace instead of a `static` keyword, and never mark declarations inside an anonymous namespace `static`. Any data that is never reassigned is `const`; any pointer that is never repointed is `const`, including double-const (`const T* const`) when both the pointee and the pointer are fixed.

## Zero-copy by default

- Prefer views over owning copies at any boundary that doesn't need ownership: `QStringView`/`QLatin1StringView` (aliased `QUtf8SV`/`QL1SV`) and `std::string_view` over `QString`/`std::string`. `QSVList = QList<QStringView>` is the standard "list of string views into one buffer" type - see `lineParts`/`fileLines`.
- Prefer `span<T>` over passing containers by value or re-copying into a new buffer just to hand data across a boundary.
- Reinterpret in place (`ras<T>`, i.e. `reinterpret_cast`) across the FFI boundary rather than converting through an intermediate representation, when the layouts genuinely match (see `ByteBuffer`/`FFIString` usage in `TaskWorker.cpp`).
- `svtostr` only converts on Qt < 6.9; on 6.9+ it is identity and returns the view unchanged. Don't rely on it to produce an owning `QString` - use `.toString()` where ownership is actually needed.
- Exception: copy explicitly, and say why if it's not obvious, when the lifetime crossing the boundary is genuinely unclear or error-prone (a view into a local about to go out of scope, a buffer another thread may mutate) - correctness beats a saved allocation.

## Project architecture

- `src/utilities/Aliases.hpp` centralizes the fixed-width integer aliases (`u8`..`u64`, `i8`..`i64`, `f32`, `f64`), `usize`/`isize`, the `atomicX` types, `result<T,E>`/`Err<E>` over `std::expected`/`std::unexpected`, and pulled-in `std::` names (`tuple`, `vector`, `span`, `shared_ptr`, `optional`, ...) via `using`. **Always use these aliases** instead of the raw standard names they wrap (`u32` not `uint32_t`, `vector` not `std::vector`, `result<T,E>` not `std::expected<T,E>`). When a standard type becomes commonly used across the codebase (not just one file), add a `using` for it there rather than qualifying `std::` at every call site.
- `src/utilities/FWD.hpp` centralizes forward declarations for Qt (and other) types used only by pointer/reference in a header, to cut include weight and compile times. If a file only needs a type by pointer/reference - no member access, no `sizeof`, no by-value storage - pull in `FWD.hpp` instead of the real header. If the type isn't there yet, add its forward declaration there rather than declaring it locally or including the full header. Include the real header only when the code needs the complete type.
- `Types.hpp` (domain types), `Enums.hpp` (global enums), `Hasher.hpp` (the custom `HashMap`/`hashset` over gxhash), `Constants.hpp` (shared constants), `Utils.hpp`/`.cpp` (free helpers) and `glazemeta.hpp` (Glaze metadata) build on `Aliases.hpp` - put things in the header matching their kind, not crammed into `Aliases.hpp`.
- **Global** constants, enums, type aliases, and free utility functions (used from more than one or two classes, or conceptually part of the app's shared vocabulary) belong in the matching `src/utilities/*.hpp`. Anything **scoped to one class or a tightly-coupled pair** (a helper struct only `TaskWorker` uses, a local enum only one menu switches on, a file-local `static constexpr` lookup table) stays local - a private nested type, a file-local `static`/anonymous-namespace symbol, or a member - right next to its one user. Don't promote something to a shared header just because it technically could be reused.
- Each top-level UI component lives in its own `src/<Component>/` directory (`TaskWorker/`, `TaskPanel/`, `BatchMenu/`, `MainWindow/`, ...). If one file there grows past ~1000-1500 lines, split it by concern rather than letting it keep growing - `MainWindow.cpp` is already split into `MainWindow{Callbacks,Connections,Functions,SaveLoad}.cpp`; follow that pattern.

## Preferred libraries - always, not situationally

- **Number parsing/formatting**: `Utils.hpp`'s `stoa<T>`/`itos`/`ftos` (fast_float/jeaiii-backed, ~1.5x faster than Qt) - never `QString::number`, `.toInt()`/`.toUInt()`/`.toDouble()`/`.toFloat()`, `std::to_string`, `snprintf`, or `std::ostringstream`. `stoa<T>(view, base = 10, bool* ok = nullptr)` mirrors `QString::toInt(bool*)`'s success signaling; `itos`/`ftos` return a stack `Digits` buffer - call `.qsv()` for a `QStringView` to display or pass to `.arg()`. Hex formatting (e.g. `U+XXXX` codepoints) has no existing helper - write a small local one rather than falling back to `QString::number(x, 16)`.
- **JSON (de)serialization**: Glaze - add a `glz::meta<T>` specialization next to the type it describes (see `Types.hpp`, `Settings.hpp`) rather than hand-writing parsing or using Qt's `QJson*` classes.
- **`HashMap<K,V>`/`hashset<E>`** (`Hasher.hpp`, built on gxhash) are the default associative containers - never `std::unordered_map`/`QHash` directly.
- **`magic_enum`** for enum reflection/(de)serialization and its bitwise operators (`using namespace magic_enum::bitwise_operators` in `Aliases.hpp`) - flag enums stay plain `enum` (not `enum class`) so the bitwise ops apply directly; scoped `enum class` is for closed, non-flag sets.
- **`[[nodiscard]]`** on any function whose dropped return value would be a silent bug (fallible operations, computed values with no side effect). The build warns on dropping these - never silence it with a `(void)` cast; handle the result.
- **`final`** on every concrete class that isn't designed as a base.

## Qt version and practices

Target is **Qt 6.8** - `CMakeLists.txt` enforces this via `QT_DISABLE_DEPRECATED_UP_TO`/`QT_ENABLE_STRICT_MODE_UP_TO` set to `0x060802`, plus `QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_TO_ASCII`, `QT_NO_CAST_FROM_BYTEARRAY`, `QT_NO_QASCONST`, `QT_NO_QEXCHANGE`, `QT_NO_QSNPRINTF`, `QT_NO_URL_CAST_FROM_STRING`, `QT_NO_FOREACH`, `QT_NO_CONTEXTLESS_CONNECT`, `QT_NO_NARROWING_CONVERSIONS_IN_CONNECT`, `QT_USE_NODISCARD_FILE_OPEN`. Write code that's clean under these - no deprecated 6.8-and-earlier APIs, no implicit ASCII/QString casts, context-bound `connect`.

"Best practices" here means respecting those compile definitions and modern signal/slot syntax - it does **not** mean adding ceremony Qt itself no longer requires. In particular:

- No `QT_BEGIN_NAMESPACE`/`QT_END_NAMESPACE` around forward declarations - a no-op for this build (Qt isn't built with a custom namespace) and just noise.
- No other boilerplate carried over from older Qt style guides that doesn't actually do anything under this build configuration.

Recurring Qt idioms:

- `u"..."_s`/`u"..."_qsv` literals (`Qt::Literals::StringLiterals`, `Aliases.hpp`) - `_qsv` (`QStringView`) by default, including as the format string in `.arg(...)` since `.arg()` returns `QString` regardless of the receiver's type; reach for `_s` only when a `QString` is actually stored or mutated.
- `Q_DECLARE_METATYPE` for types crossing signal/slot or `QVariant` boundaries; `-> void` trailing return types even on lambdas passed to `connect`.
- Background work: `QtConcurrent::run(...).then(this, ...)` - always pass `this` (or another context object) to `.then` so the continuation is queued onto the right thread, never a bare lambda.

## User-facing messages (Notice.hpp)

Anything the user sees goes through `Notice` (`src/utilities/Notice.hpp`): it logs itself in English at construction and renders translated on demand, so a message can't reach the user without reaching the log, and the literal is written once instead of twice.

- Inside a `Q_OBJECT` class: `NOTICE("text %1", Severity, Delivery, args...)` - the message literal must come first.
- Elsewhere: `Notice(NOTICE_IN("Context", "text %1"), Severity::X, Delivery::Y, { args })` - `NOTICE_IN` takes exactly two arguments.
- Show with `present(parent, notice)`, propagate as `result<T, Notice>`. `present()` derives icon and title from severity - don't reintroduce per-site dialog titles.
- Constructing a `Notice` *is* logging it. Never build one on a path that might not fail, and never log one yourself.
- Purely diagnostic logging the user never sees stays on `qWarning()`/`qCritical()` with `.noquote()` - a `Notice` there would put a never-displayed string in front of translators.
- Interactive dialogs (`QMessageBox::question`, custom button boxes) stay as they are; a `Notice` has no buttons. Record the outcome of destructive choices with a plain English `qInfo()`.

## FFI boundary

Rust functions take/return `FFIString`/`ByteBuffer` (view-only unless `cap != 0`); C++ converts with `strtoffi`/`ffitostr` at the boundary and otherwise works with `QString`/`QByteArray` on its side. Don't invent a second conversion path. FFI results are wrapped as `result<ByteBuffer, FFIString>` and friends; when the failure surfaces to the user, `E` is `Notice`, not `QString`.

## Build & verification

- Don't run a full `ninja` build, and never a link, just to check that a change compiles. Pull the exact command for each changed file out of `build/compile_commands.json` and re-run it with `-fsyntax-only` in place of `-o <obj>`.
- Rust-only changes: `cargo check --features languagetool` from `rust/` is the equivalent fast check.
- Editing `extern "C"` signatures in `rust/src/ffi.rs` needs no manual header work - `cbindgen` regenerates `rpgmtranslate_rs.h` as part of the CMake build; never hand-edit it.
- `lupdate` does not expand macros. Any macro wrapping a translatable literal must be registered in `CMakeLists.txt`'s `LUPDATE_OPTIONS` via `-tr-function-alias`, or its strings silently never reach the catalog. `QT_TR_NOOP+=X` tolerates trailing arguments; `QT_TRANSLATE_NOOP+=X` only fires on an exactly-two-argument call.
