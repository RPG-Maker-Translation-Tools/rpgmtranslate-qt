// This is a default RPGMTranslate JavaScript script. You're free to change it, add new functions, as long as rpgmLineCallback function exists and is not malformed.

/**
 * Latinize CJK/Unicode (mainly punctuation) characters to their Latin equivalents.
 *
 * @param {string} sourceText - The original (untranslated) source text for this line.
 * @param {string[]} translations - The existing translation columns for this line, in order.
 * @param {string} filename - Name of the file currently being processed.
 * @param {number} lineNumber - Zero-based index of the current line within the file.
 * @param {Object} options - Action-specific settings.
 * @param {("trim"|"wrap"|"latinize"|"custom")} options.action - Which batch action triggered this call. Built-in actions have fixed identifiers; every user script gets "custom".
 * @param {number} options.columnIndex - Index into [sourceText, ...translations] that the action targets.
 * @param {boolean} [options.trimLeading] - Present when action === "trim"; whether to strip leading whitespace.
 * @param {boolean} [options.trimTrailing] - Present when action === "trim"; whether to strip trailing whitespace.
 * @param {number} [options.wrapLength] - Present when action === "wrap"; max line length to wrap at.
 *
 * @returns {Array<string|undefined|null>} An array shaped like [sourceText, ...translations].
 *   Each entry replaces the corresponding column; entries left as `undefined` or `null`
 *   leave that column unchanged.
 */
function rpgmLineCallback(sourceText, translations, filename, lineNumber, options) {
    const REPLACEMENTS = {
        "。": ".",
        "、": ",",
        "，": ",",
        "・": "·",
        "※": "·",
        "゠": "–",
        "＝": "—",
        ー: "—",
        "「": "'",
        "」": "'",
        "〈": "'",
        "〉": "'",
        "『": '"',
        "』": '"',
        "《": '"',
        "》": '"',
        "（": "(",
        "〔": "(",
        "｟": "(",
        "〘": "(",
        "）": ")",
        "〕": ")",
        "｠": ")",
        "〙": ")",
        "｛": "{",
        "｝": "}",
        "［": "[",
        "【": "[",
        "〖": "[",
        "〚": "[",
        "］": "]",
        "】": "]",
        "〗": "]",
        "〛": "]",
        "〜": "~",
        "？": "?",
        "！": "!",
        "：": ":",
        "…": "...",
        "‥": "...",
        "　": " ",
        Ⅰ: "I",
        ⅰ: "i",
        Ⅱ: "II",
        ⅱ: "ii",
        Ⅲ: "III",
        ⅲ: "iii",
        Ⅳ: "IV",
        ⅳ: "iv",
        Ⅴ: "V",
        ⅴ: "v",
        Ⅵ: "VI",
        ⅵ: "vi",
        Ⅶ: "VII",
        ⅶ: "vii",
        Ⅷ: "VIII",
        ⅷ: "viii",
        Ⅸ: "IX",
        ⅸ: "ix",
        Ⅹ: "X",
        ⅹ: "x",
        Ⅺ: "XI",
        ⅺ: "xi",
        Ⅻ: "XII",
        ⅻ: "xii",
        Ⅼ: "L",
        ⅼ: "l",
        Ⅽ: "C",
        ⅽ: "c",
        Ⅾ: "D",
        ⅾ: "d",
        Ⅿ: "M",
        ⅿ: "m",
    };

    function latinizeString(str) {
        let result = "";
        for (const char of str) {
            result += REPLACEMENTS[char] ?? char;
        }
        return result;
    }

    const columns = [sourceText, ...translations];
    const idx = options.columnIndex;

    if (idx < 0 || idx >= columns.length || columns[idx] == null) {
        return columns.map(() => undefined);
    }

    const out = columns.map(() => undefined);
    out[idx] = latinizeString(columns[idx]);
    return out;
}
