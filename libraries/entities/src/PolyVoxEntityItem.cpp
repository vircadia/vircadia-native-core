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

#include "PolyVoxEntityItem.h"

#include <glm/gtx/transform.hpp>

#include <QByteArray>
#include <QDebug>
#include <QWriteLocker>

#include <ByteCountCoding.h>


#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

bool PolyVoxEntityItem::isEdged(PolyVoxSurfaceStyle surfaceStyle) {
    switch (surfaceStyle) {
        case PolyVoxEntityItem::SURFACE_CUBIC:
        case PolyVoxEntityItem::SURFACE_MARCHING_CUBES:
            return false;
        case PolyVoxEntityItem::SURFACE_EDGED_CUBIC:
        case PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES:
            return true;
    }
    return false;
}

const glm::vec3 PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE = glm::vec3(32, 32, 32);
const float PolyVoxEntityItem::MAX_VOXEL_DIMENSION = 128.0f;
const QByteArray PolyVoxEntityItem::DEFAULT_VOXEL_DATA(PolyVoxEntityItem::makeEmptyVoxelData());
const PolyVoxEntityItem::PolyVoxSurfaceStyle PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE =
    PolyVoxEntityItem::SURFACE_EDGED_CUBIC;
const QString PolyVoxEntityItem::DEFAULT_X_TEXTURE_URL = QString("");
const QString PolyVoxEntityItem::DEFAULT_Y_TEXTURE_URL = QString("");
const QString PolyVoxEntityItem::DEFAULT_Z_TEXTURE_URL = QString("");

