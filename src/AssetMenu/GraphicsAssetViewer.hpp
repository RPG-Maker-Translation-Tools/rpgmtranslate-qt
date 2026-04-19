#pragma once

#include "Aliases.hpp"

#include <QGraphicsView>

class GraphicsAssetViewer final : public QGraphicsView {
    Q_OBJECT

   public:
    using QGraphicsView::QGraphicsView;

    [[nodiscard]] auto currentScale() const -> f32;
    void setScaleFactor(f32 factor);

   signals:
    void rescaled();

   protected:
    void wheelEvent(QWheelEvent* event) override;
};