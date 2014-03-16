//
//  FramelessDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#include <QFile>
#include <QPushButton>
#include <QSizeGrip>

#include "Application.h"
#include "FramelessDialog.h"

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags | Qt::FramelessWindowHint) {
    setWindowOpacity(0.9);
    setAttribute(Qt::WA_DeleteOnClose);
    isResizing = false;
}

void FramelessDialog::setStyleSheet(const QString& fileName) {
    QFile globalStyleSheet(Application::resourcesPath() + "styles/global.qss");
    QFile styleSheet(Application::resourcesPath() + fileName);
    if (styleSheet.open(QIODevice::ReadOnly) && globalStyleSheet.open(QIODevice::ReadOnly) ) {
        QDir::setCurrent(Application::resourcesPath());
        QDialog::setStyleSheet(globalStyleSheet.readAll() + styleSheet.readAll());
    }
}
void FramelessDialog::mousePressEvent(QMouseEvent* mouseEvent) {
    if (abs(mouseEvent->pos().x() - size().width()) < 2 && mouseEvent->button() == Qt::LeftButton) {
        isResizing = true;
        QApplication::setOverrideCursor(Qt::SizeHorCursor);
    }
    // propagate the event
    QDialog::mousePressEvent(mouseEvent);
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* mouseEvent) {
    QApplication::restoreOverrideCursor();
    isResizing = false;
    // propagate the event
    QDialog::mouseReleaseEvent(mouseEvent);
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* mouseEvent) {
    if (isResizing) {
        resize(mouseEvent->pos().x(), size().height());
    }
    QDialog::mouseMoveEvent(mouseEvent);
}

FramelessDialog::~FramelessDialog() {

}
