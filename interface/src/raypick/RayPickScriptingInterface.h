//
//  RayPickScriptingInterface.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 8/15/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPickScriptingInterface_h
#define hifi_RayPickScriptingInterface_h

#include <QtCore/QObject>

#include "RegisteredMetaTypes.h"
#include <DependencyManager.h>

#include "PickScriptingInterface.h"

/**jsdoc
 * Synonym for {@link Picks} as used for ray picks.  Deprecated.
 *
 * @namespace RayPick
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {number} PICK_ENTITIES <em>Read-only.</em>
 * @property {number} PICK_OVERLAYS <em>Read-only.</em>
 * @property {number} PICK_AVATARS <em>Read-only.</em>
 * @property {number} PICK_HUD <em>Read-only.</em>
 * @property {number} PICK_COARSE <em>Read-only.</em>
 * @property {number} PICK_INCLUDE_INVISIBLE <em>Read-only.</em>
 * @property {number} PICK_INCLUDE_NONCOLLIDABLE <em>Read-only.</em>
 * @property {number} PICK_ALL_INTERSECTIONS <em>Read-only.</em>
 * @property {number} INTERSECTED_NONE <em>Read-only.</em>
 * @property {number} INTERSECTED_ENTITY <em>Read-only.</em>
 * @property {number} INTERSECTED_OVERLAY <em>Read-only.</em>
 * @property {number} INTERSECTED_AVATAR <em>Read-only.</em>
 * @property {number} INTERSECTED_HUD <em>Read-only.</em>
 */

class RayPickScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
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
    Q_PROPERTY(unsigned int INTERSECTED_LOCAL_ENTITY READ INTERSECTED_LOCAL_ENTITY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_OVERLAY READ INTERSECTED_OVERLAY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_AVATAR READ INTERSECTED_AVATAR CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_HUD READ INTERSECTED_HUD CONSTANT)
    SINGLETON_DEPENDENCY

public:

    /**jsdoc
     * @function RayPick.createRayPick
     * @param {Picks.RayPickProperties}
     * @returns {number}
     */
    Q_INVOKABLE unsigned int createRayPick(const QVariant& properties);

    /**jsdoc
     * @function RayPick.enableRayPick
     * @param {number} id
     */
    Q_INVOKABLE void enableRayPick(unsigned int uid);

    /**jsdoc
     * @function RayPick.disableRayPick
     * @param {number} id
     */
    Q_INVOKABLE void disableRayPick(unsigned int uid);

    /**jsdoc
     * @function RayPick.removeRayPick
     * @param {number} id
     */
    Q_INVOKABLE void removeRayPick(unsigned int uid);

    /**jsdoc
     * @function RayPick.getPrevRayPickResult
     * @param {number} id
     * @returns {RayPickResult}
     */
    Q_INVOKABLE QVariantMap getPrevRayPickResult(unsigned int uid);


    /**jsdoc
     * @function RayPick.setPrecisionPicking
     * @param {number} id
     * @param {boolean} precisionPicking
     */
    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking);

    /**jsdoc
     * @function RayPick.setIgnoreItems
     * @param {number} id
     * @param {Uuid[]) ignoreEntities
     */
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreEntities);

    /**jsdoc
     * @function RayPick.setIncludeItems
     * @param {number} id
     * @param {Uuid[]) includeEntities
     */
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeEntities);


    /**jsdoc
     * @function RayPick.isLeftHand
     * @param {number} id
     * @returns {boolean}
     */
    Q_INVOKABLE bool isLeftHand(unsigned int uid);

    /**jsdoc
     * @function RayPick.isRightHand
     * @param {number} id
     * @returns {boolean}
     */
    Q_INVOKABLE bool isRightHand(unsigned int uid);

    /**jsdoc
     * @function RayPick.isMouse
     * @param {number} id
     * @returns {boolean}
     */
    Q_INVOKABLE bool isMouse(unsigned int uid);

public slots:

    /**jsdoc
     * @function RayPick.PICK_ENTITIES
     * @returns {number}
     */
    static unsigned int PICK_ENTITIES() { return PickScriptingInterface::PICK_ENTITIES(); }

    /**jsdoc
     * @function RayPick.PICK_OVERLAYS
     * @returns {number}
     */
    static unsigned int PICK_OVERLAYS() { return PickScriptingInterface::PICK_OVERLAYS(); }

    /**jsdoc
     * @function RayPick.PICK_AVATARS
     * @returns {number}
     */
    static unsigned int PICK_AVATARS() { return PickScriptingInterface::PICK_AVATARS(); }

    /**jsdoc
     * @function RayPick.PICK_HUD
     * @returns {number}
     */
    static unsigned int PICK_HUD() { return PickScriptingInterface::PICK_HUD(); }

    /**jsdoc
     * @function RayPick.PICK_COARSE
     * @returns {number}
     */
    static unsigned int PICK_COARSE() { return PickScriptingInterface::PICK_COARSE(); }

    /**jsdoc
     * @function RayPick.PICK_INCLUDE_INVISIBLE
     * @returns {number}
     */
    static unsigned int PICK_INCLUDE_INVISIBLE() { return PickScriptingInterface::PICK_INCLUDE_INVISIBLE(); }

    /**jsdoc
     * @function RayPick.PICK_INCLUDE_NONCOLLIDABLE
     * @returns {number}
     */
    static unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return PickScriptingInterface::PICK_INCLUDE_NONCOLLIDABLE(); }

    /**jsdoc
     * @function RayPick.PICK_ALL_INTERSECTIONS
     * @returns {number}
     */
    static unsigned int PICK_ALL_INTERSECTIONS() { return PickScriptingInterface::PICK_ALL_INTERSECTIONS(); }

    /**jsdoc
     * @function RayPick.INTERSECTED_NONE
     * @returns {number}
     */
    static unsigned int INTERSECTED_NONE() { return PickScriptingInterface::INTERSECTED_NONE(); }

    /**jsdoc
     * @function RayPick.INTERSECTED_ENTITY
     * @returns {number}
     */
    static unsigned int INTERSECTED_ENTITY() { return PickScriptingInterface::INTERSECTED_ENTITY(); }

    /**jsdoc
     * @function RayPick.INTERSECTED_OVERLAY
     * @returns {number}
     */
    static unsigned int INTERSECTED_LOCAL_ENTITY() { return PickScriptingInterface::INTERSECTED_LOCAL_ENTITY(); }

    /**jsdoc
     * @function RayPick.INTERSECTED_OVERLAY
     * @returns {number}
     */
    static unsigned int INTERSECTED_OVERLAY() { return PickScriptingInterface::INTERSECTED_LOCAL_ENTITY(); }

    /**jsdoc
     * @function RayPick.INTERSECTED_AVATAR
     * @returns {number}
     */
    static unsigned int INTERSECTED_AVATAR() { return PickScriptingInterface::INTERSECTED_AVATAR(); }

    /**jsdoc
     * @function RayPick.INTERSECTED_HUD
     * @returns {number}
     */
    static unsigned int INTERSECTED_HUD() { return PickScriptingInterface::INTERSECTED_HUD(); }
};

#endif // hifi_RayPickScriptingInterface_h
