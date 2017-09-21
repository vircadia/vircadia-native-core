//
//  EntityScriptingInterface.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// TODO: How will we handle collision callbacks with Entities
//

#ifndef hifi_EntityScriptingInterface_h
#define hifi_EntityScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>

#include <DependencyManager.h>
#include <Octree.h>
#include <OctreeScriptingInterface.h>
#include <RegisteredMetaTypes.h>
#include <PointerEvent.h>

#include "PolyVoxEntityItem.h"
#include "LineEntityItem.h"
#include "PolyLineEntityItem.h"
#include "EntityTree.h"

#include "EntityEditPacketSender.h"
#include "EntitiesScriptEngineProvider.h"
#include "EntityItemProperties.h"

#include "BaseScriptEngine.h"

class EntityTree;
class MeshProxy;

// helper factory to compose standardized, async metadata queries for "magic" Entity properties
// like .script and .serverScripts.  This is used for automated testing of core scripting features
// as well as to provide early adopters a self-discoverable, consistent way to diagnose common
// problems with their own Entity scripts.
class EntityPropertyMetadataRequest {
public:
    EntityPropertyMetadataRequest(BaseScriptEngine* engine) : _engine(engine) {};
    bool script(EntityItemID entityID, QScriptValue handler);
    bool serverScripts(EntityItemID entityID, QScriptValue handler);
private:
    QPointer<BaseScriptEngine> _engine;
};

class RayToEntityIntersectionResult {
public:
    RayToEntityIntersectionResult();
    bool intersects;
    bool accurate;
    QUuid entityID;
    float distance;
    BoxFace face;
    glm::vec3 intersection;
    glm::vec3 surfaceNormal;
    EntityItemPointer entity;
};

Q_DECLARE_METATYPE(RayToEntityIntersectionResult)

QScriptValue RayToEntityIntersectionResultToScriptValue(QScriptEngine* engine, const RayToEntityIntersectionResult& results);
void RayToEntityIntersectionResultFromScriptValue(const QScriptValue& object, RayToEntityIntersectionResult& results);


/**jsdoc
 * @namespace Entities
 */
/// handles scripting of Entity commands from JS passed to assigned clients
class EntityScriptingInterface : public OctreeScriptingInterface, public Dependency  {
    Q_OBJECT

    Q_PROPERTY(float currentAvatarEnergy READ getCurrentAvatarEnergy WRITE setCurrentAvatarEnergy)
    Q_PROPERTY(float costMultiplier READ getCostMultiplier WRITE setCostMultiplier)
    Q_PROPERTY(QUuid keyboardFocusEntity READ getKeyboardFocusEntity WRITE setKeyboardFocusEntity)

    friend EntityPropertyMetadataRequest;
public:
    EntityScriptingInterface(bool bidOnSimulationOwnership);

    class ActivityTracking {
    public:
        int addedEntityCount { 0 };
        int deletedEntityCount { 0 };
        int editedEntityCount { 0 };
    };

    EntityEditPacketSender* getEntityPacketSender() const { return (EntityEditPacketSender*)getPacketSender(); }
    virtual NodeType_t getServerNodeType() const override { return NodeType::EntityServer; }
    virtual OctreeEditPacketSender* createPacketSender() override { return new EntityEditPacketSender(); }

    void setEntityTree(EntityTreePointer modelTree);
    EntityTreePointer getEntityTree() { return _entityTree; }
    void setEntitiesScriptEngine(QSharedPointer<EntitiesScriptEngineProvider> engine);
    float calculateCost(float mass, float oldVelocity, float newVelocity);

    void resetActivityTracking();
    ActivityTracking getActivityTracking() const { return _activityTracking; }
public slots:

    /**jsdoc
     * Returns `true` if the DomainServer will allow this Node/Avatar to make changes
     *
     * @function Entities.canAdjustLocks
     * @return {bool} `true` if the client can adjust locks, `false` if not.
     */
    Q_INVOKABLE bool canAdjustLocks();

    /**jsdoc
     * @function Entities.canRez
     * @return {bool} `true` if the DomainServer will allow this Node/Avatar to rez new entities
     */
    Q_INVOKABLE bool canRez();

