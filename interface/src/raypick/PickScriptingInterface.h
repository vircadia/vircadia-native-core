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
#include <Pick.h>

/**jsdoc
 * The Picks API lets you create and manage objects for repeatedly calculating intersections in different ways.
 *
 * @namespace Picks
 * @property PICK_NOTHING {number} A filter flag.  Don't intersect with anything.
 * @property PICK_ENTITIES {number} A filter flag.  Include entities when intersecting.
 * @property PICK_OVERLAYS {number} A filter flag.  Include overlays when intersecting.
 * @property PICK_AVATARS {number} A filter flag.  Include avatars when intersecting.
 * @property PICK_HUD {number} A filter flag.  Include the HUD sphere when intersecting in HMD mode.
 * @property PICK_COARSE {number} A filter flag.  Pick against coarse meshes, instead of exact meshes.
 * @property PICK_INCLUDE_INVISIBLE {number} A filter flag.  Include invisible objects when intersecting.
 * @property PICK_INCLUDE_NONCOLLIDABLE {number} A filter flag.  Include non-collidable objects when intersecting.
 * @property INTERSECTED_NONE {number} An intersection type.  Intersected nothing with the given filter flags.
 * @property INTERSECTED_ENTITY {number} An intersection type.  Intersected an entity.
 * @property INTERSECTED_OVERLAY {number} An intersection type.  Intersected an overlay.
 * @property INTERSECTED_AVATAR {number} An intersection type.  Intersected an avatar.
 * @property INTERSECTED_HUD {number} An intersection type.  Intersected the HUD sphere.
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

    void registerMetaTypes(QScriptEngine* engine);

    /**jsdoc
     * A set of properties that can be passed to {@link Picks.createPick} to create a new Pick.
     *
     * Different {@link Picks.PickType}s use different properties, and within one PickType, the properties you choose can lead to a wide range of behaviors.  For example,
     *   with PickType.Ray, depending on which optional parameters you pass, you could create a Static Ray Pick, a Mouse Ray Pick, or a Joint Ray Pick.
     *
     * @typedef {Object} Picks.PickProperties
     * @property {boolean} [enabled=false] If this Pick should start enabled or not.  Disabled Picks do not updated their pick results.
     * @property {number} [filter=Picks.PICK_NOTHING] The filter for this Pick to use, constructed using filter flags combined using bitwise OR.
     * @property {float} [maxDistance=0.0] The max distance at which this Pick will intersect.  0.0 = no max.  < 0.0 is invalid.
     * @property {string} [joint] Only for Joint or Mouse Ray Picks.  If "Mouse", it will create a Ray Pick that follows the system mouse, in desktop or HMD.
     *   If "Avatar", it will create a Joint Ray Pick that follows your avatar's head.  Otherwise, it will create a Joint Ray Pick that follows the given joint, if it
     *   exists on your current avatar.
     * @property {Vec3} [posOffset=Vec3.ZERO] Only for Joint Ray Picks.  A local joint position offset, in meters.  x = upward, y = forward, z = lateral
     * @property {Vec3} [dirOffset=Vec3.UP] Only for Joint Ray Picks.  A local joint direction offset.  x = upward, y = forward, z = lateral
     * @property {Vec3} [position] Only for Static Ray Picks.  The world-space origin of the ray.
     * @property {Vec3} [direction=-Vec3.UP] Only for Static Ray Picks.  The world-space direction of the ray.
     * @property {number} [hand=-1] Only for Stylus Picks.  An integer.  0 == left, 1 == right.  Invalid otherwise.
     */

    /**jsdoc
     * Adds a new Pick.
     * @function Picks.createPick
     * @param {Picks.PickType} type A PickType that specifies the method of picking to use
     * @param {Picks.PickProperties} properties A PickProperties object, containing all the properties for initializing this Pick
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
     * @typedef {Object} Picks.RayPickResult
     * @property {number} type The intersection type.
     * @property {bool} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {float} distance The distance to the intersection point from the origin of the ray.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {PickRay} searchRay The PickRay that was used.  Valid even if there was no intersection.
     */

    /**jsdoc
     * An intersection result for a Stylus Pick.
     *
     * @typedef {Object} Picks.StylusPickResult
     * @property {number} type The intersection type.
     * @property {bool} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {float} distance The distance to the intersection point from the origin of the ray.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {StylusTip} stylusTip The StylusTip that was used.  Valid even if there was no intersection.
     */

    /**jsdoc
     * Get the most recent pick result from this Pick.  This will be updated as long as the Pick is enabled.
     * @function Picks.getPrevPickResult
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {PickResult} The most recent intersection result.  This will be slightly different for different PickTypes.  See {@link Picks.RayPickResult} and {@link Picks.StylusPickResult}.
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
     * @returns {boolean} True if the Pick is a Joint Ray Pick with joint == "_CONTROLLER_LEFTHAND" or "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND", or a Stylus Pick with hand == 0.
     */
    Q_INVOKABLE bool isLeftHand(unsigned int uid);
    /**jsdoc
     * Check if a Pick is associated with the right hand.
     * @function Picks.isRightHand
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Joint Ray Pick with joint == "_CONTROLLER_RIGHTHAND" or "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND", or a Stylus Pick with hand == 1.
     */
    Q_INVOKABLE bool isRightHand(unsigned int uid);
    /**jsdoc
     * Check if a Pick is associated with the system mouse.
     * @function Picks.isMouse
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Mouse Ray Pick, false otherwise.
     */
    Q_INVOKABLE bool isMouse(unsigned int uid);

public slots:
    static constexpr unsigned int PICK_NOTHING() { return 0; }
    static constexpr unsigned int PICK_ENTITIES() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_ENTITIES); }
    static constexpr unsigned int PICK_OVERLAYS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_OVERLAYS); }
    static constexpr unsigned int PICK_AVATARS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_AVATARS); }
    static constexpr unsigned int PICK_HUD() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_HUD); }
    static constexpr unsigned int PICK_COARSE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_COARSE); }
    static constexpr unsigned int PICK_INCLUDE_INVISIBLE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_INCLUDE_INVISIBLE); }
    static constexpr unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_INCLUDE_NONCOLLIDABLE); }
    static constexpr unsigned int PICK_ALL_INTERSECTIONS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_ALL_INTERSECTIONS); }
    static constexpr unsigned int INTERSECTED_NONE() { return IntersectionType::NONE; }
    static constexpr unsigned int INTERSECTED_ENTITY() { return IntersectionType::ENTITY; }
    static constexpr unsigned int INTERSECTED_OVERLAY() { return IntersectionType::OVERLAY; }
    static constexpr unsigned int INTERSECTED_AVATAR() { return IntersectionType::AVATAR; }
    static constexpr unsigned int INTERSECTED_HUD() { return IntersectionType::HUD; }
};

#endif // hifi_PickScriptingInterface_h