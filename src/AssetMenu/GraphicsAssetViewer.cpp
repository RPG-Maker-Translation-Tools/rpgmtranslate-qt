#include "GraphicsAssetViewer.hpp"

#include <QWheelEvent>

void GraphicsAssetViewer::wheelEvent(QWheelEvent* const event) {
    u32 curScale = u32(roundf(currentScale() * 100));
    const bool zoomingIn = event->angleDelta().y() > 0;

    if (curScale <= 1 && !zoomingIn) {
        return;
    }

    if (curScale >= 1000 && zoomingIn) {
        return;
    }

    constexpr f32 SCALE_FACTOR = 1.15F;

    if (zoomingIn) {
        scale(SCALE_FACTOR, SCALE_FACTOR);
    } else {
        scale(1.0F / SCALE_FACTOR, 1.0F / SCALE_FACTOR);
    }

    curScale = u32(roundf(currentScale() * 100));

    if (curScale <= 1 && !zoomingIn) {
        setScaleFactor(0.01F);
    }

    if (curScale >= 1000 && zoomingIn) {
        setScaleFactor(10.0F);
    }

    emit rescaled();
}

auto GraphicsAssetViewer::currentScale() const -> f32 {
    const QTransform trf = transform();
    return f32(trf.m11());
}

void GraphicsAssetViewer::setScaleFactor(const f32 factor) {
    QTransform transform;
    transform.scale(factor, factor);
    setTransform(transform);
}