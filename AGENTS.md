# rpgmtranslate-qt

Qt 6 desktop app (C++) with a Rust core linked over FFI (`rust/src/ffi.rs`; `cbindgen` generates `rpgmtranslate_rs.h`). CMake + Ninja, Corrosion for the Rust half.

## Commits

Don't sign commits or add co-authors.

## Engineering philosophy

YAGNI: write for the actual call site, smallest complete change. Performance and readability together, not "clean code" rules.

- No OOP for its own sake: free functions, plain structs, `final` classes; `switch`/`variant` over hierarchies and pattern boilerplate. Runtime polymorphism only where genuinely runtime-variant (`QObject`).
- Provably-safe unsafe/unchecked operations preferred (bit-packing like the tagged `Translation` pointer, `rcast`, unions, placement tricks, unchecked accessors) when the precondition is provable or an FFI ABI requires it. Don't "clean up" into safer but slower code. Comments are mandatory only here: state the invariant.
- Micro-optimize touched code: pre-size and write once; sizing pass, allocate once, fill; borrow/move over copy; stack/value types over heap when bounded; combine passes, cut heap churn.
- Larger hacks only on hot paths, with a brief tradeoff note; elsewhere plain obviously-correct code. Unsure: ask or state the assumption.
- Flag code both slower and more convoluted than needed; don't rewrite working non-bottleneck code unless asked.

## C++ conventions

C++23, Clang only (GCC is never a target).

- `Result<T,E>`/`Err<E>` (`std::expected`) for fallible operations; never exceptions, error out-params, or optional + side channel. `variant` only for genuine sum types (`BatchResult`).
- Ranges/views over index loops when a view composes cleanly; manual loop when mid-iteration break/return needs bookkeeping.
- Designated initializers for aggregates. Concepts over SFINAE; deducing `this`, `if consteval`, multidimensional `operator[]`, `std::unreachable()` in exhaustive switches, `std::to_underlying`.
- `scoped_lock`, never `lock_guard`. `unique_ptr` over shared ownership; `shared_ptr` only when ownership is genuinely shared.
- Exceptions only at external boundaries (`fs::filesystem_error`, nuspell); catch narrowly.
- Casts only via `scast`/`rcast`/`ccast`; no raw or C-style casts, no macro type traits, no manual RAII where a standard wrapper exists.
- Enums: `enchantum` for reflection/(de)serialization and flag ops (`using namespace enchantum::bitwise_operators` in `Aliases.hpp`). Flag enums are plain `enum`; `enum class` only for closed, non-flag sets.

Declarations:

- Constructor call / explicit init: `auto var = Type{...}` / `auto var = Type(...)`; `Type var{...}`/`Type var(...)` prohibited.
- Factory or no initializer: concrete type, `Type var = Type::create(...);`, `Type var;`.
- Integers always spell the alias, never `auto`: `u32 count = parse(s);`.
- Zero-initialize only when code depends on the pre-write value, never "for safety"; leave values overwritten before any read uninitialized. A read reaching an unwritten value is a control-flow bug: fix it.
- `constexpr` > `const` > mutable. Every `constexpr` variable is `static constexpr`, except internal-linkage constants: anonymous namespace instead, nothing inside it `static`. Never-reassigned data and never-repointed pointers are `const` (`const T* const`).
- Members largest to smallest. `alignas` only for SIMD/cache-line width, external ABI, or false sharing.
- Names at least 3 characters, spelled out (`tableColumns`, not `tableCols`).
- Zero-copy: views (`QUtf8SV`/`QL1SV`/`string_view`, `QSVList` for views into one buffer) and `span` over owning copies; `rcast` in place across FFI when layouts match. Copy explicitly (say why if unobvious) when lifetime is unclear.

## Architecture

- Read `src/utilities/` before writing code: it is the project's vocabulary (aliases, numbers, hashing, strings, notices, FFI conversion, the tagged `Translation`).
- Always use the aliases from `src/utilities/Aliases.hpp` (Qt side via `QtAliases.hpp`); add a `using` there when a std type becomes common.
- `src/utilities/FWD.hpp`: forward declarations for types used only by pointer/reference in headers; add missing ones there, never locally.
- Each concept in its header (`Types.hpp`, `Enums.hpp`, `Constants.hpp`, `Utils.hpp`, `Hasher.hpp`, `glazemeta.hpp`), never in `Aliases.hpp`.
- Symbols used by more than one or two classes go in `src/utilities/`; symbols for one class or a tightly-coupled pair stay beside their user.
- One `src/<Component>/` per top-level UI component; vendored code in `src/3rdparty/`.
- A `.cpp` past ~1000 lines splits by concern; follow the existing `MainWindow{...}.cpp` split.

## Libraries

