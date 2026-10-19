"""Helper for openrouter-litellm.js: sends one translation request to a model through LiteLLM.

Reads {"model": ..., "request": {...}} as JSON from stdin. `request` is sent to the model as the user message, and
`request["texts"]` is the list of strings to translate. Prints the translated strings as a JSON array to stdout.
On failure, prints the error to stderr and exits with code 1.
"""

import json
import re
import sys

import litellm

# Pipes use the console codepage on Windows by default.
sys.stdin.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

# The model only knows what is written here. If you add fields to the request (see EXTRA_FIELDS in
# openrouter-litellm.js), describe them in the "Input" list below, otherwise the model does not know what they mean.
SYSTEM_PROMPT = r"""You are a professional video game localizer working on an RPG Maker game. Translate the text from sourceLanguage to translationLanguage.

Input: a JSON object with
- sourceLanguage, translationLanguage: BCP-47 language tags.
- glossary: required terms as {term, translation, note}; may be empty. Always use them exactly.
- filename: where the text is used (map, system, items, ...).
- projectContext: tone, lore and style notes for the whole game. May be absent.
- texts: the array of strings to translate.

Rules:
- Keep the meaning, tone and gameplay function. Use natural, concise phrasing that fits game windows and menus.
- Follow projectContext if present. Stay consistent across all texts.
- Keep all codes and markup exactly as written and in the same place: backslash codes such as \X, \X[...], \X<...>, \X{...}, tags such as <...>, placeholders such as %s or %d, and anything else that looks like a variable, command or condition. Translate only the human-readable text around them.
- Return exactly one string per input string, in the same order. Never add, remove, merge or split items.

Output: only a JSON array of the translated strings. No explanations."""

ATTEMPTS = 3


def parse(reply, count):
    # Some models put their reasoning before the answer.
    reply = re.sub(r"<think>.*?</think>", "", reply, flags=re.DOTALL)
    start, end = reply.find("["), reply.rfind("]")
    items = json.loads(reply[start : end + 1])

    if not isinstance(items, list) or len(items) != count or not all(isinstance(item, str) for item in items):
        raise ValueError("expected a JSON array of %d strings" % count)

    return items


job = json.load(sys.stdin)
request = job["request"]
error = None

for _ in range(ATTEMPTS):
    try:
        response = litellm.completion(
            model=job["model"],
            messages=[
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": json.dumps(request, ensure_ascii=False)},
            ],
            temperature=0.2,
            timeout=240,
        )
        # ensure_ascii keeps stdout pure ASCII, whatever the console codepage is.
        print(json.dumps(parse(response.choices[0].message.content, len(request["texts"]))))
        sys.exit(0)
    except Exception as exc:
        error = exc

print("Translation failed: %s" % error, file=sys.stderr)
sys.exit(1)
