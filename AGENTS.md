# rpgmtranslate-qt

C++23/Qt6 application. It extracts RPG Maker game text into editable plain-text files and writes translations back into the game. The Rust core is imported through Corrosion.

## Conventions

- `scripts.h` is generated from `scripts/` at build time. Don't edit it.
- `lua` and `lua.exe` are committed Lua binaries. Don't replace them.
- Bundled library licenses live in `licenses/`. A new statically linked dependency needs `licenses/<name>/` with the upstream license and a matching `LIBRARY_LIST` entry in `prepare-release.lua`.
- `CHANGELOG-ru.md`, `README-ru.md`, `README.zh-CN.md` and `docs/src/content/docs/ru/` are translations. Don't edit them unless asked.

## Verification

- Don't fully build the app. Configuring, checks and tests are fine.
- Verify with `-fsyntax-only` against `build/compile_commands.json`. Rust-only changes: `cargo check` from `rust/`.
- Update `CHANGELOG.md`, `README.md` and the English docs when user-facing behavior changes.

## General Rules

### Signing commits

Do not sign commits and don't co-author.

### Think Before Coding

Don't assume. Don't hide confusion. Surface tradeoffs.

Before implementing:

- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### Touch Only What is Necessary

You must read and work only on the things that are absolutely necessary.

Guidelines:

- No random `ls`, `git status`, `git diff` and any other commands to check stuff that you don't need to check.
- Prefer surgical searches with rg or fd.
- No manually finding what you might need: no probing of system directories - if something isn't on PATH, stop and report.

### Surgical Changes

Touch only what you must. Clean up only your own mess.

When editing existing code:

- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:

- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

Every changed line should trace directly to the user's request.

### Engineering Philosophy

For the general code style, Ponytail skill should be used. YAGNI is the default. Otherwise performance and readability together, not "clean code" rules.

- Provably-safe unsafe/unchecked operations are preferred. Rust unsafe/unwrap_unchecked/get_unchecked; reinterpret_cast, bit-packing, placement tricks, unchecked accessors - whenever the precondition is provable (or very confident from call context) or an FFI/callback ABI requires it. Don't "clean up" into safer but slower code. The one place comments are mandatory: state the invariant that makes it safe.
- Hunt micro-optimizations in every piece of code touched - unnecessary allocations, copies, repeated passes:
  - Pre-size/reserve and write once; no repeated concatenation/reallocation.
  - Borrow/move instead of copy or clone unless ownership is required.
  - Stack and value types over heap when size and lifetime are bounded.
  - Combine passes, drop redundant traversals, cut heap churn. Look for analogous patterns beyond these.
- Performance vs readability is situational. Gains without meaningful complexity: always apply. Larger hacks (unsafe tricks, manual memory, bit manipulation, cache-friendliness rewrites): judge by path hotness. Hot (audio thread, drawing, text layout, highlighting, table painting): justified, briefly explain the tradeoff. Not hot: plain obviously-correct code wins. Unsure which: ask or state the assumption.

### C++ Conventions

C++ projects target C++23.

Rules:

- Always Result<T,E>/Err<E> for fallible operations.
- ranges/views over index loops when a view composes cleanly; manual loop only when mid-iteration break/return needs extra bookkeeping.
- Concepts over SFINAE; deducing this, `if consteval`, multidimensional operator[], std::unreachable in exhaustive switches.
- No shared ownership unless explicitly required by the architecture.
- Exceptions only at external boundaries (fs::filesystem_error, throwing libraries); catch narrowly.
- No older idioms (macro type traits, manual RAII).
- constexpr > const > mutable, wherever possible. Every constexpr variable (namespace-scope or local) is static constexpr, except internal-linkage constants, which go in an anonymous namespace instead of static (and never mark declarations inside it static). Never-reassigned data is const; never-repointed pointers are const, both fixed: `const T* const`.
- Members ordered largest to smallest type. alignas only for SIMD/cache-line width, external ABI, or false-sharing avoidance (e.g. audio vs UI thread).
- Names: at least 3 characters.
- Zero-copy - views over owning copies wherever ownership isn't needed, including string views and spans. reinterpret_cast in place across FFI/C boundaries when layouts match. Copy explicitly (say why if unobvious) when lifetime is unclear or risky.

