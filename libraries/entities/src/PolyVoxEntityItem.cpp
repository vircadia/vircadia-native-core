//
//  PolyVoxEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QByteArray>
#include <QDebug>

#include <ByteCountCoding.h>

#include "PolyVoxEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"


const glm::vec3 PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE = glm::vec3(32, 32, 32);
const QByteArray PolyVoxEntityItem::DEFAULT_VOXEL_DATA(qCompress(QByteArray(0), 9));
const int PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE = 0;

EntityItemPointer PolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new PolyVoxEntityItem(entityID, properties));
}

PolyVoxEntityItem::PolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID),
    _voxelVolumeSize(PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE),
    _voxelData(PolyVoxEntityItem::DEFAULT_VOXEL_DATA),
    _voxelSurfaceStyle(PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE)
{
    _type = EntityTypes::PolyVox;
    _created = properties.getCreated();
    setProperties(properties);
}

EntityItemProperties PolyVoxEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    // properties._color = getXColor();
    // properties._colorChanged = false;
    // properties._glowLevel = getGlowLevel();
    // properties._glowLevelChanged = false;

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelVolumeSize, getVoxelVolumeSize);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelData, getVoxelData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelSurfaceStyle, getVoxelSurfaceStyle);

    return properties;
}

bool PolyVoxEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setXColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelVolumeSize, setVoxelVolumeSize);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelData, setVoxelData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelSurfaceStyle, setVoxelSurfaceStyle);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "PolyVoxEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}

int PolyVoxEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                        ReadBitstreamToTreeParams& args,
                                                        EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSize);
    READ_ENTITY_PROPERTY(PROP_VOXEL_DATA, QByteArray, setVoxelData);
    READ_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyle);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags PolyVoxEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_VOXEL_VOLUME_SIZE;
    requestedProperties += PROP_VOXEL_DATA;
    requestedProperties += PROP_VOXEL_SURFACE_STYLE;
    return requestedProperties;
}

void PolyVoxEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                           EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount, 
                                           OctreeElement::AppendState& appendState) const { 
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, getVoxelVolumeSize());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, getVoxelData());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, getVoxelSurfaceStyle());
}

void PolyVoxEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   POLYVOX EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(_position);
    qCDebug(entities) << "          dimensions:" << debugTreeVector(_dimensions);
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

