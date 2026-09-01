# CAT Features and Glossary

RPGMTranslate introduces some [CAT](https://en.wikipedia.org/wiki/Computer-assisted_translation) features.

## Glossary

Open the glossary menu with the ![](./assets/book_3.svg) button. It contains all the present terms in your translation, allows you to add new, and check the consistensy of those terms.

You can create new terms with the "+" button. Term and translation text are essential.

Glossary allows to search specific terms. Consistency checking itself now runs from the [lint menu](./text-editing.md#linting)'s "Global Check" button, alongside the other lints, rather than from a dedicated button here.

Individual terms can be disabled without deleting them from the "Lints" menu (same place plugin/tag lints are toggled) - a disabled term is kept in the glossary but excluded from linting.

[Inflections](https://en.wikipedia.org/wiki/Inflection) are not supported.

### Term/translation settings

#### Mode

Mode defines, how to match the text:

- Exact - only matches the text exactly.
- Fuzzy - only matches the text fuzzily, using a supplied threshold.
- Both - matches the text both exactly and fuzzily, in order.

#### Case sensitivity

If enabled, matches text case-sensitively. Else, case-insensitively.

#### Permissive

If enabled, also matches the text, that's more uppercased than the term, but not more lowercased.

For example, for a term "Aboba" - text "ABOBA" will match, but "aboba" won't.

### Note

All the info related to the term and its translation.

### Actions

- Edit: Glossary entries are uneditable by default.
- Remove: Remove the glossary entry.
