//
//  AvatarManager.h
//  interface/src/avatar
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarManager_h
#define hifi_AvatarManager_h

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <AvatarHashMap.h>

#include "Avatar.h"

class MyAvatar;

class AvatarManager : public AvatarHashMap {
    Q_OBJECT

public:
    AvatarManager(QObject* parent = 0);

    void init();

    MyAvatar* getMyAvatar() { return _myAvatar.data(); }
    
    void updateOtherAvatars(float deltaTime);
    void renderAvatars(Avatar::RenderMode renderMode, bool selfAvatarOnly = false);
    
    void clearOtherAvatars();
    
    Q_INVOKABLE void setLocalLights(const QVector<Model::LocalLight>& localLights);
    Q_INVOKABLE QVector<Model::LocalLight> getLocalLights() const;
    
private:
    AvatarManager(const AvatarManager& other);

    void simulateAvatarFades(float deltaTime);
    void renderAvatarFades(const glm::vec3& cameraPosition, Avatar::RenderMode renderMode);
    
    AvatarSharedPointer newSharedAvatar();
    
    // virtual override
    AvatarHash::iterator erase(const AvatarHash::iterator& iterator);
    
    QVector<AvatarSharedPointer> _avatarFades;
    QSharedPointer<MyAvatar> _myAvatar;
    
    QVector<Model::LocalLight> _localLights;
};

#endif // hifi_AvatarManager_h
