//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PickScriptingInterface_h
#define hifi_PickScriptingInterface_h

#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>
#include <PhysicsEngine.h>
#include <Pick.h>

/**jsdoc
 * The Picks API lets you create and manage objects for repeatedly calculating intersections in different ways.
 *
 * @namespace Picks
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {number} PICK_NOTHING A filter flag. Don't intersect with anything. <em>Read-only.</em>
 * @property {number} PICK_ENTITIES A filter flag. Include entities when intersecting. <em>Read-only.</em>
 * @property {number} PICK_OVERLAYS A filter flag. Include overlays when intersecting. <em>Read-only.</em>
 * @property {number} PICK_AVATARS A filter flag. Include avatars when intersecting. <em>Read-only.</em>
 * @property {number} PICK_HUD A filter flag. Include the HUD sphere when intersecting in HMD mode. <em>Read-only.</em>
 * @property {number} PICK_COARSE A filter flag. Pick against coarse meshes, instead of exact meshes. <em>Read-only.</em>
 * @property {number} PICK_INCLUDE_INVISIBLE A filter flag. Include invisible objects when intersecting. <em>Read-only.</em>
 * @property {number} PICK_INCLUDE_NONCOLLIDABLE A filter flag. Include non-collidable objects when intersecting. 
 *     <em>Read-only.</em>
 * @property {number} PICK_ALL_INTERSECTIONS <em>Read-only.</em>
 * @property {number} INTERSECTED_NONE An intersection type. Intersected nothing with the given filter flags. 
 *     <em>Read-only.</em>
 * @property {number} INTERSECTED_ENTITY An intersection type. Intersected an entity. <em>Read-only.</em>
 * @property {number} INTERSECTED_OVERLAY An intersection type. Intersected an overlay. <em>Read-only.</em>
 * @property {number} INTERSECTED_AVATAR An intersection type. Intersected an avatar. <em>Read-only.</em>
 * @property {number} INTERSECTED_HUD An intersection type. Intersected the HUD sphere. <em>Read-only.</em>
 * @property {number} perFrameTimeBudget - The max number of usec to spend per frame updating Pick results. <em>Read-only.</em>
 */

class PickScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(unsigned int PICK_NOTHING READ PICK_NOTHING CONSTANT)
    Q_PROPERTY(unsigned int PICK_ENTITIES READ PICK_ENTITIES CONSTANT)
    Q_PROPERTY(unsigned int PICK_OVERLAYS READ PICK_OVERLAYS CONSTANT)
    Q_PROPERTY(unsigned int PICK_AVATARS READ PICK_AVATARS CONSTANT)
    Q_PROPERTY(unsigned int PICK_HUD READ PICK_HUD CONSTANT)
    Q_PROPERTY(unsigned int PICK_COARSE READ PICK_COARSE CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_INVISIBLE READ PICK_INCLUDE_INVISIBLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_NONCOLLIDABLE READ PICK_INCLUDE_NONCOLLIDABLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_ALL_INTERSECTIONS READ PICK_ALL_INTERSECTIONS CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_NONE READ INTERSECTED_NONE CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_ENTITY READ INTERSECTED_ENTITY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_OVERLAY READ INTERSECTED_OVERLAY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_AVATAR READ INTERSECTED_AVATAR CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_HUD READ INTERSECTED_HUD CONSTANT)
    SINGLETON_DEPENDENCY

