#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Types.hpp"

#include <QTimer>
#include <QUrl>
#include <QWidget>

enum class SegmentType : u8 {
    Index,
    String,
    Strings,
};

struct Segment {
    Segment(const QL1SV l1sv) : l1sv(l1sv), type(SegmentType::String) {}

    Segment(const QStringView qsv) : qsv(qsv), type(SegmentType::Index) {}

    Segment(const span<const QL1SV> keys) : keys(keys), type(SegmentType::Strings) {}

    union {
        QStringView qsv;
        QLatin1StringView l1sv;
        const span<const QL1SV> keys;
    };

    const SegmentType type;
};

class LintTooltip final : public QWidget {
    Q_OBJECT

   public:
    explicit LintTooltip(QWidget* parent = nullptr);

    void
    showAt(const QPoint& cursorPos, const QList<LintEntry>& lints, const Settings* sets, const ProjectSettings* projS);
    void hide();
    void waitForEnter();

   signals:
    void linkActivated(const QUrl& url);

   protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    inline void processTextCode(QString& markdownSource, const LintEntry& lint, const ProjectSettings* sets);
    inline void processNoteTag(QString& markdownSource, const LintEntry& lint, const Settings* sets);
    inline void processPluginCommand(QString& markdownSource, const LintEntry& lint, const Settings* sets);
    inline void processCommentTag(QString& markdownSource, const LintEntry& lint, const Settings* sets);
    inline void processMisc(QString& markdownSource, const LintEntry& lint, const Settings* sets);
    inline void hide_();
    [[nodiscard]] inline auto textAreaRect() const -> QRect;
    [[nodiscard]] inline auto toDocumentPos(const QPoint& widgetPos) const -> QPointF;
    [[nodiscard]] inline auto insertPixmap(const QPixmap& pixmap) -> QString;
    template <typename... Segments>
        requires(std::same_as<Segments, Segment> && ...)
    [[nodiscard]] constexpr auto buildPath(Segments&&... segments) -> FFIString;

    array<std::pair<QUrl, QPixmap>, 2> resources;
    array<char, 32> buf;
    // Owned by Rust, refilled per processTextCode call and freed at its end -
    // value-initialized so the first free is a no-op.
    array<FFIString, 2> out{};
    QTimer timer;

    unique_ptr<QTextDocument> document;

    u32 resourceCounter = 0;

    i32 scrollOffset = 0;
    i32 contentHeight = 0;
};