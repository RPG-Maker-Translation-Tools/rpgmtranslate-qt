// This is a default RPGMTranslate JavaScript script. You're free to change it, add new functions, as long as rpgmLineCallback function exists and is not malformed.

/**
 * Your description.
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
    // Your implementation.
}
