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

EntityItemProperties MaterialEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialURL, getMaterialURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialMode, getMaterialMode);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(priority, getPriority);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(parentMaterialID, getParentMaterialID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialPos, getMaterialPos);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialScale, getMaterialScale);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialRot, getMaterialRot);
    return properties;
}

bool MaterialEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialURL, setMaterialURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMode, setMaterialMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(priority, setPriority);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentMaterialID, setParentMaterialID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialPos, setMaterialPos);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialScale, setMaterialScale);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialRot, setMaterialRot);

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
    READ_ENTITY_PROPERTY(PROP_MATERIAL_TYPE, MaterialMode, setMaterialMode);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, quint16, setPriority);
    READ_ENTITY_PROPERTY(PROP_PARENT_MATERIAL_ID, QString, setParentMaterialID);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_POS, glm::vec2, setMaterialPos);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_SCALE, glm::vec2, setMaterialScale);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_ROT, float, setMaterialRot);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags MaterialEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_MATERIAL_URL;
    requestedProperties += PROP_MATERIAL_TYPE;
    requestedProperties += PROP_MATERIAL_PRIORITY;
    requestedProperties += PROP_PARENT_MATERIAL_ID;
    requestedProperties += PROP_MATERIAL_POS;
    requestedProperties += PROP_MATERIAL_SCALE;
    requestedProperties += PROP_MATERIAL_ROT;
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
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_TYPE, (uint32_t)getMaterialMode());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, getPriority());
    APPEND_ENTITY_PROPERTY(PROP_PARENT_MATERIAL_ID, getParentMaterialID());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_POS, getMaterialPos());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_SCALE, getMaterialScale());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_ROT, getMaterialRot());
}

void MaterialEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << " MATERIAL EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                   name:" << _name;
    qCDebug(entities) << "          material json:" << _materialURL;
    qCDebug(entities) << "  current material name:" << _currentMaterialName;
    qCDebug(entities) << "          material type:" << _materialMode;
    qCDebug(entities) << "               priority:" << _priority;
    qCDebug(entities) << "     parent material ID:" << _parentMaterialID;
    qCDebug(entities) << "           material pos:" << _materialPos;
    qCDebug(entities) << "         material scale:" << _materialRot;
    qCDebug(entities) << "           material rot:" << _materialScale;
    qCDebug(entities) << "               position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "             dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "          getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "MATERIAL EntityItem Ptr:" << this;
}

void MaterialEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    EntityItem::setUnscaledDimensions(ENTITY_ITEM_DEFAULT_DIMENSIONS);
}

std::shared_ptr<NetworkMaterial> MaterialEntityItem::getMaterial() const {
    auto material = _materials.find(_currentMaterialName);
    if (material != _materials.end()) {
        return material.value();
    } else {
        return nullptr;
    }
}

