#include "LauncherWindow.h"

#include <iostream>

void LauncherWindow::keyPressEvent(QKeyEvent* event) {
    QQuickView::keyPressEvent(event);
    if (!event->isAccepted()) {
        // std::cout << "Key press event\n";
    }
}

void LauncherWindow::mousePressEvent(QMouseEvent* event) {
    QQuickView::mousePressEvent(event);
    if (!event->isAccepted()) {
        //std::cout << "mouse press event\n";
    }
}

void LauncherWindow::mouseReleaseEvent(QMouseEvent* event) {
    QQuickView::mouseReleaseEvent(event);
    if (!event->isAccepted()) {
        //std::cout << "mouse release event\n";
    }
}

void LauncherWindow::mouseMoveEvent(QMouseEvent* event) {
    QQuickView::mouseMoveEvent(event);
    if (!event->isAccepted()) {
        // std::cout << "mouse move event\n";
    }
}
