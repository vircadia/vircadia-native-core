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

void AtmospherePropertyGroup::copyToScriptValue(QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_VEC3(AtmosphereProperties, AtmosphereCenter, atmosphereCenter);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(AtmosphereProperties, AtmosphereInnerRadius, atmosphereInnerRadius);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(AtmosphereProperties, AtmosphereOuterRadius, atmosphereOuterRadius);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(AtmosphereProperties, AtmosphereMieScattering, atmosphereMieScattering);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(AtmosphereProperties, AtmosphereRayleighScattering, atmosphereRayleighScattering);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_VEC3(AtmosphereProperties, AtmosphereScatteringWavelengths, atmosphereScatteringWavelengths);
}

void AtmospherePropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(atmosphereCenter, setAtmosphereCenter);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphereInnerRadius, setAtmosphereInnerRadius);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphereOuterRadius, setAtmosphereOuterRadius);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphereMieScattering, setAtmosphereMieScattering);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(atmosphereRayleighScattering, setAtmosphereRayleighScattering);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(atmosphereScatteringWavelengths, setAtmosphereScatteringWavelengths);
}

void AtmospherePropertyGroup::debugDump() const {
    qDebug() << "   AtmospherePropertyGroup: ---------------------------------------------";
    qDebug() << "       Atmosphere Center:" << getAtmosphereCenter();
    qDebug() << "       Atmosphere Inner Radius:" << getAtmosphereInnerRadius();
    qDebug() << "       Atmosphere Outer Radius:" << getAtmosphereOuterRadius();
    qDebug() << "       ... more ...";
}

bool AtmospherePropertyGroup::appentToEditPacket(OctreePacketData* packetData,                                     
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, appendValue, getAtmosphereCenter());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, appendValue, getAtmosphereInnerRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, appendValue, getAtmosphereOuterRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, appendValue, getAtmosphereMieScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, appendValue, getAtmosphereRayleighScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, appendValue, getAtmosphereScatteringWavelengths());

    return true;
}


bool AtmospherePropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;

    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, glm::vec3, _atmosphereCenter);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, float, _atmosphereInnerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, float, _atmosphereOuterRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, float, _atmosphereMieScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, float, _atmosphereRayleighScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, glm::vec3, _atmosphereScatteringWavelengths);
    
    processedBytes += bytesRead;

    return true;
}

void AtmospherePropertyGroup::markAllChanged() {
}

EntityPropertyFlags AtmospherePropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_CENTER, atmosphereCenter);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_INNER_RADIUS, atmosphereInnerRadius);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_OUTER_RADIUS, atmosphereOuterRadius);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_MIE_SCATTERING, atmosphereMieScattering);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, atmosphereRayleighScattering);
    CHECK_PROPERTY_CHANGE(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, atmosphereScatteringWavelengths);

    return changedProperties;
}

void AtmospherePropertyGroup::getProperties(EntityItemProperties& propertiesOut) const {
}

bool AtmospherePropertyGroup::setProperties(const EntityItemProperties& properties) {
    return true;
}

EntityPropertyFlags AtmospherePropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_ATMOSPHERE_CENTER;
    requestedProperties += PROP_ATMOSPHERE_INNER_RADIUS;
    requestedProperties += PROP_ATMOSPHERE_OUTER_RADIUS;
    requestedProperties += PROP_ATMOSPHERE_MIE_SCATTERING;
    requestedProperties += PROP_ATMOSPHERE_RAYLEIGH_SCATTERING;
    requestedProperties += PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS;
    
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

    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, appendValue, getAtmosphereCenter());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, appendValue, getAtmosphereInnerRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, appendValue, getAtmosphereOuterRadius());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, appendValue, getAtmosphereMieScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, appendValue, getAtmosphereRayleighScattering());
    APPEND_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, appendValue, getAtmosphereScatteringWavelengths());
}

int AtmospherePropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_CENTER, glm::vec3, _atmosphereCenter);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, float, _atmosphereInnerRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, float, _atmosphereOuterRadius);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, float, _atmosphereMieScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, float, _atmosphereRayleighScattering);
    READ_ENTITY_PROPERTY(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, glm::vec3, _atmosphereScatteringWavelengths);

    return bytesRead;
}
