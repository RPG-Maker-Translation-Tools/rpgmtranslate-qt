#pragma once

#include <QApplication>
#include <QMouseEvent>
#include <QWidget>

// The pointer moves without a button down. Sent straight to the viewport: the offscreen platform has no pointer to move
// (and `QTest::mouseMove` moves the system cursor on Windows, which no widget of a hidden test window hears).
inline void hoverAt(QWidget* const viewport, const QPoint pos) {
    QMouseEvent move(QEvent::MouseMove, pos, viewport->mapToGlobal(pos), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(viewport, &move);
}
