#include "LauncherWindow.h"

#include <iostream>

#include <QCursor>

void LauncherWindow::keyPressEvent(QKeyEvent* event) {
    QQuickView::keyPressEvent(event);
    if (!event->isAccepted()) {
        if (event->key() == Qt::Key_Escape) {
            exit(0);
        }
    }
}

void LauncherWindow::mousePressEvent(QMouseEvent* event) {
    QQuickView::mousePressEvent(event);
    if (!event->isAccepted()) {
        if (event->button() == Qt::LeftButton) {
            _drag = true;
            _previousMousePos = QCursor::pos();
        }
    }
}

void LauncherWindow::mouseReleaseEvent(QMouseEvent* event) {
    QQuickView::mouseReleaseEvent(event);
    _drag = false;
}

void LauncherWindow::mouseMoveEvent(QMouseEvent* event) {
    QQuickView::mouseMoveEvent(event);
    if (!event->isAccepted()) {
        if (_drag) {
            QPoint cursorPos = QCursor::pos();
            QPoint offset = _previousMousePos - cursorPos;
            _previousMousePos = cursorPos;
            setPosition(position() - offset);
        }
    }
}
