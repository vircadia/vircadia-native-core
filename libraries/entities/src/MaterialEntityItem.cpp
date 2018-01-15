//
//  Created by Sam Gondelman on 1/12/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MaterialEntityItem.h"

#include "EntityItemProperties.h"

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

int MaterialEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_MATERIAL_URL, QString, setMaterialURL);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_TYPE, MaterialMode, setMaterialMode);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_BLEND_FACTOR, float, setBlendFactor);
    READ_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, int, setPriority);
    READ_ENTITY_PROPERTY(PROP_PARENT_SHAPE_ID, int, setShapeID);
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
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_TYPE, getMaterialMode());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_BLEND_FACTOR, getBlendFactor());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, getPriority());
    APPEND_ENTITY_PROPERTY(PROP_PARENT_SHAPE_ID, getShapeID());
    APPEND_ENTITY_PROPERTY(PROP_MATERIAL_BOUNDS, getMaterialBounds());
}

void MaterialEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "MATERIAL EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  name:" << _name;
    qCDebug(entities) << "                   url:" << _materialURL;
    qCDebug(entities) << "         material type:" << _materialMode;
    qCDebug(entities) << "          blend factor:" << _blendFactor;
    qCDebug(entities) << "              priority:" << _priority;
    qCDebug(entities) << "       parent shape ID:" << _shapeID;
    qCDebug(entities) << "       material bounds:" << _materialBounds;
    qCDebug(entities) << "              position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "            dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "         getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) <<   "SHAPE EntityItem Ptr:" << this;
}
