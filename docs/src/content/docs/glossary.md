---
title: "CAT Features and Glossary"
---

RPGMTranslate introduces some [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) features.

## Glossary

Open the glossary menu with the ![](./assets/book_3.svg) button. It contains all the terms of your translation, allows you to add new ones, and check the consistency of those terms. The glossary is stored per project, in `.rpgmtranslate/glossary.json`.

You can create new terms with the "+" button. Term and translation text are required.

The search input finds the next term whose term or translation contains the text (case-insensitively). Consistency checking runs from the [lint menu](./text-editing.md#linting)'s "Global Check" button with the other lints, not from a button here. It needs the source and translation languages set in [Settings > Project](./settings.md#project).

Individual terms can be disabled without deleting them from the "Lints" menu (same place plugin/tag lints are toggled) - a disabled term is kept in the glossary but excluded from linting and from the glossary passed to scripts.

Text is split into words for the project's languages. For the languages supported by the [Snowball stemmer](https://snowballstem.org/algorithms/) (most European languages, Arabic, Hindi, Indonesian, Turkish and more), words are reduced to their stems, so regular [inflections](https://en.wikipedia.org/wiki/Inflection) of a term (`sword`/`swords`) match it; irregular ones don't. Japanese, Chinese and Korean need [dictionaries](./settings.md#cjk-text-matching).

### Term/translation settings

The term and its translation each have their own settings.

#### Mode

Mode defines, how to match the text:

- Exact - only matches the text exactly.
- Fuzzy - only matches the text fuzzily, using a threshold from 0 to 1 (0.8 by default). The higher it is, the closer the text has to be.
- Both - matches the text both exactly and fuzzily, in order.

#### Case sensitivity

If enabled, matches text case-sensitively. Else, case-insensitively.

#### Permissive

If enabled, also matches the text, that's more uppercased than the term, but not more lowercased.

For example, for a term "Aboba" - text "ABOBA" will match, but "aboba" won't.

### Note

Info about the term and its translation.

### Actions

- Edit: Glossary entries are uneditable by default. Toggle this button to edit an entry, and toggle it again to finish - an empty term or translation is refused.
- Remove: Remove the glossary entry, after a confirmation.
