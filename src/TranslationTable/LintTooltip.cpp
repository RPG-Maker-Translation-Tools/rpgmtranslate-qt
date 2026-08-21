#include "LintTooltip.hpp"

#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QScreen>
#include <QTextDocument>
#include <QUrlQuery>
#include <QWheelEvent>

namespace {
constexpr i32 Y_MARGIN = 8;
constexpr i32 H_PADDING = 8;
constexpr i32 V_PADDING = 4;
constexpr i32 MAX_HEIGHT = 160;
constexpr i32 MAX_WIDTH = 360;
constexpr i32 SCROLLBAR_WIDTH = 4;
constexpr i32 COLOR_SWATCH_SIZE = 12;

constexpr void appendSegment(char* const buf, u32& pos, const Segment seg) {
    if (pos != 0) {
        buf[pos++] = '\x04';
    }

    if (seg.type == SegmentType::String) {
        buf[pos++] = '\x01';
        memcpy(buf + pos, seg.l1sv.data(), seg.l1sv.size());
        pos += scast<u32>(seg.l1sv.size());
    } else if (seg.type == SegmentType::Index) {
        buf[pos++] = '\x03';
        for (const QChar chr : seg.qsv) {
            buf[pos++] = chr.toLatin1();
        }
    } else {
        buf[pos++] = '\x01';

        for (const auto name : seg.keys) {
            memcpy(buf + pos, name.data(), name.size());
            pos += scast<u32>(name.size());
            buf[pos++] = '\x02';
        }

        buf[pos - 1] = '\0';
    }
}

[[nodiscard]] inline auto parseCaptured(const QString& str, const i32 start, const i32 num) -> Segment {
    return QStringView(str).sliced(start).chopped(num);
}

// Builds `[suggestion](lint://apply-suggestion?...)` links TranslationInput's linkActivated
// handler routes back into applySuggestion() to replace the underlined range in-place.
[[nodiscard]] inline auto
suggestionLinksMarkdown(const QStringList& suggestions, const LintType type, const i32 lintIndex) -> QString {
    QStringList links;
    links.reserve(suggestions.size());

    for (i32 suggIdx = 0; suggIdx < suggestions.size(); suggIdx++) {
        QUrl url;
        url.setScheme(u"lint"_s);
        url.setHost(u"apply-suggestion"_s);
        QUrlQuery query;
        query.addQueryItem(u"type"_s, QString(itos(scast<u8>(type)).qsv()));
        query.addQueryItem(u"lint"_s, QString(itos(lintIndex).qsv()));
        query.addQueryItem(u"index"_s, QString(itos(suggIdx).qsv()));
        url.setQuery(query);

        links << u"[%1](%2)"_qsv.arg(suggestions[suggIdx], url.toString());
    }

    return links.join(u", ");
}

[[nodiscard]] inline auto getPath(const QStringView filename, const ProjectSettings* sets) -> QByteArray {
    return (sets->actualSourcePath() % u'/' % filename % u'.' % sets->engineExtension()).toUtf8();
}

struct EntityKind {
    const QStringView file;
    const RPGMFileType type;
};

[[nodiscard]] constexpr auto entityKindFor(const i32 lintIndex) -> EntityKind {
    // NOLINTBEGIN(readability-magic-numbers)
    switch (lintIndex) {
        case 63:
            return { .file = u"Classes", .type = RPGMFileType::Classes };
        case 64:
        case 69:
            return { .file = u"Items", .type = RPGMFileType::Items };
        case 65:
        case 70:
            return { .file = u"Weapons", .type = RPGMFileType::Weapons };
        case 66:
        case 71:
            return { .file = u"Armors", .type = RPGMFileType::Armors };
        case 67:
        case 72:
            return { .file = u"Skills", .type = RPGMFileType::Skills };
        case 68:
        case 73:
            return { .file = u"States", .type = RPGMFileType::States };
        case 83:
            return { .file = u"Enemies", .type = RPGMFileType::Enemies };
        default:
            std::unreachable();
    }
    // NOLINTEND(readability-magic-numbers)
}
}  // namespace

template <typename... Segments>
    requires(std::same_as<Segments, Segment> && ...)
