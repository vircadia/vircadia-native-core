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

#include "PolyLineEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"



const float PolyLineEntityItem::DEFAULT_LINE_WIDTH = 0.1f;
const int PolyLineEntityItem::MAX_POINTS_PER_LINE = 70;


EntityItemPointer PolyLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new PolyLineEntityItem(entityID, properties) };
    return result;
}

PolyLineEntityItem::PolyLineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
EntityItem(entityItemID) ,
_lineWidth(DEFAULT_LINE_WIDTH),
_pointsChanged(true),
_points(QVector<glm::vec3>(0.0f)),
_vertices(QVector<glm::vec3>(0.0f)),
_normals(QVector<glm::vec3>(0.0f)),
_strokeWidths(QVector<float>(0.0f))
{
    _type = EntityTypes::PolyLine;
    _created = properties.getCreated();
    setProperties(properties);
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
    
    properties._glowLevel = getGlowLevel();
    properties._glowLevelChanged = false;
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
        qDebug() << "MAX POINTS REACHED!";
        return false;
    }
    glm::vec3 halfBox = getDimensions() * 0.5f;
    if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
        qDebug() << "Point is outside entity's bounding box";
        return false;
    }
    _points << point;
    _pointsChanged = true;
    return true;
}

bool PolyLineEntityItem::setStrokeWidths(const QVector<float>& strokeWidths ) {
    _strokeWidths = strokeWidths;
    return true;
}

bool PolyLineEntityItem::setNormals(const QVector<glm::vec3>& normals) {
    _normals = normals;
    if (_points.size () < 2 || _normals.size() < 2) {
        return false;
    }
    
    int minVectorSize = _normals.size();
    if (_points.size() < minVectorSize) {
        minVectorSize = _points.size();
    }
    if (_strokeWidths.size() < minVectorSize) {
        minVectorSize = _strokeWidths.size();
    }

    _vertices.clear();
    glm::vec3 v1, v2, tangent, binormal, point;
  
    for (int i = 0; i < minVectorSize-1; i++) {
        float width = _strokeWidths.at(i);
        point = _points.at(i);
        
        tangent = _points.at(i+1) - point;
        glm::vec3 normal = normals.at(i);
        binormal = glm::normalize(glm::cross(tangent, normal)) * width;
        
        //This checks to make sure binormal is not a NAN
        assert(binormal.x == binormal.x);
        v1 = point + binormal;
        v2 = point - binormal;
        _vertices << v1 << v2;
    }
    //for last point we can just assume binormals are same since it represents last two vertices of quad
    point = _points.at(_points.size() - 1);
    v1 = point + binormal;
    v2 = point - binormal;
    _vertices << v1 << v2;
    
    return true;
}

bool PolyLineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    if (points.size() > MAX_POINTS_PER_LINE) {
        return false;
    }
    if (points.size() != _points.size()) {
        _pointsChanged = true;
    }
    //Check to see if points actually changed. If they haven't, return before doing anything else
    else if (points.size() == _points.size()) {
        //same number of points, so now compare every point
        for (int i = 0; i < points.size(); i++ ) {
            if (points.at(i) != _points.at(i)){
                _pointsChanged = true;
                break;
            }
        }
    }
    if (!_pointsChanged) {
        return false;
    }

    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points.at(i);
        glm::vec3 halfBox = getDimensions() * 0.5f;
        if ((point.x < - halfBox.x || point.x > halfBox.x) ||
            (point.y < -halfBox.y || point.y > halfBox.y) ||
            (point.z < - halfBox.z || point.z > halfBox.z)) {
            qDebug() << "Point is outside entity's bounding box";
            return false;
        }
    }
    _points = points;
    return true;
}

int PolyLineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    QWriteLocker lock(&_quadReadWriteLock);
    int bytesRead = 0;
    const unsigned char* dataAt = data;
    
    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_LINE_WIDTH, float, setLineWidth);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);
    READ_ENTITY_PROPERTY(PROP_NORMALS, QVector<glm::vec3>,  setNormals);
    READ_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, QVector<float>, setStrokeWidths);
    
    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags PolyLineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_LINE_WIDTH;
    requestedProperties += PROP_LINE_POINTS;
    requestedProperties += PROP_NORMALS;
    requestedProperties += PROP_STROKE_WIDTHS;
    return requestedProperties;
}

void PolyLineEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                        EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
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
}

void PolyLineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   QUAD EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

