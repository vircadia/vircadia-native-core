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

#include "DataServerAccountInfo.h"
#include "Menu.h"

Q_DECLARE_METATYPE(DataServerAccountInfo)

static const QString RUNNING_MARKER_FILENAME = "Interface.running";

void CrashHandler::checkForAndHandleCrash() {
    QFile runningMarkerFile(runningMarkerFilePath());
    if (runningMarkerFile.exists()) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings settings;
        settings.beginGroup("Developer");
        QVariant displayCrashOptions = settings.value(MenuOption::DisplayCrashOptions);
        settings.endGroup();
        if (!displayCrashOptions.isValid()  // Option does not exist in Interface.ini so assume default behavior.
            || displayCrashOptions.toBool()) {
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
            return CrashHandler::DELETE_INTERFACE_INI;
        }
        if (option2->isChecked()) {
            return CrashHandler::RETAIN_LOGIN_AND_AVATAR_INFO;
        }
    }

    // Dialog cancelled or "do nothing" option chosen
    return CrashHandler::DO_NOTHING;
}

void CrashHandler::handleCrash(CrashHandler::Action action) {
    if (action != CrashHandler::DELETE_INTERFACE_INI && action != CrashHandler::RETAIN_LOGIN_AND_AVATAR_INFO) {
        // CrashHandler::DO_NOTHING or unexpected value
        return;
    }

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    const QString ADDRESS_MANAGER_GROUP = "AddressManager";
    const QString ADDRESS_KEY = "address";
    const QString AVATAR_GROUP = "Avatar";
    const QString DISPLAY_NAME_KEY = "displayName";
    const QString FULL_AVATAR_URL_KEY = "fullAvatarURL";
    const QString FULL_AVATAR_MODEL_NAME_KEY = "fullAvatarModelName";
    const QString ACCOUNTS_GROUP = "accounts";
    QString displayName;
    QUrl fullAvatarURL;
    QString fullAvatarModelName;
    QUrl address;
    QMap<QString, DataServerAccountInfo> accounts;

    if (action == CrashHandler::RETAIN_LOGIN_AND_AVATAR_INFO) {
        // Read login and avatar info

        qRegisterMetaType<DataServerAccountInfo>("DataServerAccountInfo");
        qRegisterMetaTypeStreamOperators<DataServerAccountInfo>("DataServerAccountInfo");

        // Location and orientation
        settings.beginGroup(ADDRESS_MANAGER_GROUP);
        address = settings.value(ADDRESS_KEY).toUrl();
        settings.endGroup();

        // Display name and avatar
        settings.beginGroup(AVATAR_GROUP);
        displayName = settings.value(DISPLAY_NAME_KEY).toString();
        fullAvatarURL = settings.value(FULL_AVATAR_URL_KEY).toUrl();
        fullAvatarModelName = settings.value(FULL_AVATAR_MODEL_NAME_KEY).toString();
        settings.endGroup();

        // Accounts
        settings.beginGroup(ACCOUNTS_GROUP);
        foreach(const QString& key, settings.allKeys()) {
            accounts.insert(key, settings.value(key).value<DataServerAccountInfo>());
        }
        settings.endGroup();
    }

    // Delete Interface.ini
    QFile settingsFile(settings.fileName());
    if (settingsFile.exists()) {
        settingsFile.remove();
    }

    if (action == CrashHandler::RETAIN_LOGIN_AND_AVATAR_INFO) {
        // Write login and avatar info

        // Location and orientation
        settings.beginGroup(ADDRESS_MANAGER_GROUP);
        settings.setValue(ADDRESS_KEY, address);
        settings.endGroup();

        // Display name and avatar
        settings.beginGroup(AVATAR_GROUP);
        settings.setValue(DISPLAY_NAME_KEY, displayName);
        settings.setValue(FULL_AVATAR_URL_KEY, fullAvatarURL);
        settings.setValue(FULL_AVATAR_MODEL_NAME_KEY, fullAvatarModelName);
        settings.endGroup();

        // Accounts
        settings.beginGroup(ACCOUNTS_GROUP);
        foreach(const QString& key, accounts.keys()) {
            settings.setValue(key, QVariant::fromValue(accounts.value(key)));
        }
        settings.endGroup();
    }
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
