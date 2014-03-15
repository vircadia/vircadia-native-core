//
//  FramelessDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#include <QFile>
#include <QPushButton>

#include "Application.h"
#include "FramelessDialog.h"

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags | Qt::FramelessWindowHint) {

    QFile styleSheet(Application::resourcesPath() + "styles/global.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        QDir::setCurrent(Application::resourcesPath());
        setStyleSheet(styleSheet.readAll());
    }
    
    setWindowOpacity(0.95);
    setAttribute(Qt::WA_DeleteOnClose);
}

FramelessDialog::~FramelessDialog() {

}
