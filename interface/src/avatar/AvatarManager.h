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
#include <PhysicsEngine.h>

#include "Avatar.h"
#include "AvatarMotionState.h"

class MyAvatar;

class AvatarManager : public AvatarHashMap {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    /// Registers the script types associated with the avatar manager.
    static void registerMetaTypes(QScriptEngine* engine);

    void init();

    MyAvatar* getMyAvatar() { return _myAvatar.get(); }
    
    void updateMyAvatar(float deltaTime);
    void updateOtherAvatars(float deltaTime);
    
    void clearOtherAvatars();
   
    bool shouldShowReceiveStats() const { return _shouldShowReceiveStats; }

    class LocalLight {
    public:
        glm::vec3 color;
        glm::vec3 direction;
    };
    
    Q_INVOKABLE void setLocalLights(const QVector<AvatarManager::LocalLight>& localLights);
    Q_INVOKABLE QVector<AvatarManager::LocalLight> getLocalLights() const;

    VectorOfMotionStates& getObjectsToDelete();
    VectorOfMotionStates& getObjectsToAdd();
    VectorOfMotionStates& getObjectsToChange();
    void handleOutgoingChanges(VectorOfMotionStates& motionStates);
    void handleCollisionEvents(CollisionEvents& collisionEvents);

    void updateAvatarPhysicsShape(const QUuid& id);
   
public slots:
    void setShouldShowReceiveStats(bool shouldShowReceiveStats) { _shouldShowReceiveStats = shouldShowReceiveStats; }
    void updateAvatarRenderStatus(bool shouldRenderAvatars);

private:
    AvatarManager(QObject* parent = 0);
    AvatarManager(const AvatarManager& other);

    void simulateAvatarFades(float deltaTime);
    
    // virtual overrides
    virtual AvatarSharedPointer newSharedAvatar();
    virtual AvatarSharedPointer addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer);
    void removeAvatarMotionState(AvatarSharedPointer avatar);
    virtual void removeAvatar(const QUuid& sessionUUID);
    
    QVector<AvatarSharedPointer> _avatarFades;
    std::shared_ptr<MyAvatar> _myAvatar;
    quint64 _lastSendAvatarDataTime = 0; // Controls MyAvatar send data rate.
    
    QVector<AvatarManager::LocalLight> _localLights;

    bool _shouldShowReceiveStats = false;

    SetOfAvatarMotionStates _avatarMotionStates;
    SetOfMotionStates _motionStatesToAdd;
    VectorOfMotionStates _motionStatesToDelete;
    VectorOfMotionStates _tempMotionStates;
};

Q_DECLARE_METATYPE(AvatarManager::LocalLight)
Q_DECLARE_METATYPE(QVector<AvatarManager::LocalLight>)

#endif // hifi_AvatarManager_h
