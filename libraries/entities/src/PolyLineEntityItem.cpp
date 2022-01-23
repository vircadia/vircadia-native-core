//
//  PolyLineEntityItem.cpp
//  libraries/entities/src
//
//  Created by Eric Levin on 6/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PolyLineEntityItem.h"

#include <QDebug>

#include <ByteCountCoding.h>
#include <Extents.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"

const float PolyLineEntityItem::DEFAULT_LINE_WIDTH = 0.1f;
const int PolyLineEntityItem::MAX_POINTS_PER_LINE = 60;

EntityItemPointer PolyLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    std::shared_ptr<PolyLineEntityItem> entity(new PolyLineEntityItem(entityID), [](PolyLineEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}


PolyLineEntityItem::PolyLineEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::PolyLine;
}

EntityItemProperties PolyLineEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(strokeWidths, getStrokeWidths);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(normals, getNormals);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(strokeColors, getStrokeColors);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(isUVModeStretch, getIsUVModeStretch);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glow, getGlow);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(faceCamera, getFaceCamera);

    return properties;
}

bool PolyLineEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePoints);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(strokeWidths, setStrokeWidths);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(normals, setNormals);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(strokeColors, setStrokeColors);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isUVModeStretch, setIsUVModeStretch);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glow, setGlow);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(faceCamera, setFaceCamera);

    return somethingChanged;
}

void PolyLineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    withWriteLock([&] {
        _points = points;
        _pointsChanged = true;
    });
    computeAndUpdateDimensions();
}

void PolyLineEntityItem::setStrokeWidths(const QVector<float>& strokeWidths) {
    withWriteLock([&] {
        _widths = strokeWidths;
        _widthsChanged = true;
    });
    computeAndUpdateDimensions();
}

void PolyLineEntityItem::setNormals(const QVector<glm::vec3>& normals) {
    withWriteLock([&] {
        _normals = normals;
        _normalsChanged = true;
    });
}

void PolyLineEntityItem::setStrokeColors(const QVector<glm::vec3>& strokeColors) {
    withWriteLock([&] {
        _colors = strokeColors;
        _colorsChanged = true;
    });
}

void PolyLineEntityItem::computeAndUpdateDimensions() {
    QVector<glm::vec3> points;
    QVector<float> widths;

    withReadLock([&] {
        points = _points;
        widths = _widths;
    });

    glm::vec3 maxHalfDim(0.5f * ENTITY_ITEM_DEFAULT_WIDTH);
    float maxWidth = 0.0f;
    for (int i = 0; i < points.length(); i++) {
        maxHalfDim = glm::max(maxHalfDim, glm::abs(points[i]));
        maxWidth = glm::max(maxWidth, i < widths.length() ? widths[i] : DEFAULT_LINE_WIDTH);
    }

    setScaledDimensions(2.0f * (maxHalfDim + maxWidth));
}

void PolyLineEntityItem::computeTightLocalBoundingBox(AABox& localBox) const {
    QVector<glm::vec3> points;
    QVector<float> widths;
    withReadLock([&] {
        points = _points;
        widths = _widths;
    });

    if (points.size() > 0) {
        Extents extents;
        float maxWidth = DEFAULT_LINE_WIDTH;
        for (int i = 0; i < points.length(); i++) {
            extents.addPoint(points[i]);
            if (i < widths.size()) {
                maxWidth = glm::max(maxWidth, widths[i]);
            }
        }
        extents.addPoint(extents.minimum - maxWidth * Vectors::ONE);
        extents.addPoint(extents.maximum + maxWidth * Vectors::ONE);

        localBox.setBox(extents.minimum, extents.maximum - extents.minimum);
    } else {
        localBox.setBox(glm::vec3(-0.5f * DEFAULT_LINE_WIDTH), glm::vec3(DEFAULT_LINE_WIDTH));
    }
}

int PolyLineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                         ReadBitstreamToTreeParams& args,
                                                         EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                         bool& somethingChanged) {
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, glm::u8vec3, setColor);
    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);

    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);
    READ_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, QVector<float>, setStrokeWidths);
    READ_ENTITY_PROPERTY(PROP_STROKE_NORMALS, QVector<glm::vec3>, setNormals);
    READ_ENTITY_PROPERTY(PROP_STROKE_COLORS, QVector<glm::vec3>, setStrokeColors);
    READ_ENTITY_PROPERTY(PROP_IS_UV_MODE_STRETCH, bool, setIsUVModeStretch);
    READ_ENTITY_PROPERTY(PROP_LINE_GLOW, bool, setGlow);
    READ_ENTITY_PROPERTY(PROP_LINE_FACE_CAMERA, bool, setFaceCamera);

    return bytesRead;
}

EntityPropertyFlags PolyLineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_TEXTURES;

    requestedProperties += PROP_LINE_POINTS;
    requestedProperties += PROP_STROKE_WIDTHS;
    requestedProperties += PROP_STROKE_NORMALS;
    requestedProperties += PROP_STROKE_COLORS;
    requestedProperties += PROP_IS_UV_MODE_STRETCH;
    requestedProperties += PROP_LINE_GLOW;
    requestedProperties += PROP_LINE_FACE_CAMERA;
    return requestedProperties;
}

void PolyLineEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                            EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                            EntityPropertyFlags& requestedProperties,
                                            EntityPropertyFlags& propertyFlags,
                                            EntityPropertyFlags& propertiesDidntFit,
                                            int& propertyCount,
                                            OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());

    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePoints());
    APPEND_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, getStrokeWidths());
    APPEND_ENTITY_PROPERTY(PROP_STROKE_NORMALS, getNormals());
    APPEND_ENTITY_PROPERTY(PROP_STROKE_COLORS, getStrokeColors());
    APPEND_ENTITY_PROPERTY(PROP_IS_UV_MODE_STRETCH, getIsUVModeStretch());
    APPEND_ENTITY_PROPERTY(PROP_LINE_GLOW, getGlow());
    APPEND_ENTITY_PROPERTY(PROP_LINE_FACE_CAMERA, getFaceCamera());
}

void PolyLineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   QUAD EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color;
    qCDebug(entities) << "            position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

QVector<glm::vec3> PolyLineEntityItem::getLinePoints() const {
    return resultWithReadLock<QVector<glm::vec3>>([&] {
        return _points;
    });
}

QVector<glm::vec3> PolyLineEntityItem::getNormals() const {
    return resultWithReadLock<QVector<glm::vec3>>([&] {
        return _normals;
    });
}

QVector<glm::vec3> PolyLineEntityItem::getStrokeColors() const {
    return resultWithReadLock<QVector<glm::vec3>>([&] {
        return _colors;
    });
}

QVector<float> PolyLineEntityItem::getStrokeWidths() const { 
    return resultWithReadLock<QVector<float>>([&] {
        return _widths;
    });
}

QString PolyLineEntityItem::getTextures() const { 
    return resultWithReadLock<QString>([&] {
        return _textures;
    });
}

void PolyLineEntityItem::setTextures(const QString& textures) {
    withWriteLock([&] {
        if (_textures != textures) {
            _textures = textures;
            _texturesChanged = true;
        }
    });
}

void PolyLineEntityItem::setColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _color = value;
        _colorsChanged = true;
    });
}

glm::u8vec3 PolyLineEntityItem::getColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _color;
    });
}

void PolyLineEntityItem::setIsUVModeStretch(bool isUVModeStretch) {
    withWriteLock([&] {
        _needsRenderUpdate |= _isUVModeStretch != isUVModeStretch;
        _isUVModeStretch = isUVModeStretch;
    });
}

void PolyLineEntityItem::setGlow(bool glow) {
    withWriteLock([&] {
        _needsRenderUpdate |= _glow != glow;
        _glow = glow;
    });
}

void PolyLineEntityItem::setFaceCamera(bool faceCamera) {
    withWriteLock([&] {
        _needsRenderUpdate |= _faceCamera != faceCamera;
        _faceCamera = faceCamera;
    });
}
