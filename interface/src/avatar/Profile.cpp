//
//  Profile.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QSettings>

#include "Profile.h"
#include "DataServerClient.h"

Profile::Profile() :
    _username(),
    _uuid(),
    _faceModelURL()
{
    
}

void Profile::clear() {
    _username.clear();
    _uuid = QUuid();
    _faceModelURL.clear();
}

void Profile::setUsername(const QString &username) {
    this->clear();
    _username = username;
    
    if (!_username.isEmpty()) {
        // we've been given a new username, ask the data-server for our UUID
        DataServerClient::getClientValueForKey(DataServerKey::UUID);
    }
}

void Profile::setUUID(const QUuid& uuid) {
    _uuid = uuid;
    
    // when the UUID is changed we need set it appropriately on our avatar instance
    Application::getInstance()->getAvatar()->setUUID(_uuid);
}

void Profile::setFaceModelURL(const QUrl& faceModelURL) {
    _faceModelURL = faceModelURL;
    
    QMetaObject::invokeMethod(&Application::getInstance()->getAvatar()->getHead().getBlendFace(),
                              "setModelURL",
                              Q_ARG(QUrl, _faceModelURL));
}

void Profile::saveData(QSettings* settings) {
    settings->beginGroup("Profile");
    
    settings->setValue("username", _username);
    settings->setValue("UUID", _uuid);
    settings->setValue("faceModelURL", _faceModelURL);
    
    settings->endGroup();
}

void Profile::loadData(QSettings* settings) {
    settings->beginGroup("Profile");
    
    _username = settings->value("username").toString();
    this->setUUID(settings->value("UUID").toUuid());
    _faceModelURL = settings->value("faceModelURL").toUrl();
    
    settings->endGroup();
}