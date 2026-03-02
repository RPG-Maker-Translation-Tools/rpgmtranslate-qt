#pragma once

#include "Aliases.hpp"
#include "Types.hpp"

#include <QTextLayout>
#include <QTimer>
#include <QWidget>

class ColoredTextLabel : public QWidget {
    Q_OBJECT

   public:
    explicit ColoredTextLabel(QWidget* parent = nullptr);

    void
    setData(QStringView text, const vector<Span>& spans, const QColor& color);

    void setText(QStringView text, const vector<Span>& spans);

    [[nodiscard]] auto text() const -> const QString&;

    auto hasHeightForWidth() const -> bool override;
    auto heightForWidth(i32 width) const -> i32 override;
    auto sizeHint() const -> QSize override;
    auto minimumSizeHint() const -> QSize override;

   protected:
    void paintEvent(QPaintEvent* /* event */) override;
    void resizeEvent(QResizeEvent* event) override;

   private:
    void ensureLayout(i32 availableWidth) const;

    QString text_;
    vector<Span> spans_;
    QColor spanColor_;

    mutable QSizeF layoutSize_;
    mutable QTextLayout layout_;
    mutable i32 cachedWidth_ = -1;
    mutable bool dirtyLayout = true;
};