constexpr auto LintTooltip::buildPath(Segments&&... segments) -> FFIString {
    u32 pos = 0;
    (appendSegment(buf.data(), pos, std::forward<Segments>(segments)), ...);
    buf[pos] = '\0';
    return strtoffi(buf.data());
}

LintTooltip::LintTooltip(QWidget* const parent) : QWidget(parent) {
    timer.setSingleShot(true);
    timer.setInterval(TOOLTIP_DELAY_MS);
    connect(&timer, &QTimer::timeout, this, &LintTooltip::hide_);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setMouseTracking(true);
}

void LintTooltip::processTextCode(QString& markdownSource, const LintEntry& lint, const ProjectSettings* const sets) {
    const auto releaseOut = [this] -> void {
        for (auto& entry : out) {
            rpgm_string_free(entry);
            entry = FFIString{};
        }
    };

    const auto fetchEntity = [&](const QStringView folder, const RPGMFileType type, const FFIString& key) -> bool {
        // key may point into buf, never into out, so releasing first is safe -
        // and necessary, since a case can fetch twice into the same slots.
        releaseOut();

        const QByteArray path = getPath(folder, sets);
        if (!rpgm_get_entity_data(strtoffi(path), sets->engineType, type, key, out.data())) {
            markdownSource += tr("Failed to fetch data: %1").arg(svtostr(ffitostr(rpgm_error())));
            return false;
        }
        return true;
    };

    const auto fetchIcon = [&](const i32 iconIndex) -> optional<QString> {
        const auto result = getIcon(iconIndex, sets->projectPath, sets->engineType);

        if (!result) {
            markdownSource += tr("Failed to fetch icon: %1").arg(result.error().translated());
            return std::nullopt;
        }

        return insertPixmap(result.value());
    };

    const auto colorSwatch = [&](const QRgb color) -> QString {
        auto pixmap = QPixmap(COLOR_SWATCH_SIZE, COLOR_SWATCH_SIZE);
        pixmap.fill(color);
        return insertPixmap(pixmap);
    };

    // NOLINTBEGIN(readability-magic-numbers)
    switch (lint.index) {
        case 27: {
            const FFIString keyUtf8 = buildPath(Segment("variables"_L1), parseCaptured(lint.captured, 3, 1));
            if (!fetchEntity(u"System", RPGMFileType::System, keyUtf8)) {
                break;
            }

            markdownSource += tr("The value of this variable will be displayed: %1").arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 28:
        case 57:
        case 97:
        case 103:
        case 119:
        case 120:
        case 122:
        case 123:
        case 125:
        case 126: {
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 3, 1), Segment("name"_L1));
            if (!fetchEntity(u"Actors", RPGMFileType::Actors, keyUtf8)) {
                break;
            }

            const char* tem;

            switch (lint.index) {
                case 28:
                    tem = QT_TR_NOOP("This pattern will be replaced with: %1");
                    break;
                default:
                    tem = QT_TR_NOOP("Actor's name: %1");
                    break;
            }

            markdownSource += tr(tem).arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 30: {
            const FFIString keyUtf8 = buildPath(Segment("currency_unit"_L1));
            if (!fetchEntity(u"System", RPGMFileType::System, keyUtf8)) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 31: {
            const QString err = lint.extra.toString();

            if (err.isEmpty()) {
                markdownSource += tr("The following color will take effect: %1").arg(colorSwatch(lint.extra.toUInt()));
            } else {
                markdownSource += tr("Failed to fetch data: %1").arg(err);
            }

            break;
        }
        case 32: {
            const auto icon = fetchIcon(stoa<i32>(parseCaptured(lint.captured, 3, 1).qsv));

            if (!icon) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(insertPixmap(icon.value()));
            break;
        }
        case 43: {
            const u32 frameNum = stoa<u32>(parseCaptured(lint.captured, 3, 1).qsv);
            markdownSource += tr("Here, the game will wait %n frame(s) (%1 sec.)", nullptr, scast<i32>(frameNum))
                                  .arg(ftos(scast<f32>(frameNum) / 60, 3).qsv());
            break;
        }
        case 58: {
            const FFIString keyUtf8 =
                buildPath(parseCaptured(lint.captured, 4, 1), Segment("name"_L1), Segment("class_id"_L1));

            if (!fetchEntity(u"Actors", RPGMFileType::Actors, keyUtf8)) {
                break;
            }

            if (!fetchEntity(
                    u"Classes",
                    RPGMFileType::Classes,
                    buildPath(Segment(QL1SV(out[1].ptr, out[1].len)), Segment("name"_L1), Segment("1"_L1))
                )) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1").arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 59: {
            static constexpr array<QL1SV, 2> keys = { "name"_L1, "nickname"_L1 };
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 4, 1), Segment(keys));
            if (!fetchEntity(u"Actors", RPGMFileType::Actors, keyUtf8)) {
                break;
            }

            markdownSource +=
                tr("This pattern will be replaced with: %2").arg(svtostr(ffitostr(out[0])), svtostr(ffitostr(out[1])));
            break;
        }
        case 63:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 83:
        case 99:
        case 105: {
            const auto [file, type] = entityKindFor(lint.index);
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 4, 1), Segment("name"_L1));
            if (!fetchEntity(file, type, keyUtf8)) {
                break;
            }

            const char* tem;

            switch (lint.index) {
                case 28:
                case 83:
                case 99:
                    tem = QT_TR_NOOP("Enemy name: %1");
                    break;
                default:
                    tem = QT_TR_NOOP("This pattern will be replaced with: %1");
                    break;
            }

            markdownSource += tr(tem).arg(svtostr(ffitostr(out[0])));
            break;
        }
        case 69:
        case 70:
        case 71:
        case 72:
        case 73: {
            const auto [file, type] = entityKindFor(lint.index);
            static constexpr array<QL1SV, 2> keys = { "name"_L1, "icon_index"_L1 };
            const FFIString keyUtf8 = buildPath(parseCaptured(lint.captured, 4, 1), Segment(keys));

            if (!fetchEntity(file, type, keyUtf8)) {
                break;
            }

            const auto icon = fetchIcon(stoa<i32>(ffitostr(out[1])));
            if (!icon) {
                break;
            }

            markdownSource += tr("This pattern will be replaced with: %1%2").arg(svtostr(ffitostr(out[0])), *icon);
            break;
        }
        case 84:
            // TODO(v1.x)
            break;
        case 109: {
            const QStringView num = QStringView(lint.captured).sliced(4).chopped(1);
            static const QLocale usLocale(QLocale::English, QLocale::UnitedStates);
            markdownSource += tr("This pattern will be replaced with: %1").arg(usLocale.toString(stoa<u16>(num)));
            break;
        }
        case 110: {
            const QStringView rgbHex = QStringView(lint.captured).sliced(4).chopped(1);
            markdownSource += tr("The following color will take effect: %1").arg(colorSwatch(stoa<u32>(rgbHex, 16)));
            break;
        }
        // TODO(v1.x)
        case 121:
        case 124:
        case 127:
            break;
        case 128:
            break;
        case 129:
            break;
        case 130:
            break;
        case 131:
            break;
        case 132:
            break;
        case 133:
            break;
        case 134:
            break;
        case 135:
            break;
        case 136:
            break;
        case 137:
            break;
        case 138:
            break;
        case 139:
            break;
        case 140:
            break;
        case 141:
            break;
        case 142:
            break;
        case 143:
            break;
        case 144:
            break;
        case 145:
            break;
        case 146:
            break;
        case 147:
            break;
        case 148:
            break;
        case 149:
            break;
        case 150:
            break;
        case 151:
            break;
        case 152:
            break;
        case 153:
            break;
        case 154:
            break;
        case 155:
            break;
        case 156:
            break;
        case 157:
            break;
        case 158:
            break;
        case 159:
            break;
        case 160:
            break;
        case 161:
            break;
        case 162:
            break;
        case 163:
            break;
        case 164:
            break;
        case 165:
            break;
        case 166:
            break;
        case 167:
            break;
        case 168:
            break;
        case 169:
            break;
        case 170:
            break;
        case 171:
            break;
        case 172:
            break;
        case 173:
            break;
        case 174:
            break;
        case 175:
            break;
        case 176:
            break;
        case 177:
            break;
        case 178:
            break;
        case 179:
            break;
        case 180:
            break;
        case 181:
            break;
        case 182:
            break;
        case 183:
            break;
        case 184:
            break;
        case 185:
            break;
        case 186:
            break;
        case 187:
            break;
        case 188:
            break;
        case 189:
            break;
        case 190:
            break;
        case 191:
            break;
        case 192:
            break;
        case 193:
            break;
        case 194:
            break;
        case 195:
            break;
        case 196:
            break;
        case 197:
            break;
        case 198:
            break;
        case 199:
            break;
        case 201:
            break;
        default:
            break;
    }
    // NOLINTEND(readability-magic-numbers)

    releaseOut();
}

