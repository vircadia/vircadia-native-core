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
    if (applicationUpdater) {

        auto buildData = applicationUpdater.data()->getBuildData();
        ApplicationVersion latestVersion = buildData.lastKey();
        _updateAvailableDetails = "v" + latestVersion.versionString + " released on "
            + QString(buildData[latestVersion]["releaseTime"]).replace("  ", " ");

        _releaseNotes = "";

        auto it = buildData.end();
        while (it != buildData.begin()) {
            --it;

            if (applicationUpdater->getCurrentVersion() < it.key()) {
                // grab the release notes for this later version
                QString releaseNotes = it.value()["releaseNotes"];
                releaseNotes.remove("<br />");
                releaseNotes.remove(QRegExp("^\n+"));
                _releaseNotes += "\n" + it.key().versionString + "\n" + releaseNotes + "\n";
            } else {
                break;
            }
        }


    }
}

const QString& UpdateDialog::updateAvailableDetails() const {
    return _updateAvailableDetails;
}

const QString& UpdateDialog::releaseNotes() const {
    return _releaseNotes;
}

void UpdateDialog::triggerUpgrade() {
    auto applicationUpdater = DependencyManager::get<AutoUpdater>();
    applicationUpdater.data()->openLatestUpdateURL();
}
