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

const int CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 50;
const quint64 MIN_TIME_BETWEEN_MY_AVATAR_DATA_SENDS = USECS_PER_SECOND / CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND;

/*@jsdoc
 * The <code>AvatarList</code> API provides information about avatars within the current domain.
 *
 * <p><strong>Warning:</strong> An API named "<code>AvatarList</code>" is also provided for Interface, client entity, and avatar 
 * scripts, however, it is a synonym for the {@link AvatarManager} API.</p>
 *
 * @namespace AvatarList
 *
 * @hifi-assignment-client
 * @hifi-server-entity
 */

class AvatarReplicas {
public:
    AvatarReplicas() {}
    void addReplica(const QUuid& parentID, AvatarSharedPointer replica);
    std::vector<QUuid> getReplicaIDs(const QUuid& parentID);
    void parseDataFromBuffer(const QUuid& parentID, const QByteArray& buffer);
    void processAvatarIdentity(const QUuid& parentID, const QByteArray& identityData, bool& identityChanged, bool& displayNameChanged);
    void removeReplicas(const QUuid& parentID);
    std::vector<AvatarSharedPointer> takeReplicas(const QUuid& parentID);
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
    
    /*@jsdoc
     * Gets the IDs of all avatars in the domain.
     * <p><strong>Warning:</strong> If the AC script is acting as an avatar (i.e., <code>Agent.isAvatar == true</code>) the 
     * avatar's ID is NOT included in results.</p>
     * @function AvatarList.getAvatarIdentifiers
     * @returns {Uuid[]} The IDs of all avatars in the domain (excluding AC script's avatar).
     * @example <caption>Report the IDS of all avatars within the domain.</caption>
     * var avatars = AvatarList.getAvatarIdentifiers();
     * print("Avatars in the domain: " + JSON.stringify(avatars));
     */
    Q_INVOKABLE QVector<QUuid> getAvatarIdentifiers();

    /*@jsdoc
     * Gets the IDs of all avatars within a specified distance from a point.
     * <p><strong>Warning:</strong> If the AC script is acting as an avatar (i.e., <code>Agent.isAvatar == true</code>) the
     * avatar's ID is NOT included in results.</p>
     * @function AvatarList.getAvatarsInRange
     * @param {Vec3} position - The point about which the search is performed.
     * @param {number} range - The search radius.
     * @returns {Uuid[]} The IDs of all avatars within the search distance from the position (excluding AC script's avatar).
     * @example <caption>Report the IDs of all avatars within 10m of the origin.</caption>
     * var RANGE = 10;
     * var avatars = AvatarList.getAvatarsInRange(Vec3.ZERO, RANGE);
     * print("Avatars near the origin: " + JSON.stringify(avatars));
     */
    Q_INVOKABLE QVector<QUuid> getAvatarsInRange(const glm::vec3& position, float rangeMeters) const;

    /*@jsdoc
     * Gets information about an avatar.
     * @function AvatarList.getAvatar
     * @param {Uuid} avatarID - The ID of the avatar.
     * @returns {ScriptAvatar} Information about the avatar.
     */
    // Null/Default-constructed QUuids will return MyAvatar
    Q_INVOKABLE virtual ScriptAvatarData* getAvatar(QUuid avatarID) { return new ScriptAvatarData(getAvatarBySessionID(avatarID)); }

    virtual AvatarSharedPointer getAvatarBySessionID(const QUuid& sessionID) const { return findAvatar(sessionID); }
    int numberOfAvatarsInRange(const glm::vec3& position, float rangeMeters);

    void setReplicaCount(int count);
    int getReplicaCount() { return _replicas.getReplicaCount(); };

    virtual void clearOtherAvatars();

signals:

    /*@jsdoc
     * Triggered when an avatar arrives in the domain.
     * @function AvatarList.avatarAddedEvent
     * @param {Uuid} sessionUUID - The ID of the avatar that arrived in the domain.
     * @returns {Signal}
     * @example <caption>Report when an avatar arrives in the domain.</caption>
     * AvatarManager.avatarAddedEvent.connect(function (sessionID) {
     *     print("Avatar arrived: " + sessionID);
     * });
     *
     * // Note: If using from the AvatarList API, replace "AvatarManager" with "AvatarList".
     */
    void avatarAddedEvent(const QUuid& sessionUUID);

