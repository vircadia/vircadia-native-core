//
//  FramelessDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#include "Application.h"
#include "FramelessDialog.h"

const int RESIZE_HANDLE_WIDTH = 7;

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags) :
QDialog(parent, flags | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
    setAttribute(Qt::WA_DeleteOnClose);

    // handle rezize and move events
    parentWidget()->installEventFilter(this);

    // handle minimize, restore and focus events
    Application::getInstance()->installEventFilter(this);
}

bool FramelessDialog::eventFilter(QObject* sender, QEvent* event) {
    switch (event->type()) {
        case QEvent::Move:

            if (sender == parentWidget()) {
                // move to upper left corner on app move
                move(parentWidget()->geometry().topLeft());
            }
            break;

        case QEvent::Resize:
            if (sender == parentWidget()) {
                // keep full app height on resizing the app
                setFixedHeight(parentWidget()->size().height());
            }
            break;

        case QEvent::WindowStateChange:
            if (parentWidget()->isMinimized()) {
                setHidden(true);
            } else {
                setHidden(false);
            }
            break;

        case QEvent::ApplicationDeactivate:
            // hide on minimize and focus lost
            setHidden(true);
            break;

        case QEvent::ApplicationActivate:
            setHidden(false);
            break;

        default:
            break;
    }

    return false;
}

FramelessDialog::~FramelessDialog() {
    deleteLater();
}

void FramelessDialog::setStyleSheetFile(const QString& fileName) {
    QFile globalStyleSheet(Application::resourcesPath() + "styles/global.qss");
    QFile styleSheet(Application::resourcesPath() + fileName);
    if (styleSheet.open(QIODevice::ReadOnly) && globalStyleSheet.open(QIODevice::ReadOnly) ) {
        QDir::setCurrent(Application::resourcesPath());
        setStyleSheet(globalStyleSheet.readAll() + styleSheet.readAll());
    }
}

void FramelessDialog::showEvent(QShowEvent* event) {
    // move to upper left corner
    move(parentWidget()->geometry().topLeft());

    // keep full app height
    setFixedHeight(parentWidget()->size().height());

    // resize parrent if width is smaller than this dialog
    if (parentWidget()->size().width() < size().width()) {
        parentWidget()->resize(size().width(), parentWidget()->size().height());
    }
}
void FramelessDialog::mousePressEvent(QMouseEvent* mouseEvent) {
    if (abs(mouseEvent->pos().x() - size().width()) < RESIZE_HANDLE_WIDTH && mouseEvent->button() == Qt::LeftButton) {
        _isResizing = true;
        QApplication::setOverrideCursor(Qt::SizeHorCursor);
    }
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* mouseEvent) {
    QApplication::restoreOverrideCursor();
    _isResizing = false;
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* mouseEvent) {
    if (_isResizing) {
        resize(mouseEvent->pos().x(), size().height());
    }
}
