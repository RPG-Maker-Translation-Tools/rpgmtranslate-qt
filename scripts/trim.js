// Default RPGMTranslate JavaScript script. Edit freely, but keep rpgmLineCallback well-formed.

/**
 * Trim leading/trailing whitespace from a single column.
 *
 * @param {string} sourceText - Untranslated source text of this line.
 * @param {string[]} translations - Existing translation columns of this line, in order.
 * @param {string} filename - Name of the file being processed.
 * @param {number} lineNumber - Zero-based line index within the file.
 * @param {Object} options - Action-specific settings.
 * @param {("trim"|"wrap"|"latinize"|"custom")} options.action - Triggering action. Built-ins keep their fixed names; user scripts always get "custom".
 * @param {number} options.columnIndex - Index into [sourceText, ...translations] that the action targets.
 * @param {boolean} [options.trimLeading] - Present when action === "trim"; whether to strip leading whitespace.
 * @param {boolean} [options.trimTrailing] - Present when action === "trim"; whether to strip trailing whitespace.
 * @param {number} [options.wrapLength] - Present when action === "wrap"; max line length to wrap at.
 *
 * @returns {Array<string|undefined|null>} Shaped like `translations`: each entry replaces its column.
 *   `undefined`/`null` entries leave the column unchanged.
 */
function rpgmLineCallback(sourceText, translations, filename, lineNumber, options) {
    const columns = [sourceText, ...translations];

    const result = translations.map(() => undefined);

    if (options.columnIndex < 1 || columns.length <= options.columnIndex) {
        return result;
    }

    let text = columns[options.columnIndex];

    if (options.trimLeading) {
        text = text.replace(/^\s+/, "");
    }

    if (options.trimTrailing) {
        text = text.replace(/\s+$/, "");
    }

    result[options.columnIndex - 1] = text;
    return result;
}
