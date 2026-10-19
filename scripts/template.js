// Default RPGMTranslate JavaScript script. Edit freely, but keep rpgmLineCallback or rpgmFileCallback well-formed.
// Define only one of the two, not both.

/**
 * Your description.
 *
 * @param {string} sourceText - Untranslated source text of this line.
 * @param {string[]} translations - Existing translation columns of this line, in order.
 * @param {string} filename - Name of the file being processed.
 * @param {number} lineNumber - Zero-based line index within the file.
 * @param {Object} options - Action-specific settings.
 * @param {("trim"|"wrap"|"latinize"|"custom")} options.action - Triggering action. Built-ins keep their fixed names; user scripts always get "custom".
 * @param {number} options.columnIndex - Index into [sourceText, ...translations] that the action targets.
 * @param {number} options.fontSize - Base message font size in pixels, from project settings (0: unset).
 * @param {string} options.sourceLanguage - Source language from Settings > Project (may be empty).
 * @param {string} options.translationLanguage - Translation language from Settings > Project (may be empty).
 * @param {Array<{term: string, translation: string, note: string}>} options.glossary - Enabled glossary terms.
 * @param {boolean} [options.trimLeading] - Present when action === "trim"; whether to strip leading whitespace.
 * @param {boolean} [options.trimTrailing] - Present when action === "trim"; whether to strip trailing whitespace.
 * @param {number} [options.wrapLength] - Present when action === "wrap"; max line length to wrap at.
 *
 * @returns {Array<string|undefined|null>} Shaped like `translations`: each entry replaces its column.
 *   `undefined`/`null` entries, a shorter array or none leave the column unchanged.
 */
function rpgmLineCallback(sourceText, translations, filename, lineNumber, options) {
    // Your implementation.
}

/**
 * Called once per file instead of per line, to send many lines to a program in one call.
 *
 * @param {string} filename - Name of the file being processed.
 * @param {Array<{source: string, translations: string[], lineNumber: number, isComment: boolean}>} rows - Every row of the file, in order. Comment rows are visible but never rewritten.
 * @param {Object} options - Same as in rpgmLineCallback.
 *
 * @returns {Array<string[]|undefined|null>} One entry per row: a `translations`-shaped array as in rpgmLineCallback, or `undefined`/`null` to leave the row unchanged.
 *
 * Approved scripts can also use rpgm.exec(program, args, { stdin, cwd, timeoutMs }) -> { code, stdout, stderr },
 * rpgm.readFile(path), rpgm.writeFile(path, text) and rpgm.log(message).
 */
// function rpgmFileCallback(filename, rows, options) {
//     return rows.map(() => undefined);
// }
