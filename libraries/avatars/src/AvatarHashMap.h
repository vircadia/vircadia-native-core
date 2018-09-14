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
#include <chrono>

#include <glm/glm.hpp>

#include <DependencyManager.h>
#include <NLPacket.h>
#include <Node.h>

#include "ScriptAvatarData.h"

#include "AvatarData.h"
#include "AssociatedTraitValues.h"

/**jsdoc
 * <strong>Note:</strong> An <code>AvatarList</code> API is also provided for Interface and client entity scripts: it is a 
 * synonym for the {@link AvatarManager} API.
 *
 * @namespace AvatarList
 *
 * @hifi-assignment-client
 */

class AvatarReplicas {
public:
    AvatarReplicas() {}
    void addReplica(const QUuid& parentID, AvatarSharedPointer replica);
    std::vector<QUuid> getReplicaIDs(const QUuid& parentID);
    void parseDataFromBuffer(const QUuid& parentID, const QByteArray& buffer);
    void processAvatarIdentity(const QUuid& parentID, const QByteArray& identityData, bool& identityChanged, bool& displayNameChanged);
    void removeReplicas(const QUuid& parentID);
    void processTrait(const QUuid& parentID, AvatarTraits::TraitType traitType, QByteArray traitBinaryData);
    void processDeletedTraitInstance(const QUuid& parentID, AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID);
    void processTraitInstance(const QUuid& parentID, AvatarTraits::TraitType traitType,
                                AvatarTraits::TraitInstanceID instanceID, QByteArray traitBinaryData);
    void setReplicaCount(int count) { _replicaCount = count; }
    int getReplicaCount() { return _replicaCount; }

private:
    std::map<QUuid, std::vector<AvatarSharedPointer>> _replicasMap;
    int _replicaCount { 0 };
};


class AvatarHashMap : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    AvatarHash getHashCopy() { QReadLocker lock(&_hashLock); return _avatarHash; }
    const AvatarHash getHashCopy() const { QReadLocker lock(&_hashLock); return _avatarHash; }
    int size() { QReadLocker lock(&_hashLock); return _avatarHash.size(); }

    // Currently, your own avatar will be included as the null avatar id.
    
    /**jsdoc
     * @function AvatarList.getAvatarIdentifiers
     * @returns {Uuid[]}
     */
    Q_INVOKABLE QVector<QUuid> getAvatarIdentifiers();

    /**jsdoc
     * @function AvatarList.getAvatarsInRange
     * @param {Vec3} position
     * @param {number} range
     * @returns {Uuid[]} 
     */
    Q_INVOKABLE QVector<QUuid> getAvatarsInRange(const glm::vec3& position, float rangeMeters) const;

    /**jsdoc
     * @function AvatarList.getAvatar
     * @param {Uuid} avatarID
     * @returns {AvatarData}
     */
    // Null/Default-constructed QUuids will return MyAvatar
    Q_INVOKABLE virtual ScriptAvatarData* getAvatar(QUuid avatarID) { return new ScriptAvatarData(getAvatarBySessionID(avatarID)); }

    virtual AvatarSharedPointer getAvatarBySessionID(const QUuid& sessionID) const { return findAvatar(sessionID); }
    int numberOfAvatarsInRange(const glm::vec3& position, float rangeMeters);

    void setReplicaCount(int count);
    int getReplicaCount() { return _replicas.getReplicaCount(); };

    virtual void clearOtherAvatars();

signals:

    /**jsdoc
     * @function AvatarList.avatarAddedEvent
     * @param {Uuid} sessionUUID
     * @returns {Signal}
     */
    void avatarAddedEvent(const QUuid& sessionUUID);

    /**jsdoc
     * @function AvatarList.avatarRemovedEvent
     * @param {Uuid} sessionUUID
     * @returns {Signal}
     */
    void avatarRemovedEvent(const QUuid& sessionUUID);

    /**jsdoc
     * @function AvatarList.avatarSessionChangedEvent
     * @param {Uuid} sessionUUID
     * @param {Uuid} oldSessionUUID
     * @returns {Signal}
     */
    void avatarSessionChangedEvent(const QUuid& sessionUUID,const QUuid& oldUUID);

public slots:

    /**jsdoc
     * @function AvatarList.isAvatarInRange
     * @param {string} position
     * @param {string} range
     * @returns {boolean}
     */
    bool isAvatarInRange(const glm::vec3 & position, const float range);

protected slots:

    /**jsdoc
     * @function AvatarList.sessionUUIDChanged
     * @param {Uuid} sessionUUID
     * @param {Uuid} oldSessionUUID
     */
    void sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID);

    /**jsdoc
     * @function AvatarList.processAvatarDataPacket
     * @param {} message
     * @param {} sendingNode
     */
    void processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
   
    /**jsdoc
     * @function AvatarList.processAvatarIdentityPacket
     * @param {} message
     * @param {} sendingNode
     */
    void processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    
    void processBulkAvatarTraits(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    
    /**jsdoc
     * @function AvatarList.processKillAvatar
     * @param {} message
     * @param {} sendingNode
     */
    void processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

protected:
    AvatarHashMap();

    virtual AvatarSharedPointer parseAvatarData(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    virtual AvatarSharedPointer newSharedAvatar();
    virtual AvatarSharedPointer addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer);
    AvatarSharedPointer newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer,
        bool& isNew);
    virtual AvatarSharedPointer findAvatar(const QUuid& sessionUUID) const; // uses a QReadLocker on the hashLock
    virtual void removeAvatar(const QUuid& sessionUUID, KillAvatarReason removalReason = KillAvatarReason::NoReason);

    virtual void handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason = KillAvatarReason::NoReason);
    
    AvatarHash _avatarHash;
    struct PendingAvatar {
        std::chrono::steady_clock::time_point creationTime;
        int transmits;
        AvatarSharedPointer avatar;
    };
    using AvatarPendingHash = QHash<QUuid, PendingAvatar>;
    AvatarPendingHash _pendingAvatars;
    mutable QReadWriteLock _hashLock;

    std::unordered_map<QUuid, AvatarTraits::TraitVersions> _processedTraitVersions;
    AvatarReplicas _replicas;

private:
    QUuid _lastOwnerSessionUUID;
};

#endif // hifi_AvatarHashMap_h
