use std::borrow::Cow;
use text_stats::Counts;

/// Counts of a piece of text, as returned over FFI.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct TextCounts {
    pub words: u32,
    pub characters: u32,
    pub characters_no_whitespace: u32,
}

impl From<Counts> for TextCounts {
    fn from(counts: Counts) -> Self {
        Self {
            words: counts.words,
            characters: counts.characters,
            characters_no_whitespace: counts.characters_no_whitespace,
        }
    }
}

/// Counts the words and characters a player will read in `text`: the `\#` line break marker becomes a newline, and
/// RPG Maker codes (`\C[2]`, `\fn<Comic Sans>`, `\{`, `\.`) and HTML-like tags (`<br>`) are left out.
pub(crate) fn count_text(text: &str) -> TextCounts {
    text_stats::count(&strip_codes(text)).into()
}

/// A code is a backslash followed by a run of letters with an optional `[...]`, `<...>` or `{...}` argument, or by a
/// single symbol, exactly as the engine reads it (the letter run is greedy).
fn strip_codes(text: &str) -> Cow<'_, str> {
    if !text.contains(['\\', '<']) {
        return Cow::Borrowed(text);
    }

    let mut out = String::with_capacity(text.len());
    let mut rest = text;

    while let Some(at) = rest.find(['\\', '<']) {
        out.push_str(&rest[..at]);
        rest = &rest[at..];

        let skipped = if rest.starts_with('\\') {
            code_len(rest)
        } else {
            tag_len(rest)
        };

        match skipped {
            // The line break marker is the only code that is not left out.
            Some(len) if rest.starts_with("\\#") => {
                out.push('\n');
                rest = &rest[len..];
            }
            Some(len) => rest = &rest[len..],
            None => {
                // Not a code: a lone backslash or `<` is ordinary text.
                out.push_str(&rest[..1]);
                rest = &rest[1..];
            }
        }
    }

    out.push_str(rest);

    Cow::Owned(out)
}

/// Length in bytes of the code at the start of `text` (which starts with a backslash).
fn code_len(text: &str) -> Option<usize> {
    let after = &text[1..];
    let first = after.chars().next()?;

    if !first.is_ascii_alphabetic() {
        // `\#`, `\{`, `\.`, `\\` ... but not `\` followed by a digit or the end of the text.
        return (!first.is_ascii_digit()).then(|| 1 + first.len_utf8());
    }

    let letters = after
        .find(|char: char| !char.is_ascii_alphabetic())
        .unwrap_or(after.len());
    let mut len = 1 + letters;

    match after[letters..].chars().next() {
        // The argument of a code may be a code itself (`\N[\V[3]]`), so the closing bracket is the one that balances.
        Some('[') => {
            let mut depth = 0;

            for (idx, char) in text[len..].char_indices() {
                match char {
                    '[' => depth += 1,
                    ']' => {
                        depth -= 1;

                        if depth == 0 {
                            len += idx + 1;
                            break;
                        }
                    }
                    _ => {}
                }
            }
        }
        Some('<') => len += text[len..].find('>').map_or(0, |end| end + 1),
        Some('{') => len += text[len..].find('}').map_or(0, |end| end + 1),
        _ => {}
    }

    Some(len)
}

/// Length in bytes of the HTML-like tag (`<br>`, `</i>`, `<WordWrap>`) at the start of `text` (which starts with `<`).
fn tag_len(text: &str) -> Option<usize> {
    let after = text[1..].strip_prefix('/').unwrap_or(&text[1..]);

    if !after.starts_with(|char: char| char.is_ascii_alphabetic()) {
        return None;
    }

    // A second `<` before the closing `>` means this one is not a tag.
    let end = text[1..].find(['<', '>']).map(|idx| idx + 1)?;

    (text.as_bytes()[end] == b'>').then_some(end + 1)
}
