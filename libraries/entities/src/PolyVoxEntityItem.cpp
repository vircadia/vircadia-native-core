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
const float PolyVoxEntityItem::MAX_VOXEL_DIMENSION = 32.0f;
const QByteArray PolyVoxEntityItem::DEFAULT_VOXEL_DATA(PolyVoxEntityItem::makeEmptyVoxelData());
const PolyVoxEntityItem::PolyVoxSurfaceStyle PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE =
    PolyVoxEntityItem::SURFACE_MARCHING_CUBES;

EntityItemPointer PolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<PolyVoxEntityItem>(entityID, properties);
}

QByteArray PolyVoxEntityItem::makeEmptyVoxelData(quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize) {
    int rawSize = voxelXSize * voxelYSize * voxelZSize;

    QByteArray uncompressedData = QByteArray(rawSize, '\0');
    QByteArray newVoxelData;
    QDataStream writer(&newVoxelData, QIODevice::WriteOnly | QIODevice::Truncate);
    writer << voxelXSize << voxelYSize << voxelZSize;

    QByteArray compressedData = qCompress(uncompressedData, 9);
    writer << compressedData;

    return newVoxelData;
}

PolyVoxEntityItem::PolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID),
    _voxelVolumeSize(PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE),
    _voxelData(PolyVoxEntityItem::DEFAULT_VOXEL_DATA),
    _voxelSurfaceStyle(PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE)
{
    _type = EntityTypes::PolyVox;
    setProperties(properties);
}

void PolyVoxEntityItem::setVoxelVolumeSize(glm::vec3 voxelVolumeSize) {
    assert((int)_voxelVolumeSize.x == _voxelVolumeSize.x);
    assert((int)_voxelVolumeSize.y == _voxelVolumeSize.y);
    assert((int)_voxelVolumeSize.z == _voxelVolumeSize.z);

    _voxelVolumeSize = voxelVolumeSize;
    if (_voxelVolumeSize.x < 1) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping x of" << _voxelVolumeSize.x << "to 1";
        _voxelVolumeSize.x = 1;
    }
    if (_voxelVolumeSize.x > MAX_VOXEL_DIMENSION) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping x of" << _voxelVolumeSize.x << "to max";
        _voxelVolumeSize.x = MAX_VOXEL_DIMENSION;
    }

    if (_voxelVolumeSize.y < 1) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping y of" << _voxelVolumeSize.y << "to 1";
        _voxelVolumeSize.y = 1;
    }
    if (_voxelVolumeSize.y > MAX_VOXEL_DIMENSION) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping y of" << _voxelVolumeSize.y << "to max";
        _voxelVolumeSize.y = MAX_VOXEL_DIMENSION;
    }

    if (_voxelVolumeSize.z < 1) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping z of" << _voxelVolumeSize.z << "to 1";
        _voxelVolumeSize.z = 1;
    }
    if (_voxelVolumeSize.z > MAX_VOXEL_DIMENSION) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping z of" << _voxelVolumeSize.z << "to max";
        _voxelVolumeSize.z = MAX_VOXEL_DIMENSION;
    }
}

EntityItemProperties PolyVoxEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelVolumeSize, getVoxelVolumeSize);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelData, getVoxelData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelSurfaceStyle, getVoxelSurfaceStyle);

    return properties;
}

bool PolyVoxEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class
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

    READ_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSize);
    READ_ENTITY_PROPERTY(PROP_VOXEL_DATA, QByteArray, setVoxelData);
    READ_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyle);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags PolyVoxEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
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

    APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, getVoxelVolumeSize());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, getVoxelData());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, (uint16_t) getVoxelSurfaceStyle());
}

void PolyVoxEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   POLYVOX EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

void PolyVoxEntityItem::setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) {
    if (voxelSurfaceStyle == _voxelSurfaceStyle) {
        return;
    }
    updateVoxelSurfaceStyle(voxelSurfaceStyle);
}