EntityItemPointer PolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    std::shared_ptr<PolyVoxEntityItem> entity(new PolyVoxEntityItem(entityID), [](PolyVoxEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
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

PolyVoxEntityItem::PolyVoxEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::PolyVox;
}

void PolyVoxEntityItem::setVoxelVolumeSize(const glm::vec3& voxelVolumeSize_) {
    auto voxelVolumeSize = glm::clamp(glm::round(voxelVolumeSize_), glm::vec3(1.0f), glm::vec3(MAX_VOXEL_DIMENSION));

    withWriteLock([&] {
        _needsRenderUpdate |= _voxelVolumeSize != voxelVolumeSize;
        _voxelVolumeSize = voxelVolumeSize;
    });
}

glm::vec3 PolyVoxEntityItem::getVoxelVolumeSize() const {
    glm::vec3 voxelVolumeSize;
    withReadLock([&] {
        voxelVolumeSize = _voxelVolumeSize;
    });
    return voxelVolumeSize;
}


EntityItemProperties PolyVoxEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelVolumeSize, getVoxelVolumeSize);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelData, getVoxelData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelSurfaceStyle, getVoxelSurfaceStyle);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(xTextureURL, getXTextureURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(yTextureURL, getYTextureURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(zTextureURL, getZTextureURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(xNNeighborID, getXNNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(yNNeighborID, getYNNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(zNNeighborID, getZNNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(xPNeighborID, getXPNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(yPNeighborID, getYPNeighborID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(zPNeighborID, getZPNeighborID);

    return properties;
}

bool PolyVoxEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelVolumeSize, setVoxelVolumeSize);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelData, setVoxelData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelSurfaceStyle, setVoxelSurfaceStyle);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(xTextureURL, setXTextureURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(yTextureURL, setYTextureURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(zTextureURL, setZTextureURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(xNNeighborID, setXNNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(yNNeighborID, setYNNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(zNNeighborID, setZNNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(xPNeighborID, setXPNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(yPNeighborID, setYPNeighborID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(zPNeighborID, setZPNeighborID);

    return somethingChanged;
}

int PolyVoxEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                        ReadBitstreamToTreeParams& args,
                                                        EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                        bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSize);
    READ_ENTITY_PROPERTY(PROP_VOXEL_DATA, QByteArray, setVoxelData);
    READ_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyle);
    READ_ENTITY_PROPERTY(PROP_X_TEXTURE_URL, QString, setXTextureURL);
    READ_ENTITY_PROPERTY(PROP_Y_TEXTURE_URL, QString, setYTextureURL);
    READ_ENTITY_PROPERTY(PROP_Z_TEXTURE_URL, QString, setZTextureURL);
    READ_ENTITY_PROPERTY(PROP_X_N_NEIGHBOR_ID, EntityItemID, setXNNeighborID);
    READ_ENTITY_PROPERTY(PROP_Y_N_NEIGHBOR_ID, EntityItemID, setYNNeighborID);
    READ_ENTITY_PROPERTY(PROP_Z_N_NEIGHBOR_ID, EntityItemID, setZNNeighborID);
    READ_ENTITY_PROPERTY(PROP_X_P_NEIGHBOR_ID, EntityItemID, setXPNeighborID);
    READ_ENTITY_PROPERTY(PROP_Y_P_NEIGHBOR_ID, EntityItemID, setYPNeighborID);
    READ_ENTITY_PROPERTY(PROP_Z_P_NEIGHBOR_ID, EntityItemID, setZPNeighborID);

    return bytesRead;
}

EntityPropertyFlags PolyVoxEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_VOXEL_VOLUME_SIZE;
    requestedProperties += PROP_VOXEL_DATA;
    requestedProperties += PROP_VOXEL_SURFACE_STYLE;
    requestedProperties += PROP_X_TEXTURE_URL;
    requestedProperties += PROP_Y_TEXTURE_URL;
    requestedProperties += PROP_Z_TEXTURE_URL;
    requestedProperties += PROP_X_N_NEIGHBOR_ID;
    requestedProperties += PROP_Y_N_NEIGHBOR_ID;
    requestedProperties += PROP_Z_N_NEIGHBOR_ID;
    requestedProperties += PROP_X_P_NEIGHBOR_ID;
    requestedProperties += PROP_Y_P_NEIGHBOR_ID;
    requestedProperties += PROP_Z_P_NEIGHBOR_ID;
    return requestedProperties;
}

void PolyVoxEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                           EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount,
                                           OctreeElement::AppendState& appendState) const {
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, getVoxelVolumeSize());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, getVoxelData());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, (uint16_t) getVoxelSurfaceStyle());
    APPEND_ENTITY_PROPERTY(PROP_X_TEXTURE_URL, getXTextureURL());
    APPEND_ENTITY_PROPERTY(PROP_Y_TEXTURE_URL, getYTextureURL());
    APPEND_ENTITY_PROPERTY(PROP_Z_TEXTURE_URL, getZTextureURL());
    APPEND_ENTITY_PROPERTY(PROP_X_N_NEIGHBOR_ID, getXNNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Y_N_NEIGHBOR_ID, getYNNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Z_N_NEIGHBOR_ID, getZNNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_X_P_NEIGHBOR_ID, getXPNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Y_P_NEIGHBOR_ID, getYPNeighborID());
    APPEND_ENTITY_PROPERTY(PROP_Z_P_NEIGHBOR_ID, getZPNeighborID());
}

void PolyVoxEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   POLYVOX EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "            position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

void PolyVoxEntityItem::setVoxelData(const QByteArray& voxelData) {
    withWriteLock([&] {
        _voxelData = voxelData;
        _voxelDataDirty = true;
    });
}

QByteArray PolyVoxEntityItem::getVoxelData() const {
    QByteArray voxelDataCopy;
    withReadLock([&] {
        voxelDataCopy = _voxelData;
    });
    return voxelDataCopy;
}


void PolyVoxEntityItem::setXTextureURL(const QString& xTextureURL) {
    withWriteLock([&] {
        _needsRenderUpdate |= _xTextureURL != xTextureURL;
        _xTextureURL = xTextureURL;
    });
}

QString PolyVoxEntityItem::getXTextureURL() const {
    QString result;
    withReadLock([&] {
        result = _xTextureURL;
    });
    return result;
}

void PolyVoxEntityItem::setYTextureURL(const QString& yTextureURL) {
    withWriteLock([&] {
        _needsRenderUpdate |= _yTextureURL != yTextureURL;
        _yTextureURL = yTextureURL;
    });
}

QString PolyVoxEntityItem::getYTextureURL() const {
    QString result;
    withReadLock([&] {
        result = _yTextureURL;
    });
    return result;
}

void PolyVoxEntityItem::setZTextureURL(const QString& zTextureURL) {
    withWriteLock([&] {
        _needsRenderUpdate |= _zTextureURL != zTextureURL;
        _zTextureURL = zTextureURL;
    });
}
QString PolyVoxEntityItem::getZTextureURL() const {
    QString result;
    withReadLock([&] {
        result = _zTextureURL;
    });
    return result;
}

void PolyVoxEntityItem::setXNNeighborID(const EntityItemID& xNNeighborID) {
    withWriteLock([&] {
        _xNNeighborID = xNNeighborID;
    });
}

EntityItemID PolyVoxEntityItem::getXNNeighborID() const {
    EntityItemID result;
    withReadLock([&] {
        result = _xNNeighborID;
    });
    return result;
}

void PolyVoxEntityItem::setYNNeighborID(const EntityItemID& yNNeighborID) {
    withWriteLock([&] {
        _yNNeighborID = yNNeighborID;
    });
}

EntityItemID PolyVoxEntityItem::getYNNeighborID() const {
    EntityItemID result;
    withReadLock([&] {
        result = _yNNeighborID;
    });
    return result;
}

void PolyVoxEntityItem::setZNNeighborID(const EntityItemID& zNNeighborID) {
    withWriteLock([&] {
        _zNNeighborID = zNNeighborID;
    });
}

EntityItemID PolyVoxEntityItem::getZNNeighborID() const {
    EntityItemID result;
    withReadLock([&] {
        result = _zNNeighborID;
    });
    return result;
}

void PolyVoxEntityItem::setXPNeighborID(const EntityItemID& xPNeighborID) {
    withWriteLock([&] {
        _xPNeighborID = xPNeighborID;
    });
}

EntityItemID PolyVoxEntityItem::getXPNeighborID() const {
    EntityItemID result;
    withReadLock([&] {
        result = _xPNeighborID;
    });
    return result;
}

void PolyVoxEntityItem::setYPNeighborID(const EntityItemID& yPNeighborID) {
    withWriteLock([&] {
        _yPNeighborID = yPNeighborID;
    });
}

EntityItemID PolyVoxEntityItem::getYPNeighborID() const {
    EntityItemID result;
    withReadLock([&] {
        result = _yPNeighborID;
    });
    return result;
}

void PolyVoxEntityItem::setZPNeighborID(const EntityItemID& zPNeighborID) {
    withWriteLock([&] {
        _zPNeighborID = zPNeighborID;
    });
}

EntityItemID PolyVoxEntityItem::getZPNeighborID() const {
    EntityItemID result;
    withReadLock([&] {
        result = _zPNeighborID;
    });
    return result;
}

glm::vec3 PolyVoxEntityItem::getSurfacePositionAdjustment() const {
    glm::vec3 result;
    withReadLock([&] {
        glm::vec3 scale = getScaledDimensions() / _voxelVolumeSize; // meters / voxel-units
        if (isEdged()) {
            result = scale / -2.0f;
        }
        return scale / 2.0f;
    });
    return result;
}

glm::mat4 PolyVoxEntityItem::voxelToLocalMatrix() const {
    glm::vec3 voxelVolumeSize;
    withReadLock([&] {
        voxelVolumeSize = _voxelVolumeSize;
    });

    glm::vec3 dimensions = getScaledDimensions();
    glm::vec3 scale = dimensions / voxelVolumeSize; // meters / voxel-units
    bool success; // TODO -- Does this actually have to happen in world space?
    glm::vec3 center = getCenterPosition(success); // this handles registrationPoint changes
    glm::vec3 position = getWorldPosition(success);
    glm::vec3 positionToCenter = center - position;

    positionToCenter -= dimensions * Vectors::HALF - getSurfacePositionAdjustment();
    glm::mat4 centerToCorner = glm::translate(glm::mat4(), positionToCenter);
    glm::mat4 scaled = glm::scale(centerToCorner, scale);
    return scaled;
}

glm::mat4 PolyVoxEntityItem::localToVoxelMatrix() const {
    glm::mat4 localToModelMatrix = glm::inverse(voxelToLocalMatrix());
    return localToModelMatrix;
}

glm::mat4 PolyVoxEntityItem::voxelToWorldMatrix(bool includeBillboard) const {
    glm::vec3 position = getWorldPosition();
    glm::mat4 translation = glm::translate(position);
    glm::mat4 rotation;
    if (includeBillboard) {
        BillboardMode billboardMode = getBillboardMode();
        glm::quat orientation = billboardMode == BillboardMode::NONE ? getWorldOrientation() : getLocalOrientation();
        rotation = glm::mat4_cast(BillboardModeHelpers::getBillboardRotation(position, orientation, billboardMode, BillboardModeHelpers::getPrimaryViewFrustumPosition()));
    } else {
        rotation = glm::mat4_cast(getWorldOrientation());
    }
    return translation * rotation * voxelToLocalMatrix();
}

glm::mat4 PolyVoxEntityItem::worldToVoxelMatrix(bool includeBillboard) const {
    return glm::inverse(voxelToWorldMatrix(includeBillboard));
}

glm::vec3 PolyVoxEntityItem::voxelCoordsToWorldCoords(const glm::vec3& voxelCoords) const {
    glm::vec3 adjustedCoords;
    if (isEdged()) {
        adjustedCoords = voxelCoords + Vectors::HALF;
    } else {
        adjustedCoords = voxelCoords - Vectors::HALF;
    }
    return glm::vec3(voxelToWorldMatrix() * glm::vec4(adjustedCoords, 1.0f));
}

glm::vec3 PolyVoxEntityItem::worldCoordsToVoxelCoords(const glm::vec3& worldCoords) const {
    glm::vec3 result = glm::vec3(worldToVoxelMatrix() * glm::vec4(worldCoords, 1.0f));
    if (isEdged()) {
        return result - Vectors::HALF;
    }
    return result + Vectors::HALF;
}

glm::vec3 PolyVoxEntityItem::voxelCoordsToLocalCoords(const glm::vec3& voxelCoords) const {
    return glm::vec3(voxelToLocalMatrix() * glm::vec4(voxelCoords, 0.0f));
}

glm::vec3 PolyVoxEntityItem::localCoordsToVoxelCoords(const glm::vec3& localCoords) const {
    return glm::vec3(localToVoxelMatrix() * glm::vec4(localCoords, 0.0f));
}

ShapeType PolyVoxEntityItem::getShapeType() const {
    if (_collisionless) {
        return SHAPE_TYPE_NONE;
    }
    return SHAPE_TYPE_COMPOUND;
}

bool PolyVoxEntityItem::isEdged() const {
    return isEdged(_voxelSurfaceStyle);
}

std::array<EntityItemID, 3> PolyVoxEntityItem::getNNeigborIDs() const {
    std::array<EntityItemID, 3> result;
    withReadLock([&] {
        result = { { _xNNeighborID, _yNNeighborID, _zNNeighborID } };
    });
    return result;
}

std::array<EntityItemID, 3> PolyVoxEntityItem::getPNeigborIDs() const {
    std::array<EntityItemID, 3> result;
    withReadLock([&] {
        result = { { _xPNeighborID, _yPNeighborID, _zPNeighborID } };
    });
    return result;
}
