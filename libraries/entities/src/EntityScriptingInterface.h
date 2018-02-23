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

/**jsdoc
 * The result of a {@link PickRay} search using {@link Entities.findRayIntersection|findRayIntersection} or 
 * {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}.
 * @typedef {object} Entities.RayToEntityIntersectionResult
 * @property {boolean} intersects - <code>true</code> if the {@link PickRay} intersected an entity, otherwise 
 *     <code>false</code>.
 * @property {boolean} accurate - Is always <code>true</code>.
 * @property {Uuid} entityID - The ID if the entity intersected, if any, otherwise <code>null</code>.
 * @property {number} distance - The distance from the {@link PickRay} origin to the intersection point.
 * @property {Vec3} intersection - The intersection point.
 * @property {Vec3} surfaceNormal - The surface normal of the entity at the intersection point.
 * @property {BoxFace} face - The face of the entity's axis-aligned box that the ray intersects.
 * @property {object} extraInfo - Extra information depending on the entity intersected. Currently, only <code>Model</code> 
 *     entities provide extra information, and the information provided depends on the <code>precisionPicking</code> parameter 
 *     value that the search function was called with.
 */
// "accurate" is currently always true because the ray intersection is always performed with an Octree::Lock.
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
    QVariantMap extraInfo;
};

Q_DECLARE_METATYPE(RayToEntityIntersectionResult)

QScriptValue RayToEntityIntersectionResultToScriptValue(QScriptEngine* engine, const RayToEntityIntersectionResult& results);
void RayToEntityIntersectionResultFromScriptValue(const QScriptValue& object, RayToEntityIntersectionResult& results);