    /*@jsdoc
     * Triggered when an avatar leaves the domain.
     * @function AvatarList.avatarRemovedEvent
     * @param {Uuid} sessionUUID - The ID of the avatar that left the domain.
     * @returns {Signal}
     * @example <caption>Report when an avatar leaves the domain.</caption>
     * AvatarManager.avatarRemovedEvent.connect(function (sessionID) {
     *     print("Avatar left: " + sessionID);
     * });
     *
     * // Note: If using from the AvatarList API, replace "AvatarManager" with "AvatarList".
     */
    void avatarRemovedEvent(const QUuid& sessionUUID);

    /*@jsdoc
     * Triggered when an avatar's session ID changes.
     * @function AvatarList.avatarSessionChangedEvent
     * @param {Uuid} newSessionUUID - The new session ID.
     * @param {Uuid} oldSessionUUID - The old session ID.
     * @returns {Signal}
     * @example <caption>Report when an avatar's session ID changes.</caption>
     * AvatarManager.avatarSessionChangedEvent.connect(function (newSessionID, oldSessionID) {
     *     print("Avatar session ID changed from " + oldSessionID + " to " + newSessionID);
     * });
     *
     * // Note: If using from the AvatarList API, replace "AvatarManager" with "AvatarList".
     */
    void avatarSessionChangedEvent(const QUuid& sessionUUID,const QUuid& oldUUID);

public slots:

    /*@jsdoc
     * Checks whether there is an avatar within a specified distance from a point.
     * @function AvatarList.isAvatarInRange
     * @param {string} position - The test position.
     * @param {string} range - The test distance.
     * @returns {boolean} <code>true</code> if there's an avatar within the specified distance of the point, <code>false</code> 
     *     if not.
     */
    bool isAvatarInRange(const glm::vec3 & position, const float range);

protected slots:

    /*@jsdoc
     * @function AvatarList.sessionUUIDChanged
     * @param {Uuid} sessionUUID - New session ID.
     * @param {Uuid} oldSessionUUID - Old session ID.
     * @deprecated This function is deprecated and will be removed.
     */
    void sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID);

    /*@jsdoc
     * @function AvatarList.processAvatarDataPacket
     * @param {object} message - Message.
     * @param {object} sendingNode - Sending node.
     * @deprecated This function is deprecated and will be removed.
     */
    void processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
   
    /*@jsdoc
     * @function AvatarList.processAvatarIdentityPacket
     * @param {object} message - Message.
     * @param {object} sendingNode - Sending node.
     * @deprecated This function is deprecated and will be removed.
     */
    void processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    
    /*@jsdoc
     * @function AvatarList.processBulkAvatarTraits
     * @param {object} message - Message.
     * @param {object} sendingNode - Sending node.
     * @deprecated This function is deprecated and will be removed.
     */
    void processBulkAvatarTraits(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    
    /*@jsdoc
     * @function AvatarList.processKillAvatar
     * @param {object} message - Message.
     * @param {object} sendingNode - Sending node.
     * @deprecated This function is deprecated and will be removed.
     */
    void processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

protected:
    AvatarHashMap();

    virtual AvatarSharedPointer parseAvatarData(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    virtual AvatarSharedPointer newSharedAvatar(const QUuid& sessionUUID);
    virtual AvatarSharedPointer addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer);
    AvatarSharedPointer newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer,
        bool& isNew);
    virtual AvatarSharedPointer findAvatar(const QUuid& sessionUUID) const; // uses a QReadLocker on the hashLock
    virtual void removeAvatar(const QUuid& sessionUUID, KillAvatarReason removalReason = KillAvatarReason::NoReason);
    
    virtual void handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason = KillAvatarReason::NoReason);
    
    mutable QReadWriteLock _hashLock;
    AvatarHash _avatarHash;

    std::unordered_map<QUuid, AvatarTraits::TraitVersions> _processedTraitVersions;
    AvatarReplicas _replicas;

private:
    QUuid _lastOwnerSessionUUID;
};

#endif // hifi_AvatarHashMap_h
