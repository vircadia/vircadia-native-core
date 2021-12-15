//
//  Created by Sam Gondelman on 1/12/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MaterialEntityItem.h"

#include "EntityItemProperties.h"

#include "QJsonDocument"
#include "QJsonArray"

EntityItemPointer MaterialEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity(new MaterialEntityItem(entityID), [](MaterialEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

// our non-pure virtual subclass for now...
MaterialEntityItem::MaterialEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Material;
}

EntityItemProperties MaterialEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialURL, getMaterialURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialMappingMode, getMaterialMappingMode);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(priority, getPriority);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(parentMaterialName, getParentMaterialName);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialMappingPos, getMaterialMappingPos);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialMappingScale, getMaterialMappingScale);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialMappingRot, getMaterialMappingRot);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialData, getMaterialData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialRepeat, getMaterialRepeat);
    return properties;
}

bool MaterialEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialURL, setMaterialURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingMode, setMaterialMappingMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(priority, setPriority);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentMaterialName, setParentMaterialName);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingPos, setMaterialMappingPos);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingScale, setMaterialMappingScale);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingRot, setMaterialMappingRot);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialData, setMaterialData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialRepeat, setMaterialRepeat);

    return somethingChanged;
}

int MaterialEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_MATERIAL_URL, QString, setMaterialURL);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_MODE, MaterialMappingMode, setMaterialMappingMode);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, quint16, setPriority);
    READ_ENTITY_PROPERTY(PROP_PARENT_MATERIAL_NAME, QString, setParentMaterialName);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_POS, glm::vec2, setMaterialMappingPos);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_SCALE, glm::vec2, setMaterialMappingScale);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_ROT, float, setMaterialMappingRot);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_DATA, QString, setMaterialData);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_REPEAT, bool, setMaterialRepeat);

    return bytesRead;
}

EntityPropertyFlags MaterialEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_MATERIAL_URL;
    requestedProperties += PROP_MATERIAL_MAPPING_MODE;
    requestedProperties += PROP_MATERIAL_PRIORITY;
    requestedProperties += PROP_PARENT_MATERIAL_NAME;
    requestedProperties += PROP_MATERIAL_MAPPING_POS;
    requestedProperties += PROP_MATERIAL_MAPPING_SCALE;
    requestedProperties += PROP_MATERIAL_MAPPING_ROT;
    requestedProperties += PROP_MATERIAL_DATA;
    requestedProperties += PROP_MATERIAL_REPEAT;
    return requestedProperties;
}

void MaterialEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_URL, getMaterialURL());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_MODE, (uint32_t)getMaterialMappingMode());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, getPriority());
    APPEND_ENTITY_PROPERTY(PROP_PARENT_MATERIAL_NAME, getParentMaterialName());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_POS, getMaterialMappingPos());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_SCALE, getMaterialMappingScale());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_ROT, getMaterialMappingRot());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_DATA, getMaterialData());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_REPEAT, getMaterialRepeat());
}

void MaterialEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << " MATERIAL EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                   name:" << _name;
    qCDebug(entities) << "           material url:" << _materialURL;
    qCDebug(entities) << "  material mapping mode:" << _materialMappingMode;
    qCDebug(entities) << "        material repeat:" << _materialRepeat;
    qCDebug(entities) << "               priority:" << _priority;
    qCDebug(entities) << "   parent material name:" << _parentMaterialName;
    qCDebug(entities) << "   material mapping pos:" << _materialMappingPos;
    qCDebug(entities) << " material mapping scale:" << _materialMappingRot;
    qCDebug(entities) << "   material mapping rot:" << _materialMappingScale;
    qCDebug(entities) << "               position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "             dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "          getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "MATERIAL EntityItem Ptr:" << this;
}

void MaterialEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    _desiredDimensions = value;
    if (_hasVertexShader || _materialMappingMode == MaterialMappingMode::PROJECTED) {
        EntityItem::setUnscaledDimensions(value);
    } else if (_materialMappingMode == MaterialMappingMode::UV) {
        EntityItem::setUnscaledDimensions(ENTITY_ITEM_DEFAULT_DIMENSIONS);
    }
}

QString MaterialEntityItem::getMaterialURL() const {
    return resultWithReadLock<QString>([&] {
        return _materialURL;
    });
}