    /**jsdoc
     * @function Entities.canRezTmp
     * @return {bool} `true` if the DomainServer will allow this Node/Avatar to rez new temporary entities
     */
    Q_INVOKABLE bool canRezTmp();

    /**jsdoc
    * @function Entities.canWriteAsseets
    * @return {bool} `true` if the DomainServer will allow this Node/Avatar to write to the asset server
    */
    Q_INVOKABLE bool canWriteAssets();

    /**jsdoc
     * Add a new entity with the specified properties. If `clientOnly` is true, the entity will
     * not be sent to the server and will only be visible/accessible on the local client.
     *
     * @function Entities.addEntity
     * @param {EntityItemProperties} properties Properties of the entity to create.
     * @param {bool} [clientOnly=false] Whether the entity should only exist locally or not.
     * @return {EntityID} The entity ID of the newly created entity. The ID will be a null
     *     UUID (`{00000000-0000-0000-0000-000000000000}`) if the entity could not be created.
     */
    Q_INVOKABLE QUuid addEntity(const EntityItemProperties& properties, bool clientOnly = false);

    /// temporary method until addEntity can be used from QJSEngine
    /// Deliberately not adding jsdoc, only used internally.
    Q_INVOKABLE QUuid addModelEntity(const QString& name, const QString& modelUrl, const QString& shapeType, bool dynamic,
                                     const glm::vec3& position, const glm::vec3& gravity);

    /**jsdoc
     * Return the properties for the specified {EntityID}.
     * not be sent to the server and will only be visible/accessible on the local client.
     * @param {EntityItemProperties} properties Properties of the entity to create.
     * @param {EntityPropertyFlags} [desiredProperties=[]] Array containing the names of the properties you
     *     would like to get. If the array is empty, all properties will be returned.
     * @return {EntityItemProperties} The entity properties for the specified entity.
     */
    Q_INVOKABLE EntityItemProperties getEntityProperties(QUuid entityID);
    Q_INVOKABLE EntityItemProperties getEntityProperties(QUuid identity, EntityPropertyFlags desiredProperties);

    /**jsdoc
     * Updates an entity with the specified properties.
     *
     * @function Entities.editEntity
     * @return {EntityID} The EntityID of the entity if the edit was successful, otherwise the null {EntityID}.
     */
    Q_INVOKABLE QUuid editEntity(QUuid entityID, const EntityItemProperties& properties);

    /**jsdoc
     * Deletes an entity.
     *
     * @function Entities.deleteEntity
     * @param {EntityID} entityID The ID of the entity to delete.
     */
    Q_INVOKABLE void deleteEntity(QUuid entityID);

    /// Allows a script to call a method on an entity's script. The method will execute in the entity script
    /// engine. If the entity does not have an entity script or the method does not exist, this call will have
    /// no effect.
    /**jsdoc
     * Call a method on an entity. If it is running an entity script (specified by the `script` property)
     * and it exposes a property with the specified name `method`, it will be called
     * using `params` as the list of arguments.
     *
     * @function Entities.callEntityMethod
     * @param {EntityID} entityID The ID of the entity to call the method on.
     * @param {string} method The name of the method to call.
     * @param {string[]} params The list of parameters to call the specified method with.
     */
    Q_INVOKABLE void callEntityMethod(QUuid entityID, const QString& method, const QStringList& params = QStringList());

    /// finds the closest model to the center point, within the radius
    /// will return a EntityItemID.isKnownID = false if no models are in the radius
    /// this function will not find any models in script engine contexts which don't have access to models
    /**jsdoc
     */
    Q_INVOKABLE QUuid findClosestEntity(const glm::vec3& center, float radius) const;

    /// finds models within the search sphere specified by the center point and radius
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE QVector<QUuid> findEntities(const glm::vec3& center, float radius) const;

    /// finds models within the box specified by the corner and dimensions
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE QVector<QUuid> findEntitiesInBox(const glm::vec3& corner, const glm::vec3& dimensions) const;

