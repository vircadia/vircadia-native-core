//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PointerScriptingInterface.h"

#include <QtCore/QVariant>
#include <GLMHelpers.h>
#include <shared/QtHelpers.h>

#include "Application.h"
#include "LaserPointer.h"
#include "StylusPointer.h"
#include "ParabolaPointer.h"

void PointerScriptingInterface::setIgnoreItems(unsigned int uid, const QScriptValue& ignoreItems) const {
    DependencyManager::get<PointerManager>()->setIgnoreItems(uid, qVectorQUuidFromScriptValue(ignoreItems));
}

void PointerScriptingInterface::setIncludeItems(unsigned int uid, const QScriptValue& includeItems) const {
    DependencyManager::get<PointerManager>()->setIncludeItems(uid, qVectorQUuidFromScriptValue(includeItems));
}

unsigned int PointerScriptingInterface::createPointer(const PickQuery::PickType& type, const QVariant& properties) {
    // Interaction with managers should always happen on the main thread
    if (QThread::currentThread() != qApp->thread()) {
        unsigned int result;
        BLOCKING_INVOKE_METHOD(this, "createPointer", Q_RETURN_ARG(unsigned int, result), Q_ARG(PickQuery::PickType, type), Q_ARG(QVariant, properties));
        return result;
    }

    switch (type) {
        case PickQuery::PickType::Ray:
            return createLaserPointer(properties);
        case PickQuery::PickType::Stylus:
            return createStylus(properties);
        case PickQuery::PickType::Parabola:
            return createParabolaPointer(properties);
        default:
            return PointerEvent::INVALID_POINTER_ID;
    }
}

/**jsdoc
 * A set of properties that can be passed to {@link Pointers.createPointer} to create a new Pointer.  Contains the relevant {@link Picks.PickProperties} to define the underlying Pick.
 * @typedef {object} Pointers.StylusPointerProperties
 * @property {boolean} [hover=false] If this pointer should generate hover events.
 * @property {boolean} [enabled=false]
 */
unsigned int PointerScriptingInterface::createStylus(const QVariant& properties) const {
    QVariantMap propertyMap = properties.toMap();

    bool hover = false;
    if (propertyMap["hover"].isValid()) {
        hover = propertyMap["hover"].toBool();
    }

    bool enabled = false;
    if (propertyMap["enabled"].isValid()) {
        enabled = propertyMap["enabled"].toBool();
    }

    return DependencyManager::get<PointerManager>()->addPointer(std::make_shared<StylusPointer>(properties, StylusPointer::buildStylusOverlay(propertyMap), hover, enabled));
}

/**jsdoc
 * A set of properties used to define the visual aspect of a Ray Pointer in the case that the Pointer is not intersecting something.  Same as a {@link Pointers.RayPointerRenderState},
 * but with an additional distance field.
 *
 * @typedef {object} Pointers.DefaultRayPointerRenderState
 * @augments Pointers.RayPointerRenderState
 * @property {number} distance The distance at which to render the end of this Ray Pointer, if one is defined.
 */
/**jsdoc
 * A set of properties which define the visual aspect of a Ray Pointer in the case that the Pointer is intersecting something.
 *
 * @typedef {object} Pointers.RayPointerRenderState
 * @property {string} name When using {@link Pointers.createPointer}, the name of this render state, used by {@link Pointers.setRenderState} and {@link Pointers.editRenderState}
 * @property {Overlays.OverlayProperties|QUuid} [start] When using {@link Pointers.createPointer}, an optionally defined overlay to represent the beginning of the Ray Pointer,
 * using the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field).
 * When returned from {@link Pointers.getPointerProperties}, the ID of the created overlay if it exists, or a null ID otherwise.
 * @property {Overlays.OverlayProperties|QUuid} [path] When using {@link Pointers.createPointer}, an optionally defined overlay to represent the path of the Ray Pointer,
 * using the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field), which <b>must</b> be <code>"line3d"</code>.
 * When returned from {@link Pointers.getPointerProperties}, the ID of the created overlay if it exists, or a null ID otherwise.
 * @property {Overlays.OverlayProperties|QUuid} [end] When using {@link Pointers.createPointer}, an optionally defined overlay to represent the end of the Ray Pointer,
 * using the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field).
 * When returned from {@link Pointers.getPointerProperties}, the ID of the created overlay if it exists, or a null ID otherwise.
 */
