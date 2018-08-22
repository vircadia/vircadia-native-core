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

#include "ZoneEntityItem.h"

#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntityEditFilters.h"

bool ZoneEntityItem::_zonesArePickable = false;
bool ZoneEntityItem::_drawZoneBoundaries = false;


const ShapeType ZoneEntityItem::DEFAULT_SHAPE_TYPE = SHAPE_TYPE_BOX;
const QString ZoneEntityItem::DEFAULT_COMPOUND_SHAPE_URL = "";
const bool ZoneEntityItem::DEFAULT_FLYING_ALLOWED = true;
const bool ZoneEntityItem::DEFAULT_GHOSTING_ALLOWED = true;
const QString ZoneEntityItem::DEFAULT_FILTER_URL = "";

EntityItemPointer ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new ZoneEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Zone;

    _shapeType = DEFAULT_SHAPE_TYPE;
    _compoundShapeURL = DEFAULT_COMPOUND_SHAPE_URL;
}

EntityItemProperties ZoneEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    // Contains a QString property, must be synchronized
    withReadLock([&] {
        _keyLightProperties.getProperties(properties);
    });

    withReadLock([&] {
        _ambientLightProperties.getProperties(properties);
    });

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);

    // Contains a QString property, must be synchronized
    withReadLock([&] {
        _skyboxProperties.getProperties(properties);
    });

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(flyingAllowed, getFlyingAllowed);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ghostingAllowed, getGhostingAllowed);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(filterURL, getFilterURL);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(hazeMode, getHazeMode);
    _hazeProperties.getProperties(properties);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightMode, getKeyLightMode);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ambientLightMode, getAmbientLightMode);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(skyboxMode, getSkyboxMode);

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

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

bool ZoneEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setSubClassProperties(properties); // set the properties in our base class

    // Contains a QString property, must be synchronized
    withWriteLock([&] {
        _keyLightPropertiesChanged = _keyLightProperties.setProperties(properties);
    });
    withWriteLock([&] {
        _ambientLightPropertiesChanged = _ambientLightProperties.setProperties(properties);
    });

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, setShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);

    // Contains a QString property, must be synchronized
    withWriteLock([&] {
        _skyboxPropertiesChanged = _skyboxProperties.setProperties(properties);
    });

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(flyingAllowed, setFlyingAllowed);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ghostingAllowed, setGhostingAllowed);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(filterURL, setFilterURL);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(hazeMode, setHazeMode);
    _hazePropertiesChanged = _hazeProperties.setProperties(properties);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightMode, setKeyLightMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ambientLightMode, setAmbientLightMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(skyboxMode, setSkyboxMode);

    somethingChanged = somethingChanged || _keyLightPropertiesChanged || _ambientLightPropertiesChanged ||
        _stagePropertiesChanged || _skyboxPropertiesChanged || _hazePropertiesChanged;

    return somethingChanged;
}

int ZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    int bytesFromKeylight;
    withWriteLock([&] {
        bytesFromKeylight = _keyLightProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData, _keyLightPropertiesChanged);
    });

    somethingChanged = somethingChanged || _keyLightPropertiesChanged;
    bytesRead += bytesFromKeylight;
    dataAt += bytesFromKeylight;

    int bytesFromAmbientlight;
    withWriteLock([&] {
        bytesFromAmbientlight = _ambientLightProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData, _ambientLightPropertiesChanged);
    });

    somethingChanged = somethingChanged || _ambientLightPropertiesChanged;
    bytesRead += bytesFromAmbientlight;
    dataAt += bytesFromAmbientlight;

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);
    READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);

    int bytesFromSkybox;
    withWriteLock([&] {
        bytesFromSkybox = _skyboxProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData, _skyboxPropertiesChanged);
    });
    somethingChanged = somethingChanged || _skyboxPropertiesChanged;
    bytesRead += bytesFromSkybox;
    dataAt += bytesFromSkybox;

    READ_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, bool, setFlyingAllowed);
    READ_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, bool, setGhostingAllowed);
    READ_ENTITY_PROPERTY(PROP_FILTER_URL, QString, setFilterURL);

    READ_ENTITY_PROPERTY(PROP_HAZE_MODE, uint32_t, setHazeMode);

    int bytesFromHaze = _hazeProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
        propertyFlags, overwriteLocalData, _hazePropertiesChanged);

    somethingChanged = somethingChanged || _hazePropertiesChanged;
    bytesRead += bytesFromHaze;
    dataAt += bytesFromHaze;

    READ_ENTITY_PROPERTY(PROP_KEY_LIGHT_MODE, uint32_t, setKeyLightMode);
    READ_ENTITY_PROPERTY(PROP_AMBIENT_LIGHT_MODE, uint32_t, setAmbientLightMode);
    READ_ENTITY_PROPERTY(PROP_SKYBOX_MODE, uint32_t, setSkyboxMode);

    return bytesRead;
}

EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    withReadLock([&] {
        requestedProperties += _keyLightProperties.getEntityProperties(params);
    });

    withReadLock([&] {
        requestedProperties += _ambientLightProperties.getEntityProperties(params);
    });

    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;

    withReadLock([&] {
        requestedProperties += _skyboxProperties.getEntityProperties(params);
    });

    requestedProperties += PROP_FLYING_ALLOWED;
    requestedProperties += PROP_GHOSTING_ALLOWED;
    requestedProperties += PROP_FILTER_URL;

    requestedProperties += PROP_HAZE_MODE;
    requestedProperties += _hazeProperties.getEntityProperties(params);

    requestedProperties += PROP_KEY_LIGHT_MODE;
    requestedProperties += PROP_AMBIENT_LIGHT_MODE;
    requestedProperties += PROP_SKYBOX_MODE;

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

    _ambientLightProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
        propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURL());

    _skyboxProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
        propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, getFlyingAllowed());
    APPEND_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, getGhostingAllowed());
    APPEND_ENTITY_PROPERTY(PROP_FILTER_URL, getFilterURL());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_MODE, (uint32_t)getHazeMode());
    _hazeProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
        propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_KEY_LIGHT_MODE, (uint32_t)getKeyLightMode());
    APPEND_ENTITY_PROPERTY(PROP_AMBIENT_LIGHT_MODE, (uint32_t)getAmbientLightMode());
    APPEND_ENTITY_PROPERTY(PROP_SKYBOX_MODE, (uint32_t)getSkyboxMode());
}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   ZoneEntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "                dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "             getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "               _hazeMode:" << EntityItemProperties::getComponentModeString(_hazeMode);
    qCDebug(entities) << "               _keyLightMode:" << EntityItemProperties::getComponentModeString(_keyLightMode);
    qCDebug(entities) << "               _ambientLightMode:" << EntityItemProperties::getComponentModeString(_ambientLightMode);
    qCDebug(entities) << "               _skyboxMode:" << EntityItemProperties::getComponentModeString(_skyboxMode);

    _keyLightProperties.debugDump();
    _ambientLightProperties.debugDump();
    _skyboxProperties.debugDump();
    _hazeProperties.debugDump();
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
    withWriteLock([&] {
        _compoundShapeURL = url;
        if (_compoundShapeURL.isEmpty() && _shapeType == SHAPE_TYPE_COMPOUND) {
            _shapeType = DEFAULT_SHAPE_TYPE;
        }
    });
}

bool ZoneEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         OctreeElementPointer& element, float& distance,
                         BoxFace& face, glm::vec3& surfaceNormal,
                         QVariantMap& extraInfo, bool precisionPicking) const {
    return _zonesArePickable;
}

bool ZoneEntityItem::findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                         const glm::vec3& acceleration, OctreeElementPointer& element, float& parabolicDistance,
                         BoxFace& face, glm::vec3& surfaceNormal,
                         QVariantMap& extraInfo, bool precisionPicking) const {
    return _zonesArePickable;
}

void ZoneEntityItem::setFilterURL(QString url) {
    withWriteLock([&] {
        _filterURL = url;
    });
    if (DependencyManager::isSet<EntityEditFilters>()) {
        auto entityEditFilters = DependencyManager::get<EntityEditFilters>();
        qCDebug(entities) << "adding filter " << url << "for zone" << getEntityItemID();
        entityEditFilters->addFilter(getEntityItemID(), url);
    }
}

QString ZoneEntityItem::getFilterURL() const { 
    QString result;
    withReadLock([&] {
        result = _filterURL;
    });
    return result;
}

bool ZoneEntityItem::hasCompoundShapeURL() const { 
    return !getCompoundShapeURL().isEmpty();
}

QString ZoneEntityItem::getCompoundShapeURL() const { 
    QString result;
    withReadLock([&] {
        result = _compoundShapeURL;
    });
    return result;
}

void ZoneEntityItem::resetRenderingPropertiesChanged() {
    withWriteLock([&] {
        _keyLightPropertiesChanged = false;
        _ambientLightPropertiesChanged = false;
        _skyboxPropertiesChanged = false;
        _hazePropertiesChanged = false;
        _stagePropertiesChanged = false;
    });
}

void ZoneEntityItem::setHazeMode(const uint32_t value) {
    if (value < COMPONENT_MODE_ITEM_COUNT && value != _hazeMode) {
        _hazeMode = value;
        _hazePropertiesChanged = true;
    }
}

uint32_t ZoneEntityItem::getHazeMode() const {
    return _hazeMode;
}

void ZoneEntityItem::setKeyLightMode(const uint32_t value) {
    if (value < COMPONENT_MODE_ITEM_COUNT && value != _keyLightMode) {
        _keyLightMode = value;
        _keyLightPropertiesChanged = true;
    }
}

uint32_t ZoneEntityItem::getKeyLightMode() const {
    return _keyLightMode;
}

void ZoneEntityItem::setAmbientLightMode(const uint32_t value) {
    if (value < COMPONENT_MODE_ITEM_COUNT && value != _ambientLightMode) {
        _ambientLightMode = value;
        _ambientLightPropertiesChanged = true;
    }
}

uint32_t ZoneEntityItem::getAmbientLightMode() const {
    return _ambientLightMode;
}

void ZoneEntityItem::setSkyboxMode(const uint32_t value) {
    if (value < COMPONENT_MODE_ITEM_COUNT && value != _skyboxMode) {
        _skyboxMode = value;
        _skyboxPropertiesChanged = true;
    }
}

uint32_t ZoneEntityItem::getSkyboxMode() const {
    return _skyboxMode;
}
