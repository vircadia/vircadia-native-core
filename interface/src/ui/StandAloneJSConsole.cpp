//
//  StandAloneJSConsole.cpp
//
//
//  Created by Clement on 1/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StandAloneJSConsole.h"

#include <QMainWindow>
#include <QDialog>
#include <QVBoxLayout>

#include <Application.h>
#include <MainWindow.h>

void StandAloneJSConsole::toggleConsole()  {
    QMainWindow* mainWindow = qApp->getWindow();
    if (!_jsConsole && !qApp->getLoginDialogPoppedUp()) {
        QDialog* dialog = new QDialog(mainWindow, Qt::WindowStaysOnTopHint);
        QVBoxLayout* layout = new QVBoxLayout(dialog);
        dialog->setLayout(layout);
        
        dialog->resize(QSize(CONSOLE_WIDTH, CONSOLE_HEIGHT));
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(new JSConsole(dialog));
        dialog->setWindowOpacity(CONSOLE_WINDOW_OPACITY);
        dialog->setWindowTitle(CONSOLE_TITLE);
        
        _jsConsole = dialog;
    }
    _jsConsole->setVisible(!_jsConsole->isVisible());
}

