#pragma once

#include "FWD.hpp"

#include <QPoint>
#include <QWidget>

class PersistentMenu : public QWidget {
    Q_OBJECT

   public:
    explicit PersistentMenu(
        QWidget* parent = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags()
    );

    [[nodiscard]] auto mouseMoved() const noexcept -> bool {
        return mouseMoved_;
    }

    void resetMouseMovedFlag() noexcept;

    [[nodiscard]] auto dragMoveEnabled() const noexcept -> bool {
        return dragMoveEnabled_;
    }

    void setDragMoveEnabled(bool enabled) noexcept;

   signals:
    void shown();
    void hidden();
    void mouseMovedChanged(bool);
    void dragMoveEnabledChanged(bool);

   protected:
    auto event(QEvent* event) -> bool override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    [[nodiscard]] virtual auto canStartDragAt(const QPoint& posInWidget) const
        -> bool;

   private:
    void setMouseMovedInternal(bool moved) noexcept;

    bool mouseMoved_ = false;
    bool dragMoveEnabled_ = true;

    bool dragging_ = false;
    QPoint pressPosGlobal_;
    QPoint startTopLeftGlobal_;
};
