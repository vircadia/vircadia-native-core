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
#include <PIDController.h>
#include <SimpleMovingAverage.h>
#include <shared/RateCounter.h>
#include <avatars-renderer/ScriptAvatar.h>
#include <AudioInjector.h>

#include "AvatarMotionState.h"
#include "MyAvatar.h"

/**jsdoc 
 * The AvatarManager API has properties and methods which manage Avatars within the same domain.
 * @namespace AvatarManager
 */

class AvatarManager : public AvatarHashMap {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    // JSDOCS Copied over from AvatarHashMap (see AvatarHashMap.h for reason)

    /**jsdoc
     * @function AvatarManager.getAvatarIdentifiers
     */

    /**jsdoc
     * @function AvatarManager.getAvatarsInRange
     * @param {Vec3} position
     * @param {float} rangeMeters
     * @returns {string[]}
     */

    /**jsdoc
     * @function AvatarManager.getAvatar
     * @param {string} avatarID
     * @returns {ScriptAvatarData}
     */

    /**jsdoc
     * @function AvatarManager.avatarAddedEvent
     * @param {string} sessionUUID
     * @returns {Signal}
     */

    /**jsdoc
     * @function AvatarManager.avatarRemovedEvent
     * @param {string} sessionUUID
     * @returns {Signal}
     */

    /**jsdoc
     * @function AvatarManager.avatarSessionChangedEvent
     * @param {string} sessionUUID
     * @param {string} oldUUID
     * @returns {Signal}
     */

    /**jsdoc
     * @function AvatarManager.isAvatarInRange
     * @param {string} position
     * @param {string} range
     * @returns {boolean}
     */

    /**jsdoc
     * @function AvatarManager.sessionUUIDChanged
     * @param {string} sessionUUID
     * @param {string} oldUUID
     */

    /**jsdoc
     * @function AvatarManager.processAvatarDataPacket
     * @param {} message
     * @param {} sendingNode
     */

    /**jsdoc
     * @function AvatarManager.processAvatarIdentityPacket
     * @param {} message
     * @param {} sendingNode
     */

    /**jsdoc
     * @function AvatarManager.processKillAvatar
     * @param {} message
     * @param {} sendingNode
     */

    /// Registers the script types associated with the avatar manager.
    static void registerMetaTypes(QScriptEngine* engine);

    virtual ~AvatarManager();

    void init();

    std::shared_ptr<MyAvatar> getMyAvatar() { return _myAvatar; }
    glm::vec3 getMyAvatarPosition() const { return _myAvatar->getWorldPosition(); }

    /**jsdoc 
     * @function AvatarManager.getAvatar
     * @param {string} avatarID
     * @returns {}
     */

    // Null/Default-constructed QUuids will return MyAvatar
    Q_INVOKABLE virtual ScriptAvatarData* getAvatar(QUuid avatarID) override { return new ScriptAvatar(getAvatarBySessionID(avatarID)); }

    AvatarSharedPointer getAvatarBySessionID(const QUuid& sessionID) const override;

    int getNumAvatarsUpdated() const { return _numAvatarsUpdated; }
    int getNumAvatarsNotUpdated() const { return _numAvatarsNotUpdated; }
    float getAvatarSimulationTime() const { return _avatarSimulationTime; }

    void updateMyAvatar(float deltaTime);
    void updateOtherAvatars(float deltaTime);

    void postUpdate(float deltaTime, const render::ScenePointer& scene);

    void clearOtherAvatars();
    void deleteAllAvatars();

    void getObjectsToRemoveFromPhysics(VectorOfMotionStates& motionStates);
    void getObjectsToAddToPhysics(VectorOfMotionStates& motionStates);
    void getObjectsToChange(VectorOfMotionStates& motionStates);
    void handleChangedMotionStates(const VectorOfMotionStates& motionStates);
    void handleCollisionEvents(const CollisionEvents& collisionEvents);