[[nodiscard]] auto LintTooltip::insertPixmap(const QPixmap& pixmap) -> QString {
    auto url = QUrl(u"icon:%1"_qsv.arg(itos(resourceCounter).qsv()));

    document->addResource(QTextDocument::ImageResource, url, pixmap);
    QString img = u"<img src=\"%1\" />"_qsv.arg(url.toString());

    resources[resourceCounter++] = { std::move(url), pixmap };

    return img;
};

void LintTooltip::processNoteTag(QString& markdownSource, const LintEntry& lint, const Settings* const sets) {
    switch (lint.index) {
        default:
            break;
    }
}

void LintTooltip::processPluginCommand(QString& markdownSource, const LintEntry& lint, const Settings* const sets) {
    switch (lint.index) {
        default:
            break;
    }
}

void LintTooltip::processCommentTag(QString& markdownSource, const LintEntry& lint, const Settings* const sets) {
    switch (lint.index) {
        default:
            break;
    }
}

void LintTooltip::processMisc(QString& markdownSource, const LintEntry& lint, const Settings* const sets) {
    switch (lint.index) {
        case 4: {
            const QStringList suggestions = lint.suggestions.toStringList();
            markdownSource += tr("Suggestions: %1").arg(suggestionLinksMarkdown(suggestions, lint.type, lint.index));
            markdownSource += u"\n\n";
            break;
        }
        default:
            break;
    }
}

