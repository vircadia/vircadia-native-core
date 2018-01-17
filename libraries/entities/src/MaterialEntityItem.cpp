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
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(blendFactor, getBlendFactor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(priority, getPriority);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeID, getShapeID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(materialBounds, getMaterialBounds);
    return properties;
}

bool MaterialEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialURL, setMaterialURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialMode, setMaterialMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(blendFactor, setBlendFactor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(priority, setPriority);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeID, setShapeID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(materialBounds, setMaterialBounds);

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

bool MaterialEntityItem::parseJSONColor(const QJsonValue& array, glm::vec3& color, bool& isSRGB) {
    if (array.isArray()) {
        QJsonArray colorArray = array.toArray();
        if (colorArray.size() >= 3 && colorArray[0].isDouble() && colorArray[1].isDouble() && colorArray[2].isDouble()) {
            isSRGB = true;
            if (colorArray.size() >= 4) {
                if (colorArray[3].isBool()) {
                    isSRGB = colorArray[3].toBool();
                }
            }
            color = glm::vec3(colorArray[0].toDouble(), colorArray[1].toDouble(), colorArray[2].toDouble());
            return true;
        }
    }
    return false;
}

void MaterialEntityItem::parseJSONMaterial(const QJsonObject& materialJSON) {
    QString name = "";
    std::shared_ptr<NetworkMaterial> material = std::make_shared<NetworkMaterial>();
    for (auto& key : materialJSON.keys()) {
        if (key == "name") {
            auto nameJSON = materialJSON.value(key);
            if (nameJSON.isString()) {
                name = nameJSON.toString();
            }
        } else if (key == "emissive") {
            glm::vec3 color;
            bool isSRGB;
            bool valid = parseJSONColor(materialJSON.value(key), color, isSRGB);
            if (valid) {
                material->setEmissive(color, isSRGB);
            }
        } else if (key == "opacity") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setOpacity(value.toDouble());
            }
        } else if (key == "albedo") {
            glm::vec3 color;
            bool isSRGB;
            bool valid = parseJSONColor(materialJSON.value(key), color, isSRGB);
            if (valid) {
                material->setAlbedo(color, isSRGB);
            }
        } else if (key == "roughness") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setRoughness(value.toDouble());
            }
        } else if (key == "fresnel") {
            glm::vec3 color;
            bool isSRGB;
            bool valid = parseJSONColor(materialJSON.value(key), color, isSRGB);
            if (valid) {
                material->setFresnel(color, isSRGB);
            }
        } else if (key == "metallic") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setMetallic(value.toDouble());
            }
        } else if (key == "scattering") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setScattering(value.toDouble());
            }
        } else if (key == "emissiveMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setEmissiveMap(value.toString());
            }
        } else if (key == "albedoMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                bool useAlphaChannel = false;
                auto opacityMap = materialJSON.find("opacityMap");
                if (opacityMap != materialJSON.end() && opacityMap->isString() && opacityMap->toString() == value.toString()) {
                    useAlphaChannel = true;
                }
                material->setAlbedoMap(value.toString(), useAlphaChannel);
            }
        } else if (key == "roughnessMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setRoughnessMap(value.toString(), false);
            }
        } else if (key == "glossMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setRoughnessMap(value.toString(), true);
            }
        } else if (key == "metallicMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setMetallicMap(value.toString(), false);
            }
        } else if (key == "specularMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setMetallicMap(value.toString(), true);
            }
        } else if (key == "normalMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setNormalMap(value.toString(), false);
            }
        } else if (key == "bumpMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setNormalMap(value.toString(), true);
            }
        } else if (key == "occlusionMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setOcclusionMap(value.toString());
            }
        } else if (key == "scatteringMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setScatteringMap(value.toString());
            }
        } else if (key == "lightMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setLightmapMap(value.toString());
            }
        }
    }
    _materials[name] = material;
    _materialNames.push_back(name);
}

int MaterialEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_MATERIAL_URL, QString, setMaterialURL);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_TYPE, MaterialMode, setMaterialMode);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_BLEND_FACTOR, float, setBlendFactor);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, uint32_t, setPriority);
    READ_ENTITY_PROPERTY(PROP_PARENT_SHAPE_ID, uint32_t, setShapeID);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_BOUNDS, glm::vec4, setMaterialBounds);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags MaterialEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_MATERIAL_URL;
    requestedProperties += PROP_MATERIAL_TYPE;
    requestedProperties += PROP_MATERIAL_BLEND_FACTOR;
    requestedProperties += PROP_MATERIAL_PRIORITY;
    requestedProperties += PROP_PARENT_SHAPE_ID;
    requestedProperties += PROP_MATERIAL_BOUNDS;
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
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_BLEND_FACTOR, getBlendFactor());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, (uint32_t)getPriority());
    APPEND_ENTITY_PROPERTY(PROP_PARENT_SHAPE_ID, (uint32_t)getShapeID());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_BOUNDS, getMaterialBounds());
}

void MaterialEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << " MATERIAL EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                   name:" << _name;
    qCDebug(entities) << "          material json:" << _materialURL;
    qCDebug(entities) << "  current material name:" << _currentMaterialName;
    qCDebug(entities) << "          material type:" << _materialMode;
    qCDebug(entities) << "           blend factor:" << _blendFactor;
    qCDebug(entities) << "               priority:" << _priority;
    qCDebug(entities) << "        parent shape ID:" << _shapeID;
    qCDebug(entities) << "        material bounds:" << _materialBounds;
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

void MaterialEntityItem::setMaterialURL(const QString& materialURLString) {
    if (materialURLString.startsWith("userData")) {
        QJsonDocument materialJSON = QJsonDocument::fromJson(getUserData().toUtf8());
        _materials.clear();
        _materialNames.clear();
        if (!materialJSON.isNull()) {
            if (materialJSON.isArray()) {
                QJsonArray materials = materialJSON.array();
                for (auto& material : materials) {
                    if (!material.isNull() && material.isObject()) {
                        parseJSONMaterial(material.toObject());
                    }
                }
            } else if (materialJSON.isObject()) {
                parseJSONMaterial(materialJSON.object());
            }
        }
    }
    _materialURL = materialURLString;

    // TODO: if URL ends with ?string, set _currentMaterialName = string

    // Since our JSON changed, the current name might not be valid anymore, so we need to update
    setCurrentMaterialName(_currentMaterialName);
}

void MaterialEntityItem::setCurrentMaterialName(const QString& currentMaterialName) {
    auto material = _materials.find(currentMaterialName);
    if (material != _materials.end()) {
        _currentMaterialName = currentMaterialName;
    } else if (_materialNames.size() > 0) {
        setCurrentMaterialName(_materialNames[0]);
    }
}

void MaterialEntityItem::setUserData(const QString& userData) {
    EntityItem::setUserData(userData);
    if (_materialURL.startsWith("userData")) {
        // Trigger material update when user data changes
        setMaterialURL(_materialURL);
    }
}