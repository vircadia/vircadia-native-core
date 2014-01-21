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
        DataServerClient::getClientValueForKey(DataServerKey::UUID, username, this);
        DataServerClient::getClientValueForKey(DataServerKey::FaceMeshURL, username, this);
        DataServerClient::getClientValueForKey(DataServerKey::SkeletonURL, username, this);
        
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
        DataServerClient::putValueForKeyAndUsername(DataServerKey::Domain, domain, _username);
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
                DataServerClient::putValueForKeyAndUsername(DataServerKey::Position,
                                                            QString(createByteArray(position)), _username);
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
        DataServerClient::putValueForKeyAndUsername(DataServerKey::Orientation, QString(createByteArray(eulerAngles)),
                                                    _username);
        
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
    for (int i = 0; i < keyList.size(); i++) {
        if (valueList[i] != " ") {
            if (keyList[i] == DataServerKey::FaceMeshURL) {
                if (userString == _username || userString == uuidStringWithoutCurlyBraces(_uuid)) {
                    qDebug("Changing user's face model URL to %s", valueList[i].toLocal8Bit().constData());
                    Application::getInstance()->getProfile()->setFaceModelURL(QUrl(valueList[i]));
                } else {
                    // mesh URL for a UUID, find avatar in our list
                    
                    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
                        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
                            Avatar* avatar = (Avatar *) node->getLinkedData();
                            
                            if (avatar->getUUID() == QUuid(userString)) {
                                QMetaObject::invokeMethod(&avatar->getHead().getFaceModel(),
                                                          "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
                            }
                        }
                    }
                }
            } else if (keyList[i] == DataServerKey::SkeletonURL) {
                if (userString == _username || userString == uuidStringWithoutCurlyBraces(_uuid)) {
                    qDebug("Changing user's skeleton URL to %s", valueList[i].toLocal8Bit().constData());
                    Application::getInstance()->getProfile()->setSkeletonModelURL(QUrl(valueList[i]));
                } else {
                    // skeleton URL for a UUID, find avatar in our list
                    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
                        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
                            Avatar* avatar = (Avatar *) node->getLinkedData();
                            
                            if (avatar->getUUID() == QUuid(userString)) {
                                QMetaObject::invokeMethod(&avatar->getSkeletonModel(), "setURL",
                                                          Q_ARG(QUrl, QUrl(valueList[i])));
                            }
                        }
                    }
                }
            } else if (keyList[i] == DataServerKey::Domain && keyList[i + 1] == DataServerKey::Position &&
                       keyList[i + 2] == DataServerKey::Orientation && valueList[i] != " " &&
                       valueList[i + 1] != " " && valueList[i + 2] != " ") {
                
                QStringList coordinateItems = valueList[i + 1].split(',');
                QStringList orientationItems = valueList[i + 2].split(',');
                
                if (coordinateItems.size() == 3 && orientationItems.size() == 3) {
                    
                    // send a node kill request, indicating to other clients that they should play the "disappeared" effect
                    NodeList::getInstance()->sendKillNode(&NODE_TYPE_AVATAR_MIXER, 1);
                    
                    qDebug() << "Changing domain to" << valueList[i].toLocal8Bit().constData() <<
                        ", position to" << valueList[i + 1].toLocal8Bit().constData() <<
                        ", and orientation to" << valueList[i + 2].toLocal8Bit().constData() <<
                        "to go to" << userString;
                    
                    NodeList::getInstance()->setDomainHostname(valueList[i]);
                    // orient the user to face the target
                    glm::quat newOrientation = glm::quat(glm::radians(glm::vec3(orientationItems[0].toFloat(),
                                                                                orientationItems[1].toFloat(),
                                                                                orientationItems[2].toFloat()))) *
                                                                                glm::angleAxis(180.0f, 0.0f, 1.0f, 0.0f);
                                                                                Application::getInstance()->getAvatar()
                                                                                ->setOrientation(newOrientation);
                    
                    // move the user a couple units away
                    const float DISTANCE_TO_USER = 2.0f;
                    glm::vec3 newPosition = glm::vec3(coordinateItems[0].toFloat(), coordinateItems[1].toFloat(),
                                                      coordinateItems[2].toFloat()
                                                      ) - newOrientation * IDENTITY_FRONT * DISTANCE_TO_USER;
                    Application::getInstance()->getAvatar()->setPosition(newPosition);
                }
                
            } else if (keyList[i] == DataServerKey::UUID) {
                // this is the user's UUID - set it on the profile
                _uuid = QUuid(valueList[i]);
            }
        }
    }
}

void Profile::setUsername(const QString& username) {
    _username = username;
}
