// Default RPGMTranslate JavaScript script. Edit freely, but keep rpgmLineCallback well-formed.

// Characters that codes which draw something are assumed to take (the line length is measured in characters).
// Every other code is zero-width. Codes are case-insensitive.
const CODE_WIDTHS = {
    V: 3, // variable value
    N: 6, // actor name
    P: 6, // party member name
    G: 1, // currency unit
    I: 2, // icon
};

// A row that would need more lines than this is left unchanged (a message window shows 4 lines).
const MAX_LINES = 4;

// MV/MZ message font size: default, step and clamp bounds of `\{` / `\}`.
const DEFAULT_FONT_SIZE = 28;
const FONT_SIZE_STEP = 12;
const MIN_FONT_SIZE = 24;
const MAX_FONT_SIZE = 96;

// The marker translation files use for a line break.
const LINE_BREAK = "\\#";

// An RPG Maker code (`\C[2]`, `\fn<Comic Sans>`, `\{`, `\.`) or HTML-like tag (`<br>`): a run of letters with an
// optional `[...]`, `<...>` or `{...}` argument, or one symbol, as the engine parses them, so plugin codes match too.
// Never split by a line break, even when its argument has spaces. The regex does not match nested brackets: a code
// inside an argument (`\N[\V[3]]`) is handled by `bracketEnd`.
const CODE = /\\(?:[A-Za-z]+(?:<[^>]*>|\{[^}]*\})?|[^A-Za-z0-9])|<\/?[A-Za-z][^<>]*>/y;

/** Index just past the `]` that closes the `[` at `open`, counting nested brackets; `open` when it is never closed. */
function bracketEnd(line, open) {
    let depth = 0;

    for (let index = open; index < line.length; index++) {
        if (line[index] === "[") {
            depth++;
        } else if (line[index] === "]" && --depth === 0) {
            return index + 1;
        }
    }

    return open;
}

// Japanese, Chinese and other scripts that are wrapped between any two characters. Hangul uses spaces.
const CJK = /[⺀-鿿ꀀ-꓏豈-﫿︰-﹏＀-￯\u{20000}-\u{3FFFF}]/u;

// Kinsoku: a line may not start with these, or end with these.
const NO_LINE_START = "、。，．・：；？！）」』】〕〉》〜…ーぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮ)]}>,.:;?!";
const NO_LINE_END = "（「『【〔〈《([{<";

/**
 * Splits a line into atoms `{ text, width, kind }` (kind: "space", "cjk", "char" or "code"). `state.size` is the font
 * size, updated by size codes and carried to the next line of the row.
 */
function toAtoms(line, state, baseSize) {
    const atoms = [];

    for (let index = 0; index < line.length; ) {
        if (line[index] === "\\" || line[index] === "<") {
            CODE.lastIndex = index;
            const match = CODE.exec(line);

            if (match) {
                let end = index + match[0].length;

                // `\N[...]`: a run of letters followed by a bracketed argument.
                if (line[end] === "[" && /[A-Za-z]$/.test(match[0])) {
                    end = bracketEnd(line, end);
                }

                const code = line.slice(index, end);
                atoms.push({ text: code, width: codeWidth(code, state, baseSize), kind: "code" });
                index = end;
                continue;
            }
        }

        const char = String.fromCodePoint(line.codePointAt(index));
        index += char.length;

        atoms.push({
            text: char,
            width: state.size / baseSize,
            kind: char === " " ? "space" : CJK.test(char) ? "cjk" : "char",
        });
    }

    return atoms;
}

/** Width of a code in characters at the current font size; size codes change `state.size`. */
function codeWidth(code, state, baseSize) {
    const scale = state.size / baseSize;

    if (code === "\\\\") {
        return scale;
    }

    // RPG Maker 2000/2003: a half-width space.
    if (code === "\\_") {
        return 0.5 * scale;
    }

    if (code === "\\{") {
        state.size = state.size <= MAX_FONT_SIZE ? state.size + FONT_SIZE_STEP : state.size;
        return 0;
    }

    if (code === "\\}") {
        state.size = state.size >= MIN_FONT_SIZE ? state.size - FONT_SIZE_STEP : state.size;
        return 0;
    }

    const named = /^\\([A-Za-z]+)(?:\[(.*)\])?$/s.exec(code);

    if (!named) {
        return 0;
    }

    const name = named[1].toUpperCase();

    if (name === "FS" && named[2] !== undefined && Number(named[2]) > 0) {
        state.size = Number(named[2]);
        return 0;
    }

    return (CODE_WIDTHS[name] ?? 0) * scale;
}

