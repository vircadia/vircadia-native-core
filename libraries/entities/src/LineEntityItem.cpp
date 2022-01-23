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

const int LineEntityItem::MAX_POINTS_PER_LINE = 70;

EntityItemPointer LineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    std::shared_ptr<LineEntityItem> entity(new LineEntityItem(entityID), [](LineEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

LineEntityItem::LineEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID)
{
    _type = EntityTypes::Line;
}

EntityItemProperties LineEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);

    return properties;
}

bool LineEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePoints);

    return somethingChanged;
}

bool LineEntityItem::appendPoint(const glm::vec3& point) {
    if (_points.size() > MAX_POINTS_PER_LINE - 1) {
        qCDebug(entities) << "MAX POINTS REACHED!";
        return false;
    }
    glm::vec3 halfBox = getScaledDimensions() * 0.5f;
    if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
        qCDebug(entities) << "Point is outside entity's bounding box";
        return false;
    }
    withWriteLock([&] {
        _needsRenderUpdate = true;
        _points << point;
    });

    return true;
}

bool LineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    if (points.size() > MAX_POINTS_PER_LINE) {
        return false;
    }
    glm::vec3 halfBox = getScaledDimensions() * 0.5f;
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points.at(i);
        if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
            qCDebug(entities) << "Point is outside entity's bounding box";
            return false;
        }
    }

    withWriteLock([&] {
        _needsRenderUpdate = true;
        _points = points;
    });

    return true;
}

int LineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                     bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, glm::u8vec3, setColor);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);

    return bytesRead;
}


EntityPropertyFlags LineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
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
    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePoints());
}

void LineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   LINE EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color;
    qCDebug(entities) << "            position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

glm::u8vec3 LineEntityItem::getColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _color;
    });
}

void LineEntityItem::setColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _color != value;
        _color = value;
    });
}

QVector<glm::vec3> LineEntityItem::getLinePoints() const { 
    QVector<glm::vec3> result;
    withReadLock([&] {
        result = _points;
    });
    return result;
}