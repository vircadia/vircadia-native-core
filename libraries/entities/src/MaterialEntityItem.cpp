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
    Pointer entity(new MaterialEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    // When you reload content, setProperties doesn't have any of the propertiesChanged flags set, so it won't trigger a material add
    entity->removeMaterial();
    entity->applyMaterial();
    return entity;
}

// our non-pure virtual subclass for now...
MaterialEntityItem::MaterialEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Material;
}

MaterialEntityItem::~MaterialEntityItem() {
    removeMaterial();
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
    return properties;
}

bool MaterialEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialURL, setMaterialURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingMode, setMaterialMappingMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(priority, setPriority);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentMaterialName, setParentMaterialName);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingPos, setMaterialMappingPos);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingScale, setMaterialMappingScale);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMappingRot, setMaterialMappingRot);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialData, setMaterialData);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "MaterialEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
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
}

void MaterialEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << " MATERIAL EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                   name:" << _name;
    qCDebug(entities) << "           material url:" << _materialURL;
    qCDebug(entities) << "  current material name:" << _currentMaterialName.c_str();
    qCDebug(entities) << "  material mapping mode:" << _materialMappingMode;
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
    EntityItem::setUnscaledDimensions(ENTITY_ITEM_DEFAULT_DIMENSIONS);
}

std::shared_ptr<NetworkMaterial> MaterialEntityItem::getMaterial() const {
    auto material = _parsedMaterials.networkMaterials.find(_currentMaterialName);
    if (material != _parsedMaterials.networkMaterials.end()) {
        return material->second;
    } else {
        return nullptr;
    }
}

void MaterialEntityItem::setMaterialURL(const QString& materialURLString, bool materialDataChanged) {
    bool usingMaterialData = materialDataChanged || materialURLString.startsWith("materialData");
    if (_materialURL != materialURLString || (usingMaterialData && materialDataChanged)) {
        removeMaterial();
        _materialURL = materialURLString;

        if (materialURLString.contains("?")) {
            auto split = materialURLString.split("?");
            _currentMaterialName = split.last().toStdString();
        }

        if (usingMaterialData) {
            _parsedMaterials = NetworkMaterialResource::parseJSONMaterials(QJsonDocument::fromJson(getMaterialData().toUtf8()), materialURLString);

            // Since our material changed, the current name might not be valid anymore, so we need to update
            setCurrentMaterialName(_currentMaterialName);
            applyMaterial();
        } else {
            _networkMaterial = MaterialCache::instance().getMaterial(materialURLString);
            auto onMaterialRequestFinished = [&](bool success) {
                if (success) {
                    _parsedMaterials = _networkMaterial->parsedMaterials;

                    setCurrentMaterialName(_currentMaterialName);
                    applyMaterial();
                }
            };
            if (_networkMaterial) {
                if (_networkMaterial->isLoaded()) {
                    onMaterialRequestFinished(!_networkMaterial->isFailed());
                } else {
                    connect(_networkMaterial.data(), &Resource::finished, this, onMaterialRequestFinished);
                }
            }
        }
    }
}

void MaterialEntityItem::setCurrentMaterialName(const std::string& currentMaterialName) {
    if (_parsedMaterials.networkMaterials.find(currentMaterialName) != _parsedMaterials.networkMaterials.end()) {
        _currentMaterialName = currentMaterialName;
    } else if (_parsedMaterials.names.size() > 0) {
        _currentMaterialName = _parsedMaterials.names[0];
    }
}

void MaterialEntityItem::setMaterialData(const QString& materialData) {
    if (_materialData != materialData) {
        _materialData = materialData;
        if (_materialURL.startsWith("materialData")) {
            // Trigger material update when material data changes
            setMaterialURL(_materialURL, true);
        }
    }
}

void MaterialEntityItem::setMaterialMappingPos(const glm::vec2& materialMappingPos) {
    if (_materialMappingPos != materialMappingPos) {
        removeMaterial();
        _materialMappingPos = materialMappingPos;
        applyMaterial();
    }
}

void MaterialEntityItem::setMaterialMappingScale(const glm::vec2& materialMappingScale) {
    if (_materialMappingScale != materialMappingScale) {
        removeMaterial();
        _materialMappingScale = materialMappingScale;
        applyMaterial();
    }
}

void MaterialEntityItem::setMaterialMappingRot(const float& materialMappingRot) {
    if (_materialMappingRot != materialMappingRot) {
        removeMaterial();
        _materialMappingRot = materialMappingRot;
        applyMaterial();
    }
}

void MaterialEntityItem::setPriority(quint16 priority) {
    if (_priority != priority) {
        removeMaterial();
        _priority = priority;
        applyMaterial();
    }
}

void MaterialEntityItem::setParentMaterialName(const QString& parentMaterialName) {
    if (_parentMaterialName != parentMaterialName) {
        removeMaterial();
        _parentMaterialName = parentMaterialName;
        applyMaterial();
    }
}

void MaterialEntityItem::setParentID(const QUuid& parentID) {
    if (getParentID() != parentID) {
        removeMaterial();
        EntityItem::setParentID(parentID);
        applyMaterial();
    }
}

void MaterialEntityItem::removeMaterial() {
    graphics::MaterialPointer material = getMaterial();
    if (!material) {
        return;
    }
    QUuid parentID = getParentID();
    if (parentID.isNull()) {
        return;
    }

    // Our parent could be an entity, an avatar, or an overlay
    if (EntityTree::removeMaterialFromEntity(parentID, material, getParentMaterialName().toStdString())) {
        return;
    }

    if (EntityTree::removeMaterialFromAvatar(parentID, material, getParentMaterialName().toStdString())) {
        return;
    }

    if (EntityTree::removeMaterialFromOverlay(parentID, material, getParentMaterialName().toStdString())) {
        return;
    }

    // if a remove fails, our parent is gone, so we don't need to retry
}

void MaterialEntityItem::applyMaterial() {
    _retryApply = false;
    graphics::MaterialPointer material = getMaterial();
    QUuid parentID = getParentID();
    if (!material || parentID.isNull()) {
        return;
    }
    Transform textureTransform;
    textureTransform.setTranslation(glm::vec3(_materialMappingPos, 0.0f));
    textureTransform.setRotation(glm::vec3(0.0f, 0.0f, glm::radians(_materialMappingRot)));
    textureTransform.setScale(glm::vec3(_materialMappingScale, 1.0f));
    material->setTextureTransforms(textureTransform);

    graphics::MaterialLayer materialLayer = graphics::MaterialLayer(material, getPriority());

    // Our parent could be an entity, an avatar, or an overlay
    if (EntityTree::addMaterialToEntity(parentID, materialLayer, getParentMaterialName().toStdString())) {
        return;
    }

    if (EntityTree::addMaterialToAvatar(parentID, materialLayer, getParentMaterialName().toStdString())) {
        return;
    }

    if (EntityTree::addMaterialToOverlay(parentID, materialLayer, getParentMaterialName().toStdString())) {
        return;
    }

    // if we've reached this point, we couldn't find our parent, so we need to try again later
    _retryApply = true;
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

void MaterialEntityItem::postParentFixup() {
    removeMaterial();
    _queryAACubeSet = false; // force an update so we contain our parent
    updateQueryAACube();
    applyMaterial();
}

void MaterialEntityItem::update(const quint64& now) {
    if (_retryApply) {
        applyMaterial();
    }

    EntityItem::update(now);
}