    /// finds models within the frustum
    /// the frustum must have the following properties:
    /// - position
    /// - orientation
    /// - projection
    /// - centerRadius
    /// this function will not find any models in script engine contexts which don't have access to entities
    Q_INVOKABLE QVector<QUuid> findEntitiesInFrustum(QVariantMap frustum) const;

	/// finds entities of the indicated type within a sphere given by the center point and radius
	/// @param {QString} string representation of entity type
	/// @param {vec3} center point
	/// @param {float} radius to search
	/// this function will not find any entities in script engine contexts which don't have access to entities
	Q_INVOKABLE QVector<QUuid> findEntitiesByType(const QString entityType, const glm::vec3& center, float radius) const;

    /// If the scripting context has visible entities, this will determine a ray intersection, the results
    /// may be inaccurate if the engine is unable to access the visible entities, in which case result.accurate
    /// will be false.
    Q_INVOKABLE RayToEntityIntersectionResult findRayIntersection(const PickRay& ray, bool precisionPicking = false,
        const QScriptValue& entityIdsToInclude = QScriptValue(), const QScriptValue& entityIdsToDiscard = QScriptValue(),
        bool visibleOnly = false, bool collidableOnly = false);

    /// Same as above but with QVectors
    RayToEntityIntersectionResult findRayIntersectionVector(const PickRay& ray, bool precisionPicking,
        const QVector<EntityItemID>& entityIdsToInclude, const QVector<EntityItemID>& entityIdsToDiscard,
        bool visibleOnly, bool collidableOnly);

    /// If the scripting context has visible entities, this will determine a ray intersection, and will block in
    /// order to return an accurate result
    Q_INVOKABLE RayToEntityIntersectionResult findRayIntersectionBlocking(const PickRay& ray, bool precisionPicking = false, const QScriptValue& entityIdsToInclude = QScriptValue(), const QScriptValue& entityIdsToDiscard = QScriptValue());

    Q_INVOKABLE bool reloadServerScripts(QUuid entityID);

    /**jsdoc
     * Query additional metadata for "magic" Entity properties like `script` and `serverScripts`.
     *
     * @function Entities.queryPropertyMetadata
     * @param {EntityID} entityID The ID of the entity.
     * @param {string} property The name of the property extended metadata is wanted for.
     * @param {ResultCallback} callback Executes callback(err, result) with the query results.
     */
    /**jsdoc
     * Query additional metadata for "magic" Entity properties like `script` and `serverScripts`.
     *
     * @function Entities.queryPropertyMetadata
     * @param {EntityID} entityID The ID of the entity.
     * @param {string} property The name of the property extended metadata is wanted for.
     * @param {Object} thisObject The scoping "this" context that callback will be executed within.
     * @param {ResultCallback} callbackOrMethodName Executes thisObject[callbackOrMethodName](err, result) with the query results.
     */
    Q_INVOKABLE bool queryPropertyMetadata(QUuid entityID, QScriptValue property, QScriptValue scopeOrCallback, QScriptValue methodOrName = QScriptValue());

    Q_INVOKABLE bool getServerScriptStatus(QUuid entityID, QScriptValue callback);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE void setLightsArePickable(bool value);
    Q_INVOKABLE bool getLightsArePickable() const;

    // FIXME move to a renderable entity interface
    Q_INVOKABLE void setZonesArePickable(bool value);
    Q_INVOKABLE bool getZonesArePickable() const;

    // FIXME move to a renderable entity interface
    Q_INVOKABLE void setDrawZoneBoundaries(bool value);
    Q_INVOKABLE bool getDrawZoneBoundaries() const;

    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setVoxelSphere(QUuid entityID, const glm::vec3& center, float radius, int value);
    Q_INVOKABLE bool setVoxelCapsule(QUuid entityID, const glm::vec3& start, const glm::vec3& end, float radius, int value);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setVoxel(QUuid entityID, const glm::vec3& position, int value);
    Q_INVOKABLE bool setAllVoxels(QUuid entityID, int value);
    Q_INVOKABLE bool setVoxelsInCuboid(QUuid entityID, const glm::vec3& lowPosition,
                                       const glm::vec3& cuboidSize, int value);

