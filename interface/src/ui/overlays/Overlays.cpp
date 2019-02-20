//
//  Overlays.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlays.h"

#include <limits>

#include <QtScript/QScriptValueIterator>

#include <shared/QtHelpers.h>
#include <OffscreenUi.h>
#include <render/Scene.h>
#include <RegisteredMetaTypes.h>

#include "Application.h"
#include "InterfaceLogging.h"

#include "ImageOverlay.h"
#include "TextOverlay.h"
#include "RectangleOverlay.h"

#include <raypick/RayPick.h>
#include <PointerManager.h>
#include <raypick/MouseTransformNode.h>
#include <PickManager.h>

#include <RenderableWebEntityItem.h>
#include "VariantMapToScriptValue.h"

#include "ui/Keyboard.h"
#include <QtQuick/QQuickWindow>

Q_LOGGING_CATEGORY(trace_render_overlays, "trace.render.overlays")

std::unordered_map<QString, QString> Overlays::_entityToOverlayTypes;
std::unordered_map<QString, QString> Overlays::_overlayToEntityTypes;

Overlays::Overlays() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    connect(pointerManager.data(), &PointerManager::hoverBeginOverlay, this, &Overlays::hoverEnterPointerEvent);
    connect(pointerManager.data(), &PointerManager::hoverContinueOverlay, this, &Overlays::hoverOverPointerEvent);
    connect(pointerManager.data(), &PointerManager::hoverEndOverlay, this, &Overlays::hoverLeavePointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerBeginOverlay, this, &Overlays::mousePressPointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerContinueOverlay, this, &Overlays::mouseMovePointerEvent);
    connect(pointerManager.data(), &PointerManager::triggerEndOverlay, this, &Overlays::mouseReleasePointerEvent);

    ADD_TYPE_MAP(Box, cube);
    ADD_TYPE_MAP(Sphere, sphere);
    _overlayToEntityTypes["rectangle3d"] = "Shape";
    ADD_TYPE_MAP(Shape, shape);
    ADD_TYPE_MAP(Model, model);
    ADD_TYPE_MAP(Text, text3d);
    ADD_TYPE_MAP(Image, image3d);
    _overlayToEntityTypes["billboard"] = "Image";
    ADD_TYPE_MAP(Web, web3d);
    ADD_TYPE_MAP(PolyLine, line3d);
    ADD_TYPE_MAP(Grid, grid);
    ADD_TYPE_MAP(Gizmo, circle3d);
}

void Overlays::cleanupAllOverlays() {
    _shuttingDown = true;
    QMap<QUuid, Overlay::Pointer> overlays;
    {
        QMutexLocker locker(&_mutex);
        overlays.swap(_overlays);
    }

    foreach(Overlay::Pointer overlay, overlays) {
        _overlaysToDelete.push_back(overlay);
    }
    cleanupOverlaysToDelete();
}

void Overlays::init() {
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>().data();
    auto pointerManager = DependencyManager::get<PointerManager>();
    connect(pointerManager.data(), &PointerManager::hoverBeginOverlay, entityScriptingInterface , &EntityScriptingInterface::hoverEnterEntity);
    connect(pointerManager.data(), &PointerManager::hoverContinueOverlay, entityScriptingInterface, &EntityScriptingInterface::hoverOverEntity);
    connect(pointerManager.data(), &PointerManager::hoverEndOverlay, entityScriptingInterface, &EntityScriptingInterface::hoverLeaveEntity);
    connect(pointerManager.data(), &PointerManager::triggerBeginOverlay, entityScriptingInterface, &EntityScriptingInterface::mousePressOnEntity);
    connect(pointerManager.data(), &PointerManager::triggerContinueOverlay, entityScriptingInterface, &EntityScriptingInterface::mouseMoveOnEntity);
    connect(pointerManager.data(), &PointerManager::triggerEndOverlay, entityScriptingInterface, &EntityScriptingInterface::mouseReleaseOnEntity);
}

void Overlays::update(float deltatime) {
    cleanupOverlaysToDelete();
}

void Overlays::cleanupOverlaysToDelete() {
    if (!_overlaysToDelete.isEmpty()) {
        render::ScenePointer scene = qApp->getMain3DScene();
        render::Transaction transaction;

        {
            do {
                Overlay::Pointer overlay = _overlaysToDelete.takeLast();

                auto itemID = overlay->getRenderItemID();
                if (render::Item::isValidID(itemID)) {
                    overlay->removeFromScene(overlay, scene, transaction);
                }
            } while (!_overlaysToDelete.isEmpty());
        }

        if (transaction.hasRemovedItems()) {
            scene->enqueueTransaction(transaction);
        }
    }
}

void Overlays::render(RenderArgs* renderArgs) {
    PROFILE_RANGE(render_overlays, __FUNCTION__);
    gpu::Batch& batch = *renderArgs->_batch;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto textureCache = DependencyManager::get<TextureCache>();

    auto size = glm::uvec2(qApp->getUiSize());
    int width = size.x;
    int height = size.y;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, -1000, 1000);

    QMutexLocker locker(&_mutex);
    foreach(Overlay::Pointer thisOverlay, _overlays) {

        // Reset all batch pipeline settings between overlay
        geometryCache->useSimpleDrawPipeline(batch);
        batch.setResourceTexture(0, textureCache->getWhiteTexture()); // FIXME - do we really need to do this??
        batch.setProjectionTransform(legacyProjection);
        batch.setModelTransform(Transform());
        batch.resetViewTransform();

        thisOverlay->render(renderArgs);
    }
}

void Overlays::disable() {
    _enabled = false;
}

void Overlays::enable() {
    _enabled = true;
}

Overlay::Pointer Overlays::take2DOverlay(const QUuid& id) {
    if (_shuttingDown) {
        return nullptr;
    }

    QMutexLocker locker(&_mutex);
    auto overlayIter = _overlays.find(id);
    if (overlayIter != _overlays.end()) {
        return _overlays.take(id);
    }
    return nullptr;
}

Overlay::Pointer Overlays::get2DOverlay(const QUuid& id) const {
    if (_shuttingDown) {
        return nullptr;
    }

    QMutexLocker locker(&_mutex);
    auto overlayIter = _overlays.find(id);
    if (overlayIter != _overlays.end()) {
        return overlayIter.value();
    }
    return nullptr;
}

QString Overlays::entityToOverlayType(const QString& type) {
    auto iter = _entityToOverlayTypes.find(type);
    if (iter != _entityToOverlayTypes.end()) {
        return iter->second;
    }
    return "unknown";
}

QString Overlays::overlayToEntityType(const QString& type) {
    auto iter = _overlayToEntityTypes.find(type);
    if (iter != _overlayToEntityTypes.end()) {
        return iter->second;
    }
    return "Unknown";
}

