# Batch Processing

RPGMTranslate allows to easily apply some batch processing to the translation. Access it through the ![](./assets/construction.svg) button.

Pick which files to run on with the file-select menu, then choose an action:

## Translate

Runs machine translation (using your configured [translation endpoint](./settings.md#translation)) over the selected files' source text, writing the result into the chosen translation column.

## Script

Runs a script over the selected files' text instead of translating it. Three scripts are built in:

- **Trim** - trims whitespace from the text.
- **Wrap** - wraps the text at a given character length. The "Use value from settings" button fills this in from the project's line length hint (see [Project settings](./settings.md#project)).
- **Latinize** - transliterates the text into Latin characters.

## Custom scripts

You can write your own script instead, using JavaScript (powered by [quickjs-ng](https://github.com/quickjs-ng/quickjs)). Custom scripts are saved to the `scripts` directory alongside the application, so they persist across sessions and are easy to share.

A script must define a function:

```js
function rpgmLineCallback(
  sourceText,
  translations,
  filename,
  lineNumber,
  options,
) {
  // ...
  return translations;
}
```

It's called once per translatable line:

- `sourceText` - the line's source text.
- `translations` - an array with the line's existing translation column(s).
- `filename` - the file the line belongs to.
- `lineNumber` - the line's index within that file.
- `options` - `{ action, columnIndex, ... }`; `action` is `"trim"`/`"wrap"`/`"latinize"`/`"custom"`, `columnIndex` is the translation column being written to, and `trimLeading`/`trimTrailing` (for trim) or `wrapLength` (for wrap) carry the built-in scripts' settings.

Return an array of strings to overwrite the translation column(s) - array element `i` maps to `translations[i]`. Returning `undefined`/`null` for an element (or a shorter array) leaves that column unchanged.
