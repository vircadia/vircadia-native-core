//
//  AvatarHashMap.h
//  libraries/avatars/src
//
//  Created by Andrew Meadows on 1/28/2014.
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

#include <functional>
#include <memory>

#include <glm/glm.hpp>

#include <DependencyManager.h>
#include <NLPacket.h>
#include <Node.h>

#include "ScriptAvatarData.h"

#include "AvatarData.h"

class AvatarHashMap : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    AvatarHash getHashCopy() { QReadLocker lock(&_hashLock); return _avatarHash; }
    const AvatarHash getHashCopy() const { QReadLocker lock(&_hashLock); return _avatarHash; }
    int size() { return _avatarHash.size(); }

    // Currently, your own avatar will be included as the null avatar id.
    
    /**jsdoc
     * @function AvatarManager.getAvatarIdentifiers
     * @returns {Uuid[]}
     */
    Q_INVOKABLE QVector<QUuid> getAvatarIdentifiers();

    /**jsdoc
     * @function AvatarManager.getAvatarsInRange
     * @param {Vec3} position
     * @param {number} range
     * @returns {Uuid[]} 
     */
    Q_INVOKABLE QVector<QUuid> getAvatarsInRange(const glm::vec3& position, float rangeMeters) const;

    // No JSDod because it's documwented in AvatarManager.
    // Null/Default-constructed QUuids will return MyAvatar
    Q_INVOKABLE virtual ScriptAvatarData* getAvatar(QUuid avatarID) { return new ScriptAvatarData(getAvatarBySessionID(avatarID)); }

    virtual AvatarSharedPointer getAvatarBySessionID(const QUuid& sessionID) const { return findAvatar(sessionID); }
    int numberOfAvatarsInRange(const glm::vec3& position, float rangeMeters);

signals:

    /**jsdoc
     * @function AvatarManager.avatarAddedEvent
     * @param {Uuid} sessionUUID
     * @returns {Signal}
     */
    void avatarAddedEvent(const QUuid& sessionUUID);

    /**jsdoc
     * @function AvatarManager.avatarRemovedEvent
     * @param {Uuid} sessionUUID
     * @returns {Signal}
     */
    void avatarRemovedEvent(const QUuid& sessionUUID);

    /**jsdoc
     * @function AvatarManager.avatarSessionChangedEvent
     * @param {Uuid} sessionUUID
     * @param {Uuid} oldSessionUUID
     * @returns {Signal}
     */
    void avatarSessionChangedEvent(const QUuid& sessionUUID,const QUuid& oldUUID);

public slots:

    /**jsdoc
     * @function AvatarManager.isAvatarInRange
     * @param {string} position
     * @param {string} range
     * @returns {boolean}
     */
    bool isAvatarInRange(const glm::vec3 & position, const float range);

protected slots:

    /**jsdoc
     * @function AvatarManager.sessionUUIDChanged
     * @param {Uuid} sessionUUID
     * @param {Uuid} oldSessionUUID
     */
    void sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID);

    /**jsdoc
     * @function AvatarManager.processAvatarDataPacket
     * @param {} message
     * @param {} sendingNode
     */
    void processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
   
    /**jsdoc
     * @function AvatarManager.processAvatarIdentityPacket
     * @param {} message
     * @param {} sendingNode
     */
    void processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    
    /**jsdoc
     * @function AvatarManager.processKillAvatar
     * @param {} message
     * @param {} sendingNode
     */
    void processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

protected:
    AvatarHashMap();

    virtual AvatarSharedPointer parseAvatarData(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    virtual AvatarSharedPointer newSharedAvatar();
    virtual AvatarSharedPointer addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer);
    AvatarSharedPointer newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer);
    virtual AvatarSharedPointer findAvatar(const QUuid& sessionUUID) const; // uses a QReadLocker on the hashLock
    virtual void removeAvatar(const QUuid& sessionUUID, KillAvatarReason removalReason = KillAvatarReason::NoReason);

    virtual void handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason = KillAvatarReason::NoReason);

    AvatarHash _avatarHash;
    // "Case-based safety": Most access to the _avatarHash is on the same thread. Write access is protected by a write-lock.
    // If you read from a different thread, you must read-lock the _hashLock. (Scripted write access is not supported).
    mutable QReadWriteLock _hashLock;

private:
    QUuid _lastOwnerSessionUUID;
};

#endif // hifi_AvatarHashMap_h
