//
//  FramelessDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#include <QDesktopWidget>
#include <QFile>
#include <QPushButton>
#include <QSizeGrip>
#include <QMainWindow>

#include "Application.h"
#include "FramelessDialog.h"

const int RESIZE_HANDLE_WIDTH = 7;

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags) :
    QDialog(parent, flags | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
    _isResizing(false) {
    setAttribute(Qt::WA_DeleteOnClose);
    installEventFilter(this);

        startTimer(20);
}

void FramelessDialog::timerEvent(QTimerEvent *event) {
    move(parentWidget()->pos().x(), parentWidget()->pos().y() + 22);
    setFixedHeight(parentWidget()->size().height());
}

void FramelessDialog::paintEvent(QPaintEvent* event) {
    //move(parentWidget()->pos());
    //resize(size().width(), parentWidget()->size().height());
}

void FramelessDialog::showEvent(QShowEvent* event) {
    QDesktopWidget desktop;

    // move to upper left
//    move(desktop.availableGeometry().x(), desktop.availableGeometry().y());

    move(parentWidget()->pos().x(), parentWidget()->pos().y() + 22);

    // keep full height
    setFixedHeight(parentWidget()->size().height());
//   resize(size().width(), desktop.availableGeometry().height());
}

void FramelessDialog::setStyleSheetFile(const QString& fileName) {
    QFile globalStyleSheet(Application::resourcesPath() + "styles/global.qss");
    QFile styleSheet(Application::resourcesPath() + fileName);
    if (styleSheet.open(QIODevice::ReadOnly) && globalStyleSheet.open(QIODevice::ReadOnly) ) {
        QDir::setCurrent(Application::resourcesPath());
        QDialog::setStyleSheet(globalStyleSheet.readAll() + styleSheet.readAll());
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

FramelessDialog::~FramelessDialog() {

}
