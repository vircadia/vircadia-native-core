#include "LauncherWindow.h"

#include <iostream>

#include <QCursor>

#ifdef Q_OS_WIN
#include <shellapi.h>
#include <propsys.h>
#include <propkey.h>
#endif

LauncherWindow::LauncherWindow() {
#ifdef Q_OS_WIN
    // On Windows, disable pinning of the launcher.
    IPropertyStore* pps;
    HWND id = (HWND)this->winId();
    if (id == NULL) {
        qDebug() << "Failed to disable pinning, window id is null";
    } else {
        HRESULT hr = SHGetPropertyStoreForWindow(id, IID_PPV_ARGS(&pps));
        if (SUCCEEDED(hr)) {
            PROPVARIANT var;
            var.vt = VT_BOOL;
            var.boolVal = VARIANT_TRUE;
            hr = pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
            pps->Release();
        }
    }
#endif
}

void LauncherWindow::keyPressEvent(QKeyEvent* event) {
    QQuickView::keyPressEvent(event);
    if (!event->isAccepted()) {
        if (event->key() == Qt::Key_Escape) {
            exit(0);
        } else if (event->key() == Qt::Key_Up) {
            _launcherState->toggleDebugState();
        } else if (event->key() == Qt::Key_Left) {
            _launcherState->gotoPreviousDebugScreen();
        } else if (event->key() == Qt::Key_Right) {
            _launcherState->gotoNextDebugScreen();
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
