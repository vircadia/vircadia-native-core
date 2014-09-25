//
//  AvatarHashMap.h
//  libraries/avatars/src
//
//  Created by Stephen AndrewMeadows on 1/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarHashMap_h
#define hifi_AvatarHashMap_h

#include <QtCore/QHash>
#include <QtCore/QSharedPointer>
#include <QtCore/QUuid>

#include <Node.h>

#include "AvatarData.h"

typedef QSharedPointer<AvatarData> AvatarSharedPointer;
typedef QHash<QUuid, AvatarSharedPointer> AvatarHash;

class AvatarHashMap : public QObject {
    Q_OBJECT
public:
    AvatarHashMap();
    
    const AvatarHash& getAvatarHash() { return _avatarHash; }
    int size() const { return _avatarHash.size(); }
    
public slots:
    void processAvatarMixerDatagram(const QByteArray& datagram, const QWeakPointer<Node>& mixerWeakPointer);
    bool containsAvatarWithDisplayName(const QString& displayName);
    
private slots:
    void sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID);
    
protected:
    virtual AvatarHash::iterator erase(const AvatarHash::iterator& iterator);
    
    bool shouldKillAvatar(const AvatarSharedPointer& sharedAvatar);
    
    virtual AvatarSharedPointer newSharedAvatar();
    AvatarSharedPointer matchingOrNewAvatar(const QUuid& nodeUUID, const QWeakPointer<Node>& mixerWeakPointer);
    
    void processAvatarDataPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processAvatarIdentityPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processAvatarBillboardPacket(const QByteArray& packet, const QWeakPointer<Node>& mixerWeakPointer);
    void processKillAvatar(const QByteArray& datagram);

    AvatarHash _avatarHash;
    QUuid _lastOwnerSessionUUID;
};

#endif // hifi_AvatarHashMap_h
