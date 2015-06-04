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


#include <QDebug>

#include <ByteCountCoding.h>

#include "LineEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"



const float LineEntityItem::DEFAULT_LINE_WIDTH = 2.0f;


EntityItemPointer LineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new LineEntityItem(entityID, properties) };
    return result;
}

LineEntityItem::LineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID) ,
    _lineWidth(DEFAULT_LINE_WIDTH),
    _pointsChanged(true),
    _points(QVector<glm::vec3>(0))
{
    _type = EntityTypes::Line;
    _created = properties.getCreated();
    setProperties(properties);
    
    
}

EntityItemProperties LineEntityItem::getProperties() const {
    
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    
    properties._color = getXColor();
    properties._colorChanged = false;
    
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineWidth, getLineWidth);
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);


    properties._glowLevel = getGlowLevel();
    properties._glowLevelChanged = false;

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

void LineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    QVector<glm::vec3> sanitizedPoints;
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points.at(i);
        // Make sure all of our points are valid numbers.
        // Must be greater than 0 because vector component is set to 0 if it is invalid data
        if (point.x > 0 && point.y > 0 && point.z > 0){
            sanitizedPoints << point;
        } else {
            qDebug() << "INVALID POINT";
        }
    }
    _points = sanitizedPoints;
    _pointsChanged = true;
}

int LineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

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
}

void LineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   LINE EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(_position);
    qCDebug(entities) << "          dimensions:" << debugTreeVector(_dimensions);
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

