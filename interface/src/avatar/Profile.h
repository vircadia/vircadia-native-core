//
//  Profile.h
//  hifi
//
//  Created by Stephen Birarda on 10/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Profile__
#define __hifi__Profile__

#include <stdint.h>

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QUuid>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "DataServerClient.h"

class Profile : public DataServerCallbackObject {
public:
    Profile(const QString& username);
    
    QString getUserString() const;
    
    const QString& getUsername() const { return _username; }

    const QString& getDisplayName() const { return _displayName; }
    
    void setUUID(const QUuid& uuid) { _uuid = uuid; }
    const QUuid& getUUID() { return _uuid; }
    
    void updateDomain(const QString& domain);
    void updatePosition(const glm::vec3 position);
    void updateOrientation(const glm::quat& orientation);
    
    QString getLastDomain() const  { return _lastDomain; }
    const glm::vec3& getLastPosition() const { return _lastPosition; }
    
    void saveData(QSettings* settings);
    void loadData(QSettings* settings);
    
    void processDataServerResponse(const QString& userString, const QStringList& keyList, const QStringList& valueList);
private:
    
    void setUsername(const QString& username);

    void setDisplayName(const QString& displaName);
    
    QString _username;
    QString _displayName;
    QUuid _uuid;
    QString _lastDomain;
    glm::vec3 _lastPosition;
    glm::vec3 _lastOrientation;
    quint64 _lastOrientationSend;
};

#endif /* defined(__hifi__Profile__) */
