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

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(parent, flags) {
    setAttribute(Qt::WA_DeleteOnClose);

    // set as floating
    setFeatures(QDockWidget::DockWidgetFloatable);

    // remove titlebar
    setTitleBarWidget(new QWidget());

    setAllowedAreas(Qt::LeftDockWidgetArea);
}

void FramelessDialog::setStyleSheetFile(const QString& fileName) {
    QFile globalStyleSheet(Application::resourcesPath() + "styles/global.qss");
    QFile styleSheet(Application::resourcesPath() + fileName);
    if (styleSheet.open(QIODevice::ReadOnly) && globalStyleSheet.open(QIODevice::ReadOnly) ) {
        QDir::setCurrent(Application::resourcesPath());
        setStyleSheet(globalStyleSheet.readAll() + styleSheet.readAll());
    }
}