/**jsdoc
 * A set of properties that can be passed to {@link Pointers.createPointer} to create a new Pointer. Contains the relevant {@link Picks.PickProperties} to define the underlying Pick.
 * @typedef {object} Pointers.LaserPointerProperties
 * @property {boolean} [faceAvatar=false] If true, the end of the Pointer will always rotate to face the avatar.
 * @property {boolean} [centerEndY=true] If false, the end of the Pointer will be moved up by half of its height.
 * @property {boolean} [lockEnd=false] If true, the end of the Pointer will lock on to the center of the object at which the pointer is pointing.
 * @property {boolean} [distanceScaleEnd=false] If true, the dimensions of the end of the Pointer will scale linearly with distance.
 * @property {boolean} [scaleWithAvatar=false] If true, the width of the Pointer's path will scale linearly with your avatar's scale.
 * @property {boolean} [followNormal=false] If true, the end of the Pointer will rotate to follow the normal of the intersected surface.
 * @property {number} [followNormalStrength=0.0] The strength of the interpolation between the real normal and the visual normal if followNormal is true. <code>0-1</code>.  If 0 or 1,
 * the normal will follow exactly.
 * @property {boolean} [enabled=false]
 * @property {Pointers.RayPointerRenderState[]|Object.<string, Pointers.RayPointerRenderState>} [renderStates] A collection of different visual states to switch between.
 * When using {@link Pointers.createPointer}, a list of RayPointerRenderStates.
 * When returned from {@link Pointers.getPointerProperties}, a map between render state names and RayPointRenderStates.
 * @property {Pointers.DefaultRayPointerRenderState[]|Object.<string, Pointers.DefaultRayPointerRenderState>} [defaultRenderStates] A collection of different visual states to use if there is no intersection.
 * When using {@link Pointers.createPointer}, a list of DefaultRayPointerRenderStates.
 * When returned from {@link Pointers.getPointerProperties}, a map between render state names and DefaultRayPointRenderStates.
 * @property {boolean} [hover=false] If this Pointer should generate hover events.
 * @property {Pointers.Trigger[]} [triggers] A list of different triggers mechanisms that control this Pointer's click event generation.
 */
