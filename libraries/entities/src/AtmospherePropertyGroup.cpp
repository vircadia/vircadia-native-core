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
    const glm::vec3 DEFAULT_CENTER = glm::vec3(0.0f, -1000.0f, 0.0f);
    const float DEFAULT_INNER_RADIUS = 1000.0f;
    const float DEFAULT_OUTER_RADIUS = 1025.0f;
    const float DEFAULT_RAYLEIGH_SCATTERING = 0.0025f;
    const float DEFAULT_MIE_SCATTERING = 0.0010f;
    const glm::vec3 DEFAULT_SCATTERING_WAVELENGTHS = glm::vec3(0.650f, 0.570f, 0.475f);

    _center = DEFAULT_CENTER;
    _innerRadius = DEFAULT_INNER_RADIUS;
    _outerRadius = DEFAULT_OUTER_RADIUS;
    _mieScattering = DEFAULT_MIE_SCATTERING;
    _rayleighScattering = DEFAULT_RAYLEIGH_SCATTERING;
    _scatteringWavelengths = DEFAULT_SCATTERING_WAVELENGTHS;
    _hasStars = true;
}

void AtmospherePropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_CENTER, Atmosphere, atmosphere, Center, center);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_INNER_RADIUS, Atmosphere, atmosphere, InnerRadius, innerRadius);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_OUTER_RADIUS, Atmosphere, atmosphere, OuterRadius, outerRadius);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_MIE_SCATTERING, Atmosphere, atmosphere, MieScattering, mieScattering);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, Atmosphere, atmosphere, RayleighScattering, rayleighScattering);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, Atmosphere, atmosphere, ScatteringWavelengths, scatteringWavelengths);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ATMOSPHERE_HAS_STARS, Atmosphere, atmosphere, HasStars, hasStars);
}

void AtmospherePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, center, glmVec3, setCenter);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, innerRadius, float, setInnerRadius);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, outerRadius, float, setOuterRadius);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, mieScattering, float, setMieScattering);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, rayleighScattering, float, setRayleighScattering);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, scatteringWavelengths, glmVec3, setScatteringWavelengths);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(atmosphere, hasStars, bool, setHasStars);
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

bool AtmospherePropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, getCenter());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, getInnerRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, getOuterRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, getMieScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, getRayleighScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, getScatteringWavelengths());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, getHasStars());

    return true;
}


bool AtmospherePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, glm::vec3, setCenter);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, float, setInnerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, float, setOuterRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, float, setMieScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, float, setRayleighScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, glm::vec3, setScatteringWavelengths);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, bool, setHasStars);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_CENTER, Center);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_INNER_RADIUS, InnerRadius);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_OUTER_RADIUS, OuterRadius);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_MIE_SCATTERING, MieScattering);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, RayleighScattering);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, ScatteringWavelengths);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ATMOSPHERE_HAS_STARS, HasStars);
    
    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

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

    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, getCenter());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, getInnerRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, getOuterRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, getMieScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, getRayleighScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, getScatteringWavelengths());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, getHasStars());
}

int AtmospherePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, glm::vec3, setCenter);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, float, setInnerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, float, setOuterRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, float, setMieScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, float, setRayleighScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, glm::vec3, setScatteringWavelengths);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, bool, setHasStars);

    return bytesRead;
}
