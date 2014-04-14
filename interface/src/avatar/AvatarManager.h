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

class AvatarManager : public QObject, public AvatarHashMap {
    Q_OBJECT
public:
    AvatarManager(QObject* parent = 0);

    void init();

    MyAvatar* getMyAvatar() { return _myAvatar.data(); }
    
    void updateOtherAvatars(float deltaTime);
    void renderAvatars(Avatar::RenderMode renderMode, bool selfAvatarOnly = false);
    
    void clearOtherAvatars();

public slots:
    void processAvatarMixerDatagram(const QByteArray& datagram, const QWeakPointer<Node>& mixerWeakPointer);
    
private:
    AvatarManager(const AvatarManager& other);
    
    AvatarSharedPointer matchingOrNewAvatar(const QUuid& nodeUUID, const QWeakPointer<Node>& mixerWeakPointer);
    
    void processAvatarDataPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processAvatarIdentityPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processAvatarBillboardPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processKillAvatar(const QByteArray& datagram);

    void simulateAvatarFades(float deltaTime);
    void renderAvatarFades(const glm::vec3& cameraPosition, Avatar::RenderMode renderMode);
    
    // virtual override
    AvatarHash::iterator erase(const AvatarHash::iterator& iterator);
    
    QVector<AvatarSharedPointer> _avatarFades;
    QSharedPointer<MyAvatar> _myAvatar;
};

#endif // hifi_AvatarManager_h
