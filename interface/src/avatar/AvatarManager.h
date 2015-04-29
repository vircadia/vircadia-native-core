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
    SINGLETON_DEPENDENCY

public:
    
    /// Registers the script types associated with the avatar manager.
    static void registerMetaTypes(QScriptEngine* engine);

    void init();

    MyAvatar* getMyAvatar() { return _myAvatar.data(); }
    
    void updateMyAvatar(float deltaTime);
    void updateOtherAvatars(float deltaTime);
    void renderAvatars(RenderArgs::RenderMode renderMode, bool postLighting = false, bool selfAvatarOnly = false);
    
    void clearOtherAvatars();
   
    bool shouldShowReceiveStats() const { return _shouldShowReceiveStats; }

    class LocalLight {
    public:
        glm::vec3 color;
        glm::vec3 direction;
    };
    
    Q_INVOKABLE void setLocalLights(const QVector<AvatarManager::LocalLight>& localLights);
    Q_INVOKABLE QVector<AvatarManager::LocalLight> getLocalLights() const;
   
public slots:
    void setShouldShowReceiveStats(bool shouldShowReceiveStats) { _shouldShowReceiveStats = shouldShowReceiveStats; }

private:
    AvatarManager(QObject* parent = 0);
    AvatarManager(const AvatarManager& other);

    void simulateAvatarFades(float deltaTime);
    void renderAvatarFades(const glm::vec3& cameraPosition, RenderArgs::RenderMode renderMode);
    
    AvatarSharedPointer newSharedAvatar();
    
    // virtual overrides
    AvatarHash::iterator erase(const AvatarHash::iterator& iterator);
    
    QVector<AvatarSharedPointer> _avatarFades;
    QSharedPointer<MyAvatar> _myAvatar;
    quint64 _lastSendAvatarDataTime = 0; // Controls MyAvatar send data rate.
    
    QVector<AvatarManager::LocalLight> _localLights;

    bool _shouldShowReceiveStats = false;
};

Q_DECLARE_METATYPE(AvatarManager::LocalLight)
Q_DECLARE_METATYPE(QVector<AvatarManager::LocalLight>)

#endif // hifi_AvatarManager_h
