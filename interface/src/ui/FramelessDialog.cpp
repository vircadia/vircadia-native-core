//
//  FramelessDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#include "FramelessDialog.h"
#include <QPushButton>
#include <QFile>

FramelessDialog::FramelessDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags | Qt::FramelessWindowHint) {
    QFile styleSheet("resources/styles/global.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }
    setWindowOpacity(0.95);
    setAttribute(Qt::WA_DeleteOnClose);
}

FramelessDialog::~FramelessDialog() {

}
