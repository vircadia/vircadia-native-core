//
//  GlobalServicesScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 9/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AccountManager.h"
#include "Application.h"
#include "ResourceCache.h"

#include "GlobalServicesScriptingInterface.h"

GlobalServicesScriptingInterface::GlobalServicesScriptingInterface() {
    AccountManager& accountManager = AccountManager::getInstance();
    connect(&accountManager, &AccountManager::usernameChanged, this, &GlobalServicesScriptingInterface::myUsernameChanged);
    connect(&accountManager, &AccountManager::logoutComplete, this, &GlobalServicesScriptingInterface::loggedOut);

    _downloading = false;
    connect(Application::getInstance(), &Application::renderingInWorldInterface, 
            this, &GlobalServicesScriptingInterface::checkDownloadInfo);
}

GlobalServicesScriptingInterface::~GlobalServicesScriptingInterface() {
    AccountManager& accountManager = AccountManager::getInstance();
    disconnect(&accountManager, &AccountManager::usernameChanged, this, &GlobalServicesScriptingInterface::myUsernameChanged);
    disconnect(&accountManager, &AccountManager::logoutComplete, this, &GlobalServicesScriptingInterface::loggedOut);
}

GlobalServicesScriptingInterface* GlobalServicesScriptingInterface::getInstance() {
    static GlobalServicesScriptingInterface sharedInstance;
    return &sharedInstance;
}

void GlobalServicesScriptingInterface::loggedOut() {
    emit GlobalServicesScriptingInterface::disconnected(QString("logout"));
}

DownloadInfoResult::DownloadInfoResult() :
    downloading(QList<float>()),
    pending(0.0f)
{
}

QScriptValue DownloadInfoResultToScriptValue(QScriptEngine* engine, const DownloadInfoResult& result) {
    QScriptValue object = engine->newObject();

    QScriptValue array = engine->newArray(result.downloading.count());
    for (int i = 0; i < result.downloading.count(); i += 1) {
        array.setProperty(i, result.downloading[i]);
    }

    object.setProperty("downloading", array);
    object.setProperty("pending", result.pending);
    return object;
}

void DownloadInfoResultFromScriptValue(const QScriptValue& object, DownloadInfoResult& result) {
    QList<QVariant> downloading = object.property("downloading").toVariant().toList();
    result.downloading.clear();
    for (int i = 0; i < downloading.count(); i += 1) {
        result.downloading.append(downloading[i].toFloat());
    }

    result.pending = object.property("pending").toVariant().toFloat();
}

DownloadInfoResult GlobalServicesScriptingInterface::getDownloadInfo() {
    DownloadInfoResult result;
    foreach(Resource* resource, ResourceCache::getLoadingRequests()) {
        result.downloading.append(resource->getProgress() * 100.0f);
    }
    result.pending = ResourceCache::getPendingRequestCount();
    return result;
}

void GlobalServicesScriptingInterface::checkDownloadInfo() {
    DownloadInfoResult downloadInfo = getDownloadInfo();
    bool downloading = downloadInfo.downloading.count() > 0 || downloadInfo.pending > 0;

    // Emit signal if downloading or have just finished.
    if (downloading || _downloading) {
        _downloading = downloading;
        emit downloadInfoChanged(downloadInfo);
    }
}

void GlobalServicesScriptingInterface::updateDownloadInfo() {
    emit downloadInfoChanged(getDownloadInfo());
}