#define SET_OVERLAY_PROP_DEFAULT(o, d)                      \
    {                                                       \
        if (add && !overlayProps.contains(#o)) {            \
            overlayProps[#o] = d;                           \
        }                                                   \
    }

#define RENAME_PROP(o, e)                                   \
    {                                                       \
        auto iter = overlayProps.find(#o);                  \
        if (iter != overlayProps.end()) {                   \
            overlayProps[#e] = iter.value();                \
        }                                                   \
    }

#define RENAME_PROP_CONVERT(o, e, C)                        \
    {                                                       \
        auto iter = overlayProps.find(#o);                  \
        if (iter != overlayProps.end()) {                   \
            overlayProps[#e] = C(iter.value());             \
        }                                                   \
    }

#define OVERLAY_TO_GROUP_ENTITY_PROP(o, g, e)               \
    {                                                       \
        auto iter = overlayProps.find(#o);                  \
        if (iter != overlayProps.end()) {                   \
            if (!overlayProps.contains(#g)) {               \
                overlayProps[#g] = QVariantMap();           \
            }                                               \
            auto map = overlayProps[#g].toMap();            \
            map[#e] = iter.value();                         \
            overlayProps[#g] = map;                         \
        }                                                   \
    }

#define OVERLAY_TO_GROUP_ENTITY_PROP_DEFAULT(o, g, e, d)    \
    {                                                       \
        auto iter = overlayProps.find(#o);                  \
        if (iter != overlayProps.end()) {                   \
            if (!overlayProps.contains(#g)) {               \
                overlayProps[#g] = QVariantMap();           \
            }                                               \
            auto map = overlayProps[#g].toMap();            \
            map[#e] = iter.value();                         \
            overlayProps[#g] = map;                         \
        } else if (add) {                                   \
            if (!overlayProps.contains(#g)) {               \
                overlayProps[#g] = QVariantMap();           \
            }                                               \
            auto map = overlayProps[#g].toMap();            \
            map[#e] = d;                                    \
            overlayProps[#g] = map;                         \
        }                                                   \
    }

#define OVERLAY_TO_ENTITY_PROP_CONVERT_DEFAULT(o, e, d, C)  \
    {                                                       \
        auto iter = overlayProps.find(#o);                  \
        if (iter != overlayProps.end()) {                   \
            overlayProps[#e] = C(iter.value());             \
        } else if (add) {                                   \
            overlayProps[#e] = C(d);                        \
        }                                                   \
    }

#define OVERLAY_TO_GROUP_ENTITY_PROP_CONVERT(o, g, e, C)    \
    {                                                       \
        auto iter = overlayProps.find(#o);                  \
        if (iter != overlayProps.end()) {                   \
            if (!overlayProps.contains(#g)) {               \
                overlayProps[#g] = QVariantMap();           \
            }                                               \
            auto map = overlayProps[#g].toMap();            \
            map[#e] = C(iter.value());                      \
            overlayProps[#g] = map;                         \
        }                                                   \
    }

#define GROUP_ENTITY_TO_OVERLAY_PROP(g, e, o)               \
    {                                                       \
        auto iter = overlayProps.find(#g);                  \
        if (iter != overlayProps.end()) {                   \
            auto map = iter.value().toMap();                \
            auto iter2 = map.find(#e);                      \
            if (iter2 != map.end()) {                       \
                overlayProps[#o] = iter2.value();           \
            }                                               \
        }                                                   \
    }

#define GROUP_ENTITY_TO_OVERLAY_PROP_CONVERT(g, e, o, C)    \
    {                                                       \
        auto iter = overlayProps.find(#g);                  \
        if (iter != overlayProps.end()) {                   \
            auto map = iter.value().toMap();                \
            auto iter2 = map.find(#e);                      \
            if (iter2 != map.end()) {                       \
                overlayProps[#o] = C(iter2.value());        \
            }                                               \
        }                                                   \
    }

static QHash<QUuid, glm::quat> savedRotations = QHash<QUuid, glm::quat>();

EntityItemProperties Overlays::convertOverlayToEntityProperties(QVariantMap& overlayProps, const QString& type, bool add, const QUuid& id) {
    glm::quat rotation;
    return convertOverlayToEntityProperties(overlayProps, rotation, type, add, id);
}

EntityItemProperties Overlays::convertOverlayToEntityProperties(QVariantMap& overlayProps, glm::quat& rotationToSave, const QString& type, bool add, const QUuid& id) {
    overlayProps["type"] = type;

    SET_OVERLAY_PROP_DEFAULT(alpha, 0.7);
    if (type != "PolyLine") {
        RENAME_PROP(p1, position);
        RENAME_PROP(start, position);
    }
    RENAME_PROP(point, position);
    RENAME_PROP(scale, dimensions);
    RENAME_PROP(size, dimensions);
    RENAME_PROP(orientation, rotation);
    RENAME_PROP(localOrientation, localRotation);
    RENAME_PROP(ignoreRayIntersection, ignorePickIntersection);

    RENAME_PROP_CONVERT(drawInFront, renderLayer, [](const QVariant& v) { return v.toBool() ? "front" : "world"; });
    RENAME_PROP_CONVERT(drawHUDLayer, renderLayer, [=](const QVariant& v) {
        bool f = v.toBool();
        if (f) {
            return QVariant("hud");
        } else if (overlayProps.contains("renderLayer")) {
            return overlayProps["renderLayer"];
        }
        return QVariant("world");
    });

    OVERLAY_TO_GROUP_ENTITY_PROP_DEFAULT(grabbable, grab, grabbable, false);

    OVERLAY_TO_GROUP_ENTITY_PROP(pulseMin, pulse, min);
    OVERLAY_TO_GROUP_ENTITY_PROP(pulseMax, pulse, max);
    OVERLAY_TO_GROUP_ENTITY_PROP(pulsePeriod, pulse, period);
    OVERLAY_TO_GROUP_ENTITY_PROP_CONVERT(colorPulse, pulse, colorMode, [](const QVariant& v) {
        float f = v.toFloat();
        if (f > 0.0f) {
            return "in";
        } else if (f < 0.0f) {
            return "out";
        }
        return "none";
    });
    OVERLAY_TO_GROUP_ENTITY_PROP_CONVERT(alphaPulse, pulse, alphaMode, [](const QVariant& v) {
        float f = v.toFloat();
        if (f > 0.0f) {
            return "in";
        } else if (f < 0.0f) {
            return "out";
        }
        return "none";
    });

    RENAME_PROP_CONVERT(textures, textures, [](const QVariant& v) {
        auto map = v.toMap();
        if (!map.isEmpty()) {
            auto json = QJsonDocument::fromVariant(map);
            if (!json.isNull()) {
                return QVariant(QString(json.toJson()));
            }
        }
        return v;
    });

    if (type == "Shape" || type == "Box" || type == "Sphere" || type == "Gizmo") {
        RENAME_PROP(solid, isSolid);
        RENAME_PROP(isFilled, isSolid);
        RENAME_PROP(filled, isSolid);
        OVERLAY_TO_ENTITY_PROP_CONVERT_DEFAULT(isSolid, primitiveMode, false, [](const QVariant& v) { return v.toBool() ? "solid" : "lines"; });

        RENAME_PROP(wire, isWire);
        RENAME_PROP_CONVERT(isWire, primitiveMode, [](const QVariant& v) { return v.toBool() ? "lines" : "solid"; });
    }

    if (type == "Shape") {
        SET_OVERLAY_PROP_DEFAULT(shape, "Hexagon");
    } else if (type == "Model") {
        RENAME_PROP(url, modelURL);
        RENAME_PROP(animationSettings, animation);
    } else if (type == "Image") {
        RENAME_PROP(url, imageURL);
    } else if (type == "Text") {
        RENAME_PROP(color, textColor);
    } else if (type == "Web") {
        RENAME_PROP(url, sourceUrl);
        RENAME_PROP_CONVERT(inputMode, inputMode, [](const QVariant& v) { return v.toString() == "Mouse" ? "mouse" : "touch"; });
    } else if (type == "Gizmo") {
        RENAME_PROP(radius, outerRadius);
        if (add || overlayProps.contains("outerRadius")) {
            float ratio = 2.0f;
            {
                auto iter = overlayProps.find("outerRadius");
                if (iter != overlayProps.end()) {
                    ratio = iter.value().toFloat() / 0.5f;
                }
            }
            glm::vec3 dimensions = glm::vec3(1.0f);
            {
                auto iter = overlayProps.find("dimensions");
                if (iter != overlayProps.end()) {
                    dimensions = vec3FromVariant(iter.value());
                } else if (!add) {
                    EntityPropertyFlags desiredProperties;
                    desiredProperties += PROP_DIMENSIONS;
                    dimensions = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id, desiredProperties).getDimensions();
                }
            }
            overlayProps["dimensions"] = vec3toVariant(ratio * dimensions);
        }

        if (add || overlayProps.contains("rotation")) {
            glm::quat rotation;
            {
                auto iter = overlayProps.find("rotation");
                if (iter != overlayProps.end()) {
                    rotation = quatFromVariant(iter.value());
                } else if (!add) {
                    EntityPropertyFlags desiredProperties;
                    desiredProperties += PROP_ROTATION;
                    rotation = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id, desiredProperties).getRotation();
                }
            }
            overlayProps["rotation"] = quatToVariant(glm::angleAxis(-(float)M_PI_2, rotation * Vectors::RIGHT) * rotation);
        }
        if (add || overlayProps.contains("localRotation")) {
            glm::quat rotation;
            {
                auto iter = overlayProps.find("localRotation");
                if (iter != overlayProps.end()) {
                    rotation = quatFromVariant(iter.value());
                } else if (!add) {
                    EntityPropertyFlags desiredProperties;
                    desiredProperties += PROP_LOCAL_ROTATION;
                    rotation = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id, desiredProperties).getLocalRotation();
                }
            }
            overlayProps["localRotation"] = quatToVariant(glm::angleAxis(-(float)M_PI_2, rotation * Vectors::RIGHT) * rotation);
        }

        {
            RENAME_PROP(color, innerStartColor);
            RENAME_PROP(color, innerEndColor);
            RENAME_PROP(color, outerStartColor);
            RENAME_PROP(color, outerEndColor);

            RENAME_PROP(startColor, innerStartColor);
            RENAME_PROP(startColor, outerStartColor);

            RENAME_PROP(endColor, innerEndColor);
            RENAME_PROP(endColor, outerEndColor);

            RENAME_PROP(innerColor, innerStartColor);
            RENAME_PROP(innerColor, innerEndColor);

            RENAME_PROP(outerColor, outerStartColor);
            RENAME_PROP(outerColor, outerEndColor);
        }

        {
            RENAME_PROP(alpha, innerStartAlpha);
            RENAME_PROP(alpha, innerEndAlpha);
            RENAME_PROP(alpha, outerStartAlpha);
            RENAME_PROP(alpha, outerEndAlpha);

            RENAME_PROP(startAlpha, innerStartAlpha);
            RENAME_PROP(startAlpha, outerStartAlpha);

            RENAME_PROP(endAlpha, innerEndAlpha);
            RENAME_PROP(endAlpha, outerEndAlpha);

            RENAME_PROP(innerAlpha, innerStartAlpha);
            RENAME_PROP(innerAlpha, innerEndAlpha);

            RENAME_PROP(outerAlpha, outerStartAlpha);
            RENAME_PROP(outerAlpha, outerEndAlpha);
        }

        OVERLAY_TO_GROUP_ENTITY_PROP(startAt, ring, startAngle);
        OVERLAY_TO_GROUP_ENTITY_PROP(endAt, ring, endAngle);
        OVERLAY_TO_GROUP_ENTITY_PROP(innerRadius, ring, innerRadius);

        OVERLAY_TO_GROUP_ENTITY_PROP(innerStartColor, ring, innerStartColor);
        OVERLAY_TO_GROUP_ENTITY_PROP(innerEndColor, ring, innerEndColor);
        OVERLAY_TO_GROUP_ENTITY_PROP(outerStartColor, ring, outerStartColor);
        OVERLAY_TO_GROUP_ENTITY_PROP(outerEndColor, ring, outerEndColor);
        OVERLAY_TO_GROUP_ENTITY_PROP(innerStartAlpha, ring, innerStartAlpha);
        OVERLAY_TO_GROUP_ENTITY_PROP(innerEndAlpha, ring, innerEndAlpha);
        OVERLAY_TO_GROUP_ENTITY_PROP(outerStartAlpha, ring, outerStartAlpha);
        OVERLAY_TO_GROUP_ENTITY_PROP(outerEndAlpha, ring, outerEndAlpha);

        OVERLAY_TO_GROUP_ENTITY_PROP(hasTickMarks, ring, hasTickMarks);
        OVERLAY_TO_GROUP_ENTITY_PROP(majorTickMarksAngle, ring, majorTickMarksAngle);
        OVERLAY_TO_GROUP_ENTITY_PROP(minorTickMarksAngle, ring, minorTickMarksAngle);
        OVERLAY_TO_GROUP_ENTITY_PROP(majorTickMarksLength, ring, majorTickMarksLength);
        OVERLAY_TO_GROUP_ENTITY_PROP(minorTickMarksLength, ring, minorTickMarksLength);
        OVERLAY_TO_GROUP_ENTITY_PROP(majorTickMarksColor, ring, majorTickMarksColor);
        OVERLAY_TO_GROUP_ENTITY_PROP(minorTickMarksColor, ring, minorTickMarksColor);
    } else if (type == "PolyLine") {
        RENAME_PROP(startPoint, p1);
        RENAME_PROP(start, p1);
        RENAME_PROP(endPoint, p2);
        RENAME_PROP(end, p2);

        RENAME_PROP(p1, position);
        RENAME_PROP_CONVERT(p1, p1, [](const QVariant& v) { return vec3toVariant(glm::vec3(0.0f)); });
        RENAME_PROP_CONVERT(p2, p2, [=](const QVariant& v) {
            glm::vec3 position;
            auto iter2 = overlayProps.find("position");
            if (iter2 != overlayProps.end()) {
                position = vec3FromVariant(iter2.value());
            } else if (!add) {
                EntityPropertyFlags desiredProperties;
                desiredProperties += PROP_POSITION;
                position = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id, desiredProperties).getPosition();
            }
            return vec3toVariant(vec3FromVariant(v) - position);
        });

        RENAME_PROP(localStart, p1);
        RENAME_PROP(localEnd, p2);

        {
            QVariantList points;
            {
                auto iter = overlayProps.find("p1");
                if (iter != overlayProps.end()) {
                    points.push_back(iter.value());
                }
            }
            {
                auto iter = overlayProps.find("p2");
                if (iter != overlayProps.end()) {
                    points.push_back(iter.value());
                }
            }
            overlayProps["linePoints"] = points;
        }
        {
            auto iter = overlayProps.find("lineWidth");
            if (iter != overlayProps.end()) {
                QVariantList widths;
                QVariant width = iter.value();
                widths.append(width);
                widths.append(width);
                overlayProps["strokeWidths"] = widths;
            }
        }

        RENAME_PROP_CONVERT(glow, glow, [](const QVariant& v) { return v.toFloat() > 0.0f ? true : false; });
        SET_OVERLAY_PROP_DEFAULT(faceCamera, true);
        {
            QVariantList normals;
            normals.append(vec3toVariant(Vectors::UP));
            normals.append(vec3toVariant(Vectors::UP));
            SET_OVERLAY_PROP_DEFAULT(normals, normals);
        }

        SET_OVERLAY_PROP_DEFAULT(textures, PathUtils::resourcesUrl() + "images/whitePixel.png");
    }

    if (type == "Text" || type == "Image" || type == "Grid" || type == "Web") {
        glm::quat originalRotation = ENTITY_ITEM_DEFAULT_ROTATION;
        {
            auto iter = overlayProps.find("rotation");
            if (iter != overlayProps.end()) {
                originalRotation = quatFromVariant(iter.value());
            } else {
                iter = overlayProps.find("localRotation");
                if (iter != overlayProps.end()) {
                    originalRotation = quatFromVariant(iter.value());
                } else if (!add) {
                    auto iter2 = savedRotations.find(id);
                    if (iter2 != savedRotations.end()) {
                        originalRotation = iter2.value();
                    }
                }
            }
        }

        if (!add) {
            savedRotations[id] = originalRotation;
        } else {
            rotationToSave = originalRotation;
        }

        glm::vec3 dimensions = ENTITY_ITEM_DEFAULT_DIMENSIONS;
        {
            auto iter = overlayProps.find("dimensions");
            if (iter != overlayProps.end()) {
                bool valid = false;
                dimensions = vec3FromVariant(iter.value(), valid);
                if (!valid) {
                    dimensions = glm::vec3(vec2FromVariant(iter.value()), 0.0f);
                }
            } else if (!add) {
                EntityPropertyFlags desiredProperties;
                desiredProperties += PROP_DIMENSIONS;
                dimensions = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id, desiredProperties).getDimensions();
            }
        }

        bool rotateX = dimensions.y < 0.0f;
        bool rotateY = dimensions.x < 0.0f;

        {
            glm::quat rotation = originalRotation;
            if (rotateX) {
                rotation = glm::angleAxis((float)M_PI, rotation * Vectors::RIGHT) * rotation;
            }
            if (rotateY) {
                rotation = glm::angleAxis((float)M_PI, rotation * Vectors::UP) * rotation;
            }

            overlayProps["localRotation"] = quatToVariant(rotation);
            overlayProps["dimensions"] = vec3toVariant(glm::abs(dimensions));
        }
    }

    QScriptEngine scriptEngine;
    QScriptValue props = variantMapToScriptValue(overlayProps, scriptEngine);
    EntityItemProperties toReturn;
    EntityItemPropertiesFromScriptValueHonorReadOnly(props, toReturn);
    return toReturn;
}

QVariantMap Overlays::convertEntityToOverlayProperties(const EntityItemProperties& properties) {
    QScriptEngine scriptEngine;
    QVariantMap overlayProps = EntityItemPropertiesToScriptValue(&scriptEngine, properties).toVariant().toMap();

    QString type = overlayProps["type"].toString();
    overlayProps["type"] = entityToOverlayType(type);

    if (type != "PolyLine") {
        RENAME_PROP(position, p1);
        RENAME_PROP(position, start);
    }
    RENAME_PROP(position, point);
    RENAME_PROP(dimensions, scale);
    RENAME_PROP(dimensions, size);
    RENAME_PROP(ignorePickIntersection, ignoreRayIntersection);

    {
        RENAME_PROP_CONVERT(primitiveMode, isSolid, [](const QVariant& v) { return v.toString() == "solid" ? true : false; });
        RENAME_PROP(isSolid, solid);
        RENAME_PROP(isSolid, isFilled);
        RENAME_PROP(isSolid, filled);

        RENAME_PROP_CONVERT(primitiveMode, isWire, [](const QVariant& v) { return v.toString() == "lines" ? true : false; });
        RENAME_PROP(isWire, wire);
    }

    RENAME_PROP_CONVERT(renderLayer, drawInFront, [](const QVariant& v) { return v.toString() == "front" ? true : false; });
    RENAME_PROP_CONVERT(renderLayer, drawHUDLayer, [](const QVariant& v) { return v.toString() == "hud" ? true : false; });

    GROUP_ENTITY_TO_OVERLAY_PROP(grab, grabbable, grabbable);

    GROUP_ENTITY_TO_OVERLAY_PROP(pulse, min, pulseMin);
    GROUP_ENTITY_TO_OVERLAY_PROP(pulse, max, pulseMax);
    GROUP_ENTITY_TO_OVERLAY_PROP(pulse, period, pulsePeriod);
    GROUP_ENTITY_TO_OVERLAY_PROP_CONVERT(pulse, colorMode, colorPulse, [](const QVariant& v) {
        QString f = v.toString();
        if (f == "in") {
            return 1.0f;
        } else if (f == "out") {
            return -1.0f;
        }
        return 0.0f;
    });
    GROUP_ENTITY_TO_OVERLAY_PROP_CONVERT(pulse, alphaMode, alphaPulse, [](const QVariant& v) {
        QString f = v.toString();
        if (f == "in") {
            return 1.0f;
        } else if (f == "out") {
            return -1.0f;
        }
        return 0.0f;
    });

    if (type == "Model") {
        RENAME_PROP(modelURL, url);
        RENAME_PROP(animation, animationSettings);
    } else if (type == "Image") {
        RENAME_PROP(imageURL, url);
    } else if (type == "Text") {
        RENAME_PROP(textColor, color);
    } else if (type == "Web") {
        RENAME_PROP(sourceUrl, url);
        RENAME_PROP_CONVERT(inputMode, inputMode, [](const QVariant& v) { return v.toString() == "mouse" ? "Mouse" : "Touch"; });
    } else if (type == "Gizmo") {
        RENAME_PROP_CONVERT(dimensions, outerRadius, [](const QVariant& v) { return 2.0f * vec3FromVariant(v).x; });
        RENAME_PROP(outerRadius, radius);

        RENAME_PROP_CONVERT(rotation, rotation, [](const QVariant& v) {
            glm::quat rot = quatFromVariant(v);
            return quatToVariant(glm::angleAxis((float)M_PI_2, rot * Vectors::RIGHT) * rot);
        });
        RENAME_PROP_CONVERT(localRotation, localRotation, [](const QVariant& v) {
            glm::quat rot = quatFromVariant(v);
            return quatToVariant(glm::angleAxis((float)M_PI_2, rot * Vectors::RIGHT) * rot);
        });

        GROUP_ENTITY_TO_OVERLAY_PROP(ring, startAngle, startAt);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, endAngle, endAt);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, innerRadius, innerRadius);

        GROUP_ENTITY_TO_OVERLAY_PROP(ring, innerStartColor, innerStartColor);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, innerEndColor, innerEndColor);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, outerStartColor, outerStartColor);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, outerEndColor, outerEndColor);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, innerStartAlpha, innerStartAlpha);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, innerEndAlpha, innerEndAlpha);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, outerStartAlpha, outerStartAlpha);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, outerEndAlpha, outerEndAlpha);

        GROUP_ENTITY_TO_OVERLAY_PROP(ring, hasTickMarks, hasTickMarks);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, majorTickMarksAngle, majorTickMarksAngle);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, minorTickMarksAngle, minorTickMarksAngle);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, majorTickMarksLength, majorTickMarksLength);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, minorTickMarksLength, minorTickMarksLength);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, majorTickMarksColor, majorTickMarksColor);
        GROUP_ENTITY_TO_OVERLAY_PROP(ring, minorTickMarksColor, minorTickMarksColor);
    } else if (type == "PolyLine") {
        QVector<glm::vec3> points = qVectorVec3FromScriptValue(scriptEngine.newVariant(overlayProps["linePoints"]));
        glm::vec3 position = vec3FromVariant(overlayProps["position"]);
        if (points.length() > 1) {
            overlayProps["p1"] = vec3toVariant(points[0] + position);
            overlayProps["p2"] = vec3toVariant(points[1] + position);

            overlayProps["localStart"] = vec3toVariant(points[0]);
            overlayProps["localEnd"] = vec3toVariant(points[1]);
        }

        RENAME_PROP(p1, startPoint);
        RENAME_PROP(p1, start);
        RENAME_PROP(p2, endPoint);
        RENAME_PROP(p2, end);

        QVector<float> widths = qVectorFloatFromScriptValue(scriptEngine.newVariant(overlayProps["strokeWidths"]));
        if (widths.length() > 0) {
            overlayProps["lineWidth"] = widths[0];
        }

        RENAME_PROP_CONVERT(glow, glow, [](const QVariant& v) { return v.toBool() ? 1.0f : 0.0f; });
    }

    // Do at the end, in case this type was rotated above
    RENAME_PROP(rotation, orientation);
    RENAME_PROP(localRotation, localOrientation);

    return overlayProps;
}

QUuid Overlays::addOverlay(const QString& type, const QVariant& properties) {
    if (_shuttingDown) {
        return UNKNOWN_ENTITY_ID;
    }

    if (QThread::currentThread() != thread()) {
        QUuid result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "addOverlay", Q_RETURN_ARG(QUuid, result), Q_ARG(const QString&, type), Q_ARG(const QVariant&, properties));
        return result;
    }

    Overlay::Pointer overlay;
    if (type == ImageOverlay::TYPE) {
#if !defined(DISABLE_QML)
        overlay = Overlay::Pointer(new ImageOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
#endif
    } else if (type == TextOverlay::TYPE) {
#if !defined(DISABLE_QML)
        overlay = Overlay::Pointer(new TextOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
#endif
    } else if (type == RectangleOverlay::TYPE) {
        overlay = Overlay::Pointer(new RectangleOverlay(), [](Overlay* ptr) { ptr->deleteLater(); });
    }

    if (overlay) {
        overlay->setProperties(properties.toMap());
        return add2DOverlay(overlay);
    }

    QString entityType = overlayToEntityType(type);
    if (entityType == "Unknown") {
        return UNKNOWN_ENTITY_ID;
    }

    QVariantMap propertyMap = properties.toMap();
    if (type == "rectangle3d") {
        propertyMap["shape"] = "Quad";
    }
    glm::quat rotationToSave;
    QUuid id = DependencyManager::get<EntityScriptingInterface>()->addEntityInternal(convertOverlayToEntityProperties(propertyMap, rotationToSave, entityType, true), entity::HostType::LOCAL);
    if (entityType == "Text" || entityType == "Image" || entityType == "Grid" || entityType == "Web") {
        savedRotations[id] = rotationToSave;
    }
    return id;
}

QUuid Overlays::add2DOverlay(const Overlay::Pointer& overlay) {
    if (_shuttingDown) {
        return UNKNOWN_ENTITY_ID;
    }

    QUuid thisID = QUuid::createUuid();
    overlay->setID(thisID);
    overlay->setStackOrder(_stackOrder++);
    {
        QMutexLocker locker(&_mutex);
        _overlays[thisID] = overlay;
    }

    return thisID;
}

QUuid Overlays::cloneOverlay(const QUuid& id) {
    if (_shuttingDown) {
        return UNKNOWN_ENTITY_ID;
    }

    if (QThread::currentThread() != thread()) {
        QUuid result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "cloneOverlay", Q_RETURN_ARG(QUuid, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return add2DOverlay(Overlay::Pointer(overlay->createClone(), [](Overlay* ptr) { ptr->deleteLater(); }));
    }

    return DependencyManager::get<EntityScriptingInterface>()->cloneEntity(id);
}

bool Overlays::editOverlay(const QUuid& id, const QVariant& properties) {
    if (_shuttingDown) {
        return false;
    }

    auto overlay = get2DOverlay(id);
    if (overlay) {
        if (QThread::currentThread() != thread()) {
            // NOTE editOverlay can be called very frequently in scripts and can't afford to
            // block waiting on the main thread.  Additionally, no script actually
            // examines the return value and does something useful with it, so use a non-blocking
            // invoke and just always return true
            QMetaObject::invokeMethod(this, "editOverlay", Q_ARG(const QUuid&, id), Q_ARG(const QVariant&, properties));
            return true;
        }

        overlay->setProperties(properties.toMap());
        return true;
    }

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    auto propertyMap = properties.toMap();
    EntityItemProperties entityProperties = convertOverlayToEntityProperties(propertyMap, entityScriptingInterface->getEntityType(id), false, id);
    return !entityScriptingInterface->editEntity(id, entityProperties).isNull();
}

bool Overlays::editOverlays(const QVariant& propertiesById) {
    if (_shuttingDown) {
        return false;
    }

    bool deferOverlays = QThread::currentThread() != thread();

    QVariantMap deferred;
    const QVariantMap map = propertiesById.toMap();
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    for (const auto& key : map.keys()) {
        QUuid id = QUuid(key);
        const QVariant& properties = map[key];

        Overlay::Pointer overlay = get2DOverlay(id);
        if (overlay) {
            if (deferOverlays) {
                deferred[key] = properties;
                continue;
            }
            overlay->setProperties(properties.toMap());
        } else {
            auto propertyMap = properties.toMap();
            entityScriptingInterface->editEntity(id, convertOverlayToEntityProperties(propertyMap, entityScriptingInterface->getEntityType(id), false, id));
        }
    }

    // For 2D/QML overlays, we need to perform the edit on the main thread
    if (!deferred.empty()) {
        // NOTE see comment on editOverlay for why this is not a blocking call
        QMetaObject::invokeMethod(this, "editOverlays", Q_ARG(const QVariant&, deferred));
    }

    return true;
}

void Overlays::deleteOverlay(const QUuid& id) {
    if (_shuttingDown) {
        return;
    }

    Overlay::Pointer overlay = take2DOverlay(id);
    if (overlay) {
        _overlaysToDelete.push_back(overlay);
        emit overlayDeleted(id);
        return;
    }

    DependencyManager::get<EntityScriptingInterface>()->deleteEntity(id);
    emit overlayDeleted(id);
}

QString Overlays::getOverlayType(const QUuid& id) {
    if (_shuttingDown) {
        return "";
    }

    if (QThread::currentThread() != thread()) {
        QString result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "getOverlayType", Q_RETURN_ARG(QString, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return overlay->getType();
    }

    return entityToOverlayType(DependencyManager::get<EntityScriptingInterface>()->getEntityType(id));
}

QObject* Overlays::getOverlayObject(const QUuid& id) {
    if (QThread::currentThread() != thread()) {
        QObject* result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "getOverlayObject", Q_RETURN_ARG(QObject*, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return qobject_cast<QObject*>(&(*overlay));
    }

    return DependencyManager::get<EntityScriptingInterface>()->getEntityObject(id);
}

QUuid Overlays::getOverlayAtPoint(const glm::vec2& point) {
    if (_shuttingDown || !_enabled) {
        return UNKNOWN_ENTITY_ID;
    }

    QMutexLocker locker(&_mutex);
    QMapIterator<QUuid, Overlay::Pointer> i(_overlays);
    unsigned int bestStackOrder = 0;
    QUuid bestID = UNKNOWN_ENTITY_ID;
    while (i.hasNext()) {
        i.next();
        auto thisOverlay = std::dynamic_pointer_cast<Overlay2D>(i.value());
        if (thisOverlay && thisOverlay->getVisible() && thisOverlay->isLoaded() &&
            thisOverlay->getBoundingRect().contains(point.x, point.y, false)) {
            if (thisOverlay->getStackOrder() > bestStackOrder) {
                bestID = i.key();
                bestStackOrder = thisOverlay->getStackOrder();
            }
        }
    }

    return bestID;
}

QVariant Overlays::getProperty(const QUuid& id, const QString& property) {
    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        if (overlay->supportsGetProperty()) {
            return overlay->getProperty(property);
        }
        return QVariant();
    }

    QVariantMap overlayProperties = convertEntityToOverlayProperties(DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id));
    auto propIter = overlayProperties.find(property);
    if (propIter != overlayProperties.end()) {
        return propIter.value();
    }
    return QVariant();
}

QVariantMap Overlays::getProperties(const QUuid& id, const QStringList& properties) {
    Overlay::Pointer overlay = get2DOverlay(id);
    QVariantMap result;
    if (overlay) {
        if (overlay->supportsGetProperty()) {
            for (const auto& property : properties) {
                result.insert(property, overlay->getProperty(property));
            }
        }
        return result;
    }

    QVariantMap overlayProperties = convertEntityToOverlayProperties(DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id));
    for (const auto& property : properties) {
        auto propIter = overlayProperties.find(property);
        if (propIter != overlayProperties.end()) {
            result.insert(property, propIter.value());
        }
    }
    return result;
}

QVariantMap Overlays::getOverlaysProperties(const QVariant& propertiesById) {
    QVariantMap map = propertiesById.toMap();
    QVariantMap result;
    for (const auto& key : map.keys()) {
        result[key] = getProperties(QUuid(key), map[key].toStringList());
    }
    return result;
}

RayToOverlayIntersectionResult Overlays::findRayIntersection(const PickRay& ray, bool precisionPicking,
                                                             const QScriptValue& overlayIDsToInclude,
                                                             const QScriptValue& overlayIDsToDiscard,
                                                             bool visibleOnly, bool collidableOnly) {
    const QVector<EntityItemID> include = qVectorEntityItemIDFromScriptValue(overlayIDsToInclude);
    const QVector<EntityItemID> discard = qVectorEntityItemIDFromScriptValue(overlayIDsToDiscard);

    return findRayIntersectionVector(ray, precisionPicking, include, discard, visibleOnly, collidableOnly);
}

RayToOverlayIntersectionResult Overlays::findRayIntersectionVector(const PickRay& ray, bool precisionPicking,
                                                                   const QVector<EntityItemID>& include,
                                                                   const QVector<EntityItemID>& discard,
                                                                   bool visibleOnly, bool collidableOnly) {
    unsigned int searchFilter = PickFilter::getBitMask(PickFilter::FlagBit::LOCAL_ENTITIES);

    if (!precisionPicking) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COARSE);
    }

    if (visibleOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::VISIBLE);
    }

    if (collidableOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COLLIDABLE);
    }
    auto result = DependencyManager::get<EntityScriptingInterface>()->evalRayIntersectionVector(ray, PickFilter(searchFilter), include, discard);

    RayToOverlayIntersectionResult overlayResult;
    overlayResult.overlayID = result.entityID;
    overlayResult.intersects = result.intersects;
    overlayResult.intersection = result.intersection;
    overlayResult.distance = result.distance;
    overlayResult.surfaceNormal = result.surfaceNormal;
    overlayResult.face = result.face;
    overlayResult.extraInfo = result.extraInfo;
    return overlayResult;
}

ParabolaToOverlayIntersectionResult Overlays::findParabolaIntersectionVector(const PickParabola& parabola, bool precisionPicking,
                                                                             const QVector<EntityItemID>& include,
                                                                             const QVector<EntityItemID>& discard,
                                                                             bool visibleOnly, bool collidableOnly) {
    unsigned int searchFilter = PickFilter::getBitMask(PickFilter::FlagBit::LOCAL_ENTITIES);

    if (!precisionPicking) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COARSE);
    }

    if (visibleOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::VISIBLE);
    }

    if (collidableOnly) {
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::COLLIDABLE);
    }
    auto result = DependencyManager::get<EntityScriptingInterface>()->evalParabolaIntersectionVector(parabola, PickFilter(searchFilter), include, discard);

    ParabolaToOverlayIntersectionResult overlayResult;
    overlayResult.overlayID = result.entityID;
    overlayResult.intersects = result.intersects;
    overlayResult.intersection = result.intersection;
    overlayResult.parabolicDistance = result.parabolicDistance;
    overlayResult.surfaceNormal = result.surfaceNormal;
    overlayResult.face = result.face;
    overlayResult.extraInfo = result.extraInfo;
    return overlayResult;
}

QScriptValue RayToOverlayIntersectionResultToScriptValue(QScriptEngine* engine, const RayToOverlayIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    QScriptValue overlayIDValue = quuidToScriptValue(engine, value.overlayID);
    obj.setProperty("overlayID", overlayIDValue);
    obj.setProperty("distance", value.distance);
    obj.setProperty("face", boxFaceToString(value.face));

    QScriptValue intersection = vec3ToScriptValue(engine, value.intersection);
    obj.setProperty("intersection", intersection);
    QScriptValue surfaceNormal = vec3ToScriptValue(engine, value.surfaceNormal);
    obj.setProperty("surfaceNormal", surfaceNormal);
    obj.setProperty("extraInfo", engine->toScriptValue(value.extraInfo));
    return obj;
}

void RayToOverlayIntersectionResultFromScriptValue(const QScriptValue& object, RayToOverlayIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    QScriptValue overlayIDValue = object.property("overlayID");
    quuidFromScriptValue(overlayIDValue, value.overlayID);
    value.distance = object.property("distance").toVariant().toFloat();
    value.face = boxFaceFromString(object.property("face").toVariant().toString());

    QScriptValue intersection = object.property("intersection");
    if (intersection.isValid()) {
        vec3FromScriptValue(intersection, value.intersection);
    }
    QScriptValue surfaceNormal = object.property("surfaceNormal");
    if (surfaceNormal.isValid()) {
        vec3FromScriptValue(surfaceNormal, value.surfaceNormal);
    }
    value.extraInfo = object.property("extraInfo").toVariant().toMap();
}

bool Overlays::isLoaded(const QUuid& id) {
    if (QThread::currentThread() != thread()) {
        bool result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "isLoaded", Q_RETURN_ARG(bool, result), Q_ARG(const QUuid&, id));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return overlay->isLoaded();
    }

    return DependencyManager::get<EntityScriptingInterface>()->isLoaded(id);
}

QSizeF Overlays::textSize(const QUuid& id, const QString& text) {
    if (QThread::currentThread() != thread()) {
        QSizeF result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "textSize", Q_RETURN_ARG(QSizeF, result), Q_ARG(const QUuid&, id), Q_ARG(QString, text));
        return result;
    }

    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        if (auto textOverlay = std::dynamic_pointer_cast<TextOverlay>(overlay)) {
            return textOverlay->textSize(text);
        }
        return QSizeF(0.0f, 0.0f);
    } else {
        return DependencyManager::get<EntityScriptingInterface>()->textSize(id, text);
    }
}

bool Overlays::isAddedOverlay(const QUuid& id) {
    Overlay::Pointer overlay = get2DOverlay(id);
    if (overlay) {
        return true;
    }

    return DependencyManager::get<EntityScriptingInterface>()->isAddedEntity(id);
}

void Overlays::sendMousePressOnOverlay(const QUuid& id, const PointerEvent& event) {
    mousePressPointerEvent(id, event);
}

void Overlays::sendMouseReleaseOnOverlay(const QUuid& id, const PointerEvent& event) {
    mouseReleasePointerEvent(id, event);
}

void Overlays::sendMouseMoveOnOverlay(const QUuid& id, const PointerEvent& event) {
    mouseMovePointerEvent(id, event);
}

void Overlays::sendHoverEnterOverlay(const QUuid& id, const PointerEvent& event) {
    hoverEnterPointerEvent(id, event);
}

void Overlays::sendHoverOverOverlay(const QUuid& id, const PointerEvent& event) {
    hoverOverPointerEvent(id, event);
}

void Overlays::sendHoverLeaveOverlay(const QUuid& id, const PointerEvent& event) {
    hoverLeavePointerEvent(id, event);
}

float Overlays::width() {
    if (QThread::currentThread() != thread()) {
        float result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "width", Q_RETURN_ARG(float, result));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().width();
}

float Overlays::height() {
    if (QThread::currentThread() != thread()) {
        float result;
        PROFILE_RANGE(script, __FUNCTION__);
        BLOCKING_INVOKE_METHOD(this, "height", Q_RETURN_ARG(float, result));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getWindow()->size().height();
}

static uint32_t toPointerButtons(const QMouseEvent& event) {
    uint32_t buttons = 0;
    buttons |= event.buttons().testFlag(Qt::LeftButton) ? PointerEvent::PrimaryButton : 0;
    buttons |= event.buttons().testFlag(Qt::RightButton) ? PointerEvent::SecondaryButton : 0;
    buttons |= event.buttons().testFlag(Qt::MiddleButton) ? PointerEvent::TertiaryButton : 0;
    return buttons;
}

static PointerEvent::Button toPointerButton(const QMouseEvent& event) {
    switch (event.button()) {
        case Qt::LeftButton:
            return PointerEvent::PrimaryButton;
        case Qt::RightButton:
            return PointerEvent::SecondaryButton;
        case Qt::MiddleButton:
            return PointerEvent::TertiaryButton;
        default:
            return PointerEvent::NoButtons;
    }
}

RayToOverlayIntersectionResult getPrevPickResult() {
    RayToOverlayIntersectionResult overlayResult;
    overlayResult.intersects = false;
    auto pickResult = DependencyManager::get<PickManager>()->getPrevPickResultTyped<RayPickResult>(DependencyManager::get<EntityTreeRenderer>()->getMouseRayPickID());
    if (pickResult) {
        overlayResult.intersects = pickResult->type == IntersectionType::LOCAL_ENTITY;
        if (overlayResult.intersects) {
            overlayResult.intersection = pickResult->intersection;
            overlayResult.distance = pickResult->distance;
            overlayResult.surfaceNormal = pickResult->surfaceNormal;
            overlayResult.overlayID = pickResult->objectID;
            overlayResult.extraInfo = pickResult->extraInfo;
        }
    }
    return overlayResult;
}

PointerEvent Overlays::calculateOverlayPointerEvent(const QUuid& id, const PickRay& ray,
                                                    const RayToOverlayIntersectionResult& rayPickResult, QMouseEvent* event,
                                                    PointerEvent::EventType eventType) {
    glm::vec2 pos2D = RayPick::projectOntoEntityXYPlane(id, rayPickResult.intersection);
    return PointerEvent(eventType, PointerManager::MOUSE_POINTER_ID, pos2D, rayPickResult.intersection, rayPickResult.surfaceNormal,
                        ray.direction, toPointerButton(*event), toPointerButtons(*event), event->modifiers());
}

void Overlays::hoverEnterPointerEvent(const QUuid& id, const PointerEvent& event) {
    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeyIDs().contains(id)) {
        emit hoverEnterOverlay(id, event);
    }
}

void Overlays::hoverOverPointerEvent(const QUuid& id, const PointerEvent& event) {
    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeyIDs().contains(id)) {
        emit hoverOverOverlay(id, event);
    }
}

void Overlays::hoverLeavePointerEvent(const QUuid& id, const PointerEvent& event) {
    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeyIDs().contains(id)) {
        emit hoverLeaveOverlay(id, event);
    }
}

std::pair<float, QUuid> Overlays::mousePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mousePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = getPrevPickResult();
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        PointerEvent pointerEvent = calculateOverlayPointerEvent(_currentClickingOnOverlayID, ray, rayPickResult, event, PointerEvent::Press);
        mousePressPointerEvent(_currentClickingOnOverlayID, pointerEvent);
        return { rayPickResult.distance, rayPickResult.overlayID };
    }
    emit mousePressOffOverlay();
    return { FLT_MAX, UNKNOWN_ENTITY_ID };
}

void Overlays::mousePressPointerEvent(const QUuid& id, const PointerEvent& event) {
    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeyIDs().contains(id)) {
        emit mousePressOnOverlay(id, event);
    }
}

bool Overlays::mouseDoublePressEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseDoublePressEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = getPrevPickResult();
    if (rayPickResult.intersects) {
        _currentClickingOnOverlayID = rayPickResult.overlayID;

        auto pointerEvent = calculateOverlayPointerEvent(_currentClickingOnOverlayID, ray, rayPickResult, event, PointerEvent::Press);
        emit mouseDoublePressOnOverlay(_currentClickingOnOverlayID, pointerEvent);
        return true;
    }
    emit mouseDoublePressOffOverlay();
    return false;
}

bool Overlays::mouseReleaseEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseReleaseEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = getPrevPickResult();
    if (rayPickResult.intersects) {
        auto pointerEvent = calculateOverlayPointerEvent(rayPickResult.overlayID, ray, rayPickResult, event, PointerEvent::Release);
        mouseReleasePointerEvent(rayPickResult.overlayID, pointerEvent);
    }

    _currentClickingOnOverlayID = UNKNOWN_ENTITY_ID;
    return false;
}

void Overlays::mouseReleasePointerEvent(const QUuid& id, const PointerEvent& event) {
    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeyIDs().contains(id)) {
        emit mouseReleaseOnOverlay(id, event);
    }
}

bool Overlays::mouseMoveEvent(QMouseEvent* event) {
    PerformanceTimer perfTimer("Overlays::mouseMoveEvent");

    PickRay ray = qApp->computePickRay(event->x(), event->y());
    RayToOverlayIntersectionResult rayPickResult = getPrevPickResult();
    if (rayPickResult.intersects) {
        auto pointerEvent = calculateOverlayPointerEvent(rayPickResult.overlayID, ray, rayPickResult, event, PointerEvent::Move);
        mouseMovePointerEvent(rayPickResult.overlayID, pointerEvent);

        // If previously hovering over a different overlay then leave hover on that overlay.
        if (_currentHoverOverOverlayID != UNKNOWN_ENTITY_ID && rayPickResult.overlayID != _currentHoverOverOverlayID) {
            auto pointerEvent = calculateOverlayPointerEvent(_currentHoverOverOverlayID, ray, rayPickResult, event, PointerEvent::Move);
            hoverLeavePointerEvent(_currentHoverOverOverlayID, pointerEvent);
        }

        // If hovering over a new overlay then enter hover on that overlay.
        if (rayPickResult.overlayID != _currentHoverOverOverlayID) {
            hoverEnterPointerEvent(rayPickResult.overlayID, pointerEvent);
        }

        // Hover over current overlay.
        hoverOverPointerEvent(rayPickResult.overlayID, pointerEvent);

        _currentHoverOverOverlayID = rayPickResult.overlayID;
    } else {
        // If previously hovering an overlay then leave hover.
        if (_currentHoverOverOverlayID != UNKNOWN_ENTITY_ID) {
            auto pointerEvent = calculateOverlayPointerEvent(_currentHoverOverOverlayID, ray, rayPickResult, event, PointerEvent::Move);
            hoverLeavePointerEvent(_currentHoverOverOverlayID, pointerEvent);

            _currentHoverOverOverlayID = UNKNOWN_ENTITY_ID;
        }
    }
    return false;
}

void Overlays::mouseMovePointerEvent(const QUuid& id, const PointerEvent& event) {
    auto keyboard = DependencyManager::get<Keyboard>();
    // Do not send keyboard key event to scripts to prevent malignant scripts from gathering what you typed
    if (!keyboard->getKeyIDs().contains(id)) {
        emit mouseMoveOnOverlay(id, event);
    }
}

QVector<QUuid> Overlays::findOverlays(const glm::vec3& center, float radius) {
    PROFILE_RANGE(script_entities, __FUNCTION__);

    QVector<QUuid> result;
    auto entityTree = DependencyManager::get<EntityScriptingInterface>()->getEntityTree();
    if (entityTree) {
        unsigned int searchFilter = PickFilter::getBitMask(PickFilter::FlagBit::LOCAL_ENTITIES);
        // For legacy reasons, this only finds visible objects
        searchFilter = searchFilter | PickFilter::getBitMask(PickFilter::FlagBit::VISIBLE);
        entityTree->withReadLock([&] {
            entityTree->evalEntitiesInSphere(center, radius, PickFilter(searchFilter), result);
        });
    }
    return result;
}

/**jsdoc
 * <p>An overlay may be one of the following types:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>2D/3D</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>image</code></td><td>2D</td><td>An image. Synonym: <code>billboard</code>.</td></tr>
 *     <tr><td><code>rectangle</code></td><td>2D</td><td>A rectangle.</td></tr>
 *     <tr><td><code>text</code></td><td>2D</td><td>Text.</td></tr>
 *     <tr><td><code>cube</code></td><td>3D</td><td>A cube. Can also use a <code>shape</code> overlay to create a cube.</td></tr>
 *     <tr><td><code>sphere</code></td><td>3D</td><td>A sphere. Can also use a <code>shape</code> overlay to create a sphere.</td></tr>
 *     <tr><td><code>rectangle3d</code></td><td>3D</td><td>A rectangle.</td></tr>
 *     <tr><td><code>shape</code></td><td>3D</td><td>A geometric shape, such as a cube, sphere, or cylinder.</td></tr>
 *     <tr><td><code>model</code></td><td>3D</td><td>A model.</td></tr>
 *     <tr><td><code>text3d</code></td><td>3D</td><td>Text.</td></tr>
 *     <tr><td><code>image3d</code></td><td>3D</td><td>An image.</td></tr>
 *     <tr><td><code>web3d</code></td><td>3D</td><td>Web content.</td></tr>
 *     <tr><td><code>line3d</code></td><td>3D</td><td>A line.</td></tr>
 *     <tr><td><code>grid</code></td><td>3D</td><td>A grid of lines in a plane.</td></tr>
 *     <tr><td><code>circle3d</code></td><td>3D</td><td>A circle.</td></tr>
 *   </tbody>
 * </table>
 * <p>2D overlays are rendered on the display surface in desktop mode and on the HUD surface in HMD mode. 3D overlays are
 * rendered at a position and orientation in-world, but are deprecated (use local entities instead).<p>
 * <p>Each overlay type has different {@link Overlays.OverlayProperties|OverlayProperties}.</p>
 * @typedef {string} Overlays.OverlayType
 */

/**jsdoc
 * Different overlay types have different properties: some common to all overlays (listed below) and some specific to each
 * {@link Overlays.OverlayType|OverlayType} (linked to below). The properties are accessed as an object of property names and
 * values.  3D Overlays are local entities, internally, so they also support the corresponding entity properties.
 *
 * @typedef {object} Overlays.OverlayProperties
 * @property {Uuid} id - The ID of the overlay. <em>Read-only.</em>
 * @property {Overlays.OverlayType} type - The overlay type. <em>Read-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @see The different entity types have additional properties as follows:
 * @see {@link Overlays.OverlayProperties-Image|OverlayProperties-Image}
 * @see {@link Overlays.OverlayProperties-Text|OverlayProperties-Text}
 * @see {@link Overlays.OverlayProperties-Rectangle|OverlayProperties-Rectangle}
 * @see {@link Overlays.OverlayProperties-Cube|OverlayProperties-Cube}
 * @see {@link Overlays.OverlayProperties-Sphere|OverlayProperties-Sphere}
 * @see {@link Overlays.OverlayProperties-Rectangle3D|OverlayProperties-Rectangle3D}
 * @see {@link Overlays.OverlayProperties-Shape|OverlayProperties-Shape}
 * @see {@link Overlays.OverlayProperties-Model|OverlayProperties-Model}
 * @see {@link Overlays.OverlayProperties-Text3D|OverlayProperties-Text3D}
 * @see {@link Overlays.OverlayProperties-Image3D|OverlayProperties-Image3D}
 * @see {@link Overlays.OverlayProperties-Web|OverlayProperties-Web}
 * @see {@link Overlays.OverlayProperties-Line|OverlayProperties-Line}
 * @see {@link Overlays.OverlayProperties-Grid|OverlayProperties-Grid}
 * @see {@link Overlays.OverlayProperties-Circle|OverlayProperties-Circle}
 */

/**jsdoc
 * The <code>"Image"</code> {@link Overlays.OverlayType|OverlayType} is a 2D image.
 * @typedef {object} Overlays.OverlayProperties-Image
 * @property {Rect} bounds - The position and size of the image display area, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value of the image display area = <code>bounds.x</code>.
 *     <em>Write-only.</em>
 *  @property {number} y - Integer top, y-coordinate value of the image display area = <code>bounds.y</code>.
 *     <em>Write-only.</em>
 * @property {number} width - Integer width of the image display area = <code>bounds.width</code>. <em>Write-only.</em>
 *  @property {number} height - Integer height of the image display area = <code>bounds.height</code>. <em>Write-only.</em>
 * @property {string} imageURL - The URL of the image file to display. The image is scaled to fit to the <code>bounds</code>.
 *     <em>Write-only.</em>
 *  @property {Vec2} subImage=0,0 - Integer coordinates of the top left pixel to start using image content from.
 *     <em>Write-only.</em>
 * @property {Color} color=0,0,0 - The color to apply over the top of the image to colorize it. <em>Write-only.</em>
 *  @property {number} alpha=0.0 - The opacity of the color applied over the top of the image, <code>0.0</code> -
 *     <code>1.0</code>. <em>Write-only.</em>
 */

/**jsdoc
 * The <code>"Text"</code> {@link Overlays.OverlayType|OverlayType} is for 2D text.
 * @typedef {object} Overlays.OverlayProperties-Text
 * @property {Rect} bounds - The position and size of the rectangle, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 *
 * @property {number} margin=0 - Sets the <code>leftMargin</code> and <code>topMargin</code> values, in pixels.
 *     <em>Write-only.</em>
 * @property {number} leftMargin=0 - The left margin's size, in pixels. This value is also used for the right margin.
 *     <em>Write-only.</em>
 * @property {number} topMargin=0 - The top margin's size, in pixels. This value is also used for the bottom margin.
 *     <em>Write-only.</em>
 * @property {string} text="" - The text to display. Text does not automatically wrap; use <code>\n</code> for a line break. Text
 *     is clipped to the <code>bounds</code>. <em>Write-only.</em>
 * @property {number} font.size=18 - The size of the text, in pixels. <em>Write-only.</em>
 * @property {number} lineHeight=18 - The height of a line of text, in pixels. <em>Write-only.</em>
 * @property {Color} color=255,255,255 - The color of the text. Synonym: <code>textColor</code>. <em>Write-only.</em>
 * @property {number} alpha=1.0 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. <em>Write-only.</em>
 * @property {Color} backgroundColor=0,0,0 - The color of the background rectangle. <em>Write-only.</em>
 * @property {number} backgroundAlpha=0.7 - The opacity of the background rectangle. <em>Write-only.</em>
 */

/**jsdoc
 * The <code>"Rectangle"</code> {@link Overlays.OverlayType|OverlayType} is for 2D rectangles.
 * @typedef {object} Overlays.OverlayProperties-Rectangle
 * @property {Rect} bounds - The position and size of the rectangle, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 *
 * @property {Color} color=0,0,0 - The color of the overlay. <em>Write-only.</em>
 * @property {number} alpha=1.0 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. <em>Write-only.</em>
 * @property {number} borderWidth=1 - Integer width of the border, in pixels. The border is drawn within the rectangle's bounds.
 *     It is not drawn unless either <code>borderColor</code> or <code>borderAlpha</code> are specified. <em>Write-only.</em>
 * @property {number} radius=0 - Integer corner radius, in pixels. <em>Write-only.</em>
 * @property {Color} borderColor=0,0,0 - The color of the border. <em>Write-only.</em>
 * @property {number} borderAlpha=1.0 - The opacity of the border, <code>0.0</code> - <code>1.0</code>.
 *     <em>Write-only.</em>
 */

/**jsdoc
 * The <code>"Cube"</code> {@link Overlays.OverlayType|OverlayType} is for 3D cubes.
 * @typedef {object} Overlays.OverlayProperties-Cube
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 */

/**jsdoc
 * The <code>"Sphere"</code> {@link Overlays.OverlayType|OverlayType} is for 3D spheres.
 * @typedef {object} Overlays.OverlayProperties-Sphere
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 */

/**jsdoc
 * The <code>"Rectangle3D"</code> {@link Overlays.OverlayType|OverlayType} is for 3D rectangles.
 * @typedef {object} Overlays.OverlayProperties-Rectangle3D
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 */

/**jsdoc
 * <p>A <code>shape</code> {@link Overlays.OverlayType|OverlayType} may display as one of the following geometrical shapes:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Dimensions</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"Circle"</code></td><td>2D</td><td>A circle oriented in 3D.</td></td></tr>
 *     <tr><td><code>"Cone"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Cube"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Cylinder"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Dodecahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Hexagon"</code></td><td>3D</td><td>A hexagonal prism.</td></tr>
 *     <tr><td><code>"Icosahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Line"</code></td><td>1D</td><td>A line oriented in 3D.</td></tr>
 *     <tr><td><code>"Octagon"</code></td><td>3D</td><td>An octagonal prism.</td></tr>
 *     <tr><td><code>"Octahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Quad"</code></td><td>2D</td><td>A square oriented in 3D.</tr>
 *     <tr><td><code>"Sphere"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Tetrahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Torus"</code></td><td>3D</td><td><em>Not implemented.</em></td></tr>
 *     <tr><td><code>"Triangle"</code></td><td>3D</td><td>A triangular prism.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Overlays.Shape
 */

/**jsdoc
 * The <code>"Shape"</code> {@link Overlays.OverlayType|OverlayType} is for 3D shapes.
 * @typedef {object} Overlays.OverlayProperties-Shape
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Overlays.Shape} shape=Hexagon - The geometrical shape of the overlay.
 */

/**jsdoc
 * The <code>"Model"</code> {@link Overlays.OverlayType|OverlayType} is for 3D models.
 * @typedef {object} Overlays.OverlayProperties-Model
 * @property {string} name - The name of the overlay.
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {string} url - The URL of the FBX or OBJ model used for the overlay.
 * @property {number} loadPriority=0.0 - The priority for loading and displaying the overlay. Overlays with higher values load
 *     first. <CURRENTLY NOT USED>
 * @property {object.<name, url>} textures - Maps the named textures in the model to the JPG or PNG images in the urls.
 * @property {string[]} jointNames - The names of the joints - if any - in the model. <em>Read-only.</em>
 * @property {Quat[]} jointRotations - The relative rotations of the model's joints.
 * @property {Vec3[]} jointTranslations - The relative translations of the model's joints.
 * @property {Quat[]} jointOrientations - The absolute orientations of the model's joints, in world coordinates. <em>Read-only.</em>
 * @property {Vec3[]} jointPositions - The absolute positions of the model's joints, in world coordinates. <em>Read-only.</em>
 * @property {string} animationSettings.url="" - The URL of an FBX file containing an animation to play.
 * @property {number} animationSettings.fps=0 - The frame rate (frames/sec) to play the animation at.
 * @property {number} animationSettings.firstFrame=0 - The frame to start playing at.
 * @property {number} animationSettings.lastFrame=0 - The frame to finish playing at.
 * @property {number} animationSettings.currentFrame=0 - The current frame being played.
 * @property {boolean} animationSettings.running=false - Whether or not the animation is playing.
 * @property {boolean} animationSettings.loop=false - Whether or not the animation should repeat in a loop.
 * @property {boolean} animationSettings.hold=false - Whether or not when the animation finishes, the rotations and
 *     translations of the last frame played should be maintained.
 * @property {boolean} animationSettings.allowTranslation=false - Whether or not translations contained in the animation should
 *     be played.
 */

/**jsdoc
 * The <code>"Text3D"</code> {@link Overlays.OverlayType|OverlayType} is for 3D text.
 * @typedef {object} Overlays.OverlayProperties-Text3D
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {boolean} isFacingAvatar - If <code>true< / code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 * @property {string} text="" - The text to display.Text does not automatically wrap; use <code>\n< / code> for a line break.
 * @property {number} textAlpha=1 - The text alpha value.
 * @property {Color} backgroundColor=0,0,0 - The background color.
 * @property {number} backgroundAlpha=0.7 - The background alpha value.
 * @property {number} lineHeight=1 - The height of a line of text in meters.
 * @property {number} leftMargin=0.1 - The left margin, in meters.
 * @property {number} topMargin=0.1 - The top margin, in meters.
 * @property {number} rightMargin=0.1 - The right margin, in meters.
 * @property {number} bottomMargin=0.1 - The bottom margin, in meters.
 */

/**jsdoc
 * The <code>"Image3D"</code> {@link Overlays.OverlayType|OverlayType} is for 3D images.
 * @typedef {object} Overlays.OverlayProperties-Image3D
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 * @property {string} url - The URL of the PNG or JPG image to display.
 * @property {Rect} subImage - The portion of the image to display. Defaults to the full image.
 * @property {boolean} emissive - If <code>true</code>, the overlay is displayed at full brightness, otherwise it is rendered
 *     with scene lighting.
 * @property {bool} keepAspectRatio=true - overlays will maintain the aspect ratio when the subImage is applied.
 */

/**jsdoc
 * The <code>"Web"</code> {@link Overlays.OverlayType|OverlayType} is for 3D web surfaces.
 * @typedef {object} Overlays.OverlayProperties-Web
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 * @property {string} url - The URL of the Web page to display.
 * @property {string} scriptURL="" - The URL of a JavaScript file to inject into the Web page.
 * @property {number} dpi=30 - The dots per inch to display the Web page at, on the overlay.
 * @property {number} maxFPS=10 - The maximum update rate for the Web overlay content, in frames/second.
 * @property {string} inputMode=Touch - The user input mode to use - either <code>"Touch"</code> or <code>"Mouse"</code>.
 */

/**jsdoc
 * The <code>"Line"</code> {@link Overlays.OverlayType|OverlayType} is for 3D lines.
 * @typedef {object} Overlays.OverlayProperties-Line
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Uuid} endParentID=null - The avatar, entity, or overlay that the end point of the line is parented to.
 * @property {number} endParentJointIndex=65535 - Integer value specifying the skeleton joint that the end point of the line is
 *     attached to if <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint". <CURRENTLY BROKEN>
 * @property {Vec3} start - The start point of the line. Synonyms: <code>startPoint</code> and <code>p1</code>.
 * @property {Vec3} end - The end point of the line. Synonyms: <code>endPoint</code> and <code>p2</code>.
 * @property {Vec3} localStart - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>start</code>. Synonym: <code>localPosition</code>.
 * @property {Vec3} localEnd - The local position of the overlay relative to its parent if the overlay has a
 *     <code>endParentID</code> set, otherwise the same value as <code>end</code>. <CURRENTLY BROKEN>
 * @property {number} length - The length of the line, in meters. This can be set after creating a line with start and end
 *     points. <CURRENTLY BROKEN>
 * @property {number} glow=0 - If <code>glow > 0</code>, the line is rendered with a glow.
 * @property {number} lineWidth=0.02 - Width of the line, in meters.
 */

/**jsdoc
 * The <code>"Grid"</code> {@link Overlays.OverlayType|OverlayType} is for 3D grid.
 * @typedef {object} Overlays.OverlayProperties-Grid
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {boolean} followCamera=true - If <code>true</code>, the grid is always visible even as the camera moves to another position.
 * @property {number} majorGridEvery=5 - Integer number of <code>minorGridEvery</code> intervals at which to draw a thick grid line. Minimum value = <code>1</code>.
 * @property {number} minorGridEvery=1 - Real number of meters at which to draw thin grid lines. Minimum value = <code>0.001</code>.
 */

/**jsdoc
 * The <code>"Circle"</code> {@link Overlays.OverlayType|OverlayType} is for 3D circle.
 * @typedef {object} Overlays.OverlayProperties-Circle
 * @property {string} name - The name of the overlay.
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 *
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.  Synonym: <code>localOrientation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} ignorePickIntersection=false - If <code>true</code>, picks ignore the overlay.  <code>ignoreRayIntersection</code> is a synonym.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of objects in the world, but behind the HUD.
 * @property {boolean} drawHUDLayer=false - If <code>true</code>, the overlay is rendered in front of everything, including the HUD.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {number} startAt = 0 - The counter - clockwise angle from the overlay's x-axis that drawing starts at, in degrees.
 * @property {number} endAt = 360 - The counter - clockwise angle from the overlay's x-axis that drawing ends at, in degrees.
 * @property {number} outerRadius = 1 - The outer radius of the overlay, in meters.Synonym: <code>radius< / code>.
 * @property {number} innerRadius = 0 - The inner radius of the overlay, in meters.
 * @property {Color} color = 255, 255, 255 - The color of the overlay.Setting this value also sets the values of
 *     <code>innerStartColor< / code>, <code>innerEndColor< / code>, <code>outerStartColor< / code>, and <code>outerEndColor< / code>.
 * @property {Color} startColor - Sets the values of <code>innerStartColor< / code> and <code>outerStartColor< / code>.
 *     <em>Write - only.< / em>
 * @property {Color} endColor - Sets the values of <code>innerEndColor< / code> and <code>outerEndColor< / code>.
 *     <em>Write - only.< / em>
 * @property {Color} innerColor - Sets the values of <code>innerStartColor< / code> and <code>innerEndColor< / code>.
 *     <em>Write - only.< / em>
 * @property {Color} outerColor - Sets the values of <code>outerStartColor< / code> and <code>outerEndColor< / code>.
 *     <em>Write - only.< / em>
 * @property {Color} innerStartcolor - The color at the inner start point of the overlay.
 * @property {Color} innerEndColor - The color at the inner end point of the overlay.
 * @property {Color} outerStartColor - The color at the outer start point of the overlay.
 * @property {Color} outerEndColor - The color at the outer end point of the overlay.
 * @property {number} alpha = 0.5 - The opacity of the overlay, <code>0.0< / code> -<code>1.0< / code>.Setting this value also sets
 *     the values of <code>innerStartAlpha< / code>, <code>innerEndAlpha< / code>, <code>outerStartAlpha< / code>, and
 *     <code>outerEndAlpha< / code>.Synonym: <code>Alpha< / code>; <em>write - only< / em>.
 * @property {number} startAlpha - Sets the values of <code>innerStartAlpha< / code> and <code>outerStartAlpha< / code>.
 *     <em>Write - only.< / em>
 * @property {number} endAlpha - Sets the values of <code>innerEndAlpha< / code> and <code>outerEndAlpha< / code>.
 *     <em>Write - only.< / em>
 * @property {number} innerAlpha - Sets the values of <code>innerStartAlpha< / code> and <code>innerEndAlpha< / code>.
 *     <em>Write - only.< / em>
 * @property {number} outerAlpha - Sets the values of <code>outerStartAlpha< / code> and <code>outerEndAlpha< / code>.
 *     <em>Write - only.< / em>
 * @property {number} innerStartAlpha = 0 - The alpha at the inner start point of the overlay.
 * @property {number} innerEndAlpha = 0 - The alpha at the inner end point of the overlay.
 * @property {number} outerStartAlpha = 0 - The alpha at the outer start point of the overlay.
 * @property {number} outerEndAlpha = 0 - The alpha at the outer end point of the overlay.
 *
 * @property {boolean} hasTickMarks = false - If <code>true< / code>, tick marks are drawn.
 * @property {number} majorTickMarksAngle = 0 - The angle between major tick marks, in degrees.
 * @property {number} minorTickMarksAngle = 0 - The angle between minor tick marks, in degrees.
 * @property {number} majorTickMarksLength = 0 - The length of the major tick marks, in meters.A positive value draws tick marks
 *     outwards from the inner radius; a negative value draws tick marks inwards from the outer radius.
 * @property {number} minorTickMarksLength = 0 - The length of the minor tick marks, in meters.A positive value draws tick marks
 *     outwards from the inner radius; a negative value draws tick marks inwards from the outer radius.
 * @property {Color} majorTickMarksColor = 0, 0, 0 - The color of the major tick marks.
 * @property {Color} minorTickMarksColor = 0, 0, 0 - The color of the minor tick marks.
 */
