//
//  CrashHandler.cpp
//  interface/src
//
//  Created by David Rowe on 24 Aug 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CrashHandler.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <PathUtils.h>
#include <QRadioButton>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "Menu.h"

static const QString RUNNING_MARKER_FILENAME = "Interface.running";

void CrashHandler::checkForAndHandleCrash() {
    QFile runningMarkerFile(runningMarkerFilePath());
    if (runningMarkerFile.exists()) {
        QSettings settings;
        settings.beginGroup("Developer");
        if (settings.value(MenuOption::DisplayCrashOptions).toBool()) {
            Action action = promptUserForAction();
            if (action != DO_NOTHING) {
                handleCrash(action);
            }
        }
    }
}

CrashHandler::Action CrashHandler::promptUserForAction() {
    QDialog crashDialog;
    crashDialog.setWindowTitle("Interface Crashed Last Run");

    QVBoxLayout* layout = new QVBoxLayout;

    QLabel* label = new QLabel("What would you like to do?");
    layout->addWidget(label);

    QRadioButton* option1 = new QRadioButton("Delete Interface.ini");
    QRadioButton* option2 = new QRadioButton("Delete Interface.ini but retain login and avatar info.");
    QRadioButton* option3 = new QRadioButton("Continue with my current Interface.ini");
    option3->setChecked(true);
    layout->addWidget(option1);
    layout->addWidget(option2);
    layout->addWidget(option3);
    layout->addSpacing(12);
    layout->addStretch();

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(buttons);
    crashDialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));

    crashDialog.setLayout(layout);

    int result = crashDialog.exec();

    if (result == QDialog::Accepted) {
        if (option1->isChecked()) {
            return CrashHandler::DELETE_INTERFACE;
        }
        if (option2->isChecked()) {
            return CrashHandler::RETAIN_LOGIN_AND_AVATAR_INFO;
        }
    }

    // Dialog cancelled or "do nothing" option chosen
    return CrashHandler::DO_NOTHING;
}

void CrashHandler::handleCrash(CrashHandler::Action action) {
    // TODO
}

void CrashHandler::writeRunningMarkerFiler() {
    QFile runningMarkerFile(runningMarkerFilePath());
    if (!runningMarkerFile.exists()) {
        runningMarkerFile.open(QIODevice::WriteOnly);
        runningMarkerFile.close();
    }
}
void CrashHandler::deleteRunningMarkerFile() {
    QFile runningMarkerFile(runningMarkerFilePath());
    if (runningMarkerFile.exists()) {
        runningMarkerFile.remove();
    }
}

const QString CrashHandler::runningMarkerFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + RUNNING_MARKER_FILENAME;
}
