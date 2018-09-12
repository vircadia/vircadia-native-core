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

#include <set>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <AvatarHashMap.h>
#include <PhysicsEngine.h>
#include <PIDController.h>
#include <SimpleMovingAverage.h>
#include <shared/RateCounter.h>
#include <avatars-renderer/ScriptAvatar.h>
#include <AudioInjector.h>
#include <workload/Space.h>

#include "AvatarMotionState.h"
#include "MyAvatar.h"
#include "OtherAvatar.h"

using SortedAvatar = std::pair<float, std::shared_ptr<Avatar>>;

/**jsdoc 
 * The AvatarManager API has properties and methods which manage Avatars within the same domain.
 *
 * <p><strong>Note:</strong> This API is also provided to Interface and client entity scripts as the synonym, 
 * <code>AvatarList</code>. For assignment client scripts, see the separate {@link AvatarList} API.
 *
 * @namespace AvatarManager
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @borrows AvatarList.getAvatarIdentifiers as getAvatarIdentifiers
 * @borrows AvatarList.getAvatarsInRange as getAvatarsInRange
 * @borrows AvatarList.avatarAddedEvent as avatarAddedEvent
 * @borrows AvatarList.avatarRemovedEvent as avatarRemovedEvent
 * @borrows AvatarList.avatarSessionChangedEvent as avatarSessionChangedEvent
 * @borrows AvatarList.isAvatarInRange as isAvatarInRange
 * @borrows AvatarList.sessionUUIDChanged as sessionUUIDChanged
 * @borrows AvatarList.processAvatarDataPacket as processAvatarDataPacket
 * @borrows AvatarList.processAvatarIdentityPacket as processAvatarIdentityPacket
 * @borrows AvatarList.processKillAvatar as processKillAvatar
 */

class AvatarManager : public AvatarHashMap {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    /// Registers the script types associated with the avatar manager.
    static void registerMetaTypes(QScriptEngine* engine);

    virtual ~AvatarManager();

    void init();
    void setSpace(workload::SpacePointer& space );

    std::shared_ptr<MyAvatar> getMyAvatar() { return _myAvatar; }
    glm::vec3 getMyAvatarPosition() const { return _myAvatar->getWorldPosition(); }

    /**jsdoc 
     * @function AvatarManager.getAvatar
     * @param {Uuid} avatarID
     * @returns {AvatarData}
     */
    // Null/Default-constructed QUuids will return MyAvatar
    Q_INVOKABLE virtual ScriptAvatarData* getAvatar(QUuid avatarID) override { return new ScriptAvatar(getAvatarBySessionID(avatarID)); }

    AvatarSharedPointer getAvatarBySessionID(const QUuid& sessionID) const override;

    int getNumAvatarsUpdated() const { return _numAvatarsUpdated; }
    int getNumAvatarsNotUpdated() const { return _numAvatarsNotUpdated; }
    float getAvatarSimulationTime() const { return _avatarSimulationTime; }

    void updateMyAvatar(float deltaTime);
    void updateOtherAvatars(float deltaTime);
    void sendIdentityRequest(const QUuid& avatarID) const;

    void setMyAvatarDataPacketsPaused(bool puase);

    void postUpdate(float deltaTime, const render::ScenePointer& scene);

    void clearOtherAvatars() override;
    void deleteAllAvatars();

    void getObjectsToRemoveFromPhysics(VectorOfMotionStates& motionStates);
    void getObjectsToAddToPhysics(VectorOfMotionStates& motionStates);
    void getObjectsToChange(VectorOfMotionStates& motionStates);

    void handleChangedMotionStates(const VectorOfMotionStates& motionStates);
    void handleCollisionEvents(const CollisionEvents& collisionEvents);

    /**jsdoc
     * @function AvatarManager.getAvatarDataRate
     * @param {Uuid} sessionID
     * @param {string} [rateName=""]
     * @returns {number}
     */
    Q_INVOKABLE float getAvatarDataRate(const QUuid& sessionID, const QString& rateName = QString("")) const;

    /**jsdoc
     * @function AvatarManager.getAvatarUpdateRate
     * @param {Uuid} sessionID
     * @param {string} [rateName=""]
     * @returns {number}
     */
    Q_INVOKABLE float getAvatarUpdateRate(const QUuid& sessionID, const QString& rateName = QString("")) const;

    /**jsdoc
     * @function AvatarManager.getAvatarSimulationRate
     * @param {Uuid} sessionID
     * @param {string} [rateName=""]
     * @returns {number}
     */
    Q_INVOKABLE float getAvatarSimulationRate(const QUuid& sessionID, const QString& rateName = QString("")) const;

