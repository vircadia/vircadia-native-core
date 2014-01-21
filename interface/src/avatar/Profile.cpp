//
//  Profile.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QSettings>

#include <NodeList.h>
#include <UUID.h>

#include "Application.h"
#include "Profile.h"
#include "Util.h"

Profile::Profile(const QString &username) :
    _username(),
    _uuid(),
    _lastDomain(),
    _lastPosition(0.0, 0.0, 0.0),
    _lastOrientationSend(0),
 	_faceModelURL()
{
    if (!username.isEmpty()) {
        setUsername(username);
        
        // we've been given a new username, ask the data-server for profile
        DataServerClient::getClientValueForKey(DataServerKey::UUID, this);
        DataServerClient::getClientValueForKey(DataServerKey::FaceMeshURL, this);
        DataServerClient::getClientValueForKey(DataServerKey::SkeletonURL, this);
        
        // send our current domain server to the data-server
        updateDomain(NodeList::getInstance()->getDomainHostname());
    }
}

QString Profile::getUserString() const {
    if (_uuid.isNull()) {
        return _username;
    } else {
        return uuidStringWithoutCurlyBraces(_uuid);
    }
}

void Profile::setUUID(const QUuid& uuid) {
    _uuid = uuid;
    
    DataServerClient::setClientIdentifier(_username);
    
    // when the UUID is changed we need set it appropriately on our avatar instance
    Application::getInstance()->getAvatar()->setUUID(_uuid);
}

void Profile::setFaceModelURL(const QUrl& faceModelURL) {
    _faceModelURL = faceModelURL;
    
    QMetaObject::invokeMethod(&Application::getInstance()->getAvatar()->getHead().getFaceModel(),
        "setURL", Q_ARG(QUrl, _faceModelURL));
}

void Profile::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    _skeletonModelURL = skeletonModelURL;
    
    QMetaObject::invokeMethod(&Application::getInstance()->getAvatar()->getSkeletonModel(),
        "setURL", Q_ARG(QUrl, _skeletonModelURL));
}

void Profile::updateDomain(const QString& domain) {
    if (_lastDomain != domain) {
        _lastDomain = domain;
        
        // send the changed domain to the data-server
        DataServerClient::putValueForKey(DataServerKey::Domain, domain.toLocal8Bit().constData());
    }
}

static QByteArray createByteArray(const glm::vec3& vector) {
    return QByteArray::number(vector.x) + ',' + QByteArray::number(vector.y) + ',' + QByteArray::number(vector.z);
}

void Profile::updatePosition(const glm::vec3 position) {
    if (_lastPosition != position) {
        
        static timeval lastPositionSend = {};
        const uint64_t DATA_SERVER_POSITION_UPDATE_INTERVAL_USECS = 5 * 1000 * 1000;
        const float DATA_SERVER_POSITION_CHANGE_THRESHOLD_METERS = 1;
        
        if (usecTimestampNow() - usecTimestamp(&lastPositionSend) >= DATA_SERVER_POSITION_UPDATE_INTERVAL_USECS &&
            (fabsf(_lastPosition.x - position.x) >= DATA_SERVER_POSITION_CHANGE_THRESHOLD_METERS ||
             fabsf(_lastPosition.y - position.y) >= DATA_SERVER_POSITION_CHANGE_THRESHOLD_METERS ||
             fabsf(_lastPosition.z - position.z) >= DATA_SERVER_POSITION_CHANGE_THRESHOLD_METERS))  {
                
                // if it has been 5 seconds since the last position change and the user has moved >= the threshold
                // in at least one of the axis then send the position update to the data-server
                
                _lastPosition = position;
                
                // update the lastPositionSend to now
                gettimeofday(&lastPositionSend, NULL);
                
                // send the changed position to the data-server
                DataServerClient::putValueForKey(DataServerKey::Position, createByteArray(position).constData());
            }
    }
}

void Profile::updateOrientation(const glm::quat& orientation) {
    glm::vec3 eulerAngles = safeEulerAngles(orientation);
    if (_lastOrientation == eulerAngles) {
        return;
    }
    const uint64_t DATA_SERVER_ORIENTATION_UPDATE_INTERVAL_USECS = 5 * 1000 * 1000;
    const float DATA_SERVER_ORIENTATION_CHANGE_THRESHOLD_DEGREES = 5.0f;
    
    uint64_t now = usecTimestampNow();
    if (now - _lastOrientationSend >= DATA_SERVER_ORIENTATION_UPDATE_INTERVAL_USECS &&
            glm::distance(_lastOrientation, eulerAngles) >= DATA_SERVER_ORIENTATION_CHANGE_THRESHOLD_DEGREES) {
        DataServerClient::putValueForKey(DataServerKey::Orientation, createByteArray(eulerAngles).constData());
        
        _lastOrientation = eulerAngles;
        _lastOrientationSend = now;
    }
}

void Profile::saveData(QSettings* settings) {
    settings->beginGroup("Profile");
    
    settings->setValue("username", _username);
    settings->setValue("UUID", _uuid);
    settings->setValue("faceModelURL", _faceModelURL);
    settings->setValue("skeletonModelURL", _skeletonModelURL);
    
    settings->endGroup();
}

void Profile::loadData(QSettings* settings) {
    settings->beginGroup("Profile");
    
    setUsername(settings->value("username").toString());
    this->setUUID(settings->value("UUID").toUuid());
    _faceModelURL = settings->value("faceModelURL").toUrl();
    _skeletonModelURL = settings->value("skeletonModelURL").toUrl();
    
    settings->endGroup();
}

void Profile::processDataServerResponse(const QString& userString, const QStringList& keyList, const QStringList& valueList) {
    qDebug() << "Called with" << keyList << valueList;
}

void Profile::setUsername(const QString& username) {
    _username = username;
    DataServerClient::setClientIdentifier(_username);
}
