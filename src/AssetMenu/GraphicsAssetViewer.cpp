#include "GraphicsAssetViewer.hpp"

#include <QWheelEvent>

namespace {
constexpr f32 SCALE_FACTOR = 1.15F;
constexpr u32 SCALE_PERCENT_MULTIPLIER = 100;
constexpr u32 MAX_SCALE_PERCENT = 1000;
constexpr f32 MIN_SCALE_FACTOR = 0.01F;
constexpr f32 MAX_SCALE_FACTOR = 10.0F;
}  // namespace

void GraphicsAssetViewer::wheelEvent(QWheelEvent* const event) {
    u32 curScale = scast<u32>(roundf(currentScale() * SCALE_PERCENT_MULTIPLIER));
    const bool zoomingIn = event->angleDelta().y() > 0;

    if (curScale <= 1 && !zoomingIn) {
        return;
    }

    if (curScale >= MAX_SCALE_PERCENT && zoomingIn) {
        return;
    }

    if (zoomingIn) {
        scale(SCALE_FACTOR, SCALE_FACTOR);
    } else {
        scale(1.0F / SCALE_FACTOR, 1.0F / SCALE_FACTOR);
    }

    curScale = scast<u32>(roundf(currentScale() * SCALE_PERCENT_MULTIPLIER));

    if (curScale <= 1 && !zoomingIn) {
        setScaleFactor(MIN_SCALE_FACTOR);
    }

    if (curScale >= MAX_SCALE_PERCENT && zoomingIn) {
        setScaleFactor(MAX_SCALE_FACTOR);
    }

    emit rescaled();
}

auto GraphicsAssetViewer::currentScale() const -> f32 {
    const QTransform trf = transform();
    return scast<f32>(trf.m11());
}

void GraphicsAssetViewer::setScaleFactor(const f32 factor) {
    QTransform transform;
    transform.scale(factor, factor);
    setTransform(transform);
}