unsigned int PointerScriptingInterface::createLaserPointer(const QVariant& properties) const {
    QVariantMap propertyMap = properties.toMap();

    bool faceAvatar = false;
    if (propertyMap["faceAvatar"].isValid()) {
        faceAvatar = propertyMap["faceAvatar"].toBool();
    }

    bool centerEndY = true;
    if (propertyMap["centerEndY"].isValid()) {
        centerEndY = propertyMap["centerEndY"].toBool();
    }

    bool lockEnd = false;
    if (propertyMap["lockEnd"].isValid()) {
        lockEnd = propertyMap["lockEnd"].toBool();
    }

    bool distanceScaleEnd = false;
    if (propertyMap["distanceScaleEnd"].isValid()) {
        distanceScaleEnd = propertyMap["distanceScaleEnd"].toBool();
    }

    bool scaleWithAvatar = false;
    if (propertyMap["scaleWithAvatar"].isValid()) {
        scaleWithAvatar = propertyMap["scaleWithAvatar"].toBool();
    }

    bool followNormal = false;
    if (propertyMap["followNormal"].isValid()) {
        followNormal = propertyMap["followNormal"].toBool();
    }
    float followNormalStrength = 0.0f;
    if (propertyMap["followNormalStrength"].isValid()) {
        followNormalStrength = propertyMap["followNormalStrength"].toFloat();
    }

    bool enabled = false;
    if (propertyMap["enabled"].isValid()) {
        enabled = propertyMap["enabled"].toBool();
    }

    RenderStateMap renderStates;
    if (propertyMap["renderStates"].isValid()) {
        QList<QVariant> renderStateVariants = propertyMap["renderStates"].toList();
        for (const QVariant& renderStateVariant : renderStateVariants) {
            if (renderStateVariant.isValid()) {
                QVariantMap renderStateMap = renderStateVariant.toMap();
                if (renderStateMap["name"].isValid()) {
                    std::string name = renderStateMap["name"].toString().toStdString();
                    renderStates[name] = LaserPointer::buildRenderState(renderStateMap);
                }
            }
        }
    }

    DefaultRenderStateMap defaultRenderStates;
    if (propertyMap["defaultRenderStates"].isValid()) {
        QList<QVariant> renderStateVariants = propertyMap["defaultRenderStates"].toList();
        for (const QVariant& renderStateVariant : renderStateVariants) {
            if (renderStateVariant.isValid()) {
                QVariantMap renderStateMap = renderStateVariant.toMap();
                if (renderStateMap["name"].isValid() && renderStateMap["distance"].isValid()) {
                    std::string name = renderStateMap["name"].toString().toStdString();
                    float distance = renderStateMap["distance"].toFloat();
                    defaultRenderStates[name] = std::pair<float, std::shared_ptr<StartEndRenderState>>(distance, LaserPointer::buildRenderState(renderStateMap));
                }
            }
        }
    }

    bool hover = false;
    if (propertyMap["hover"].isValid()) {
        hover = propertyMap["hover"].toBool();
    }

    PointerTriggers triggers;
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    if (propertyMap["triggers"].isValid()) {
        QList<QVariant> triggerVariants = propertyMap["triggers"].toList();
        for (const QVariant& triggerVariant : triggerVariants) {
            if (triggerVariant.isValid()) {
                QVariantMap triggerMap = triggerVariant.toMap();
                if (triggerMap["action"].isValid() && triggerMap["button"].isValid()) {
                    controller::Endpoint::Pointer endpoint = userInputMapper->endpointFor(controller::Input(triggerMap["action"].toUInt()));
                    if (endpoint) {
                        std::string button = triggerMap["button"].toString().toStdString();
                        triggers.emplace_back(endpoint, button);
                    }
                }
            }
        }
    }

    return DependencyManager::get<PointerManager>()->addPointer(std::make_shared<LaserPointer>(properties, renderStates, defaultRenderStates, hover, triggers,
                                                                                               faceAvatar, followNormal, followNormalStrength, centerEndY, lockEnd,
                                                                                               distanceScaleEnd, scaleWithAvatar, enabled));
}

