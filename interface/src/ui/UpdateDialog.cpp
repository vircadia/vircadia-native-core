//
//  UpdateDialog.cpp
//  hifi
//
//  Created by Leonardo Murillo on 6/3/15.
//
//

#include "UpdateDialog.h"
#include "DependencyManager.h"
#include <AutoUpdate.h>

HIFI_QML_DEF(UpdateDialog)

UpdateDialog::UpdateDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    auto applicationUpdater = DependencyManager::get<AutoUpdate>();
    int currentVersion = QCoreApplication::applicationVersion().toInt();
    int latestVersion = applicationUpdater.data()->getBuildData().lastKey();
    int versionsBehind = latestVersion - currentVersion;
    _updateAvailableDetails = "v" + QString::number(latestVersion) + " released on " + applicationUpdater.data()->getBuildData()[latestVersion]["releaseTime"];
    _updateAvailableDetails += "\nYou are " + QString::number(versionsBehind) + " versions behind";
    _releaseNotes = applicationUpdater.data()->getBuildData()[latestVersion]["releaseNotes"];
}

QString UpdateDialog::updateAvailableDetails() const {
    return _updateAvailableDetails;
}

QString UpdateDialog::releaseNotes() const {
    return _releaseNotes;
}

void UpdateDialog::closeDialog() {
    hide();
}

void UpdateDialog::hide() {
    ((QQuickItem*)parent())->setEnabled(false);
}

void UpdateDialog::triggerUpgrade() {
    auto applicationUpdater = DependencyManager::get<AutoUpdate>();
    applicationUpdater.data()->performAutoUpdate(applicationUpdater.data()->getBuildData().lastKey());
}