public:
    unsigned int createRayPick(const QVariant& properties);
    unsigned int createStylusPick(const QVariant& properties);
    unsigned int createCollisionPick(const QVariant& properties);
    unsigned int createParabolaPick(const QVariant& properties);

    void registerMetaTypes(QScriptEngine* engine);

    /**jsdoc
     * Adds a new Pick.
     * Different {@link PickType}s use different properties, and within one PickType, the properties you choose can lead to a wide range of behaviors.  For example,
     *   with PickType.Ray, depending on which optional parameters you pass, you could create a Static Ray Pick, a Mouse Ray Pick, or a Joint Ray Pick.
     * @function Picks.createPick
     * @param {PickType} type A PickType that specifies the method of picking to use
     * @param {Picks.RayPickProperties|Picks.StylusPickProperties|Picks.ParabolaPickProperties|Picks.CollisionPickProperties} properties A PickProperties object, containing all the properties for initializing this Pick
     * @returns {number} The ID of the created Pick.  Used for managing the Pick.  0 if invalid.
     */
    Q_INVOKABLE unsigned int createPick(const PickQuery::PickType type, const QVariant& properties);

    /**jsdoc
     * Enables a Pick.
     * @function Picks.enablePick
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     */
    Q_INVOKABLE void enablePick(unsigned int uid);

    /**jsdoc
     * Disables a Pick.
     * @function Picks.disablePick
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     */
    Q_INVOKABLE void disablePick(unsigned int uid);

    /**jsdoc
     * Removes a Pick.
     * @function Picks.removePick
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     */
    Q_INVOKABLE void removePick(unsigned int uid);

    /**jsdoc
     * An intersection result for a Ray Pick.
     *
     * @typedef {object} RayPickResult
     * @property {number} type The intersection type.
     * @property {boolean} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {number} distance The distance to the intersection point from the origin of the ray.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {PickRay} searchRay The PickRay that was used.  Valid even if there was no intersection.
     */

    /**jsdoc
     * An intersection result for a Stylus Pick.
     *
     * @typedef {object} StylusPickResult
     * @property {number} type The intersection type.
     * @property {boolean} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {number} distance The distance to the intersection point from the origin of the ray.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {StylusTip} stylusTip The StylusTip that was used.  Valid even if there was no intersection.
     */

     /**jsdoc
     * An intersection result for a Parabola Pick.
     *
     * @typedef {object} ParabolaPickResult
     * @property {number} type The intersection type.
     * @property {boolean} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {number} distance The distance to the intersection point from the origin of the parabola, not along the parabola.
     * @property {number} parabolicDistance The distance to the intersection point from the origin of the parabola, along the parabola.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {PickParabola} parabola The PickParabola that was used.  Valid even if there was no intersection.
     */

     /**jsdoc
     * An intersection result for a Collision Pick.
     *
     * @typedef {object} CollisionPickResult
     * @property {boolean} intersects If there was at least one valid intersection (intersectingObjects.length > 0)
     * @property {IntersectingObject[]} intersectingObjects The collision information of each object which intersect with the CollisionRegion.
     * @property {CollisionRegion} collisionRegion The CollisionRegion that was used. Valid even if there was no intersection.
     */

    /**jsdoc
    * Information about the Collision Pick's intersection with an object
    *
    * @typedef {object} IntersectingObject
    * @property {QUuid} id The ID of the object.
    * @property {number} type The type of the object, either Picks.INTERSECTED_ENTITY() or Picks.INTERSECTED_AVATAR()
    * @property {CollisionContact[]} collisionContacts Pairs of points representing penetration information between the pick and the object
    */

     /**jsdoc
     * A pair of points that represents part of an overlap between a Collision Pick and an object in the physics engine. Points which are further apart represent deeper overlap
     *
     * @typedef {object} CollisionContact
     * @property {Vec3} pointOnPick A point representing a penetration of the object's surface into the volume of the pick, in world space.
     * @property {Vec3} pointOnObject A point representing a penetration of the pick's surface into the volume of the found object, in world space.
     * @property {Vec3} normalOnPick The normalized vector pointing away from the pick, representing the direction of collision.
     */

    /**jsdoc
     * Get the most recent pick result from this Pick.  This will be updated as long as the Pick is enabled.
     * @function Picks.getPrevPickResult
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {RayPickResult|StylusPickResult|ParabolaPickResult|CollisionPickResult} The most recent intersection result.  This will be different for different PickTypes.
     */
    Q_INVOKABLE QVariantMap getPrevPickResult(unsigned int uid);

    /**jsdoc
     * Sets whether or not to use precision picking.
     * @function Picks.setPrecisionPicking
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @param {boolean} precisionPicking Whether or not to use precision picking
     */
    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking);

    /**jsdoc
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to ignore during intersection.  Not used by Stylus Picks.
     * @function Picks.setIgnoreItems
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @param {Uuid[]} ignoreItems A list of IDs to ignore.
     */
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreItems);

    /**jsdoc
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to include during intersection, instead of intersecting with everything.  Stylus
     *   Picks <b>only</b> intersect with objects in their include list.
     * @function Picks.setIncludeItems
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @param {Uuid[]} includeItems A list of IDs to include.
     */
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeItems);

    /**jsdoc
     * Check if a Pick is associated with the left hand.
     * @function Picks.isLeftHand
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Joint Ray or Parabola Pick with joint == "_CONTROLLER_LEFTHAND" or "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND", or a Stylus Pick with hand == 0.
     */
    Q_INVOKABLE bool isLeftHand(unsigned int uid);

    /**jsdoc
     * Check if a Pick is associated with the right hand.
     * @function Picks.isRightHand
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Joint Ray or Parabola Pick with joint == "_CONTROLLER_RIGHTHAND" or "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND", or a Stylus Pick with hand == 1.
     */
    Q_INVOKABLE bool isRightHand(unsigned int uid);

    /**jsdoc
     * Check if a Pick is associated with the system mouse.
     * @function Picks.isMouse
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Mouse Ray or Parabola Pick, false otherwise.
     */
    Q_INVOKABLE bool isMouse(unsigned int uid);

    // FIXME: Move to other property definitions.
    Q_PROPERTY(unsigned int perFrameTimeBudget READ getPerFrameTimeBudget WRITE setPerFrameTimeBudget)

    unsigned int getPerFrameTimeBudget() const;
    void setPerFrameTimeBudget(unsigned int numUsecs);

