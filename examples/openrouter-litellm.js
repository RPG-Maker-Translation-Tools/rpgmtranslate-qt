// Example batch script: translates the selected files through an OpenRouter model, using a Python helper and LiteLLM.
//
// rpgmFileCallback runs once per file: it collects the rows whose target column is empty, sends them to Python in
// chunks (rpgm.exec), and openrouter_litellm.py (next to this file) asks the model through LiteLLM.
//
// Setup:
//   1. Install Python 3 and run `pip install litellm`.
//   2. Create an OpenRouter API key and set it in OPENROUTER_API_KEY *before* starting RPGMTranslate. LiteLLM reads
//      it; this script never stores it.
//   3. Set the project's source and translation languages in Settings > Project. Terms from the glossary menu are
//      sent to the model with every request.
//   4. Copy this file to the `scripts` directory and put openrouter_litellm.py beside it (or anywhere you like; then set
//      PYTHON_MODULE below to its full path). Optionally fill EXTRA_FIELDS with project context.
//   5. Open Batch menu, pick this script, and the column to translate into. The first run asks you to approve the
//      script, because it starts programs.
//
// Only rows with an empty target column are translated, so rerunning is safe: on a failed request or Cancel, what is
// translated so far in the file is kept, the error goes to the application log, and the next run continues.

// Any model name LiteLLM understands; for OpenRouter it is "openrouter/<provider>/<model>".
const MODEL = "openrouter/google/gemma-4-31b-it:free";

// Rows per request. Lower it for small-context models, raise it to save requests.
const CHUNK_SIZE = 40;

// Maximum time for one request, in milliseconds.
const REQUEST_TIMEOUT_MS = 300000;

// Python helper, which holds the system prompt. A relative path is relative to the `scripts` directory. For another
// location, use a full path with forward slashes: "C:/Users/me/rpgmtranslate/openrouter_litellm.py".
// The first-run approval covers only this file, not the helper.
const PYTHON_MODULE = "openrouter_litellm.py";

// Extra context for the model: project context, a character list, style rules... Every non-empty field is sent with
// each request, next to `texts`.
//
// The app does not track project context, so write it here. Describe any new field in SYSTEM_PROMPT in
// openrouter_litellm.py (`projectContext` already is). For per-file data, add it to the `request` object in
// rpgmFileCallback instead.
const EXTRA_FIELDS = {
    projectContext: "",
    // For example: "A grimdark fantasy RPG. Characters speak formally. The hero is called Aldric."
};

// "python" on Windows, usually "python3" on Linux; the one that starts is remembered.
let pythonProgram = null;

function runPython(job) {
    const options = { stdin: JSON.stringify(job), timeoutMs: REQUEST_TIMEOUT_MS };
    const candidates = pythonProgram ? [pythonProgram] : ["python", "python3"];

    for (const program of candidates) {
        let result;

        try {
            result = rpgm.exec(program, [PYTHON_MODULE], options);
        } catch (error) {
            // Only "program not found" moves on to the next candidate; cancellation and timeouts do not.
            if (String(error.message).startsWith("Cannot start process")) {
                continue;
            }

            throw error;
        }

        pythonProgram = program;

        if (result.code !== 0) {
            throw new Error(result.stderr.trim() || "Python exited with code " + result.code);
        }

        return JSON.parse(result.stdout);
    }

    throw new Error("Python was not found. Install Python 3 and make sure it is in PATH.");
}

function nonEmptyFields(fields) {
    return Object.fromEntries(Object.entries(fields).filter(([, value]) => value !== "" && value != null));
}

function rpgmFileCallback(filename, rows, options) {
    if (!options.sourceLanguage || !options.translationLanguage) {
        throw new Error("Set the source and translation languages in Settings > Project first.");
    }

    // Index into `translations`; options.columnIndex counts the source text as column 0.
    const column = options.columnIndex - 1;
    const results = rows.map(() => undefined);

    const pending = [];

    rows.forEach((row, index) => {
        if (!row.isComment && row.source.trim() !== "" && row.translations[column] === "") {
            pending.push(index);
        }
    });

    let done = 0;

    for (let start = 0; start < pending.length; start += CHUNK_SIZE) {
        const chunk = pending.slice(start, start + CHUNK_SIZE);

        let translated;

        try {
            translated = runPython({
                model: MODEL,
                request: {
                    sourceLanguage: options.sourceLanguage,
                    translationLanguage: options.translationLanguage,
                    glossary: options.glossary,
                    filename,
                    ...nonEmptyFields(EXTRA_FIELDS),
                    texts: chunk.map((index) => rows[index].source),
                },
            });
        } catch (error) {
            // Keep what is already translated; the next run continues from here.
            rpgm.log(filename + ": stopped after " + done + " of " + pending.length + " rows: " + error.message);
            break;
        }

        chunk.forEach((index, position) => {
            const translations = [];
            translations[column] = translated[position];
            results[index] = translations;
        });

        done += chunk.length;
    }

    rpgm.log(filename + ": translated " + done + " of " + pending.length + " rows.");

    return results;
}