void LintTooltip::showAt(
    const QPoint& cursorPos,
    const QList<LintEntry>& lints,
    const Settings* const sets,
    const ProjectSettings* projS
) {
    document = make_unique<QTextDocument>(this);
    resourceCounter = 0;

    QString markdownSource;

    for (const auto& lint : lints) {
        const char* const msg = lint.translationId;

        if (lint.type == LintType::Custom || lint.type == LintType::LanguageToolCheck) {
            // Custom and LanguageTool lints carry a pre-formatted tooltip rather than a translation id.
            markdownSource += lint.extra.toString();
        } else if (msg != nullptr && *msg != 0) {
            markdownSource += tr(msg);
        } else {
            markdownSource += tr("Not yet implemented - work in progress.");
        }

        markdownSource += u"\n\n";

        switch (lint.type) {
            case TextCode: {
                processTextCode(markdownSource, lint, projS);
                break;
            }
            case NoteTag: {
                processNoteTag(markdownSource, lint, sets);
                break;
            }
            case PluginCommand: {
                processPluginCommand(markdownSource, lint, sets);
                break;
            }
            case CommentTag: {
                processCommentTag(markdownSource, lint, sets);
                break;
            }
            case Misc: {
                processMisc(markdownSource, lint, sets);
                break;
            }
            case Custom: {
                // The whole tooltip is the user-written text emitted above.
                break;
            }
            case LanguageToolCheck: {
                const QStringList suggestions = lint.suggestions.toStringList();

                if (!suggestions.isEmpty()) {
                    markdownSource += tr("Suggestions: %1").arg(suggestionLinksMarkdown(suggestions, lint.type, lint.index));
                    markdownSource += u"\n\n";
                }

                break;
            }
        }

        markdownSource += u"\n\n";
    }

    document->setDocumentMargin(0);
    document->setMarkdown(markdownSource, QTextDocument::MarkdownDialectCommonMark);

    for (const auto& [url, resource] : span(resources.data(), resourceCounter)) {
        document->addResource(QTextDocument::ImageResource, url, resource);
    }

    document->setTextWidth(-1);  // measure unwrapped first
    const i32 naturalWidth = scast<i32>(document->idealWidth());
    const i32 textWidth = min(naturalWidth, MAX_WIDTH);
    document->setTextWidth(textWidth);

    contentHeight = scast<i32>(document->size().height());
    scrollOffset = 0;

    const u16 width = scast<u16>(textWidth) + (H_PADDING * 2);
    const u16 height = min<u16>(scast<u16>(contentHeight) + (V_PADDING * 2), MAX_HEIGHT);
    resize(width, height);

    QPoint pos = cursorPos;
    pos.setY(pos.y() - Y_MARGIN - height);

    if (const QScreen* const screen = qApp->screenAt(cursorPos)) {
        const QRect screenRect = screen->availableGeometry();

        if (pos.x() + width > screenRect.right()) {
            pos.setX(screenRect.right() - width);
        }

        if (pos.x() < screenRect.left()) {
            pos.setX(screenRect.left());
        }

        if (pos.y() < screenRect.top()) {
            pos.setY(screenRect.top());
        }

        if (pos.y() + height > screenRect.bottom()) {
            pos.setY(screenRect.bottom() - height);
        }
    }

    move(pos);
    show();
    update();
}

