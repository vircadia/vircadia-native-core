//
//  ZoneEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ZoneEntityItem.h"

bool ZoneEntityItem::_zonesArePickable = false;
bool ZoneEntityItem::_drawZoneBoundaries = false;


const ShapeType ZoneEntityItem::DEFAULT_SHAPE_TYPE = SHAPE_TYPE_BOX;
const QString ZoneEntityItem::DEFAULT_COMPOUND_SHAPE_URL = "";
const bool ZoneEntityItem::DEFAULT_FLYING_ALLOWED = true;
const bool ZoneEntityItem::DEFAULT_GHOSTING_ALLOWED = true;


EntityItemPointer ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new ZoneEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Zone;

    _shapeType = DEFAULT_SHAPE_TYPE;
    _compoundShapeURL = DEFAULT_COMPOUND_SHAPE_URL;

    _backgroundMode = BACKGROUND_MODE_INHERIT;
}

EntityItemProperties ZoneEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    
    _keyLightProperties.getProperties(properties);
    
    _stageProperties.getProperties(properties);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundMode, getBackgroundMode);

    _skyboxProperties.getProperties(properties);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(flyingAllowed, getFlyingAllowed);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ghostingAllowed, getGhostingAllowed);

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    bool somethingChangedInKeyLight = _keyLightProperties.setProperties(properties);
    
    bool somethingChangedInStage = _stageProperties.setProperties(properties);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, setShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundMode, setBackgroundMode);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(flyingAllowed, setFlyingAllowed);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ghostingAllowed, setGhostingAllowed);

    bool somethingChangedInSkybox = _skyboxProperties.setProperties(properties);

    somethingChanged = somethingChanged  || somethingChangedInKeyLight || somethingChangedInStage || somethingChangedInSkybox;

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ZoneEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

int ZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    int bytesFromKeylight = _keyLightProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                                                 propertyFlags, overwriteLocalData, somethingChanged);
    
    bytesRead += bytesFromKeylight;
    dataAt += bytesFromKeylight;

    int bytesFromStage = _stageProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                               propertyFlags, overwriteLocalData, somethingChanged);
    
    bytesRead += bytesFromStage;
    dataAt += bytesFromStage;

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);
    READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundMode);

    int bytesFromSkybox = _skyboxProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                           propertyFlags, overwriteLocalData, somethingChanged);
    bytesRead += bytesFromSkybox;
    dataAt += bytesFromSkybox;

    READ_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, bool, setFlyingAllowed);
    READ_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, bool, setGhostingAllowed);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += _keyLightProperties.getEntityProperties(params);

    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_BACKGROUND_MODE;
    requestedProperties += _stageProperties.getEntityProperties(params);
    requestedProperties += _skyboxProperties.getEntityProperties(params);

    requestedProperties += PROP_FLYING_ALLOWED;
    requestedProperties += PROP_GHOSTING_ALLOWED;

    return requestedProperties;
}

void ZoneEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    _keyLightProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                         propertyFlags, propertiesDidntFit, propertyCount, appendState);

    _stageProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);


    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURL());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, (uint32_t)getBackgroundMode()); // could this be a uint16??

    _skyboxProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, getFlyingAllowed());
    APPEND_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, getGhostingAllowed());
}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   ZoneEntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "                dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "             getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "               _backgroundMode:" << EntityItemProperties::getBackgroundModeString(_backgroundMode);

    _keyLightProperties.debugDump();
    _stageProperties.debugDump();
    _skyboxProperties.debugDump();
}

ShapeType ZoneEntityItem::getShapeType() const {
    // Zones are not allowed to have a SHAPE_TYPE_NONE... they are always at least a SHAPE_TYPE_BOX
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURL() ? SHAPE_TYPE_COMPOUND : DEFAULT_SHAPE_TYPE;
    } else {
        return _shapeType == SHAPE_TYPE_NONE ? DEFAULT_SHAPE_TYPE : _shapeType;
    }
}

void ZoneEntityItem::setCompoundShapeURL(const QString& url) {
    _compoundShapeURL = url;
    if (_compoundShapeURL.isEmpty() && _shapeType == SHAPE_TYPE_COMPOUND) {
        _shapeType = DEFAULT_SHAPE_TYPE;
    }
}

bool ZoneEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance, 
                         BoxFace& face, glm::vec3& surfaceNormal,
                         void** intersectedObject, bool precisionPicking) const {

    return _zonesArePickable;
}