void MaterialEntityItem::setMaterialURL(const QString& materialURLString, bool userDataChanged) {
    bool usingUserData = materialURLString.startsWith("userData");
    if (_materialURL != materialURLString || (usingUserData && userDataChanged)) {
        removeMaterial();
        _materialURL = materialURLString;

        // TODO: if URL ends with ?string, try to set _currentMaterialName = string

        if (usingUserData) {
            QJsonDocument materialJSON = QJsonDocument::fromJson(getUserData().toUtf8());
            _materials.clear();
            _materialNames.clear();
            if (!materialJSON.isNull()) {
                if (materialJSON.isArray()) {
                    QJsonArray materials = materialJSON.array();
                    for (auto material : materials) {
                        if (!material.isNull() && material.isObject()) {
                            auto networkMaterial = NetworkMaterialResource::parseJSONMaterial(material.toObject());
                            _materials[networkMaterial.first] = networkMaterial.second;
                            _materialNames.push_back(networkMaterial.first);
                        }
                    }
                } else if (materialJSON.isObject()) {
                    auto networkMaterial = NetworkMaterialResource::parseJSONMaterial(materialJSON.object());
                    _materials[networkMaterial.first] = networkMaterial.second;
                    _materialNames.push_back(networkMaterial.first);
                }
            }
            // Since our material changed, the current name might not be valid anymore, so we need to update
            setCurrentMaterialName(_currentMaterialName);
            applyMaterial();
        } else {
            _networkMaterial = MaterialCache::instance().getMaterial(materialURLString);
            auto onMaterialRequestFinished = [&](bool success) {
                if (success) {
                    _materials[_networkMaterial->name] = _networkMaterial->networkMaterial;
                    _materialNames.push_back(_networkMaterial->name);

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

void MaterialEntityItem::setCurrentMaterialName(const QString& currentMaterialName) {
    auto material = _materials.find(currentMaterialName);
    if (material != _materials.end()) {
        _currentMaterialName = currentMaterialName;
    } else if (_materialNames.size() > 0) {
        _currentMaterialName = _materialNames[0];
    }
}

void MaterialEntityItem::setUserData(const QString& userData) {
    if (_userData != userData) {
        EntityItem::setUserData(userData);
        if (_materialURL.startsWith("userData")) {
            // Trigger material update when user data changes
            setMaterialURL(_materialURL, true);
        }
    }
}

void MaterialEntityItem::setMaterialPos(const glm::vec2& materialPos) {
    if (_materialPos != materialPos) {
        removeMaterial();
        _materialPos = materialPos;
        applyMaterial();
    }
}

void MaterialEntityItem::setMaterialScale(const glm::vec2& materialScale) {
    if (_materialScale != materialScale) {
        removeMaterial();
        _materialScale = materialScale;
        applyMaterial();
    }
}

void MaterialEntityItem::setMaterialRot(const float& materialRot) {
    if (_materialRot != materialRot) {
        removeMaterial();
        _materialRot = materialRot;
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

void MaterialEntityItem::setParentMaterialID(const QString& parentMaterialID) {
    if (_parentMaterialID != parentMaterialID) {
        removeMaterial();
        _parentMaterialID = parentMaterialID;
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

void MaterialEntityItem::setClientOnly(bool clientOnly) {
    if (getClientOnly() != clientOnly) {
        removeMaterial();
        EntityItem::setClientOnly(clientOnly);
        applyMaterial();
    }
}

void MaterialEntityItem::setOwningAvatarID(const QUuid& owningAvatarID) {
    if (getOwningAvatarID() != owningAvatarID) {
        removeMaterial();
        EntityItem::setOwningAvatarID(owningAvatarID);
        applyMaterial();
    }
}

void MaterialEntityItem::removeMaterial() {
    graphics::MaterialPointer material = getMaterial();
    QUuid parentID = getClientOnly() ? getOwningAvatarID() : getParentID();
    if (!material || parentID.isNull()) {
        return;
    }

    // Our parent could be an entity, an avatar, or an overlay
    EntityTreePointer tree = getTree();
    if (tree) {
        EntityItemPointer entity = tree->findEntityByEntityItemID(parentID);
        if (entity) {
            entity->removeMaterial(material, getParentMaterialID());
            return;
        }
    }

    if (EntityTree::removeMaterialFromAvatar(parentID, material, getParentMaterialID())) {
        return;
    }

    if (EntityTree::removeMaterialFromOverlay(parentID, material, getParentMaterialID())) {
        return;
    }

    // if a remove fails, our parent is gone, so we don't need to retry
}

void MaterialEntityItem::applyMaterial() {
    _retryApply = false;
    graphics::MaterialPointer material = getMaterial();
    QUuid parentID = getClientOnly() ? getOwningAvatarID() : getParentID();
    if (!material || parentID.isNull()) {
        return;
    }
    Transform textureTransform;
    textureTransform.setTranslation(glm::vec3(_materialPos, 0));
    textureTransform.setRotation(glm::vec3(0, 0, glm::radians(_materialRot)));
    textureTransform.setScale(glm::vec3(_materialScale, 1));
    material->setTextureTransforms(textureTransform);
    material->setPriority(getPriority());

    // Our parent could be an entity, an avatar, or an overlay
    EntityTreePointer tree = getTree();
    if (tree) {
        EntityItemPointer entity = tree->findEntityByEntityItemID(parentID);
        if (entity) {
            entity->addMaterial(material, getParentMaterialID());
            return;
        }
    }

    if (EntityTree::addMaterialToAvatar(parentID, material, getParentMaterialID())) {
        return;
    }

    if (EntityTree::addMaterialToOverlay(parentID, material, getParentMaterialID())) {
        return;
    }

    // if we've reached this point, we couldn't find our parent, so we need to try again later
    _retryApply = true;
}

void MaterialEntityItem::postAdd() {
    removeMaterial();
    applyMaterial();
}

void MaterialEntityItem::preDelete() {
    EntityItem::preDelete();
    removeMaterial();
}

void MaterialEntityItem::postParentFixup() {
    removeMaterial();
    applyMaterial();
}

void MaterialEntityItem::update(const quint64& now) {
    if (_retryApply) {
        applyMaterial();
    }

    EntityItem::update(now);
}