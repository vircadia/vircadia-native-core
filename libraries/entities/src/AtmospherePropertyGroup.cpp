//
//  AtmospherePropertyGroup.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <OctreePacketData.h>

#include "AtmospherePropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

AtmospherePropertyGroup::AtmospherePropertyGroup() {
    _center = glm::vec3(0.0f);
    _innerRadius = 0.0f;
    _outerRadius = 0.0f;
    _mieScattering = 0.0f;
    _rayleighScattering = 0.0f;
    _scatteringWavelengths = glm::vec3(0.0f);
    _hasStars = true;
}

void AtmospherePropertyGroup::copyToScriptValue(QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_VEC3(Atmosphere, Center, center);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Atmosphere, InnerRadius, innerRadius);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Atmosphere, OuterRadius, outerRadius);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Atmosphere, MieScattering, mieScattering);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Atmosphere, RayleighScattering, rayleighScattering);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_VEC3(Atmosphere, ScatteringWavelengths, scatteringWavelengths);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(Atmosphere, HasStars, hasStars);
}

void AtmospherePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_VEC3(atmosphere, center, setCenter);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphere, innerRadius, setInnerRadius);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphere, outerRadius, setOuterRadius);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphere, mieScattering, setMieScattering);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphere, rayleighScattering, setRayleighScattering);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_VEC3(atmosphere, scatteringWavelengths, setScatteringWavelengths);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_BOOL(atmosphere, hasStars, setHasStars);
}

void AtmospherePropertyGroup::debugDump() const {
    qDebug() << "   AtmospherePropertyGroup: ---------------------------------------------";
    qDebug() << "       Center:" << getCenter() << " has changed:" << centerChanged();
    qDebug() << "       Inner Radius:" << getInnerRadius() << " has changed:" << innerRadiusChanged();
    qDebug() << "       Outer Radius:" << getOuterRadius() << " has changed:" << outerRadiusChanged();
    qDebug() << "       Mie Scattering:" << getMieScattering() << " has changed:" << mieScatteringChanged();
    qDebug() << "       Rayleigh Scattering:" << getRayleighScattering() << " has changed:" << rayleighScatteringChanged();
    qDebug() << "       Scattering Wavelengths:" << getScatteringWavelengths() << " has changed:" << scatteringWavelengthsChanged();
    qDebug() << "       Has Stars:" << getHasStars() << " has changed:" << hasStarsChanged();
}

bool AtmospherePropertyGroup::appentToEditPacket(OctreePacketData* packetData,                                     
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, appendValue, getCenter());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, appendValue, getInnerRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, appendValue, getOuterRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, appendValue, getMieScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, appendValue, getRayleighScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, appendValue, getScatteringWavelengths());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, appendValue, getHasStars());

    return true;
}


bool AtmospherePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;

    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, glm::vec3, _center);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, float, _innerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, float, _outerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, float, _mieScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, float, _rayleighScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, glm::vec3, _scatteringWavelengths);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, bool, _hasStars);
    
    processedBytes += bytesRead;

    return true;
}

void AtmospherePropertyGroup::markAllChanged() {
    _centerChanged = true;
    _innerRadiusChanged = true;
    _outerRadiusChanged = true;
    _mieScatteringChanged = true;
    _rayleighScatteringChanged = true;
    _scatteringWavelengthsChanged = true;
    _hasStarsChanged = true;
}

EntityPropertyFlags AtmospherePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_CENTER, center);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_INNER_RADIUS, innerRadius);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_OUTER_RADIUS, outerRadius);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_MIE_SCATTERING, mieScattering);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, rayleighScattering);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, scatteringWavelengths);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_HAS_STARS, hasStars);

    return changedProperties;
}

void AtmospherePropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, Center, getCenter);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, InnerRadius, getInnerRadius);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, OuterRadius, getOuterRadius);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, MieScattering, getMieScattering);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, MieScattering, getMieScattering);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, RayleighScattering, getRayleighScattering);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, ScatteringWavelengths, getScatteringWavelengths);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Atmosphere, HasStars, getHasStars);
}

bool AtmospherePropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, Center, center, setCenter);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, InnerRadius, innerRadius, setInnerRadius);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, OuterRadius, outerRadius, setOuterRadius);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, MieScattering, mieScattering, setMieScattering);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, RayleighScattering, rayleighScattering, setRayleighScattering);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, ScatteringWavelengths, scatteringWavelengths, setScatteringWavelengths);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Atmosphere, HasStars, hasStars, setHasStars);

    return somethingChanged;
}

EntityPropertyFlags AtmospherePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_ATMOSPHERE_CENTER;
    requestedProperties += PROP_ATMOSPHERE_INNER_RADIUS;
    requestedProperties += PROP_ATMOSPHERE_OUTER_RADIUS;
    requestedProperties += PROP_ATMOSPHERE_MIE_SCATTERING;
    requestedProperties += PROP_ATMOSPHERE_RAYLEIGH_SCATTERING;
    requestedProperties += PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS;
    requestedProperties += PROP_ATMOSPHERE_HAS_STARS;
    
    return requestedProperties;
}
    
void AtmospherePropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, 
                                OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, appendValue, getCenter());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, appendValue, getInnerRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, appendValue, getOuterRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, appendValue, getMieScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, appendValue, getRayleighScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, appendValue, getScatteringWavelengths());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, appendValue, getHasStars());
}

int AtmospherePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, glm::vec3, _center);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, float, _innerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, float, _outerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, float, _mieScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, float, _rayleighScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, glm::vec3, _scatteringWavelengths);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, bool, _hasStars);

    return bytesRead;
}