void LintTooltip::hide() {
    if (timer.isActive()) {
        hide_();
    }
}

void LintTooltip::hide_() {
    QWidget::hide();
    scrollOffset = 0;
}

void LintTooltip::waitForEnter() {
    timer.start();
}

void LintTooltip::enterEvent(QEnterEvent* const /* event */) {
    timer.stop();
}

void LintTooltip::leaveEvent(QEvent* const /* event */) {
    timer.start();
}

void LintTooltip::wheelEvent(QWheelEvent* const event) {
    const i32 viewportHeight = height() - (V_PADDING * 2);
    const i32 maxScroll = qMax(0, contentHeight - viewportHeight);

    scrollOffset = qBound(0, scrollOffset - (event->angleDelta().y() / 4), maxScroll);
    update();
    event->accept();
}

auto LintTooltip::textAreaRect() const -> QRect {
    return QRect(0, 0, width(), height()).adjusted(H_PADDING, V_PADDING, -H_PADDING, -V_PADDING);
}

auto LintTooltip::toDocumentPos(const QPoint& widgetPos) const -> QPointF {
    const QRect area = textAreaRect();
    return { widgetPos - area.topLeft() + QPoint(0, scrollOffset) };
}

void LintTooltip::mouseMoveEvent(QMouseEvent* const event) {
    const QString anchor = document->documentLayout()->anchorAt(toDocumentPos(event->pos()));
    setCursor(anchor.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
    QWidget::mouseMoveEvent(event);
}

void LintTooltip::mouseReleaseEvent(QMouseEvent* const event) {
    if (event->button() == Qt::LeftButton) {
        const QString anchor = document->documentLayout()->anchorAt(toDocumentPos(event->pos()));

        if (!anchor.isEmpty()) {
            emit linkActivated(QUrl(anchor));
        }
    }

    QWidget::mouseReleaseEvent(event);
}

void LintTooltip::paintEvent(QPaintEvent* const /* event */) {
    if (document->isEmpty()) {
        return;
    }

    const auto bodyRect = QRect(0, 0, width(), height());
    const QPalette& pal = palette();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(pal.color(QPalette::Highlight));
    painter.setBrush(pal.color(QPalette::Base));
    painter.drawRect(bodyRect.adjusted(0, 0, -1, -1));

    const QRect textArea = textAreaRect();
    painter.setClipRect(textArea);
    painter.save();
    painter.translate(textArea.topLeft() - QPoint(0, scrollOffset));

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette = pal;
    document->documentLayout()->draw(&painter, ctx);

    painter.restore();
    painter.setClipping(false);

    const i32 maxScroll = contentHeight - textArea.height();

    if (maxScroll > 0) {
        const i32 trackHeight = textArea.height();
        const i32 thumbHeight = qMax(20, trackHeight * textArea.height() / contentHeight);
        const i32 thumbY = textArea.top() + ((scrollOffset * (trackHeight - thumbHeight)) / maxScroll);

        painter.setPen(Qt::NoPen);
        painter.setBrush(pal.color(QPalette::Highlight));
        painter
            .drawRoundedRect(QRect(bodyRect.right() - SCROLLBAR_WIDTH - 2, thumbY, SCROLLBAR_WIDTH, thumbHeight), 2, 2);
    }
}
