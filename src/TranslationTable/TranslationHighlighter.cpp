#include "TranslationHighlighter.hpp"

#include "PluginSequences.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QColor>
#include <QFile>
#include <QTextCharFormat>

TranslationHighlighter::TranslationHighlighter(
    const TranslationSettings* const translationSettings,
#ifdef ENABLE_NUSPELL
    const nuspell::Dictionary* const dictionary,
    const bool* const dictionaryReady,
#endif
    QTextDocument* const document
) :
    QSyntaxHighlighter(document),
    translationSettings(translationSettings)
#ifdef ENABLE_NUSPELL
    ,
    dictionary(dictionary),
    isDictionaryReady(dictionaryReady)
#endif
{
}

// Getting colors for \C[n] from img/system/Window.png
constexpr u8 WINDOW_COLOR_OFFSET_X = 96;
constexpr u8 WINDOW_COLOR_OFFSET_Y = 144;
// Color square takes 12x12 space
constexpr u8 WINDOW_COLOR_SQUARE_SIZE = 12;
// 8 colors per row
constexpr u8 WINDOW_COLOR_ROW_COUNT = 8;
constexpr u8 WINDOW_COLOR_COUNT = 32;

constexpr u8 ICONSET_OLDER_ICON_SIZE = 24;
constexpr u8 ICONSET_NEWER_ICON_SIZE = 32;
constexpr u8 ICONSET_ROW_ICON_COUNT = 16;

// For \V[n], we need to get variables from System file
// For \P[n], there's no way to get the value. This is runtime-controlled,
// however we can probably extract party member names from the save file

// For \G, we need to get currency unit from System file

// For \I[n], we need to get the icon from img/system/IconSet.rpgmvp. One icon
// is 32 px, 16 icons per row.

// For \af[x], we need to get the actor face from img/faces.
// For \ac[x], we need to get the actor class from Actors file
// For \an[x], we need to get the actor nickname from Actors file

// For \pf[x], \pc[x] and \pn[x], there's no way to get the value. This is
// runtime-controlled, however we can probably extract party member names from
// the save file

// For \nc[x], \ni[x], \nw[x], \na[x], \ns[x] and \nt[x], we need to get the
// respective value from the data file.
// For \ii[x], \iw[x], \ia[x], \is[x], \it[x], we need to get the respective
// value from the data file AND also the icon.

[[nodiscard]] auto getWindowColors(const QString& windowImagePath)
    -> result<array<QRgb, WINDOW_COLOR_COUNT>, QString> {
    auto file = QFile(windowImagePath);

    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file %1: %2"_L1.arg(
            windowImagePath,
            file.errorString()
        );

        return Err(
            QObject::tr("Failed to open file %1: %2")
                .arg(windowImagePath, file.errorString())
        );
    }

    const QByteArray imageData = file.readAll();
    const QImage img = QImage::fromData(imageData);

    array<QRgb, WINDOW_COLOR_COUNT> colors;

    for (const auto row : range(0, 4)) {
        for (const auto col : range(0, 8)) {
            colors[(row * col) + col] = img.pixel(QPoint(
                WINDOW_COLOR_OFFSET_X + (col * WINDOW_COLOR_SQUARE_SIZE),
                WINDOW_COLOR_OFFSET_Y + (row * WINDOW_COLOR_SQUARE_SIZE)
            ));
        }
    }

    return colors;
}

[[nodiscard]] auto getIcon(
    const QString& iconsetPath,
    const u32 iconIndex,
    const EngineType engineType
) -> result<QImage, QString> {
    u8 iconSize;

    if (engineType == EngineType::New) {
        iconSize = ICONSET_NEWER_ICON_SIZE;
    } else {
        iconSize = ICONSET_OLDER_ICON_SIZE;
    }

    auto file = QFile(iconsetPath);

    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file %1: %2"_L1.arg(
            iconsetPath,
            file.errorString()
        );

        return Err(file.errorString());
    }

    const QByteArray imageData = file.readAll();
    ByteBuffer out;

    const QString extension =
        iconsetPath.sliced(iconsetPath.lastIndexOf(u'.') + 1).toLower();

    if (extension == "rpgmvp"_L1 || extension == "png_"_L1) {
        const QByteArray pathUtf8 = iconsetPath.toUtf8();

        const bool success = rpgm_decrypt_asset(
            FFIString{ .ptr = pathUtf8.data(), .len = u32(pathUtf8.size()) },
            &out
        );

        if (!success) {
            return Err(ffitostr(rpgm_error()).toString());
        }
    }

    const QImage img = extension == "rpgmvp"_L1 || extension == "png_"_L1
                           ? QImage::fromData(out.ptr, i32(out.len))
                           : QImage::fromData(imageData);

    u32 offset = (iconSize * iconIndex);
    const u32 boundedOffset = offset % img.width();
    u32 row = 0;

    while (offset >= img.width()) {
        offset -= img.width();
        row++;
    }

    return img.copy(QRect(boundedOffset, row, iconSize, iconSize));
}