    /**jsdoc
     * @function AvatarManager.getAvatarDataRate
     * @param {string} sessionID
     * @param {string} [rateName=""]
     * @returns {number}
     */

    Q_INVOKABLE float getAvatarDataRate(const QUuid& sessionID, const QString& rateName = QString("")) const;
    
    /**jsdoc
     * @function AvatarManager.getAvatarUpdateRate
     * @param {string} sessionID
     * @param {string} [rateName=""]
     * @returns {number}
     */
    
    Q_INVOKABLE float getAvatarUpdateRate(const QUuid& sessionID, const QString& rateName = QString("")) const;
    
    /**jsdoc
     * @function AvatarManager.getAvatarSimulationRate
     * @param {string} sessionID
     * @param {string} [rateName=""]
     * @returns {number}
     */
    
    Q_INVOKABLE float getAvatarSimulationRate(const QUuid& sessionID, const QString& rateName = QString("")) const;

    /**jsdoc
     * @function AvatarManager.findRayIntersection
     * @param {PickRay} ray
     * @param {} avatarIdsToInclude
     * @param {} avatarIdsToDiscard
     * @returns {RayToAvatarIntersectionResult}
     */
    
    Q_INVOKABLE RayToAvatarIntersectionResult findRayIntersection(const PickRay& ray,
                                                                  const QScriptValue& avatarIdsToInclude = QScriptValue(),
                                                                  const QScriptValue& avatarIdsToDiscard = QScriptValue());
    /**jsdoc
     * @function AvatarManager.findRayIntersectionVector
     * @param {PickRay} ray
     * @param {} avatarsToInclude
     * @param {} avatarIdsToDiscard
     * @returns {RayToAvatarIntersectionResult}
     */

    Q_INVOKABLE RayToAvatarIntersectionResult findRayIntersectionVector(const PickRay& ray,
                                                                        const QVector<EntityItemID>& avatarsToInclude,
                                                                        const QVector<EntityItemID>& avatarsToDiscard);

    // TODO: remove this HACK once we settle on optimal default sort coefficients
    /**jsdoc
     * @function AvatarManager.getAvatarSortCoefficient
     * @param {string} name
     * @returns {number}
     */
    
    Q_INVOKABLE float getAvatarSortCoefficient(const QString& name);
   
    /**jsdoc
     * @function AvatarManager.setAvatarSortCoefficient
     * @param {string} name
     * @param {string} value
     */

    Q_INVOKABLE void setAvatarSortCoefficient(const QString& name, const QScriptValue& value);

    float getMyAvatarSendRate() const { return _myAvatarSendRate.rate(); }

public slots:

    /**jsdoc
     * @function AvatarManager.updateAvatarRenderStatus
     * @param {boolean} shouldRenderAvatars
     */

    void updateAvatarRenderStatus(bool shouldRenderAvatars);

private:
    explicit AvatarManager(QObject* parent = 0);
    explicit AvatarManager(const AvatarManager& other);

    void simulateAvatarFades(float deltaTime);

    AvatarSharedPointer newSharedAvatar() override;
    void deleteMotionStates();
    void handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason = KillAvatarReason::NoReason) override;

    QVector<AvatarSharedPointer> _avatarsToFade;

    using AvatarMotionStateMap = QMap<Avatar*, AvatarMotionState*>;
    AvatarMotionStateMap _motionStates;
    VectorOfMotionStates _motionStatesToRemoveFromPhysics;
    VectorOfMotionStates _motionStatesToDelete;
    SetOfMotionStates _motionStatesToAddToPhysics;

    std::shared_ptr<MyAvatar> _myAvatar;
    quint64 _lastSendAvatarDataTime = 0; // Controls MyAvatar send data rate.

    std::list<AudioInjectorPointer> _collisionInjectors;

    RateCounter<> _myAvatarSendRate;
    int _numAvatarsUpdated { 0 };
    int _numAvatarsNotUpdated { 0 };
    float _avatarSimulationTime { 0.0f };
    bool _shouldRender { true };
};

#endif // hifi_AvatarManager_h
