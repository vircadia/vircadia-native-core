//
//  UpdateDialog.cpp
//  interface/src/ui
//
//  Created by Leonardo Murillo on 6/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UpdateDialog.h"

#include <AutoUpdater.h>

#include "DependencyManager.h"

HIFI_QML_DEF(UpdateDialog)

UpdateDialog::UpdateDialog(QQuickItem* parent) :
    OffscreenQmlDialog(parent)
{
    auto applicationUpdater = DependencyManager::get<AutoUpdater>();
    int currentVersion = QCoreApplication::applicationVersion().toInt();
    int latestVersion = applicationUpdater.data()->getBuildData().lastKey();
    int versionsBehind = latestVersion - currentVersion;
    _updateAvailableDetails = "v" + QString::number(latestVersion) + " released on " + applicationUpdater.data()->getBuildData()[latestVersion]["releaseTime"];
    _updateAvailableDetails += "\nYou are " + QString::number(versionsBehind) + " versions behind";
    _releaseNotes = applicationUpdater.data()->getBuildData()[latestVersion]["releaseNotes"];
}

const QString& UpdateDialog::updateAvailableDetails() const {
    return _updateAvailableDetails;
}

const QString& UpdateDialog::releaseNotes() const {
    return _releaseNotes;
}

void UpdateDialog::closeDialog() {
    hide();
}

void UpdateDialog::hide() {
    ((QQuickItem*)parent())->setEnabled(false);
}

void UpdateDialog::triggerUpgrade() {
    auto applicationUpdater = DependencyManager::get<AutoUpdater>();
    applicationUpdater.data()->performAutoUpdate(applicationUpdater.data()->getBuildData().lastKey());
}