    Q_INVOKABLE bool setAllPoints(QUuid entityID, const QVector<glm::vec3>& points);
    Q_INVOKABLE bool appendPoint(QUuid entityID, const glm::vec3& point);

    Q_INVOKABLE void dumpTree() const;

    Q_INVOKABLE QUuid addAction(const QString& actionTypeString, const QUuid& entityID, const QVariantMap& arguments);
    Q_INVOKABLE bool updateAction(const QUuid& entityID, const QUuid& actionID, const QVariantMap& arguments);
    Q_INVOKABLE bool deleteAction(const QUuid& entityID, const QUuid& actionID);
    Q_INVOKABLE QVector<QUuid> getActionIDs(const QUuid& entityID);
    Q_INVOKABLE QVariantMap getActionArguments(const QUuid& entityID, const QUuid& actionID);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 voxelCoordsToWorldCoords(const QUuid& entityID, glm::vec3 voxelCoords);
    Q_INVOKABLE glm::vec3 worldCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 worldCoords);
    Q_INVOKABLE glm::vec3 voxelCoordsToLocalCoords(const QUuid& entityID, glm::vec3 voxelCoords);
    Q_INVOKABLE glm::vec3 localCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 localCoords);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 getAbsoluteJointTranslationInObjectFrame(const QUuid& entityID, int jointIndex);
    Q_INVOKABLE glm::quat getAbsoluteJointRotationInObjectFrame(const QUuid& entityID, int jointIndex);
    Q_INVOKABLE bool setAbsoluteJointTranslationInObjectFrame(const QUuid& entityID, int jointIndex, glm::vec3 translation);
    Q_INVOKABLE bool setAbsoluteJointRotationInObjectFrame(const QUuid& entityID, int jointIndex, glm::quat rotation);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 getLocalJointTranslation(const QUuid& entityID, int jointIndex);
    Q_INVOKABLE glm::quat getLocalJointRotation(const QUuid& entityID, int jointIndex);
    Q_INVOKABLE bool setLocalJointTranslation(const QUuid& entityID, int jointIndex, glm::vec3 translation);
    Q_INVOKABLE bool setLocalJointRotation(const QUuid& entityID, int jointIndex, glm::quat rotation);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setLocalJointRotations(const QUuid& entityID, const QVector<glm::quat>& rotations);
    Q_INVOKABLE bool setLocalJointTranslations(const QUuid& entityID, const QVector<glm::vec3>& translations);
    Q_INVOKABLE bool setLocalJointsData(const QUuid& entityID,
                                        const QVector<glm::quat>& rotations,
                                        const QVector<glm::vec3>& translations);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE int getJointIndex(const QUuid& entityID, const QString& name);
    Q_INVOKABLE QStringList getJointNames(const QUuid& entityID);


    Q_INVOKABLE QVector<QUuid> getChildrenIDs(const QUuid& parentID);
    Q_INVOKABLE QVector<QUuid> getChildrenIDsOfJoint(const QUuid& parentID, int jointIndex);
    Q_INVOKABLE bool isChildOfParent(QUuid childID, QUuid parentID);

    Q_INVOKABLE QString getNestableType(QUuid id);

    Q_INVOKABLE QUuid getKeyboardFocusEntity() const;
    Q_INVOKABLE void setKeyboardFocusEntity(QUuid id);

    Q_INVOKABLE void sendMousePressOnEntity(QUuid id, PointerEvent event);
    Q_INVOKABLE void sendMouseMoveOnEntity(QUuid id, PointerEvent event);
    Q_INVOKABLE void sendMouseReleaseOnEntity(QUuid id, PointerEvent event);

    Q_INVOKABLE void sendClickDownOnEntity(QUuid id, PointerEvent event);
    Q_INVOKABLE void sendHoldingClickOnEntity(QUuid id, PointerEvent event);
    Q_INVOKABLE void sendClickReleaseOnEntity(QUuid id, PointerEvent event);

    Q_INVOKABLE void sendHoverEnterEntity(QUuid id, PointerEvent event);
    Q_INVOKABLE void sendHoverOverEntity(QUuid id, PointerEvent event);
    Q_INVOKABLE void sendHoverLeaveEntity(QUuid id, PointerEvent event);

    Q_INVOKABLE bool wantsHandControllerPointerEvents(QUuid id);

    Q_INVOKABLE void emitScriptEvent(const EntityItemID& entityID, const QVariant& message);

    Q_INVOKABLE bool AABoxIntersectsCapsule(const glm::vec3& low, const glm::vec3& dimensions,
                                            const glm::vec3& start, const glm::vec3& end, float radius);

    // FIXME move to a renderable entity interface
    Q_INVOKABLE void getMeshes(QUuid entityID, QScriptValue callback);

    /**jsdoc
     * Returns object to world transform, excluding scale
     *
     * @function Entities.getEntityTransform
     * @param {EntityID} entityID The ID of the entity whose transform is to be returned
     * @return {Mat4} Entity's object to world transform, excluding scale
     */
    Q_INVOKABLE glm::mat4 getEntityTransform(const QUuid& entityID);


    /**jsdoc
     * Returns object to world transform, excluding scale
     *
     * @function Entities.getEntityLocalTransform
     * @param {EntityID} entityID The ID of the entity whose local transform is to be returned
     * @return {Mat4} Entity's object to parent transform, excluding scale
     */
    Q_INVOKABLE glm::mat4 getEntityLocalTransform(const QUuid& entityID);

