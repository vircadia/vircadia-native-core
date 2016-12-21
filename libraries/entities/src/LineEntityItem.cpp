//
//  LineEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LineEntityItem.h"

#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"

const float LineEntityItem::DEFAULT_LINE_WIDTH = 2.0f;
const int LineEntityItem::MAX_POINTS_PER_LINE = 70;


EntityItemPointer LineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new LineEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

LineEntityItem::LineEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID),
    _lineWidth(DEFAULT_LINE_WIDTH),
    _pointsChanged(true),
    _points(QVector<glm::vec3>(0))
{
    _type = EntityTypes::Line;
}

EntityItemProperties LineEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    
    properties._color = getXColor();
    properties._colorChanged = false;
    
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineWidth, getLineWidth);
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);

    return properties;
}

bool LineEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class
    
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineWidth, setLineWidth);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePoints);


    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "LineEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}

bool LineEntityItem::appendPoint(const glm::vec3& point) {
    if (_points.size() > MAX_POINTS_PER_LINE - 1) {
        qCDebug(entities) << "MAX POINTS REACHED!";
        return false;
    }
    glm::vec3 halfBox = getDimensions() * 0.5f;
    if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
        qCDebug(entities) << "Point is outside entity's bounding box";
        return false;
    }
    _points << point;
    _pointsChanged = true;
    return true;
}

bool LineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    if (points.size() > MAX_POINTS_PER_LINE) {
        return false;
    }
    glm::vec3 halfBox = getDimensions() * 0.5f;
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points.at(i);
        if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
            qCDebug(entities) << "Point is outside entity's bounding box";
            return false;
        }
    }
    _points = points;
    _pointsChanged = true;
    return true;
}

int LineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                     bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_LINE_WIDTH, float, setLineWidth);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_LINE_WIDTH;
    requestedProperties += PROP_LINE_POINTS;
    return requestedProperties;
}

void LineEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                        EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                        EntityPropertyFlags& requestedProperties,
                                        EntityPropertyFlags& propertyFlags,
                                        EntityPropertyFlags& propertiesDidntFit,
                                        int& propertyCount, 
                                        OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, getLineWidth());
    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePoints());
}

void LineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   LINE EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