public slots:

    /**jsdoc
     * @function Picks.PICK_NOTHING
     * @returns {number}
     */
    static constexpr unsigned int PICK_NOTHING() { return 0; }

    /**jsdoc
     * @function Picks.PICK_ENTITIES
     * @returns {number}
     */
    static constexpr unsigned int PICK_ENTITIES() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_ENTITIES); }

    /**jsdoc
     * @function Picks.PICK_OVERLAYS
     * @returns {number}
     */
    static constexpr unsigned int PICK_OVERLAYS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_OVERLAYS); }

    /**jsdoc
     * @function Picks.PICK_AVATARS
     * @returns {number}
     */
    static constexpr unsigned int PICK_AVATARS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_AVATARS); }

    /**jsdoc
     * @function Picks.PICK_HUD
     * @returns {number}
     */
    static constexpr unsigned int PICK_HUD() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_HUD); }

    /**jsdoc
     * @function Picks.PICK_COARSE
     * @returns {number}
     */
    static constexpr unsigned int PICK_COARSE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_COARSE); }

    /**jsdoc
     * @function Picks.PICK_INCLUDE_INVISIBLE
     * @returns {number}
     */
    static constexpr unsigned int PICK_INCLUDE_INVISIBLE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_INCLUDE_INVISIBLE); }

    /**jsdoc
     * @function Picks.PICK_INCLUDE_NONCOLLIDABLE
     * @returns {number}
     */
    static constexpr unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_INCLUDE_NONCOLLIDABLE); }

    /**jsdoc
     * @function Picks.PICK_ALL_INTERSECTIONS
     * @returns {number}
     */
    static constexpr unsigned int PICK_ALL_INTERSECTIONS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_ALL_INTERSECTIONS); }

    /**jsdoc
     * @function Picks.INTERSECTED_NONE
     * @returns {number}
     */
    static constexpr unsigned int INTERSECTED_NONE() { return IntersectionType::NONE; }

    /**jsdoc
     * @function Picks.INTERSECTED_ENTITY
     * @returns {number}
     */
    static constexpr unsigned int INTERSECTED_ENTITY() { return IntersectionType::ENTITY; }

    /**jsdoc
     * @function Picks.INTERSECTED_OVERLAY
     * @returns {number}
     */
    static constexpr unsigned int INTERSECTED_OVERLAY() { return IntersectionType::OVERLAY; }

    /**jsdoc
     * @function Picks.INTERSECTED_AVATAR
     * @returns {number}
     */
    static constexpr unsigned int INTERSECTED_AVATAR() { return IntersectionType::AVATAR; }

    /**jsdoc
     * @function Picks.INTERSECTED_HUD
     * @returns {number}
     */
    static constexpr unsigned int INTERSECTED_HUD() { return IntersectionType::HUD; }

protected:
    static std::shared_ptr<TransformNode> createTransformNode(const QVariantMap& propMap);
};

#endif // hifi_PickScriptingInterface_h