signals:
    void collisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

    void canAdjustLocksChanged(bool canAdjustLocks);
    void canRezChanged(bool canRez);
    void canRezTmpChanged(bool canRez);
    void canWriteAssetsChanged(bool canWriteAssets);

    void mousePressOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseMoveOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void clickDownOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void holdingClickOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void clickReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void hoverEnterEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void hoverOverEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void hoverLeaveEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void enterEntity(const EntityItemID& entityItemID);
    void leaveEntity(const EntityItemID& entityItemID);

    void deletingEntity(const EntityItemID& entityID);
    void addingEntity(const EntityItemID& entityID);
    void clearingEntities();
    void debitEnergySource(float value);

    void webEventReceived(const EntityItemID& entityItemID, const QVariant& message);

protected:
    void withEntitiesScriptEngine(std::function<void(QSharedPointer<EntitiesScriptEngineProvider>)> function) {
        std::lock_guard<std::recursive_mutex> lock(_entitiesScriptEngineLock);
        function(_entitiesScriptEngine);
    };
private:
    bool actionWorker(const QUuid& entityID, std::function<bool(EntitySimulationPointer, EntityItemPointer)> actor);
    bool polyVoxWorker(QUuid entityID, std::function<bool(PolyVoxEntityItem&)> actor);
    bool setPoints(QUuid entityID, std::function<bool(LineEntityItem&)> actor);
    void queueEntityMessage(PacketType packetType, EntityItemID entityID, const EntityItemProperties& properties);

    EntityItemPointer checkForTreeEntityAndTypeMatch(const QUuid& entityID,
                                                     EntityTypes::EntityType entityType = EntityTypes::Unknown);


    /// actually does the work of finding the ray intersection, can be called in locking mode or tryLock mode
    RayToEntityIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType,
        bool precisionPicking, const QVector<EntityItemID>& entityIdsToInclude, const QVector<EntityItemID>& entityIdsToDiscard,
        bool visibleOnly = false, bool collidableOnly = false);

    EntityTreePointer _entityTree;

    std::recursive_mutex _entitiesScriptEngineLock;
    QSharedPointer<EntitiesScriptEngineProvider> _entitiesScriptEngine;

    bool _bidOnSimulationOwnership { false };
    float _currentAvatarEnergy = { FLT_MAX };
    float getCurrentAvatarEnergy() { return _currentAvatarEnergy; }
    void setCurrentAvatarEnergy(float energy);

    ActivityTracking _activityTracking;
    float costMultiplier = { 0.01f };
    float getCostMultiplier();
    void setCostMultiplier(float value);
};

#endif // hifi_EntityScriptingInterface_h
