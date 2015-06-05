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
    
}

UpdateDialog::~UpdateDialog() {
}

void UpdateDialog::displayDialog() {
    setUpdateAvailableDetails("");
    show();
}

void UpdateDialog::setUpdateAvailableDetails(const QString& updateAvailableDetails) {
    if (updateAvailableDetails == "") {
        auto applicationUpdater = DependencyManager::get<AutoUpdate>();
        _updateAvailableDetails = "This is just a test " + QString::number(applicationUpdater.data()->getBuildData().lastKey());
        qDebug() << "[LEOTEST] We are updating the text in the dialog";
        emit updateAvailableDetailsChanged();
    }
}

QString UpdateDialog::updateAvailableDetails() const {
    return _updateAvailableDetails;
}

void UpdateDialog::hide() {
    ((QQuickItem*)parent())->setEnabled(false);
}

void UpdateDialog::triggerBuildDownload(const int &buildNumber) {
    qDebug() << "[LEOTEST] Triggering download of build number: " << QString::number(buildNumber);
}