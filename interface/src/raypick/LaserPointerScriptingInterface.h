//
//  LaserPointerScriptingInterface.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerScriptingInterface_h
#define hifi_LaserPointerScriptingInterface_h

#include <QtCore/QObject>

#include "DependencyManager.h"
#include <PointerManager.h>

class LaserPointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

/**jsdoc
 * Synonym for {@link Pointers} as used for laser pointers.  Deprecated.
 *
 * @namespace LaserPointers
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
public:

    /**jsdoc
     * @function LaserPointers.createLaserPointer
     * @param {Pointers.LaserPointerProperties} properties
     * @returns {number}
     */
    Q_INVOKABLE unsigned int createLaserPointer(const QVariant& properties) const;

    /**jsdoc
     * @function LaserPointers.enableLaserPointer
     * @param {number} id
     */
    Q_INVOKABLE void enableLaserPointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->enablePointer(uid); }

    /**jsdoc
     * @function LaserPointers.disableLaserPointer
     * @param {number} id
     */
    Q_INVOKABLE void disableLaserPointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->disablePointer(uid); }

    /**jsdoc
     * @function LaserPointers.removeLaserPointer
     * @param {number} id
     */
    Q_INVOKABLE void removeLaserPointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->removePointer(uid); }

    /**jsdoc
     * @function LaserPointers.editRenderState
     * @param {number} id
     * @param {string} renderState
     * @param {Pointers.RayPointerRenderState} properties
     */
    Q_INVOKABLE void editRenderState(unsigned int uid, const QString& renderState, const QVariant& properties) const;

    /**jsdoc
     * @function LaserPointers.setRenderState
     * @param {string} renderState
     * @param {number} id
     */
    Q_INVOKABLE void setRenderState(unsigned int uid, const QString& renderState) const { DependencyManager::get<PointerManager>()->setRenderState(uid, renderState.toStdString()); }

    /**jsdoc
     * @function LaserPointers.getPrevRayPickResult
     * @param {number} id
     * @returns {RayPickResult}
     */
    Q_INVOKABLE QVariantMap getPrevRayPickResult(unsigned int uid) const;


    /**jsdoc
     * @function LaserPointers.setPrecisionPicking
     * @param {number} id
     * @param {boolean} precisionPicking
     */
    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking) const { DependencyManager::get<PointerManager>()->setPrecisionPicking(uid, precisionPicking); }

    /**jsdoc
     * @function LaserPointers.setLaserLength
     * @param {number} id
     * @param {number} laserLength
     */
    Q_INVOKABLE void setLaserLength(unsigned int uid, float laserLength) const { DependencyManager::get<PointerManager>()->setLength(uid, laserLength); }

    /**jsdoc
     * @function LaserPointers.setIgnoreItems
     * @param {number} id
     * @param {Uuid[]} ignoreItems
     */
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreEntities) const;

    /**jsdoc
     * @function LaserPointers.setIncludeItems
     * @param {number} id
     * @param {Uuid[]} includeItems
     */
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeEntities) const;


    /**jsdoc
     * @function LaserPointers.setLockEndUUID
     * @param {number} id
     * @param {Uuid} itemID
     * @param {boolean} isAvatar
     * @param {Mat4} [offsetMat]
     */
    Q_INVOKABLE void setLockEndUUID(unsigned int uid, const QUuid& objectID, bool isAvatar, const glm::mat4& offsetMat = glm::mat4()) const { DependencyManager::get<PointerManager>()->setLockEndUUID(uid, objectID, isAvatar, offsetMat); }


    /**jsdoc
     * @function LaserPointers.isLeftHand
     * @param {number} id
     * @returns {boolean}
     */
    Q_INVOKABLE bool isLeftHand(unsigned int uid) { return DependencyManager::get<PointerManager>()->isLeftHand(uid); }

    /**jsdoc
     * @function LaserPointers.isRightHand
     * @param {number} id
     * @returns {boolean}
     */
    Q_INVOKABLE bool isRightHand(unsigned int uid) { return DependencyManager::get<PointerManager>()->isRightHand(uid); }

    /**jsdoc
     * @function LaserPointers.isMouse
     * @param {number} id
     * @returns {boolean}
     */
    Q_INVOKABLE bool isMouse(unsigned int uid) { return DependencyManager::get<PointerManager>()->isMouse(uid); }
};

#endif // hifi_LaserPointerScriptingInterface_h
