//
//  Profile.h
//  hifi
//
//  Created by Stephen Birarda on 10/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Profile__
#define __hifi__Profile__

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QUuid>

#include <glm/glm.hpp>

class Profile {
public:
    Profile(const QString& username);
    
    QString getUserString() const;
    
    const QString& getUsername() const { return _username; }
    
    void setUUID(const QUuid& uuid);
    const QUuid& getUUID() { return _uuid; }
    
    void setFaceModelURL(const QUrl& faceModelURL);
    const QUrl& getFaceModelURL() const { return _faceModelURL; }
    
    void updateDomain(const QString& domain);
    void updatePosition(const glm::vec3 position);
    
    QString getLastDomain() const  { return _lastDomain; }
    const glm::vec3& getLastPosition() const { return _lastPosition; }
    
    void saveData(QSettings* settings);
    void loadData(QSettings* settings);
private:
    QString _username;
    QUuid _uuid;
    QString _lastDomain;
    glm::vec3 _lastPosition;
    QUrl _faceModelURL;
};

#endif /* defined(__hifi__Profile__) */
