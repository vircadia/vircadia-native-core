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
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QtCore/QUrl>

#include "Application.h"
#include "Menu.h"

#include <RunningMarker.h>
#include <SettingHandle.h>
#include <SettingHelpers.h>


bool CrashHandler::checkForResetSettings(bool wasLikelyCrash, bool suppressPrompt) {
    QSettings::setDefaultFormat(JSON_FORMAT);
    QSettings settings;
    settings.beginGroup("Developer");
    QVariant displayCrashOptions = settings.value(MenuOption::DisplayCrashOptions);
    QVariant askToResetSettingsOption = settings.value(MenuOption::AskToResetSettings);
    settings.endGroup();
    bool askToResetSettings = askToResetSettingsOption.isValid() && askToResetSettingsOption.toBool();

    // If option does not exist in Interface.ini so assume default behavior.
    bool displaySettingsResetOnCrash = !displayCrashOptions.isValid() || displayCrashOptions.toBool();

    if (suppressPrompt) {
        return wasLikelyCrash;
    }

    if (wasLikelyCrash || askToResetSettings) {
        if (displaySettingsResetOnCrash || askToResetSettings) {
            Action action = promptUserForAction(wasLikelyCrash);
            if (action != DO_NOTHING) {
                handleCrash(action);
            }
        }
    }
    return wasLikelyCrash;
}

CrashHandler::Action CrashHandler::promptUserForAction(bool showCrashMessage) {
    QDialog crashDialog;
    QLabel* label;
    if (showCrashMessage) {
        crashDialog.setWindowTitle("Interface Crashed Last Run");
        label = new QLabel("If you are having trouble starting would you like to reset your settings?");
    } else {
        crashDialog.setWindowTitle("Reset Settings");
        label = new QLabel("Would you like to reset your settings?");
    }

    QVBoxLayout* layout = new QVBoxLayout;

    layout->addWidget(label);

    QRadioButton* option1 = new QRadioButton("Reset all my settings");
    QRadioButton* option2 = new QRadioButton("Reset my settings but keep essential info");
    QRadioButton* option3 = new QRadioButton("Continue with my current settings");
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
            return CrashHandler::RETAIN_IMPORTANT_INFO;
        }
    }

    // Dialog cancelled or "do nothing" option chosen
    return CrashHandler::DO_NOTHING;
}

void CrashHandler::handleCrash(CrashHandler::Action action) {
    if (action != CrashHandler::DELETE_INTERFACE_INI && action != CrashHandler::RETAIN_IMPORTANT_INFO) {
        // CrashHandler::DO_NOTHING or unexpected value
        return;
    }

    QSettings settings;
    const QString ADDRESS_MANAGER_GROUP = "AddressManager";
    const QString ADDRESS_KEY = "address";
    const QString AVATAR_GROUP = "Avatar";
    const QString DISPLAY_NAME_KEY = "displayName";
    const QString FULL_AVATAR_URL_KEY = "fullAvatarURL";
    const QString FULL_AVATAR_MODEL_NAME_KEY = "fullAvatarModelName";
    const QString TUTORIAL_COMPLETE_FLAG_KEY = "tutorialComplete";

    QString displayName;
    QUrl fullAvatarURL;
    QString fullAvatarModelName;
    QUrl address;
    bool tutorialComplete = false;

    if (action == CrashHandler::RETAIN_IMPORTANT_INFO) {
        // Read avatar info

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

        // Tutorial complete
        tutorialComplete = settings.value(TUTORIAL_COMPLETE_FLAG_KEY).toBool();
    }

    // Delete Interface.ini
    QFile settingsFile(settings.fileName());
    if (settingsFile.exists()) {
        settingsFile.remove();
    }

    if (action == CrashHandler::RETAIN_IMPORTANT_INFO) {
        // Write avatar info

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

        // Tutorial complete
        settings.setValue(TUTORIAL_COMPLETE_FLAG_KEY, tutorialComplete);
    }
}

