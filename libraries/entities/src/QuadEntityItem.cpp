//
//  QuadEntityItem.cpp
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

#include "QuadEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"



const float QuadEntityItem::DEFAULT_LINE_WIDTH = 0.1f;
const int QuadEntityItem::MAX_POINTS_PER_LINE = 70;


EntityItemPointer QuadEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new QuadEntityItem(entityID, properties) };
    return result;
}

QuadEntityItem::QuadEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
EntityItem(entityItemID) ,
_lineWidth(DEFAULT_LINE_WIDTH),
_pointsChanged(true),
_points(QVector<glm::vec3>(0)),
_vertices(QVector<glm::vec3>(0))
{
    _type = EntityTypes::Quad;
    _created = properties.getCreated();
    setProperties(properties);
}

EntityItemProperties QuadEntityItem::getProperties() const {
    _quadReadWriteLock.lockForWrite();
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class
    
    
    properties._color = getXColor();
    properties._colorChanged = false;
    
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineWidth, getLineWidth);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(normals, getNormals);
    
    
    properties._glowLevel = getGlowLevel();
    properties._glowLevelChanged = false;
    _quadReadWriteLock.unlock();
    return properties;
}

bool QuadEntityItem::setProperties(const EntityItemProperties& properties) {
    _quadReadWriteLock.lockForWrite();
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class
    
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineWidth, setLineWidth);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePoints);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(normals, setNormals);
    
    
    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "QuadEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
            "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    
    _quadReadWriteLock.unlock();
    return somethingChanged;
    
}

bool QuadEntityItem::appendPoint(const glm::vec3& point) {
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

bool QuadEntityItem::setNormals(const QVector<glm::vec3> &normals) {
    if (_points.size () < 2) {
        return false;
    }
    _normals = normals;
    _vertices.clear();
    //Go through and create vertices for triangle strip based on normals
    if (_normals.size() != _points.size()) {
        return false;
    }
    glm::vec3 v1, v2, tangent, binormal, point;
    for (int i = 0; i < _points.size()-1; i++) {
        float width = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * .1) + .02;
        point = _points.at(i);
        //Get tangent
        tangent = _points.at(i+1) - point;
        binormal = glm::normalize(glm::cross(tangent, normals.at(i))) * width;
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

bool QuadEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
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
        glm::vec3 pos = getPosition();
        glm::vec3 halfBox = getDimensions() * 0.5f;
        if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
            qDebug() << "Point is outside entity's bounding box";
            return false;
        }
        
    }
    _points = points;
    //All our points are valid and at least one point has changed, now create quads from points

    return true;
}

int QuadEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    _quadReadWriteLock.lockForWrite();
    int bytesRead = 0;
    const unsigned char* dataAt = data;
    
    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_LINE_WIDTH, float, setLineWidth);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);
    READ_ENTITY_PROPERTY(PROP_NORMALS, QVector<glm::vec3>,  setNormals);
    
    _quadReadWriteLock.unlock();
    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags QuadEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_LINE_WIDTH;
    requestedProperties += PROP_LINE_POINTS;
    requestedProperties += PROP_NORMALS;
    return requestedProperties;
}

void QuadEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                        EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                        EntityPropertyFlags& requestedProperties,
                                        EntityPropertyFlags& propertyFlags,
                                        EntityPropertyFlags& propertiesDidntFit,
                                        int& propertyCount,
                                        OctreeElement::AppendState& appendState) const {
    
    bool successPropertyFits = true;
    
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, getLineWidth());
    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePoints());
    APPEND_ENTITY_PROPERTY(PROP_NORMALS, getNormals());
}

void QuadEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   QUAD EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

