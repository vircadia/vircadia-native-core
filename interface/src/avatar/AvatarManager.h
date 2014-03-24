//
//  AvatarManager.h
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarManager__
#define __hifi__AvatarManager__

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
    
    void processAvatarDataPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processAvatarIdentityPacket(const QByteArray& packet);
    void processAvatarBillboardPacket(const QByteArray& packet);
    void processKillAvatar(const QByteArray& datagram);

    void simulateAvatarFades(float deltaTime);
    void renderAvatarFades(const glm::vec3& cameraPosition, Avatar::RenderMode renderMode);
    
    // virtual override
    AvatarHash::iterator erase(const AvatarHash::iterator& iterator);
    
    QVector<AvatarSharedPointer> _avatarFades;
    QSharedPointer<MyAvatar> _myAvatar;
};

#endif /* defined(__hifi__AvatarManager__) */
