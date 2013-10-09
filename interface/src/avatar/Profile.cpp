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

Profile::Profile(const QString &username) :
    _username(username),
    _uuid(),
    _lastDomain(),
    _lastPosition(0.0, 0.0, 0.0),
 	_faceModelURL()
{
    if (!_username.isEmpty()) {
        // we've been given a new username, ask the data-server for profile
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

void Profile::updatePositionInDomain(const QString& domain, const glm::vec3 position) {
    if (!_username.isEmpty()) {
        bool updateRequired =  false;
        
        if (_lastDomain != domain) {
            _lastDomain = domain;
            updateRequired = true;
        }
        
        if (_lastPosition != position) {
            _lastPosition = position;
            updateRequired = true;
        }
        
        if (updateRequired) {
            // either the domain or position or both have changed, time to send update to data-server
            
        }
    }
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