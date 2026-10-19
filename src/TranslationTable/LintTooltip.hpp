#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Types.hpp"

#include <QTimer>
#include <QUrl>
#include <QWidget>

// The most images (icons, color swatches) one tooltip can hold.
static constexpr usize MAX_RESOURCES = 16;

// The room for the key of an entity (`3\x04name`), with its terminator. A key that doesn't fit is not looked up.
static constexpr usize PATH_BUFFER_SIZE = 64;

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

struct TagReference;

class LintTooltip final : public QWidget {
    Q_OBJECT

   public:
    explicit LintTooltip(QWidget* parent = nullptr);
    ~LintTooltip() override;

    void showAt(const QPoint& cursorPos, const QList<LintEntry>& lints, const ProjectSettings* projS);
    // Hides once the mouse has left the tooltip
    void hide();

    // Hides at once
    void dismiss() { hide_(); }

    void waitForEnter();

   signals:
    void linkActivated(const QUrl& url);
    void suggestionActivated(LintType type, i32 lintIndex, i32 suggestionIndex);

   protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    inline void processTextCode(QString& markdownSource, const LintEntry& lint, const ProjectSettings* sets);
    inline void processReferences(
        QString& markdownSource,
        const LintEntry& lint,
        const ProjectSettings* sets,
        span<const TagReference> table
    );
    inline void releaseOut();
    [[nodiscard]] inline auto fetchEntityData(
        QString& markdownSource,
        const ProjectSettings* sets,
        QStringView folder,
        RPGMFileType type,
        const FFIString& key
    ) -> bool;
    inline void hide_();
    [[nodiscard]] inline auto textAreaRect() const -> QRect;
    [[nodiscard]] inline auto toDocumentPos(const QPoint& widgetPos) const -> QPointF;
    [[nodiscard]] inline auto insertPixmap(const QPixmap& pixmap) -> QString;
    template <typename... Segments>
        requires(std::same_as<Segments, Segment> && ...)
    [[nodiscard]] constexpr auto buildPath(Segments&&... segments) -> FFIString;

    array<pair<QUrl, QPixmap>, MAX_RESOURCES> resources;
    array<char, PATH_BUFFER_SIZE> buf;
    // Owned by Rust, refilled per processTextCode call and freed at its end -
    // value-initialized so the first free is a no-op.
    array<FFIString, 2> out{};
    QTimer timer;

    unique_ptr<QTextDocument> document;

    u32 resourceCounter = 0;

    i32 scrollOffset = 0;
    i32 contentHeight = 0;
};