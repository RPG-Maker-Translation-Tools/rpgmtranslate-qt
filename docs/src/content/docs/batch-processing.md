---
title: "Batch Processing"
---

Apply batch processing to the translation through the ![](./assets/construction.svg) button.

It runs a script over the text of the files picked in its file select menu (no file is selected by default), writing into the translation column you choose. Three scripts are built in:

- **Trim** - trims leading, trailing, or leading and trailing whitespace from the text.
- **Wrap** - wraps the text at a given character length (20-255). The "Use value from settings" button fills this in from the project's line length hint (see [Project settings](./settings.md#project)). See [How Wrap works](#how-wrap-works).
- **Latinize** - transliterates the text into Latin characters.

The same three scripts can be run on a single cell with the "Trim Cell", "Wrap Cell" and "Latinize Cell" shortcuts, see [Settings > Controls](./settings.md#controls).

## How Wrap works

Wrap approximates how the game draws text, since it cannot know every real width. Check the result against the line length hint.

- Line length is counted in characters. Existing line breaks are kept; only the lines that are too long get broken.
- RPG Maker codes and tags (`\C[2]`, `\fn<Comic Sans>`, `<br>`, plugin codes) take no width and are never split, even if an argument has spaces. Codes that draw game-dependent content (variable values, actor and party names, currency unit, icons) count as an estimated number of characters. Nested codes (`\N[\V[3]]`) count as one.
- Size codes (`\{`, `\}`, `\fs[n]`) are taken into account: characters after a bigger font size take more room, and after a smaller one - less. For this to be accurate, set the "Hint font size" in the [project settings](./settings.md#line-length-hint) to your game's base font size.
- Lines are broken at spaces. Japanese and Chinese text is broken between any two characters, but punctuation that must not start or end a line (such as `。`, `、`, `「`) is kept off the line edges. A single word longer than the line is not cut, it just overflows.
- Spaces left hanging at the end of a line are removed.
- A row is left untouched if it uses the engine's own `<WordWrap>` tag, or if wrapping it would take more than 4 lines (the message window shows 4). The rows left alone because of the line count are written to the application log.

## Custom scripts

Or write your own script in JavaScript (powered by [quickjs-ng](https://github.com/quickjs-ng/quickjs)). The new script button adds one from a template. Edit it in the script editor (the pencil button opens a larger window with syntax highlighting), and rename it in the script selector. Scripts are saved as `.js` files in the `scripts` directory of the [application data directory](./installation.md#runtime-data-files) when the project closes or the app exits. Any `.js` file placed there shows up in the selector.

Each callback call may run for 15 seconds at most (time spent waiting in `rpgm.exec` doesn't count), and a script may use up to 64 MiB of memory.

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
- `options` - `{ action, columnIndex, sourceLanguage, translationLanguage, ... }`; `action` is `"trim"`/`"wrap"`/`"latinize"`/`"custom"`, `columnIndex` is the translation column being written to, `sourceLanguage`/`translationLanguage` and `fontSize` are taken from the project settings, `glossary` is the array of enabled glossary terms as `{ term, translation, note }`, and `trimLeading`/`trimTrailing` (for trim) or `wrapLength` (for wrap) carry the built-in scripts' settings.

Return an array of strings to overwrite the translation column(s) - array element `i` maps to `translations[i]`. Returning `undefined`/`null` for an element (or a shorter array, or nothing) leaves that column unchanged.

### Processing a whole file at once

A script can define `rpgmFileCallback(filename, rows, options)` instead of `rpgmLineCallback` - defining both is an error. It is called once per file, which lets the script send many lines to a program in a single call:

- `rows` - every row of the file, in order, as `{ source, translations, lineNumber, isComment }`. Comment rows are visible but never rewritten.
- Return an array with one entry per row. Each entry is either a `translations`-shaped array (same meaning as the return value of `rpgmLineCallback`) or `undefined`/`null` to leave the row unchanged.

## What scripts can do

Besides plain JavaScript, a custom script can use:

- `rpgm.exec(program, args, { stdin, cwd, timeoutMs })` - runs a program and returns `{ code, stdout, stderr }`. The program is looked up in the `scripts` directory first (so put the programs a script calls beside it), then in `PATH`, and `cwd` is the `scripts` directory unless given. It throws if the program cannot start, is cancelled or exceeds `timeoutMs` (120000 by default). A non-zero exit code is returned, not thrown.
- `rpgm.readFile(path)` and `rpgm.writeFile(path, text)` - read and write UTF-8 text files.
- `rpgm.log(message)` - writes a line to the application log. When the batch finishes, the last 30 lines a script logged are also shown to you.

Because these can do anything your user account can, the first time a custom script runs you are asked to allow it. The answer is remembered for that exact script text; if you edit the script, you are asked again. The built-in scripts only get `rpgm.log`.

This is how AI models can be used: call a local server such as Ollama from a script (see below), or run a Python script that uses a library of your choice.

### Example: translating through Ollama

This is a script for a [Script translation endpoint](./settings.md#endpoints). It sends the text to a local Ollama server with `curl` and returns the answer. Change `model` to a model you have pulled.

```js
function rpgmTranslateCallback(
  text,
  sourceLanguage,
  translationLanguage,
  glossary,
  filename,
) {
  const terms = glossary
    .map((entry) => `${entry.term} = ${entry.translation}`)
    .join("\n");

  const request = {
    model: "gemma3",
    stream: false,
    messages: [
      {
        role: "system",
        content:
          `Translate the user's text from ${sourceLanguage} to ${translationLanguage}. ` +
          "Keep every \\code and [tag] unchanged. Answer with the translation only." +
          (terms ? `\nUse these terms:\n${terms}` : ""),
      },
      { role: "user", content: text },
    ],
  };

  const response = rpgm.exec(
    "curl",
    ["-sS", "http://localhost:11434/api/chat", "-d", "@-"],
    {
      stdin: JSON.stringify(request),
      timeoutMs: 120000,
    },
  );

  if (response.code !== 0) {
    throw new Error(response.stderr);
  }

  return JSON.parse(response.stdout).message.content.trim();
}
```

### Example: batch translation with OpenRouter and LiteLLM

[`examples/openrouter-litellm.js`](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/blob/main/examples/openrouter-litellm.js) translates whole files with any OpenRouter model. `rpgmFileCallback` collects the rows with an empty target column and sends them in chunks via `rpgm.exec` to a Python helper ([`openrouter_litellm.py`](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/blob/main/examples/openrouter_litellm.py), which holds the system prompt), which calls the model through [LiteLLM](https://github.com/BerriAI/litellm).

1. Install Python 3 and run `pip install litellm`.
2. Set the `OPENROUTER_API_KEY` environment variable before starting RPGMTranslate. The key is read by LiteLLM and is never stored in the script.
3. Set the source and translation languages in Settings > Project. Glossary terms are sent to the model with every request.
4. Copy the script to the `scripts` directory, put `openrouter_litellm.py` beside it (or anywhere you like, and set `PYTHON_MODULE` at the top of the script to its full path).
5. Open the Batch menu, pick the script and the column to translate into, and approve it when asked.

Change `MODEL` at the top of the script to use another model. The application does not track project context; to send extra data, fill `EXTRA_FIELDS` and describe the fields in the helper's system prompt. Only empty rows are translated, so after a failure or Cancel, running the script again continues where it stopped; the error goes to the application log.
