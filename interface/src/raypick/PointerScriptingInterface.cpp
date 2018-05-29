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
 * A set of properties used to define the visual aspect of a Ray Pointer in the case that the Pointer is intersecting something.
 *
 * @typedef {object} Pointers.RayPointerRenderState
 * @property {string} name The name of this render state, used by {@link Pointers.setRenderState} and {@link Pointers.editRenderState}
 * @property {Overlays.OverlayProperties} [start] All of the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field).
 * An overlay to represent the beginning of the Ray Pointer, if desired.
 * @property {Overlays.OverlayProperties} [path] All of the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field), which <b>must</b> be <code>"line3d"</code>.
 * An overlay to represent the path of the Ray Pointer, if desired.
 * @property {Overlays.OverlayProperties} [end] All of the properties you would normally pass to {@link Overlays.addOverlay}, plus the type (as a <code>type</code> field).
 * An overlay to represent the end of the Ray Pointer, if desired.
 */
/**jsdoc
 * A trigger mechanism for Ray Pointers.
 *
 * @typedef {object} Pointers.Trigger
 * @property {Controller.Standard|Controller.Actions|function} action This can be a built-in Controller action, like Controller.Standard.LTClick, or a function that evaluates to >= 1.0 when you want to trigger <code>button</code>.
 * @property {string} button Which button to trigger.  "Primary", "Secondary", "Tertiary", and "Focus" are currently supported.  Only "Primary" will trigger clicks on web surfaces.  If "Focus" is triggered,
 * it will try to set the entity or overlay focus to the object at which the Pointer is aimed.  Buttons besides the first three will still trigger events, but event.button will be "None".
 */
/**jsdoc
 * A set of properties that can be passed to {@link Pointers.createPointer} to create a new Pointer. Contains the relevant {@link Picks.PickProperties} to define the underlying Pick.
 * @typedef {object} Pointers.LaserPointerProperties
 * @property {boolean} [faceAvatar=false] Ray Pointers only.  If true, the end of the Pointer will always rotate to face the avatar.
 * @property {boolean} [centerEndY=true] Ray Pointers only.  If false, the end of the Pointer will be moved up by half of its height.
 * @property {boolean} [lockEnd=false] Ray Pointers only.  If true, the end of the Pointer will lock on to the center of the object at which the laser is pointing.
 * @property {boolean} [distanceScaleEnd=false] Ray Pointers only.  If true, the dimensions of the end of the Pointer will scale linearly with distance.
 * @property {boolean} [scaleWithAvatar=false] Ray Pointers only.  If true, the width of the Pointer's path will scale linearly with your avatar's scale.
 * @property {boolean} [enabled=false]
 * @property {Pointers.RayPointerRenderState[]} [renderStates] Ray Pointers only.  A list of different visual states to switch between.
 * @property {Pointers.DefaultRayPointerRenderState[]} [defaultRenderStates] Ray Pointers only.  A list of different visual states to use if there is no intersection.
 * @property {boolean} [hover=false] If this Pointer should generate hover events.
 * @property {Pointers.Trigger[]} [triggers] Ray Pointers only.  A list of different triggers mechanisms that control this Pointer's click event generation.
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

    bool enabled = false;
    if (propertyMap["enabled"].isValid()) {
        enabled = propertyMap["enabled"].toBool();
    }

    LaserPointer::RenderStateMap renderStates;
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

    LaserPointer::DefaultRenderStateMap defaultRenderStates;
    if (propertyMap["defaultRenderStates"].isValid()) {
        QList<QVariant> renderStateVariants = propertyMap["defaultRenderStates"].toList();
        for (const QVariant& renderStateVariant : renderStateVariants) {
            if (renderStateVariant.isValid()) {
                QVariantMap renderStateMap = renderStateVariant.toMap();
                if (renderStateMap["name"].isValid() && renderStateMap["distance"].isValid()) {
                    std::string name = renderStateMap["name"].toString().toStdString();
                    float distance = renderStateMap["distance"].toFloat();
                    defaultRenderStates[name] = std::pair<float, RenderState>(distance, LaserPointer::buildRenderState(renderStateMap));
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
                                                                                               faceAvatar, centerEndY, lockEnd, distanceScaleEnd, scaleWithAvatar, enabled));
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