void MaterialEntityItem::setMaterialURL(const QString& materialURL) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialURL != materialURL;
        _materialURL = materialURL;
    });
}

QString MaterialEntityItem::getMaterialData() const {
    return resultWithReadLock<QString>([&] {
        return _materialData;
    });
}

void MaterialEntityItem::setMaterialData(const QString& materialData) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialData != materialData;
        _materialData = materialData;
    });
}

MaterialMappingMode MaterialEntityItem::getMaterialMappingMode() const {
    return resultWithReadLock<MaterialMappingMode>([&] {
        return _materialMappingMode;
    });
}

void MaterialEntityItem::setMaterialMappingMode(MaterialMappingMode mode) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialMappingMode != mode;
        _materialMappingMode = mode;
    });
    setUnscaledDimensions(_desiredDimensions);
}

quint16 MaterialEntityItem::getPriority() const {
    return resultWithReadLock<quint16>([&] {
        return _priority;
    });
}

void MaterialEntityItem::setPriority(quint16 priority) {
    withWriteLock([&] {
        _needsRenderUpdate |= _priority != priority;
        _priority = priority;
    });
}

QString MaterialEntityItem::getParentMaterialName() const {
    return resultWithReadLock<QString>([&] {
        return _parentMaterialName;
    });
}

void MaterialEntityItem::setParentMaterialName(const QString& parentMaterialName) {
    withWriteLock([&] {
        _needsRenderUpdate |= _parentMaterialName != parentMaterialName;
        _parentMaterialName = parentMaterialName;
    });
}

glm::vec2 MaterialEntityItem::getMaterialMappingPos() const {
    return resultWithReadLock<glm::vec2>([&] {
        return _materialMappingPos;
    });
}

void MaterialEntityItem::setMaterialMappingPos(const glm::vec2& materialMappingPos) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialMappingPos != materialMappingPos;
        _materialMappingPos = materialMappingPos;
    });
}

glm::vec2 MaterialEntityItem::getMaterialMappingScale() const {
    return resultWithReadLock<glm::vec2>([&] {
        return _materialMappingScale;
    });
}

void MaterialEntityItem::setMaterialMappingScale(const glm::vec2& materialMappingScale) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialMappingScale != materialMappingScale;
        _materialMappingScale = materialMappingScale;
    });
}

float MaterialEntityItem::getMaterialMappingRot() const {
    return resultWithReadLock<float>([&] {
        return _materialMappingRot;
    });
}

void MaterialEntityItem::setMaterialMappingRot(float materialMappingRot) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialMappingRot != materialMappingRot;
        _materialMappingRot = materialMappingRot;
    });
}

bool MaterialEntityItem::getMaterialRepeat() const {
    return resultWithReadLock<bool>([&] {
        return _materialRepeat;
    });
}

void MaterialEntityItem::setMaterialRepeat(bool value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _materialRepeat != value;
        _materialRepeat = value;
    });
}

void MaterialEntityItem::setParentID(const QUuid& parentID) {
    if (parentID != getParentID()) {
        EntityItem::setParentID(parentID);
        _hasVertexShader = false;
    }
}

AACube MaterialEntityItem::calculateInitialQueryAACube(bool& success) {
    AACube aaCube = EntityItem::calculateInitialQueryAACube(success);
    // A Material entity's queryAACube contains its parent's queryAACube
    auto parent = getParentPointer(success);
    if (success && parent) {
        success = false;
        AACube parentQueryAACube = parent->calculateInitialQueryAACube(success);
        if (success) {
            aaCube += parentQueryAACube.getMinimumPoint();
            aaCube += parentQueryAACube.getMaximumPoint();
        }
    }
    return aaCube;
}

void MaterialEntityItem::setHasVertexShader(bool hasVertexShader) {
    bool prevHasVertexShader = _hasVertexShader;
    _hasVertexShader = hasVertexShader;

    if (hasVertexShader && !prevHasVertexShader) {
        setLocalPosition(glm::vec3(0.0f));
        setLocalOrientation(glm::quat());
        setUnscaledDimensions(EntityTree::getUnscaledDimensionsForID(getParentID()));
    } else if (!hasVertexShader && prevHasVertexShader) {
        setUnscaledDimensions(_desiredDimensions);
    }
}