Architecture:

- src/utilities - shared utilities: Aliases.hpp, FWD.hpp, Enums.hpp, Types.hpp, Constants.hpp, Hasher.hpp, Utils.hpp, Numbers.hpp.
- Rules:
  - Symbols used by more than one or two classes go in the matching utilities header. Symbols scoped to one class or a tightly-coupled pair stay local, beside their user. Don't promote just because reuse is possible.
  - One `src/<Component>/` directory per top-level UI component; shared code in `src/utilities/`, vendored in `src/3rdparty/`. A `.cpp` past ~1000 lines splits by concern (`Component{Concern1,Concern2}.cpp`); follow existing splits.
  - Serialization into formats: always Glaze, not std and not Qt.
  - Enum reflection: enchantum.
  - Strings: `%` concatenation through StringBuilder.hpp or Qt is preferred. std::format when the end result is UTF-8.

#### Rust FFI

- If Rust FFI is small, keep it as a hand-written header. FFIs with a handful of functions need their header generated with cbindgen.
- FFI uses only sentinel values and types. Return booleans to indicate errors, make writes across FFI using pointers.

#### Qt

- Qt 6.8 is the target, under strict defines and no legacy boilerplate.
- String-view aliases (QUtf8SV/QL1SV/QSVList), never raw Qt names. `u"..."_qsv` by default (also as `.arg()` format string); `u"..."_s` only when a QString is stored or mutated.
- Strings: `%` concatenation plus one multi-argument `.arg(a, b, ...)`. No chained `.arg().arg()`, no repeated `+=`.
- Q_DECLARE_METATYPE for types crossing signal/slot or QVariant.
- User-facing messages: Notice.hpp. Show with `present(parent, notice)`. Diagnostics the user never sees: `qWarning()`/`qCritical()`. Dialogs with their own buttons stay QMessageBox.

#### FLTK

- FLTK 1.4+ - very different API from older versions. Double-check installed headers directly.
- Fl_Tabs: construct with real, non-zero geometry; lay child groups out at the same rect before end(). tab_height() derives from `child(i)->y() - y()`, so Fl_Flex + client_area() before children exist collapses the strip and hides the client area.
- Do not use built-in FLTK dialogs: use tinyfiledialogs wrappers from Dialog.hpp, and convenient loggers from Logger.hpp.

#### Sanity checks

For complex C/C++ libraries (Qt, FLTK - especially FLTK), read the actual source instead of guessing.

## Custom Lua

Helper scripts run on stripped Lua 5.5 fork.

Invocation: `lua.exe script.lua [args...]`;

Absent entirely: package/require/coroutine/debug/math/utf8. Scripts are self-contained.

Available:

- base: assert/dofile/error/getmetatable/ipairs/loadfile/next/pairs/pcall/print/rawequal/rawget/rawlen/rawset/select/setmetatable/tonumber/tostring/type/xpcall/\_G/\_VERSION
- string: byte/char/find/format/gmatch/gsub/len/lower/match/rep/sub/upper (also as methods: s:gsub(...))
- table: concat/insert/remove/sort/unpack
- io: close/flush/open/popen/read/type/write/io.stdin/stdout/stderr; handles have read/write/flush/seek/close/setvbuf
- os: execute/exit/getenv/remove/rename

- Lines: loop on f:read("l") or slurp with f:read("a").
- io.read always reads stdin and io.write always writes stdout; use an io.open handle for files.
- No math: x // 1 for floor/string.format("%d", x) to print integers.

Other:

- LUA_NOCVTN2S: no number->string coercion.
- LUA_NOCVTS2N: no string->number coercion in the VM.
- LUA_32BITS.