    /**jsdoc
     * @function AvatarManager.findRayIntersection
     * @param {PickRay} ray
     * @param {Uuid[]} [avatarsToInclude=[]]
     * @param {Uuid[]} [avatarsToDiscard=[]]
     * @returns {RayToAvatarIntersectionResult}
     */
    Q_INVOKABLE RayToAvatarIntersectionResult findRayIntersection(const PickRay& ray,
                                                                  const QScriptValue& avatarIdsToInclude = QScriptValue(),
                                                                  const QScriptValue& avatarIdsToDiscard = QScriptValue());
    /**jsdoc
     * @function AvatarManager.findRayIntersectionVector
     * @param {PickRay} ray
     * @param {Uuid[]} avatarsToInclude
     * @param {Uuid[]} avatarsToDiscard
     * @returns {RayToAvatarIntersectionResult}
     */
    Q_INVOKABLE RayToAvatarIntersectionResult findRayIntersectionVector(const PickRay& ray,
                                                                        const QVector<EntityItemID>& avatarsToInclude,
                                                                        const QVector<EntityItemID>& avatarsToDiscard);

    Q_INVOKABLE ParabolaToAvatarIntersectionResult findParabolaIntersectionVector(const PickParabola& pick,
                                                                                  const QVector<EntityItemID>& avatarsToInclude,
                                                                                  const QVector<EntityItemID>& avatarsToDiscard);

    /**jsdoc
     * @function AvatarManager.getAvatarSortCoefficient
     * @param {string} name
     * @returns {number}
     */
    // TODO: remove this HACK once we settle on optimal default sort coefficients
    Q_INVOKABLE float getAvatarSortCoefficient(const QString& name);

    /**jsdoc
     * @function AvatarManager.setAvatarSortCoefficient
     * @param {string} name
     * @param {number} value
     */
    Q_INVOKABLE void setAvatarSortCoefficient(const QString& name, const QScriptValue& value);

    /**jsdoc
     * Used in the PAL for getting PAL-related data about avatars nearby. Using this method is faster
     * than iterating over each avatar and obtaining data about them in JavaScript, as that method
     * locks and unlocks each avatar's data structure potentially hundreds of times per update tick.
     * @function AvatarManager.getPalData
     * @param {string[]} specificAvatarIdentifiers - A list of specific Avatar Identifiers about
     * which you want to get PAL data
     * @returns {object}
     */
    Q_INVOKABLE QVariantMap getPalData(const QList<QString> specificAvatarIdentifiers = QList<QString>());

    float getMyAvatarSendRate() const { return _myAvatarSendRate.rate(); }
    int getIdentityRequestsSent() const { return _identityRequestsSent; }

    void queuePhysicsChange(const OtherAvatarPointer& avatar);
    void buildPhysicsTransaction(PhysicsEngine::Transaction& transaction);
    void handleProcessedPhysicsTransaction(PhysicsEngine::Transaction& transaction);

public slots:
    /**jsdoc
     * @function AvatarManager.updateAvatarRenderStatus
     * @param {boolean} shouldRenderAvatars
     */
    void updateAvatarRenderStatus(bool shouldRenderAvatars);

protected:
    AvatarSharedPointer addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) override;

private:
    explicit AvatarManager(QObject* parent = 0);
    explicit AvatarManager(const AvatarManager& other);

    void simulateAvatarFades(float deltaTime);

    AvatarSharedPointer newSharedAvatar() override;
    void handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason = KillAvatarReason::NoReason) override;

    QVector<AvatarSharedPointer> _avatarsToFade;

    using SetOfOtherAvatars = std::set<OtherAvatarPointer>;
    SetOfOtherAvatars _avatarsToChangeInPhysics;

    std::shared_ptr<MyAvatar> _myAvatar;
    quint64 _lastSendAvatarDataTime = 0; // Controls MyAvatar send data rate.

    std::list<AudioInjectorPointer> _collisionInjectors;

    RateCounter<> _myAvatarSendRate;
    int _numAvatarsUpdated { 0 };
    int _numAvatarsNotUpdated { 0 };
    float _avatarSimulationTime { 0.0f };
    bool _shouldRender { true };
    bool _myAvatarDataPacketsPaused { false };
    mutable int _identityRequestsSent { 0 };

    mutable std::mutex _spaceLock;
    workload::SpacePointer _space;
    std::vector<int32_t> _spaceProxiesToDelete;
};

#endif // hifi_AvatarManager_h