- Numbers only through `Numbers.hpp` and `QtNumbers.hpp`; never std/Qt conversions.
- Glaze for every format; `glz::meta<T>` beside its type. Never hand-rolled parsing or `QJson*`.
- `HashMap`/`HashSet` (`Hasher.hpp`, gxhash) are the only associative containers; never `std::unordered_map`/`QHash`/`std::hash`. `operator[]` is lookup-only; add with `emplace`/`insert_or_assign`.
- Strings: `%` concatenation; `std::format` only for UTF-8 results where `%` gets messy.
- `[[nodiscard]]` wherever a dropped result is a silent bug; never `(void)`-cast it. `final` on every concrete class not designed as a base.

## Qt

- Qt 6.8: `CMakeLists.txt` sets `QT_DISABLE_DEPRECATED_UP_TO`/`QT_ENABLE_STRICT_MODE_UP_TO=0x060802` and the `QT_NO_*` cast/`foreach`/contextless-connect defines. Code stays clean under them; no `QT_BEGIN_NAMESPACE` or other legacy boilerplate.
- Raw Qt view names are never spelled: `QUtf8SV`/`QL1SV`/`QSVList`. Qt may be < 6.9 (Debian stable): `svtostr` converts only there; use `.toString()` when ownership is needed.
- `u"..."_qsv` by default (also as `.arg()` format string); `u"..."_s` only when a `QString` is stored or mutated. `%` plus one multi-argument `.arg(a, b, ...)`; no chained `.arg()`, no repeated `+=`.
- `Q_DECLARE_METATYPE` for types crossing signals/`QVariant`. `-> void` on lambdas passed to `connect`.
- Background work: `QtConcurrent::run(...).then(this, ...)`, always with a context object.
- `lupdate` doesn't expand macros: a macro wrapping a translatable literal needs a `-tr-function-alias` in `LUPDATE_OPTIONS`. `QT_TR_NOOP+=X` tolerates trailing arguments; `QT_TRANSLATE_NOOP+=X` matches only exactly-two-argument calls.
- `HEADERS` in `CMakeLists.txt` is never compiled; it exists so `qt_add_translations()` sees `tr()` in headers, so list new such headers there. New `.cpp` files go in `SOURCES`.

## User-facing messages

- Everything the user sees goes through `Notice` (`src/utilities/Notice.hpp`): logged in English at construction, translated on display.
- In a `Q_OBJECT` class: `NOTICE("text %1", Severity, Delivery, args...)`, literal first. Elsewhere: `Notice(NOTICE_IN("Context", "text %1"), Severity::X, Delivery::Y, { args })` (`NOTICE_IN` takes exactly two arguments).
- Show with `present(parent, notice)` (icon/title from severity, no per-site titles); propagate as `Result<T, Notice>`, including FFI failures that reach the user.
- Constructing a `Notice` logs it: never build one on a path that might not fail, never log it again.
- Diagnostics the user never sees: `qWarning()`/`qCritical()` with `.noquote()`, not a `Notice`.
- Dialogs with their own buttons stay `QMessageBox`; record destructive choices with an English `qInfo()`.

## Rust FFI

- `gxhash` lives in `rust/src/ffi.rs`, declared by `Hasher.hpp` and the generated header with the same signature.
- `cbindgen` regenerates `rpgmtranslate_rs.h` during the CMake build; never hand-edit it.
- Rust takes/returns `FFIString`/`ByteBuffer` (view-only unless `cap != 0`). C++ converts only via `strtoffi`/`ffitostr` and otherwise uses `QString`/`QByteArray`. Results are `Result<ByteBuffer, FFIString>` and the like.
- Std-dependent or large Rust dependencies are their own crates (own manifest, `FetchContent` + `corrosion_import_crate`).

## Lua scripts

The `*.lua` helper scripts must run on a stripped Lua 5.5: self-contained (no `require`/`package`), no `math`/`utf8`/`coroutine`/`debug`, no `load`, `io.lines`, `os.time`/`os.date`/`os.clock`, `table.pack`/`table.move`, `string.pack`. No implicit number/string coercion (`"n=" .. 5` errors; use `string.format("%d", n)`), floor with `x // 1`, numbers are 32-bit. Read files via `io.open` handles with `f:read("l")`/`f:read("a")`.

## Build and verification

- Never build, link or launch: no `cmake --build`, `ninja`, running the app, smoke tests. Build-system targets (`update_translations`, ...) count as building: ask first.
- Verify with `-fsyntax-only`: take the file's command from `build/compile_commands.json`, replace `-o <obj>`. New file: reuse a sibling's command. No database: configuring is allowed, building isn't.
- Rust-only changes: `cargo check --features languagetool` from `rust/`.
- After finishing a part of the work, update `CHANGELOG.md` with concise, user-facing entries.
- English docs only; don't touch translated ones (`README-ru.md`, `README.zh-CN.md`, `CHANGELOG-ru.md`, `docs/src/content/docs/{ru,zh}/`).
- A new statically-linked dependency needs `licenses/<name>/` with the real upstream license and a matching `LIBRARY_LIST` entry in `prepare-release.lua`.
