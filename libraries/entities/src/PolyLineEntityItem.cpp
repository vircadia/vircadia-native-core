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


#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"
#include "PolyLineEntityItem.h"

const float PolyLineEntityItem::DEFAULT_LINE_WIDTH = 0.1f;
const int PolyLineEntityItem::MAX_POINTS_PER_LINE = 70;


EntityItemPointer PolyLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new PolyLineEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

PolyLineEntityItem::PolyLineEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::PolyLine;
}

EntityItemProperties PolyLineEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    QWriteLocker lock(&_quadReadWriteLock);
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class


    properties._color = getXColor();
    properties._colorChanged = false;

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineWidth, getLineWidth);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(normals, getNormals);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(strokeWidths, getStrokeWidths);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);
    return properties;
}

bool PolyLineEntityItem::setProperties(const EntityItemProperties& properties) {
    QWriteLocker lock(&_quadReadWriteLock);
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineWidth, setLineWidth);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePoints);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(normals, setNormals);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(strokeWidths, setStrokeWidths);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "PolyLineEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}


bool PolyLineEntityItem::appendPoint(const glm::vec3& point) {
    if (_points.size() > MAX_POINTS_PER_LINE - 1) {
        qCDebug(entities) << "MAX POINTS REACHED!";
        return false;
    }

    _points << point;
    _pointsChanged = true;

    calculateScaleAndRegistrationPoint();

    return true;
}


bool PolyLineEntityItem::setStrokeWidths(const QVector<float>& strokeWidths) {
    withWriteLock([&] {
        _strokeWidths = strokeWidths;
        _strokeWidthsChanged = true;
    });
    return true;
}

bool PolyLineEntityItem::setNormals(const QVector<glm::vec3>& normals) {
    withWriteLock([&] {
        _normals = normals;
        _normalsChanged = true;
    });
    return true;
}

bool PolyLineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    if (points.size() > MAX_POINTS_PER_LINE) {
        return false;
    }
    bool result = false;
    withWriteLock([&] {
        //Check to see if points actually changed. If they haven't, return before doing anything else
        if (points.size() != _points.size()) {
            _pointsChanged = true;
        } else if (points.size() == _points.size()) {
            //same number of points, so now compare every point
            for (int i = 0; i < points.size(); i++) {
                if (points.at(i) != _points.at(i)) {
                    _pointsChanged = true;
                    break;
                }
            }
        }
        if (!_pointsChanged) {
            return;
        }

        _points = points;

        result = true;
    });

    if (result) {
        calculateScaleAndRegistrationPoint();
    }

    return result;
}

void PolyLineEntityItem::calculateScaleAndRegistrationPoint() {
    glm::vec3 high(0.0f, 0.0f, 0.0f);
    glm::vec3 low(0.0f, 0.0f, 0.0f);
    int pointCount = 0;
    glm::vec3 firstPoint;
    withReadLock([&] {
        pointCount = _points.size();
        if (pointCount > 0) {
            firstPoint = _points.at(0);
        }
        for (int i = 0; i < pointCount; i++) {
            const glm::vec3& point = _points.at(i);
            high = glm::max(point, high);
            low = glm::min(point, low);
        }
    });

    float magnitudeSquared = glm::length2(low - high);
    vec3 newScale { 1 };
    vec3 newRegistrationPoint { 0.5f };

    const float EPSILON = 0.0001f;
    const float EPSILON_SQUARED = EPSILON * EPSILON;
    const float HALF_LINE_WIDTH = 0.075f; // sadly _strokeWidths() don't seem to correspond to reality, so just use a flat assumption of the stroke width
    const vec3 QUARTER_LINE_WIDTH { HALF_LINE_WIDTH * 0.5f };
    if (pointCount > 1 && magnitudeSquared > EPSILON_SQUARED) {
        newScale = glm::abs(high) + glm::abs(low) + vec3(HALF_LINE_WIDTH);
        // Center the poly line in the bounding box
        glm::vec3 startPointInScaleSpace = firstPoint - low;
        startPointInScaleSpace += QUARTER_LINE_WIDTH;
        newRegistrationPoint = startPointInScaleSpace / newScale;
    } 

    // if Polyline has only one or fewer points, use default dimension settings
    setDimensions(newScale);
    EntityItem::setRegistrationPoint(newRegistrationPoint);
}

int PolyLineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                         ReadBitstreamToTreeParams& args,
                                                         EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                         bool& somethingChanged) {

    QWriteLocker lock(&_quadReadWriteLock);
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_LINE_WIDTH, float, setLineWidth);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);
    READ_ENTITY_PROPERTY(PROP_NORMALS, QVector<glm::vec3>, setNormals);
    READ_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, QVector<float>, setStrokeWidths);
    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags PolyLineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_LINE_WIDTH;
    requestedProperties += PROP_LINE_POINTS;
    requestedProperties += PROP_NORMALS;
    requestedProperties += PROP_STROKE_WIDTHS;
    requestedProperties += PROP_TEXTURES;
    return requestedProperties;
}

void PolyLineEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                            EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                            EntityPropertyFlags& requestedProperties,
                                            EntityPropertyFlags& propertyFlags,
                                            EntityPropertyFlags& propertiesDidntFit,
                                            int& propertyCount,
                                            OctreeElement::AppendState& appendState) const {

    QWriteLocker lock(&_quadReadWriteLock);
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, getLineWidth());
    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePoints());
    APPEND_ENTITY_PROPERTY(PROP_NORMALS, getNormals());
    APPEND_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, getStrokeWidths());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());
}

void PolyLineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   QUAD EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}



QVector<glm::vec3> PolyLineEntityItem::getLinePoints() const { 
    QVector<glm::vec3> result;
    withReadLock([&] {
        result = _points;
    });
    return result; 
}

QVector<glm::vec3> PolyLineEntityItem::getNormals() const { 
    QVector<glm::vec3> result;
    withReadLock([&] {
        result = _normals;
    });
    return result;
}

QVector<float> PolyLineEntityItem::getStrokeWidths() const { 
    QVector<float> result;
    withReadLock([&] {
        result = _strokeWidths;
    });
    return result;
}

QString PolyLineEntityItem::getTextures() const { 
    QString result;
    withReadLock([&] {
        result = _textures;
    });
    return result;
}

void PolyLineEntityItem::setTextures(const QString& textures) {
    withWriteLock([&] {
        if (_textures != textures) {
            _textures = textures;
            _texturesChangedFlag = true;
        }
    });
}