/** Whether a line may be broken between `atoms[index - 1]` and `atoms[index]`. */
function canBreakBefore(atoms, index) {
    const previous = atoms[index - 1];
    const next = atoms[index];

    if (previous.kind === "space") {
        return next.kind !== "space";
    }

    return (
        previous.kind === "cjk" &&
        next.kind === "cjk" &&
        !NO_LINE_START.includes(next.text) &&
        !NO_LINE_END.includes(previous.text)
    );
}

/** Breaks `atoms` into lines no wider than `limit` where possible; words longer than the limit overflow. */
function wrapAtoms(atoms, limit) {
    const lines = [];
    let start = 0;
    let width = 0;
    let lastBreak = -1;

    for (let index = 0; index < atoms.length; index++) {
        const atom = atoms[index];

        if (index > start && canBreakBefore(atoms, index)) {
            lastBreak = index;
        }

        width += atom.width;

        // A space hanging at the end of a line is dropped, so it never forces a break.
        if (atom.kind !== "space" && width > limit && lastBreak > start) {
            lines.push(atoms.slice(start, lastBreak));
            start = lastBreak;
            lastBreak = -1;
            width = atoms.slice(start, index + 1).reduce((sum, item) => sum + item.width, 0);
        }
    }

    lines.push(atoms.slice(start));

    return lines;
}

function joinAtoms(atoms) {
    let end = atoms.length;

    while (end > 0 && atoms[end - 1].kind === "space") {
        end--;
    }

    return atoms
        .slice(0, end)
        .map((atom) => atom.text)
        .join("");
}

/**
 * Word-wrap a column to a max line length, keeping existing `\#` breaks. Codes are zero width (or the width they
 * draw), size codes scale the characters after them, and a row needing more than MAX_LINES lines, or using
 * `<WordWrap>`, is left unchanged.
 *
 * @param {string} sourceText - Untranslated source text of this line.
 * @param {string[]} translations - Existing translation columns of this line, in order.
 * @param {string} filename - Name of the file being processed.
 * @param {number} lineNumber - Zero-based line index within the file.
 * @param {Object} options - Action-specific settings.
 * @param {("trim"|"wrap"|"latinize"|"custom")} options.action - Triggering action. Built-ins keep their fixed names; user scripts always get "custom".
 * @param {number} options.columnIndex - Index into [sourceText, ...translations] that the action targets.
 * @param {number} [options.wrapLength] - Present when action === "wrap"; max line length, in characters.
 * @param {number} [options.fontSize] - Base message font size in pixels (0 or missing: 28).
 *
 * @returns {Array<string|undefined|null>} Shaped like `translations`: each entry replaces its column.
 *   `undefined`/`null` entries leave the column unchanged.
 */
function rpgmLineCallback(sourceText, translations, filename, lineNumber, options) {
    const columns = [sourceText, ...translations];
    const result = new Array(translations.length).fill(undefined);

    if (options.columnIndex < 1 || columns.length <= options.columnIndex || !(options.wrapLength > 0)) {
        return result;
    }

    const text = columns[options.columnIndex];

    if (/<WordWrap>/i.test(text)) {
        return result;
    }

    const baseSize = options.fontSize > 0 ? options.fontSize : DEFAULT_FONT_SIZE;
    const state = { size: baseSize };
    const wrapped = [];

    for (const line of text.split(LINE_BREAK)) {
        for (const atoms of wrapAtoms(toAtoms(line, state, baseSize), options.wrapLength)) {
            wrapped.push(joinAtoms(atoms));
        }
    }

    const wrappedText = wrapped.join(LINE_BREAK);

    if (wrappedText === text) {
        return result;
    }

    if (wrapped.length > MAX_LINES) {
        if (typeof rpgm !== "undefined") {
            rpgm.log(`${filename}:${lineNumber + 1}: left unwrapped, it would take ${wrapped.length} lines.`);
        }

        return result;
    }

    result[options.columnIndex - 1] = wrappedText;
    return result;
}