/**jsdoc
* The rendering properties of the parabolic path
*
* @typedef {object} Pointers.ParabolaProperties
* @property {Color} color=255,255,255 The color of the parabola.
* @property {number} alpha=1.0 The alpha of the parabola.
* @property {number} width=0.01 The width of the parabola, in meters.
* @property {boolean} isVisibleInSecondaryCamera=false The width of the parabola, in meters.
*/
/**jsdoc
* A set of properties used to define the visual aspect of a Parabola Pointer in the case that the Pointer is not intersecting something.  Same as a {@link Pointers.ParabolaPointerRenderState},
* but with an additional distance field.
*
* @typedef {object} Pointers.DefaultParabolaPointerRenderState
* @augments Pointers.ParabolaPointerRenderState
* @property {number} distance The distance along the parabola at which to render the end of this Parabola Pointer, if one is defined.
*/
/**jsdoc
* A set of properties used to define the visual aspect of a Parabola Pointer in the case that the Pointer is intersecting something.
*
* @typedef {object} Pointers.ParabolaPointerRenderState
* @property {string} name When using {@link Pointers.createPointer}, the name of this render state, used by {@link Pointers.setRenderState} and {@link Pointers.editRenderState}
* @property {Overlays.OverlayProperties|QUuid} [start] When using {@link Pointers.createPointer}, an optionally defined overlay to represent the beginning of the Parabola Pointer,
* using the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field).
* When returned from {@link Pointers.getPointerProperties}, the ID of the created overlay if it exists, or a null ID otherwise.
* @property {Pointers.ParabolaProperties} [path]  When using {@link Pointers.createPointer}, the optionally defined rendering properties of the parabolic path defined by the Parabola Pointer.
* Not defined in {@link Pointers.getPointerProperties}.
* @property {Overlays.OverlayProperties|QUuid} [end] When using {@link Pointers.createPointer}, an optionally defined overlay to represent the end of the Parabola Pointer,
* using the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field).
* When returned from {@link Pointers.getPointerProperties}, the ID of the created overlay if it exists, or a null ID otherwise.
*/
/**jsdoc
* A set of properties that can be passed to {@link Pointers.createPointer} to create a new Pointer. Contains the relevant {@link Picks.PickProperties} to define the underlying Pick.
* @typedef {object} Pointers.ParabolaPointerProperties
* @property {boolean} [faceAvatar=false] If true, the end of the Pointer will always rotate to face the avatar.
* @property {boolean} [centerEndY=true] If false, the end of the Pointer will be moved up by half of its height.
* @property {boolean} [lockEnd=false] If true, the end of the Pointer will lock on to the center of the object at which the pointer is pointing.
* @property {boolean} [distanceScaleEnd=false] If true, the dimensions of the end of the Pointer will scale linearly with distance.
* @property {boolean} [scaleWithAvatar=false] If true, the width of the Pointer's path will scale linearly with your avatar's scale.
* @property {boolean} [followNormal=false] If true, the end of the Pointer will rotate to follow the normal of the intersected surface.
* @property {number} [followNormalStrength=0.0] The strength of the interpolation between the real normal and the visual normal if followNormal is true. <code>0-1</code>.  If 0 or 1,
* the normal will follow exactly.
* @property {boolean} [enabled=false]
* @property {Pointers.ParabolaPointerRenderState[]|Object.<string, Pointers.ParabolaPointerRenderState>} [renderStates] A collection of different visual states to switch between.
* When using {@link Pointers.createPointer}, a list of ParabolaPointerRenderStates.
* When returned from {@link Pointers.getPointerProperties}, a map between render state names and ParabolaPointerRenderStates.
* @property {Pointers.DefaultParabolaPointerRenderState[]|Object.<string, Pointers.DefaultParabolaPointerRenderState>} [defaultRenderStates] A collection of different visual states to use if there is no intersection.
* When using {@link Pointers.createPointer}, a list of DefaultParabolaPointerRenderStates.
* When returned from {@link Pointers.getPointerProperties}, a map between render state names and DefaultParabolaPointerRenderStates.
* @property {boolean} [hover=false] If this Pointer should generate hover events.
* @property {Pointers.Trigger[]} [triggers] A list of different triggers mechanisms that control this Pointer's click event generation.
*/
unsigned int PointerScriptingInterface::createParabolaPointer(const QVariant& properties) const {
    QVariantMap propertyMap = properties.toMap();

    bool faceAvatar = false;
    if (propertyMap["faceAvatar"].isValid()) {
        faceAvatar = propertyMap["faceAvatar"].toBool();
    }

    bool centerEndY = true;
    if (propertyMap["centerEndY"].isValid()) {
        centerEndY = propertyMap["centerEndY"].toBool();
    }

    bool lockEnd = false;
    if (propertyMap["lockEnd"].isValid()) {
        lockEnd = propertyMap["lockEnd"].toBool();
    }

    bool distanceScaleEnd = false;
    if (propertyMap["distanceScaleEnd"].isValid()) {
        distanceScaleEnd = propertyMap["distanceScaleEnd"].toBool();
    }

    bool scaleWithAvatar = false;
    if (propertyMap["scaleWithAvatar"].isValid()) {
        scaleWithAvatar = propertyMap["scaleWithAvatar"].toBool();
    }

    bool followNormal = false;
    if (propertyMap["followNormal"].isValid()) {
        followNormal = propertyMap["followNormal"].toBool();
    }
    float followNormalStrength = 0.0f;
    if (propertyMap["followNormalStrength"].isValid()) {
        followNormalStrength = propertyMap["followNormalStrength"].toFloat();
    }

    bool enabled = false;
    if (propertyMap["enabled"].isValid()) {
        enabled = propertyMap["enabled"].toBool();
    }

    RenderStateMap renderStates;
    if (propertyMap["renderStates"].isValid()) {
        QList<QVariant> renderStateVariants = propertyMap["renderStates"].toList();
        for (const QVariant& renderStateVariant : renderStateVariants) {
            if (renderStateVariant.isValid()) {
                QVariantMap renderStateMap = renderStateVariant.toMap();
                if (renderStateMap["name"].isValid()) {
                    std::string name = renderStateMap["name"].toString().toStdString();
                    renderStates[name] = ParabolaPointer::buildRenderState(renderStateMap);
                }
            }
        }
    }

    DefaultRenderStateMap defaultRenderStates;
    if (propertyMap["defaultRenderStates"].isValid()) {
        QList<QVariant> renderStateVariants = propertyMap["defaultRenderStates"].toList();
        for (const QVariant& renderStateVariant : renderStateVariants) {
            if (renderStateVariant.isValid()) {
                QVariantMap renderStateMap = renderStateVariant.toMap();
                if (renderStateMap["name"].isValid() && renderStateMap["distance"].isValid()) {
                    std::string name = renderStateMap["name"].toString().toStdString();
                    float distance = renderStateMap["distance"].toFloat();
                    defaultRenderStates[name] = std::pair<float, std::shared_ptr<StartEndRenderState>>(distance, ParabolaPointer::buildRenderState(renderStateMap));
                }
            }
        }
    }

    bool hover = false;
    if (propertyMap["hover"].isValid()) {
        hover = propertyMap["hover"].toBool();
    }

    PointerTriggers triggers;
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    if (propertyMap["triggers"].isValid()) {
        QList<QVariant> triggerVariants = propertyMap["triggers"].toList();
        for (const QVariant& triggerVariant : triggerVariants) {
            if (triggerVariant.isValid()) {
                QVariantMap triggerMap = triggerVariant.toMap();
                if (triggerMap["action"].isValid() && triggerMap["button"].isValid()) {
                    controller::Endpoint::Pointer endpoint = userInputMapper->endpointFor(controller::Input(triggerMap["action"].toUInt()));
                    if (endpoint) {
                        std::string button = triggerMap["button"].toString().toStdString();
                        triggers.emplace_back(endpoint, button);
                    }
                }
            }
        }
    }

    return DependencyManager::get<PointerManager>()->addPointer(std::make_shared<ParabolaPointer>(properties, renderStates, defaultRenderStates, hover, triggers,
                                                                                                  faceAvatar, followNormal, followNormalStrength, centerEndY, lockEnd, distanceScaleEnd,
                                                                                                  scaleWithAvatar, enabled));
}

void PointerScriptingInterface::editRenderState(unsigned int uid, const QString& renderState, const QVariant& properties) const {
    QVariantMap propMap = properties.toMap();

    QVariant startProps;
    if (propMap["start"].isValid()) {
        startProps = propMap["start"];
    }

    QVariant pathProps;
    if (propMap["path"].isValid()) {
        pathProps = propMap["path"];
    }

    QVariant endProps;
    if (propMap["end"].isValid()) {
        endProps = propMap["end"];
    }

    DependencyManager::get<PointerManager>()->editRenderState(uid, renderState.toStdString(), startProps, pathProps, endProps);
}

QVariantMap PointerScriptingInterface::getPrevPickResult(unsigned int uid) const { 
    QVariantMap result;
    auto pickResult = DependencyManager::get<PointerManager>()->getPrevPickResult(uid);
    if (pickResult) {
        result = pickResult->toVariantMap();
    }
    return result;
}

QVariantMap PointerScriptingInterface::getPointerProperties(unsigned int uid) const {
    return DependencyManager::get<PointerManager>()->getPointerProperties(uid);
}