/**jsdoc
 * The Entities API provides facilities to create and interact with entities. Entities are 2D and 3D objects that are visible
 * to everyone and typically are persisted to the domain. For Interface scripts, the entities available are those that 
 * Interface has displayed and so knows about.
 *
 * @namespace Entities
 * @property {number} currentAvatarEnergy - <strong>Deprecated</strong>
 * @property {number} costMultiplier - <strong>Deprecated</strong>
 * @property {Uuid} keyboardFocusEntity - Get or set the {@link Entities.EntityType|Web} entity that has keyboard focus.
 *     If no entity has keyboard focus, get returns <code>null</code>; set to <code>null</code> or {@link Uuid|Uuid.NULL} to 
 *     clear keyboard focus.
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
     * Check whether or not you can change the <code>locked</code> property of entities. Locked entities have their 
     * <code>locked</code> property set to <code>true</code> and cannot be edited or deleted. Whether or not you can change 
     * entities' <code>locked</code> properties is configured in the domain server's permissions.
     * @function Entities.canAdjustLocks
     * @returns {boolean} <code>true</code> if the client can change the <code>locked</code> property of entities,
     *     otherwise <code>false</code>.
     * @example <caption>Set an entity's <code>locked</code> property to true if you can.</caption>
     * if (Entities.canAdjustLocks()) {
     *     Entities.editEntity(entityID, { locked: true });
     * } else {
     *     Window.alert("You do not have the permissions to set an entity locked!");
     * }
     */
    Q_INVOKABLE bool canAdjustLocks();

    /**jsdoc
     * Check whether or not you can rez (create) new entities in the domain.
     * @function Entities.canRez
     * @returns {boolean} <code>true</code> if the domain server will allow this script to rez (create) new entities, 
     *     otherwise <code>false</code>.
     */
    Q_INVOKABLE bool canRez();

    /**jsdoc
     * Check whether or not you can rez (create) new temporary entities in the domain. Temporary entities are entities with a
     * finite <code>lifetime</code> property value set.
     * @function Entities.canRezTmp
     * @returns {boolean} <code>true</code> if the domain server will allow this script to rez (create) new temporary
     *     entities, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool canRezTmp();

    /**jsdoc
     * Check whether or not you can rez (create) new certified entities in the domain. Certified entities are entities that have
     * PoP certificates.
     * @function Entities.canRezCertified
     * @returns {boolean} <code>true</code> if the domain server will allow this script to rez (create) new certified
     *     entities, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool canRezCertified();

    /**jsdoc
     * Check whether or not you can rez (create) new temporary certified entities in the domain. Temporary entities are entities
     * with a finite  <code>lifetime</code> property value set. Certified entities are entities that have PoP certificates.
     * @function Entities.canRezTmpCertified
     * @returns {boolean} <code>true</code> if the domain server will allow this script to rez (create) new temporary 
     *     certified entities, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool canRezTmpCertified();

    /**jsdoc
     * Check whether or not you can make changes to the asset server's assets.
     * @function Entities.canWriteAssets
     * @returns {boolean} <code>true</code> if the domain server will allow this script to make changes to the to the asset 
     *     server's assets, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool canWriteAssets();

    /**jsdoc
     * Add a new entity with specified properties.
     * @function Entities.addEntity
     * @param {Entities.EntityProperties} properties - The properties of the entity to create.
     * @param {boolean} [clientOnly=false] - If <code>true</code>, the entity is created as an avatar entity, otherwise it
     *     is created on the server. An avatar entity follows you to each domain you visit, rendering at the same world 
     *     coordinates unless it's parented to your avatar.
     * @returns {Uuid} The ID of the entity if successfully created, otherwise {@link Uuid|Uuid.NULL}.
     * @example <caption>Create a box entity in front of your avatar.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     * print("Entity created: " + entityID);
     */
    Q_INVOKABLE QUuid addEntity(const EntityItemProperties& properties, bool clientOnly = false);

    /// temporary method until addEntity can be used from QJSEngine
    /// Deliberately not adding jsdoc, only used internally.
    Q_INVOKABLE QUuid addModelEntity(const QString& name, const QString& modelUrl, const QString& shapeType, bool dynamic,
                                     const glm::vec3& position, const glm::vec3& gravity);

    /**jsdoc
     * Get the properties of an entity.
     * @function Entities.getEntityProperties
     * @param {Uuid} entityID - The ID of the entity to get the properties of.
     * @param {string[]} [desiredProperties=[]] - Array of the names of the properties to get. If the array is empty,
     *     all properties are returned.
     * @returns {Entities.EntityProperties} The properties of the entity; <code>{}</code> if <code>entityID</code> is not the 
     *     ID of an entity.
     * @example <caption>Report the color of a new box entity.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     * var properties = Entities.getEntityProperties(entityID, ["color"]);
     * print("Entity color: " + JSON.stringify(properties.color));
     */
    Q_INVOKABLE EntityItemProperties getEntityProperties(QUuid entityID);
    Q_INVOKABLE EntityItemProperties getEntityProperties(QUuid identity, EntityPropertyFlags desiredProperties);

    /**jsdoc
     * Update an entity with specified properties.
     * @function Entities.editEntity
     * @param {Uuid} entityID - The ID of the entity to edit.
     * @param {Entities.EntityProperties} properties - The properties to update the entity with.
     * @returns {Uuid} The ID of the entity if the edit was successful, otherwise <code>null</code>.
     * @example <caption>Change the color of an entity.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     * var properties = Entities.getEntityProperties(entityID, ["color"]);
     * print("Entity color: " + JSON.stringify(properties.color));
     *
     * Entities.editEntity(entityID, {
     *     color: { red: 255, green: 0, blue: 0 }
     * });
     * properties = Entities.getEntityProperties(entityID, ["color"]);
     * print("Entity color: " + JSON.stringify(properties.color));
     */
    Q_INVOKABLE QUuid editEntity(QUuid entityID, const EntityItemProperties& properties);

    /**jsdoc
     * Delete an entity.
     * @function Entities.deleteEntity
     * @param {Uuid} entityID - The ID of the entity to delete.
     * @example <caption>Delete an entity a few seconds after creating it.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     *
     * Script.setTimeout(function () {
     *     Entities.deleteEntity(entityID);
     * }, 3000);
     */
    Q_INVOKABLE void deleteEntity(QUuid entityID);

    /**jsdoc
     * Call a method on an entity in the same context as this function is called. Allows a script 
     * to call a method on an entity's script. The method will execute in the entity script engine. 
     * If the entity does not have an  entity script or the method does not exist, this call will 
     * have no effect. If it is running an entity script (specified by the `script` property)
     * and it exposes a property with the specified name `method`, it will be called
     * using `params` as the list of arguments. If this is called within an entity script, the
     * method will be executed on the client in the entity script engine in which it was called. If
     * this is called in an entity server script, the method will be executed on the entity server 
     * script engine.
     *
     * @function Entities.callEntityMethod
     * @param {EntityID} entityID The ID of the entity to call the method on.
     * @param {string} method The name of the method to call.
     * @param {string[]} params The list of parameters to call the specified method with.
     */
    Q_INVOKABLE void callEntityMethod(QUuid entityID, const QString& method, const QStringList& params = QStringList());

    /**jsdoc
    * Call a server method on an entity. Allows a client entity script to call a method on an 
    * entity's server script. The method will execute in the entity server script engine. If 
    * the entity does not have an entity server script or the method does not exist, this call will 
    * have no effect. If the entity is running an entity script (specified by the `serverScripts` property)
    * and it exposes a property with the specified name `method`, it will be called using `params` as 
    * the list of arguments.
    *
    * @function Entities.callEntityServerMethod
    * @param {EntityID} entityID The ID of the entity to call the method on.
    * @param {string} method The name of the method to call.
    * @param {string[]} params The list of parameters to call the specified method with.
    */
    Q_INVOKABLE void callEntityServerMethod(QUuid entityID, const QString& method, const QStringList& params = QStringList());

    /**jsdoc
    * Call a client method on an entity on a specific client node. Allows a server entity script to call a 
    * method on an entity's client script for a particular client. The method will execute in the entity script 
    * engine on that single client. If the entity does not have an entity script or the method does not exist, or
    * the client is not connected to the domain, or you attempt to make this call outside of the entity server 
    * script, this call will have no effect.
    *
    * @function Entities.callEntityClientMethod
    * @param {SessionID} clientSessionID The session ID of the client to call the method on.
    * @param {EntityID} entityID The ID of the entity to call the method on.
    * @param {string} method The name of the method to call.
    * @param {string[]} params The list of parameters to call the specified method with.
    */
    Q_INVOKABLE void callEntityClientMethod(QUuid clientSessionID, QUuid entityID, const QString& method, const QStringList& params = QStringList());

    /**jsdoc
     * Find the entity with a position closest to a specified point and within a specified radius.
     * @function Entities.findClosestEntity
     * @param {Vec3} center - The point about which to search.
     * @param {number} radius - The radius within which to search.
     * @returns {Uuid} The ID of the entity that is closest to the <code>center</code> and within the <code>radius</code> if
     *     there is one, otherwise <code>null</code>.
     * @example <caption>Find the closest entity within 10m of your avatar.</caption>
     * var entityID = Entities.findClosestEntity(MyAvatar.position, 10);
     * print("Closest entity: " + entityID);
     */
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE QUuid findClosestEntity(const glm::vec3& center, float radius) const;

    /**jsdoc
     * Find all entities that intersect a sphere defined by a center point and radius.
     * @function Entities.findEntities
     * @param {Vec3} center - The point about which to search.
     * @param {number} radius - The radius within which to search.
     * @returns {Uuid[]} An array of entity IDs that were found that intersect the search sphere.
     * @example <caption>Report how many entities are within 10m of your avatar.</caption>
     * var entityIDs = Entities.findEntities(MyAvatar.position, 10);
     * print("Number of entities within 10m: " + entityIDs.length);
     */
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE QVector<QUuid> findEntities(const glm::vec3& center, float radius) const;

    /**jsdoc
     * Find all entities whose axis-aligned boxes intersect a search axis-aligned box defined by its minimum coordinates corner
     * and dimensions.
     * @function Entities.findEntitiesInBox
     * @param {Vec3} corner - The corner of the search AA box with minimum co-ordinate values.
     * @param {Vec3} dimensions - The dimensions of the search AA box.
     * @returns {Uuid[]} An array of entity IDs whose AA boxes intersect the search AA box.
     */
    /// this function will not find any models in script engine contexts which don't have access to models
    Q_INVOKABLE QVector<QUuid> findEntitiesInBox(const glm::vec3& corner, const glm::vec3& dimensions) const;

    /**jsdoc
     * Find all entities whose axis-aligned boxes intersect a search frustum.
     * @function Entities.findEntitiesInFrustum
     * @param {ViewFrustum} frustum - The frustum to search in. The <code>position</code>, <code>orientation</code>, 
     *     <code>projection</code>, and <code>centerRadius</code> properties must be specified.
     * @returns {Uuid[]} An array of entity IDs axis-aligned boxes intersect the frustum.
     * @example <caption>Report the number of entities in view.</caption>
     * var entityIDs = Entities.findEntitiesInFrustum(Camera.frustum);
     * print("Number of entities in view: " + entityIDs.length);
     */
    /// this function will not find any models in script engine contexts which don't have access to entities
    Q_INVOKABLE QVector<QUuid> findEntitiesInFrustum(QVariantMap frustum) const;

    /**jsdoc
     * Find all entities of a particular type that intersect a sphere defined by a center point and radius.
     * @function Entities.findEntitiesByType
     * @param {Entities.EntityType} entityType - The type of entity to search for.
     * @param {Vec3} center - The point about which to search.
     * @param {number} radius - The radius within which to search.
     * @returns {Uuid[]} An array of entity IDs of the specified type that intersect the search sphere.
     * @example <caption>Report the number of Model entities within 10m of your avatar.</caption>
     * var entityIDs = Entities.findEntitiesByType("Model", MyAvatar.position, 10);
     * print("Number of Model entities within 10m: " + entityIDs.length);
     */
    /// this function will not find any entities in script engine contexts which don't have access to entities
    Q_INVOKABLE QVector<QUuid> findEntitiesByType(const QString entityType, const glm::vec3& center, float radius) const;

    /**jsdoc
     * Find the first entity intersected by a {@link PickRay}. <code>Light</code> and <code>Zone</code> entities are not 
     * intersected unless they've been configured as pickable using {@link Entities.setLightsArePickable|setLightsArePickable}
     * and {@link Entities.setZonesArePickable|setZonesArePickable}, respectively.<br />
     * @function Entities.findRayIntersection
     * @param {PickRay} pickRay - The PickRay to use for finding entities.
     * @param {boolean} [precisionPicking=false] - If <code>true</code> and the intersected entity is a <code>Model</code> 
     *     entity, the result's <code>extraInfo</code> property includes more information than it otherwise would.
     * @param {Uuid[]} [entitiesToInclude=[]] - If not empty then the search is restricted to these entities.
     * @param {Uuid[]} [entitiesToDiscard=[]] - Entities to ignore during the search.
     * @param {boolean} [visibleOnly=false] - If <code>true</code> then only entities that are 
     *     <code>{@link Entities.EntityProperties|visible}<code> are searched.
     * @param {boolean} [collideableOnly=false] - If <code>true</code> then only entities that are not 
     *     <code>{@link Entities.EntityProperties|collisionless}</code> are searched.
     * @returns {Entities.RayToEntityIntersectionResult} The result of the search for the first intersected entity.
     * @example <caption>Find the entity directly in front of your avatar.</caption>
     * var pickRay = {
     *     origin: MyAvatar.position,
     *     direction: Quat.getFront(MyAvatar.orientation)
     * };
     *
     * var intersection = Entities.findRayIntersection(pickRay, true);
     * if (intersection.intersects) {
     *     print("Entity in front of avatar: " + intersection.entityID);
     * } else {
     *     print("No entity in front of avatar.");
     * }
     */
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

    /**jsdoc
     * Find the first entity intersected by a {@link PickRay}. <code>Light</code> and <code>Zone</code> entities are not 
     * intersected unless they've been configured as pickable using {@link Entities.setLightsArePickable|setLightsArePickable} 
     * and {@link Entities.setZonesArePickable|setZonesArePickable}, respectively.<br />
     * This is a synonym for {@link Entities.findRayIntersection|findRayIntersection}.
     * @function Entities.findRayIntersectionBlocking
     * @param {PickRay} pickRay - The PickRay to use for finding entities.
     * @param {boolean} [precisionPicking=false] - If <code>true</code> and the intersected entity is a <code>Model</code>
     *     entity, the result's <code>extraInfo</code> property includes more information than it otherwise would.
     * @param {Uuid[]} [entitiesToInclude=[]] - If not empty then the search is restricted to these entities.
     * @param {Uuid[]} [entitiesToDiscard=[]] - Entities to ignore during the search.
     * @param {boolean} [visibleOnly=false] - If <code>true</code> then only entities that are
     *     <code>{@link Entities.EntityProperties|visible}<code> are searched.
     * @deprecated This function is deprecated and will soon be removed. Use 
     *    {@link Entities.findRayIntersection|findRayIntersection} instead; it blocks and performs the same function.
     */
    /// If the scripting context has visible entities, this will determine a ray intersection, and will block in
    /// order to return an accurate result
    Q_INVOKABLE RayToEntityIntersectionResult findRayIntersectionBlocking(const PickRay& ray, bool precisionPicking = false, 
        const QScriptValue& entityIdsToInclude = QScriptValue(), const QScriptValue& entityIdsToDiscard = QScriptValue());

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
    Q_INVOKABLE bool queryPropertyMetadata(QUuid entityID, QScriptValue property, QScriptValue scopeOrCallback, 
        QScriptValue methodOrName = QScriptValue());

    Q_INVOKABLE bool getServerScriptStatus(QUuid entityID, QScriptValue callback);

    /**jsdoc
     * Set whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Light} entities. Ray picks are 
     *     done using {@link Entities.findRayIntersection|findRayIntersection}or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.setLightsArePickable
     * @param {boolean} value - Set <code>true</code> to make ray picks intersect the bounding box of 
     *     {@link Entities.EntityType|Light} entities, otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE void setLightsArePickable(bool value);

    /**jsdoc
     * Get whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Light} entities. Ray picks are 
     *     done using {@link Entities.findRayIntersection|findRayIntersection}or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.getLightsArePickable
     * @returns {boolean} <code>true</code> if ray picks intersect the bounding box of {@link Entities.EntityType|Light} 
     *     entities, otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool getLightsArePickable() const;

    /**jsdoc
     * Set whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Zone} entities. Ray picks are 
     *     done using {@link Entities.findRayIntersection|findRayIntersection}or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.setZonesArePickable
     * @param {boolean} value - Set <code>true</code> to make ray picks intersect the bounding box of 
     *     {@link Entities.EntityType|Zone} entities, otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE void setZonesArePickable(bool value);

    /**jsdoc
     * Get whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Zone} entities. Ray picks are 
     *     done using {@link Entities.findRayIntersection|findRayIntersection}or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.getZonesArePickable
     * @returns {boolean} <code>true</code> if ray picks intersect the bounding box of {@link Entities.EntityType|Zone} 
     *      entities, otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool getZonesArePickable() const;

    /**jsdoc
     * Set whether or not {@link Entities.EntityType|Zone} entities' boundaries should be drawn. <em>Currently not used.</em>
     * @function Entities.setDrawZoneBoundaries
     * @param {boolean} value - Set to <code>true</code> if {@link Entities.EntityType|Zone} entities' boundaries should be 
     *     drawn, <code>false</code> otherwise.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE void setDrawZoneBoundaries(bool value);

    /**jsdoc
    * Get whether or not {@link Entities.EntityType|Zone} entities' boundaries should be drawn. <em>Currently not used.</em>
    * @function Entities.getDrawZoneBoundaries
    * @returns {boolean} <code>true</code> if {@link Entities.EntityType|Zone} entities' boundaries should be drawn, 
    *    <code>false</code> otherwise.
    */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool getDrawZoneBoundaries() const;


    /**jsdoc
     * Set the values of all voxels in a spherical portion of a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxelSphere
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} center - The center of the sphere of voxels to set, in world coordinates.
     * @param {number} radius - If radius of the sphere of voxels to set, in world coordinates.
     * @param {number} value - If <code>value % 256 == 0</code> then the each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox sphere.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 }));
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: position,
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 32, y: 32, z: 32 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setVoxelSphere(polyVox, position, 0.9, 255);
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setVoxelSphere(QUuid entityID, const glm::vec3& center, float radius, int value);
    
    /**jsdoc
     * Set the values of all voxels in a capsule-shaped portion of a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxelCapsule
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} start - The center of the sphere of voxels to set, in world coordinates.
     * @param {Vec3} end - The center of the sphere of voxels to set, in world coordinates.
     * @param {number} radius - The radius of the capsule cylinder and spherical ends, in world coordinates.
     * @param {number} value - If <code>value % 256 == 0</code> then the each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox capsule shape.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 }));
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: position,
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 32, y: 32, z: 32 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * var startPosition = Vec3.sum({ x: -0.5, y: 0, z: 0 }, position);
     * var endPosition = Vec3.sum({ x: 0.5, y: 0, z: 0 }, position);
     * Entities.setVoxelCapsule(polyVox, startPosition, endPosition, 0.5, 255);
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setVoxelCapsule(QUuid entityID, const glm::vec3& start, const glm::vec3& end, float radius, int value);

    /**jsdoc
     * Set the value of a particular voxels in a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxel
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} position - The position relative to the minimum axes values corner of the entity. The 
     *     <code>position</code> coordinates are rounded to the nearest integer to get the voxel coordinate. The minimum axes 
     *     corner voxel is <code>{ x: 0, y: 0, z: 0 }</code>.
     * @param {number} value - If <code>value % 256 == 0</code> then the voxel is cleared, otherwise the voxel is set.
     * @example <caption>Create a cube PolyVox entity and clear the minimum axes corner voxel.</caption>
     * var entity = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(entity, 1);
     * Entities.setVoxel(entity, { x: 0, y: 0, z: 0 }, 0);
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setVoxel(QUuid entityID, const glm::vec3& position, int value);

    /**jsdoc
     * Set the values of all voxels in a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setAllVoxels
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {number} value - If <code>value % 256 == 0</code> then the each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox cube.</caption>
     * var entity = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(entity, 1);
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setAllVoxels(QUuid entityID, int value);

    /**jsdoc
     * Set the values of all voxels in a cubic portion of a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxelsInCuboid
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} lowPosition - The position of the minimum axes value corner of the cube of voxels to set, in voxel 
     *     coordinates.
     * @param {Vec3} cuboidSize - The size of the cube of voxels to set, in voxel coordinates.
     * @param {number} value - If <code>value % 256 == 0</code> then the each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox cube and clear the voxels in one corner.</caption>
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(polyVox, 1);
     * var cuboidPosition = { x: 12, y: 12, z: 12 };
     * var cuboidSize = { x: 4, y: 4, z: 4 };
     * Entities.setVoxelsInCuboid(polyVox, cuboidPosition, cuboidSize, 0);
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setVoxelsInCuboid(QUuid entityID, const glm::vec3& lowPosition, const glm::vec3& cuboidSize, int value);

    /**jsdoc
     * Convert voxel coordinates in a {@link Entities.EntityType|PolyVox} entity to world coordinates. Voxel coordinates are 
     * relative to the minimum axes values corner of the entity with a scale of <code>Vec3.ONE</code> being the dimensions of 
     * each voxel.
     * @function Entities.voxelCoordsToWorldCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} voxelCoords - The voxel coordinates. May be fractional and outside the entity's bounding box.
     * @returns {Vec3} The world coordinates of the <code>voxelCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}.
     * @example <caption>Create a PolyVox cube with the 0,0,0 voxel replaced by a sphere.</caption>
     * // Cube PolyVox with 0,0,0 voxel missing.
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(polyVox, 1);
     * Entities.setVoxel(polyVox, { x: 0, y: 0, z: 0 }, 0);
     *
     * // Red sphere in 0,0,0 corner position.
     * var cornerPosition = Entities.voxelCoordsToWorldCoords(polyVox, { x: 0, y: 0, z: 0 });
     * var voxelDimensions = Vec3.multiply(2 / 16, Vec3.ONE);
     * var sphere = Entities.addEntity({
     *     type: "Sphere",
     *     position: Vec3.sum(cornerPosition, Vec3.multiply(0.5, voxelDimensions)),
     *     dimensions: voxelDimensions,
     *     color: { red: 255, green: 0, blue: 0 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 voxelCoordsToWorldCoords(const QUuid& entityID, glm::vec3 voxelCoords);

    /**jsdoc
     * Convert world coordinates to voxel coordinates in a {@link Entities.EntityType|PolyVox} entity. Voxel coordinates are 
     * relative to the minimum axes values corner of the entity, with a scale of <code>Vec3.ONE</code> being the dimensions of 
     * each voxel.
     * @function Entities.worldCoordsToVoxelCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} worldCoords - The world coordinates. May be outside the entity's bounding box.
     * @returns {Vec3} The voxel coordinates of the <code>worldCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}. The value may be fractional.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 worldCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 worldCoords);

    /**jsdoc
     * Convert voxel coordinates in a {@link Entities.EntityType|PolyVox} entity to local coordinates relative to the minimum 
     * axes value corner of the entity, with the scale being the same as world coordinates.
     * @function Entities.voxelCoordsToLocalCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} voxelCoords - The voxel coordinates. May be fractional and outside the entity's bounding box.
     * @returns {Vec3} The local coordinates of the <code>voxelCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}.
     * @example <caption>Get the world dimensions of a voxel in a PolyVox entity.</code>
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * var voxelDimensions = Entities.voxelCoordsToLocalCoords(polyVox, Vec3.ONE);
     * print("Voxel dimensions: " + JSON.stringify(voxelDimensions));
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 voxelCoordsToLocalCoords(const QUuid& entityID, glm::vec3 voxelCoords);

    /**jsdoc
     * Convert local coordinates to voxel coordinates in a {@link Entities.EntityType|PolyVox} entity. Local coordinates are 
     * relative to the minimum axes value corner of the entity, with the scale being the same as world coordinates.
     * @function Entities.localCoordsToVoxelCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} localCoords - The local coordinates. May be outside the entity's bounding box.
     * @returns {Vec3} The voxel coordinates of the <code>worldCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}. The value may be fractional.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 localCoordsToVoxelCoords(const QUuid& entityID, glm::vec3 localCoords);

    /**jsdoc
     * Set the <code>linePoints</code> property of a {@link Entities.EntityType|Line} entity.
     * @function Entities.setAllPoints
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Line} entity.
     * @param {Vec3[]} points - The array of points to set the entity's <code>linePoints</code> property to.
     * @returns {boolean} <code>true</code> if the entity's property was updated, <code>false</code> otherwise. The property 
     *     may fail to be updated if the entity does not exist, the entity is not a {@link Entities.EntityType|Line} entity, 
     *     one of the points is outside the entity's dimensions, or the number of points is greater than the maximum allowed.
     * @example <caption>Change the shape of a Line entity.</caption>
     * // Draw a horizontal line between two points.
     * var entity = Entities.addEntity({
     *     type: "Line",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 2, y: 2, z: 1 },
     *     linePoints: [
     *         { x: -1, y: 0, z: 0 },
     *         { x:1, y: -0, z: 0 }
     *     ],
     *     color: { red: 255, green: 0, blue: 0 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * // Change the line to be a "V".
     * Script.setTimeout(function () {
     *     Entities.setAllPoints(entity, [
     *         { x: -1, y: 1, z: 0 },
     *         { x: 0, y: -1, z: 0 },
     *         { x: 1, y: 1, z: 0 },
     *     ]);
     * }, 2000);
     */
    Q_INVOKABLE bool setAllPoints(QUuid entityID, const QVector<glm::vec3>& points);
    
    /**jsdoc
     * Append a point to a {@link Entities.EntityType|Line} entity.
     * @function Entities.appendPoint
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Line} entity.
     * @param {Vec3} point - The point to add to the line. The coordinates are relative to the entity's position.
     * @returns {boolean} <code>true</code> if the point was added to the line, <code>false</code> otherwise. The point may 
     *     fail to be added if the entity does not exist, the entity is not a {@link Entities.EntityType|Line} entity, the 
     *     point is outside the entity's dimensions, or the maximum number of points has been reached.
     * @example <caption>Append a point to a Line entity.</caption>
     * // Draw a line between two points.
     * var entity = Entities.addEntity({
     *     type: "Line",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 2, y: 2, z: 1 },
     *     linePoints: [
     *         { x: -1, y: 1, z: 0 },
     *         { x: 0, y: -1, z: 0 }
     *     ],
     *     color: { red: 255, green: 0, blue: 0 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * // Add a third point to create a "V".
     * Entities.appendPoint(entity, { x: 1, y: 1, z: 0 });
     */
    Q_INVOKABLE bool appendPoint(QUuid entityID, const glm::vec3& point);

    /**jsdoc
     * Dumps debug information about all entities in Interface's local in-memory tree of entities it knows about &mdash; domain
     * and client-only &mdash; to the program log.
     * @function Entities.dumpTree
     */
    Q_INVOKABLE void dumpTree() const;

    Q_INVOKABLE QUuid addAction(const QString& actionTypeString, const QUuid& entityID, const QVariantMap& arguments);
    Q_INVOKABLE bool updateAction(const QUuid& entityID, const QUuid& actionID, const QVariantMap& arguments);
    Q_INVOKABLE bool deleteAction(const QUuid& entityID, const QUuid& actionID);
    Q_INVOKABLE QVector<QUuid> getActionIDs(const QUuid& entityID);
    Q_INVOKABLE QVariantMap getActionArguments(const QUuid& entityID, const QUuid& actionID);


    /**jsdoc
     * Get the translation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's coordinate frame.
     * @function Entities.getAbsoluteJointTranslationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Vec3} The translation of the joint in the entity's coordinate frame if the entity is a
     *     {@link Entities.EntityType|Model} entity, the entity is loaded, and the joint index is valid; otherwise 
     *     <code>{@link Vec3(0)|Vec3.ZERO}</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 getAbsoluteJointTranslationInObjectFrame(const QUuid& entityID, int jointIndex);

    /**jsdoc
     * Get the trannslation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's coordinate frame.
     * @function Entities.getAbsoluteJointRotationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Quat} The rotation of the joint in the entity's coordinate frame if the entity is a
     *     {@link Entities.EntityType|Model} entity, the entity is loaded, and the joint index is valid; otherwise 
     *     <code>{@link Quat(0)|Quat.IDENTITY}</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::quat getAbsoluteJointRotationInObjectFrame(const QUuid& entityID, int jointIndex);

    /**jsdoc
     * Set the translation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's coordinate frame.
     * @function Entities.setAbsoluteJointTranslationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Vec3} translation - The translation relative to the entity's coordinate frame to set the joint to.
     * @returns {boolean} <code>true</code>if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the translation is different to the joint's current translation; otherwise 
     *     <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setAbsoluteJointTranslationInObjectFrame(const QUuid& entityID, int jointIndex, glm::vec3 translation);

    /**jsdoc
     * Set the rotation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's coordinate frame.
     * @function Entities.setAbsoluteJointRotationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Quat} rotation - The rotation relative to the entity's coordinate frame to set the joint to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the rotation is different to the joint's current rotation; otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setAbsoluteJointRotationInObjectFrame(const QUuid& entityID, int jointIndex, glm::quat rotation);


    /**jsdoc
     * Get the local translation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getLocalJointTranslation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Vec3} The local translation of the joint if the entity is a {@link Entities.EntityType|Model} entity, the 
     *     entity is loaded, and the joint index is valid; otherwise <code>{@link Vec3(0)|Vec3.ZERO}</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::vec3 getLocalJointTranslation(const QUuid& entityID, int jointIndex);

    /**jsdoc
     * Get the local rotation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getLocalJointRotation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Quat} The local rotation of the joint if the entity is a {@link Entities.EntityType|Model} entity, the entity 
     *     is loaded, and the joint index is valid; otherwise <code>{@link Quat(0)|Quat.IDENTITY}</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE glm::quat getLocalJointRotation(const QUuid& entityID, int jointIndex);

    /**jsdoc
     * Set the local translation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointTranslation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Vec3} translation - The local translation to set the joint to.
     * @returns {boolean} <code>true</code>if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the translation is different to the joint's current translation; otherwise 
     *     <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setLocalJointTranslation(const QUuid& entityID, int jointIndex, glm::vec3 translation);

    /**jsdoc
     * Set the local rotation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointRotation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Quat} rotation - The local rotation to set the joint to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the rotation is different to the joint's current rotation; otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setLocalJointRotation(const QUuid& entityID, int jointIndex, glm::quat rotation);


    /**jsdoc
     * Set the local translations of joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointTranslations
     * @param {Uuid} entityID - The ID of the entity.
     * @param {Vec3[]} translations - The local translations to set the joints to.
     * @returns {boolean} <code>true</code>if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the model has joints, and at least one of the translations is different to the model's current translations; 
     *     otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setLocalJointTranslations(const QUuid& entityID, const QVector<glm::vec3>& translations);

    /**jsdoc
     * Set the local rotations of joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointRotations
     * @param {Uuid} entityID - The ID of the entity.
     * @param {Quat[]} rotations - The local rotations to set the joints to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the model has joints, and at least one of the rotations is different to the model's current rotations; otherwise 
     *     <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setLocalJointRotations(const QUuid& entityID, const QVector<glm::quat>& rotations);

    /**jsdoc
     * Set the local rotations and translations of joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointsData
     * @param {Uuid} entityID - The ID of the entity.
     * @param {Quat[]} rotations - The local rotations to set the joints to.
     * @param {Vec3[]} translations - The local translations to set the joints to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded,
     *     the model has joints, and at least one of the rotations or translations is different to the model's current values; 
     *     otherwise <code>false</code>.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE bool setLocalJointsData(const QUuid& entityID,
                                        const QVector<glm::quat>& rotations,
                                        const QVector<glm::vec3>& translations);


    /**jsdoc
     * Get the index of a named joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getJointIndex
     * @param {Uuid} entityID - The ID of the entity.
     * @param {string} name - The name of the joint.
     * @returns {number} The integer index of the joint if the entity is a {@link Entities.EntityType|Model} entity, the entity 
     *     is loaded, and the joint is present; otherwise <code>-1</code>. The joint indexes are in order per 
     *     {@link Entities.getJointNames|getJointNames}.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE int getJointIndex(const QUuid& entityID, const QString& name);

    /**jsdoc
     * Get the names of all the joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getJointNames
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Model} entity.
     * @returns {string[]} The names of all the joints in the entity if it is a {@link Entities.EntityType|Model} entity and 
     *     is loaded, otherwise <code>[]</code>. The joint names are in order per {@link Entities.getJointIndex|getJointIndex}.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE QStringList getJointNames(const QUuid& entityID);


    /**jsdoc
     * Get the IDs of entities, overlays, and avatars that are directly parented to an entity.
     * @function Entities.getChildrenIDs
     * @param {Uuid} parentID - The ID of the entity to get the children IDs of.
     * @returns {Uuid[]} An array of entity, overlay, and avatar IDs that are parented directly to the <code>parentID</code> 
     *     entity. Does not include children's children, etc. The array is empty if no children can be found, 
     *     <code>parentID</code> cannot be found, or <code>parentID</code> is not an entity.
     * @example <caption>Report the children of an entity.</caption>
     * function createEntity(description, position, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Sphere",
     *         position: position,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     print(description + ": " + entity);
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     * var root = createEntity("Root", position, Uuid.NULL);
     * var child = createEntity("Child", Vec3.sum(position, { x: 0, y: -1, z: 0 }), root);
     * var grandChild = createEntity("Grandchild", Vec3.sum(position, { x: 0, y: -2, z: 0 }), child);
     *
     * var children = Entities.getChildrenIDs(root);
     * print("Children of root: " + JSON.stringify(children));  // Only the child entity.
     */
    Q_INVOKABLE QVector<QUuid> getChildrenIDs(const QUuid& parentID);

    /**jsdoc
     * Get the IDs of entities, overlays, and avatars that are directly parented to an entity, overlay, or avatar model's joint.
     * @function Entities.getChildrenIDsOfJoint
     * @param {Uuid} parentID - The ID of the entity, overlay, or avatar to get the children IDs of.
     * @param {number} jointIndex - Integer number of the model joint to get the children IDs of.
     * @returns {Uuid[]} An array of entity, overlay, and avatar IDs that are parented directly to the <code>parentID</code> 
     *     entity, overlay, or avatar at the <code>jointIndex</code> joint. Does not include children's children, etc. The 
     *     array is empty if no children can be found or <code>parentID</code> cannot be found.
     * @example <caption>Report the children of your avatar's right hand.</caption>
     * function createEntity(description, position, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Sphere",
     *         position: position,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     print(description + ": " + entity);
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     * var root = createEntity("Root", position, Uuid.NULL);
     * var child = createEntity("Child", Vec3.sum(position, { x: 0, y: -1, z: 0 }), root);
     *
     * Entities.editEntity(root, {
     *     parentID: MyAvatar.sessionUUID,
     *     parentJointIndex: MyAvatar.getJointIndex("RightHand")
     * });
     *
     * var children = Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, MyAvatar.getJointIndex("RightHand"));
     * print("Children of hand: " + JSON.stringify(children));  // Only the root entity.
     */
    Q_INVOKABLE QVector<QUuid> getChildrenIDsOfJoint(const QUuid& parentID, int jointIndex);

    /**jsdoc
     * Check whether an entity or overlay has a an entity as an ancestor (parent, parent's parent, etc.).
     * @function Entities.isChildOfParent
     * @param {Uuid} childID - The ID of the child entity or overlay to test for being a child, grandchild, etc.
     * @param {Uuid} parentID - The ID of the parent entity to test for being a parent, grandparent, etc.
     * @returns {boolean} <code>true</code> if the <code>childID></code> entity or overlay has the <code>parentID</code> entity 
     *     as a parent or grandparent etc.
     * @example <caption>Check that a grandchild entity is a child of its grandparent.</caption>
     * function createEntity(description, position, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Sphere",
     *         position: position,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     print(description + ": " + entity);
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     * var root = createEntity("Root", position, Uuid.NULL);
     * var child = createEntity("Child", Vec3.sum(position, { x: 0, y: -1, z: 0 }), root);
     * var grandChild = createEntity("Grandchild", Vec3.sum(position, { x: 0, y: -2, z: 0 }), child);
     *
     * print("grandChild has root as parent: " + Entities.isChildOfParent(grandChild, root));  // true
     */
    Q_INVOKABLE bool isChildOfParent(QUuid childID, QUuid parentID);

    /**jsdoc
     * Get the type &mdash; entity, overlay, or avatar &mdash; of an in-world item.
     * @function Entities.getNestableType
     * @param {Uuid} id - The ID of the item to get the type of.
     * @returns {string} The type of the item: <code>"entity"</code> if the item is an entity, <code>"overlay"</code> if the 
     *    the item is an overlay, <code>"avatar"</code> if the item is an avatar; otherwise <code>"unknown"</code> if the item 
     *    cannot be found.
     * @example <caption>Print some nestable types.</caption>
     * var entity = Entities.addEntity({
     *     type: "Sphere",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 1, z: -2 })),
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * print(Entities.getNestableType(entity));  // "entity"
     * print(Entities.getNestableType(Uuid.generate()));  // "unknown"
     */
    Q_INVOKABLE QString getNestableType(QUuid id);

    /**jsdoc
     * Get the ID of the {@link Entities.EntityType|Web} entity that has keyboard focus.
     * @function Entities.getKeyboardFocusEntity
     * @returns {Uuid} The ID of the {@link Entities.EntityType|Web} entity that has focus, if any, otherwise <code>null</code>.
     */
    Q_INVOKABLE QUuid getKeyboardFocusEntity() const;

    /**jsdoc
     * Set the {@link Entities.EntityType|Web} entity that has keyboard focus.
     * @function Entities.setKeyboardFocusEntity
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Web} entity to set keyboard focus to. Use 
     *     <code>null</code> or {@link Uuid|Uuid.NULL} to unset keyboard focus from an entity.
     */
    Q_INVOKABLE void setKeyboardFocusEntity(const EntityItemID& id);

    Q_INVOKABLE void sendMousePressOnEntity(const EntityItemID& id, const PointerEvent& event);
    Q_INVOKABLE void sendMouseMoveOnEntity(const EntityItemID& id, const PointerEvent& event);
    Q_INVOKABLE void sendMouseReleaseOnEntity(const EntityItemID& id, const PointerEvent& event);

    Q_INVOKABLE void sendClickDownOnEntity(const EntityItemID& id, const PointerEvent& event);
    Q_INVOKABLE void sendHoldingClickOnEntity(const EntityItemID& id, const PointerEvent& event);
    Q_INVOKABLE void sendClickReleaseOnEntity(const EntityItemID& id, const PointerEvent& event);

    Q_INVOKABLE void sendHoverEnterEntity(const EntityItemID& id, const PointerEvent& event);
    Q_INVOKABLE void sendHoverOverEntity(const EntityItemID& id, const PointerEvent& event);
    Q_INVOKABLE void sendHoverLeaveEntity(const EntityItemID& id, const PointerEvent& event);

    Q_INVOKABLE bool wantsHandControllerPointerEvents(QUuid id);

    Q_INVOKABLE void emitScriptEvent(const EntityItemID& entityID, const QVariant& message);

    /**jsdoc
     * Check whether an axis-aligned box and a capsule intersect.
     * @function Entities.AABoxIntersectsCapsule
     * @param {Vec3} brn - The bottom right near (minimum axes values) corner of the AA box.
     * @param {Vec3} dimensions - The dimensions of the AA box.
     * @param {Vec3} start - One end of the capsule.
     * @param {Vec3} end - The other end of the capsule.
     * @param {number} radius - The radiues of the capsule.
     * @returns {boolean} <code>true</code> if the AA box and capsule intersect, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool AABoxIntersectsCapsule(const glm::vec3& low, const glm::vec3& dimensions,
                                            const glm::vec3& start, const glm::vec3& end, float radius);

    /**jsdoc
     * Get the meshes in a {@link Entities.EntityType|Model} or {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.getMeshes
     * @param {Uuid} entityID - The ID of the <code>Model</code> or <code>PolyVox</code> entity to get the meshes of.
     * @param {Entities~getMeshesCallback} callback - The function to call upon completion.
     */
     /**jsdoc
      * Called when {@link Entities.getMeshes} is complete.
      * @callback Entities~getMeshesCallback
      * @param {MeshProxy[]} meshes - If <code>success<</code> is <code>true</code>, a {@link MeshProxy} per mesh in the 
      *     <code>Model</code> or <code>PolyVox</code> entity; otherwise <code>undefined</code>. 
      * @param {boolean} success - <code>true</code> if the {@link Entities.getMeshes} call was successful, <code>false</code> 
      *     otherwise. The call may be unsuccessful if the requested entity could not be found.
     */
    // FIXME move to a renderable entity interface
    Q_INVOKABLE void getMeshes(QUuid entityID, QScriptValue callback);

    /**jsdoc
     * Get the object to world transform, excluding scale, of an entity.
     * @function Entities.getEntityTransform
     * @param {Uuid} entityID - The ID of the entity.
     * @returns {Mat4} The entity's object to world transform excluding scale; i.e., translation and rotation, with scale of 1.
     * @example <caption>Position and rotation in an entity's world transform.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 1, z: -2 }));
     * var orientation = MyAvatar.orientation;
     * print("Position: " + JSON.stringify(position));
     * print("Orientation: " + JSON.stringify(orientation));
     *
     * var entityID = Entities.addEntity({
     *     type: "Sphere",
     *     position: position,
     *     rotation: orientation,
     *     dimensions: Vec3.HALF,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * var transform = Entities.getEntityTransform(entityID);
     * print("Transform: " + JSON.stringify(transform));
     * print("Translation: " + JSON.stringify(Mat4.extractTranslation(transform)));  // Same as position.
     * print("Rotation: " + JSON.stringify(Mat4.extractRotation(transform)));  // Same as orientation.
     * print("Scale: " + JSON.stringify(Mat4.extractScale(transform)));  // { x: 1, y: 1, z: 1 }
     */
    Q_INVOKABLE glm::mat4 getEntityTransform(const QUuid& entityID);

    /**jsdoc
     * Get the object to parent transform, excluding scale, of an entity.
     * @function Entities.getEntityLocalTransform
     * @param {Uuid} entityID - The ID of the entity.
     * @returns {Mat4} The entity's object to parent transform excluding scale; i.e., translation and rotation, with scale of 1. 
     * @example <caption>Position and rotation in an entity's local transform.</caption>
     * function createEntity(position, rotation, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Box",
     *         position: position,
     *         rotation: rotation,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     *
     * var parent = createEntity(position, MyAvatar.orientation, Uuid.NULL);
     *
     * var childTranslation = { x: 0, y: -1.5, z: 0 };
     * var childRotation = Quat.fromPitchYawRollDegrees(0, 45, 0);
     * var child = createEntity(Vec3.sum(position, childTranslation), Quat.multiply(childRotation, MyAvatar.orientation), parent);
     *
     * var transform = Entities.getEntityLocalTransform(child);
     * print("Transform: " + JSON.stringify(transform));
     * print("Translation: " + JSON.stringify(Mat4.extractTranslation(transform)));  // childTranslation
     * print("Rotation: " + JSON.stringify(Quat.safeEulerAngles(Mat4.extractRotation(transform))));  // childRotation
     * print("Scale: " + JSON.stringify(Mat4.extractScale(transform)));  // { x: 1, y: 1, z: 1 }     */
    Q_INVOKABLE glm::mat4 getEntityLocalTransform(const QUuid& entityID);

    /**jsdoc
    * Get the static certificate for an entity. The static certificate contains static properties of the item which cannot 
    * be altered.
    * @function Entities.getStaticCertificateJSON
    * @param {Uuid} entityID - The ID of the entity to get the static certificate for.
    * @returns {string} The entity's static certificate as a JSON string.
    */
    Q_INVOKABLE QString getStaticCertificateJSON(const QUuid& entityID);

    /**jsdoc
     * Verify the entity's proof of provenance, i.e., that the entity's <code>certificateID</code> property was produced by 
     * High Fidelity signing the entity's static certificate JSON.
     * @function Entities.verifyStaticCertificateProperties
     * @param {Uuid} entityID - The ID of the entity to verify.
     * @returns {boolean} <code>true</code> if the entity's <code>certificateID</code> property is present and its value 
     *     matches the entity's static certificate JSON, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool verifyStaticCertificateProperties(const QUuid& entityID);

signals:
    void collisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

    /**jsdoc
     * Triggered when your ability to change the <code>locked</code> property of entities changes.
     * @function Entities.canAdjustLocksChanged
     * @param {boolean} canAdjustLocks - <code>true</code> if the script can change the <code>locked</code> property of an 
     *     entity, otherwise <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when your ability to change locks changes.</caption>
     * function onCanAdjustLocksChanged(canAdjustLocks) {
     *     print("You can adjust entity locks: " + canAdjustLocks);
     * }
     * Entities.canAdjustLocksChanged.connect(onCanAdjustLocksChanged);
     */
    void canAdjustLocksChanged(bool canAdjustLocks);

    /**jsdoc
     * Triggered when your ability to rez (create) entities changes.
     * @function Entities.canRezChanged
     * @param {boolean} canRez - <code>true</code> if the script can rez (create) entities, otherwise <code>false</code>.
     * @returns {Signal}
     */
    void canRezChanged(bool canRez);

    /**jsdoc
     * Triggered when your ability to rez (create) temporary entities changes. Temporary entities are entities with a finite
     * <code>lifetime</code> property value set.
     * @function Entities.canRezTmpChanged
     * @param {boolean} canRezTmp - <code>true</code> if the script can rez (create) temporary entities, otherwise 
     *     <code>false</code>.
     * @returns {Signal}
     */
    void canRezTmpChanged(bool canRezTmp);

    /**jsdoc
     * Triggered when your ability to rez (create) certified entities changes. Certified entities are entities that have PoP
     * certificates.
     * @function Entities.canRezCertifiedChanged
     * @param {boolean} canRezCertified - <code>true</code> if the script can rez (create) certified entities, otherwise 
     *     <code>false</code>.
     * @returns {Signal}
     */
    void canRezCertifiedChanged(bool canRezCertified);

    /**jsdoc
     * Triggered when your ability to rez (create) temporary certified entities changes. Temporary entities are entities with a
     * finite <code>lifetime</code> property value set. Certified entities are entities that have PoP certificates.
     * @function Entities.canRezTmpCertifiedChanged
     * @param {boolean} canRezTmpCertified - <code>true</code> if the script can rez (create) temporary certified entities,
     *     otherwise <code>false</code>.
     * @returns {Signal}
     */
    void canRezTmpCertifiedChanged(bool canRezTmpCertified);

    /**jsdoc
     * Triggered when your ability to make changes to the asset server's assets changes.
     * @function Entities.canWriteAssetsChanged
     * @param {boolean} canWriteAssets - <code>true</code> if the script can change the <code>?</code> property of an entity,
     *     otherwise <code>false</code>.
     * @returns {Signal}
     */
    void canWriteAssetsChanged(bool canWriteAssets);

    // TODO
    void mousePressOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseDoublePressOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseMoveOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mousePressOffEntity();
    void mouseDoublePressOffEntity();

    void clickDownOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void holdingClickOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void clickReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void hoverEnterEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void hoverOverEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void hoverLeaveEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void enterEntity(const EntityItemID& entityItemID);
    void leaveEntity(const EntityItemID& entityItemID);

    /**jsdoc
     * Triggered when an entity is deleted.
     * @function Entities.deletingEntity
     * @param {Uuid} entityID - The ID of the entity deleted.
     * @returns {Signal}
     * @example <caption>Report when an entity is deleted.</caption>
     * Entities.deletingEntity.connect(function (entityID) {
     *     print("Deleted entity: " + entityID);
     * });
     */
    void deletingEntity(const EntityItemID& entityID);

    /**jsdoc
     * Triggered when an entity is added to Interface's local in-memory tree of entities it knows about. This may occur when 
     * entities are loaded upon visiting a domain, when the user rotates their view so that more entities become visible, and 
     * when a domain or client-only entity is added (e.g., by {@Entities.addEntity|addEntity}).
     * @function Entities.addingEntity
     * @param {Uuid} entityID - The ID of the entity added.
     * @returns {Signal}
     * @example <caption>Report when an entity is added.</caption>
     * Entities.addingEntity.connect(function (entityID) {
     *     print("Added entity: " + entityID);
     * });
     */
    void addingEntity(const EntityItemID& entityID);

    /**jsdoc
     * Triggered when you disconnect from a domain, at which time Interface's local in-memory tree of entities it knows about
     * is cleared.
     * @function Entities.clearingEntities
     * @returns {Signal}
     * @example <caption>Report when Interfaces's entity tree is cleared.</caption>
     * Entities.clearingEntities.connect(function () {
     *     print("Entities cleared");
     * });
     */
    void clearingEntities();
    
    /**jsdoc
     * @function Entities.debitEnergySource
     * @param {number} value
     * @returns {Signal}
     * @deprecated This function is deprecated and will soon be removed.
     */
    void debitEnergySource(float value);

    void webEventReceived(const EntityItemID& entityItemID, const QVariant& message);

protected:
    void withEntitiesScriptEngine(std::function<void(QSharedPointer<EntitiesScriptEngineProvider>)> function) {
        std::lock_guard<std::recursive_mutex> lock(_entitiesScriptEngineLock);
        function(_entitiesScriptEngine);
    };

private slots:
    void handleEntityScriptCallMethodPacket(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode);

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