void TranslationHighlighter::highlightBlock(const QString& text) {
    if (text.isEmpty()) {
        return;
    }

    const u32 size = text.size();
    QTextCharFormat fmt;

#ifdef ENABLE_NUSPELL
    if (dictionary != nullptr && isDictionaryReady != nullptr &&
        *isDictionaryReady) {
        fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        fmt.setUnderlineColor(Qt::red);

        const auto matches = wordRegex.globalMatchView(text);

        for (const auto& match : matches) {
            const QString word = match.captured();

            const QByteArray utf8Word = word.toUtf8();
            const bool misspelled = !dictionary->spell(
                string_view(utf8Word.data(), utf8Word.size())
            );

            if (misspelled) {
                const i32 start = i32(match.capturedStart());
                const i32 len = i32(match.capturedLength());

                vector<string> suggestions;
                dictionary->suggest(
                    string_view(utf8Word.data(), utf8Word.size()),
                    suggestions
                );

                QList<QString> qsuggestions;

                for (const auto& suggestion : suggestions) {
                    qsuggestions.push_back(QString::fromUtf8(suggestion));
                }

                fmt.setToolTip(tr("Nuspell: Misspelled word. Suggestions: %1")
                                   .arg(qsuggestions.join(", "_L1)));

                setFormat(start, len, fmt);
            }
        }
    }
#endif

    // TODO: Use settings->translation.whitespaceCharacters
    fmt.setBackground(QColor(255, 0, 0, 80));

    u32 lead = 0;

    if ((translationSettings->miscLints & MiscLints::LeadingWhitespace) != 0) {
        while (lead < size && text.at(lead).isSpace()) {
            lead++;
        }

        if (lead > 0) {
            fmt.setToolTip(u"Misc: Leading whitespace."_s);
            setFormat(0, lead, fmt);
        }
    }

    if ((translationSettings->miscLints & MiscLints::TrailingWhitespace) != 0) {
        i32 lastNonSpace = size - 1;

        while (lastNonSpace >= 0 && text.at(lastNonSpace).isSpace()) {
            lastNonSpace--;
        }

        const u32 trailStart = lastNonSpace + 1;
        const u32 trailLen = size - trailStart;

        if (trailLen > 0 && trailStart >= lead) {
            fmt.setToolTip(u"Misc: Trailing whitespace."_s);
            setFormat(trailStart, trailLen, fmt);
        }
    }

    if ((translationSettings->miscLints & MiscLints::ContiguousWhitespace) !=
        0) {
        u32 idx = 0;

        while (idx < size) {
            if (text.at(idx) == u' ') {
                const u32 start = idx;

                while (idx < size && text.at(idx) == u' ') {
                    idx++;
                }

                const u32 len = idx - start;

                if (len >= 2) {
                    fmt.setToolTip(u"Misc: More than 2 consecutive spaces."_s);
                    setFormat(start, len, fmt);
                }
            } else {
                idx++;
            }
        }
    }

    if ((translationSettings->miscLints & MiscLints::UnclosedPunctuation) !=
        0) {
        struct Punctuation {
            QChar open;
            QChar close;
            bool symmetric;
        };

        // TODO: Not a full list
        constexpr array<Punctuation, 15> puncts = {
            // ASCII
            Punctuation{ .open = u'(', .close = u')', .symmetric = false },
            { .open = u'[', .close = u']', .symmetric = false },
            { .open = u'{', .close = u'}', .symmetric = false },
            { .open = u'"', .close = u'"', .symmetric = true },
            { .open = u'\'', .close = u'\'', .symmetric = true },

            // European
            { .open = u'«', .close = u'»', .symmetric = false },
            { .open = u'‹', .close = u'›', .symmetric = false },
            { .open = u'“', .close = u'”', .symmetric = false },
            { .open = u'‘', .close = u'’', .symmetric = false },

            // CJK
            { .open = u'「', .close = u'」', .symmetric = false },
            { .open = u'『', .close = u'』', .symmetric = false },
            { .open = u'（', .close = u'）', .symmetric = false },
            { .open = u'【', .close = u'】', .symmetric = false },
            { .open = u'《', .close = u'》', .symmetric = false },
            { .open = u'〈', .close = u'〉', .symmetric = false },
        };

        struct StackEntry {
            u32 pos;
            QChar expectedClose;
        };

        vector<StackEntry> stack;

        const auto findPunctuation =
            [puncts](const QChar chr) -> optional<Punctuation> {
            for (const auto punctuation : puncts) {
                if (punctuation.open == chr || punctuation.close == chr) {
                    return punctuation;
                }
            }

            return nullopt;
        };

        for (const auto idx : range(0, size)) {
            const QChar chr = text.at(idx);
            const optional<Punctuation> punctuation = findPunctuation(chr);

            if (!punctuation) {
                continue;
            }

            if (punctuation->symmetric) {
                if (!stack.empty() && stack.back().expectedClose == chr) {
                    stack.pop_back();
                } else {
                    stack.push_back({ .pos = u32(idx), .expectedClose = chr });
                }
            } else if (chr == punctuation->open) {
                stack.push_back(
                    { .pos = u32(idx), .expectedClose = punctuation->close }
                );
            } else if (chr == punctuation->close) {
                if (!stack.empty() && stack.back().expectedClose == chr) {
                    stack.pop_back();
                } else {
                    fmt.setToolTip(u"Misc: Unmatched closing punctuation."_s);
                    setFormat(idx, 1, fmt);
                }
            }
        }

        for (const auto entry : stack) {
            fmt.setToolTip(u"Misc: Unclosed punctuation."_s);
            setFormat(entry.pos, 1, fmt);
        }
    }

    fmt.setUnderlineStyle(QTextCharFormat::NoUnderline);
    fmt.setBackground(QBrush());
    fmt.setForeground(Qt::red);

    const auto matches = TextCodes::REGEXP.globalMatchView(text);

    // TODO: Color the text after \C[x>0] tags

    for (const auto& match : matches) {
        for (const auto idx : range(1, match.lastCapturedIndex() + 1)) {
            if (match.capturedLength(idx) > 0) {
                fmt.setToolTip(tr(TextCodes::TOOLTIPS[idx - 1]));
                fmt.setProperty(69, match.captured(idx));
                setFormat(
                    match.capturedStart(idx),
                    match.capturedLength(idx),
                    fmt
                );
                break;
            }
        }
    }
}
