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

#include "AtmospherePropertyGroup.h"

QScriptValue AtmospherePropertyGroup::copyToScriptValue(QScriptEngine* engine, bool skipDefaults) const {
    QScriptValue result;
    return result;
}

void AtmospherePropertyGroup::copyFromScriptValue(const QScriptValue& object) {
}

void AtmospherePropertyGroup::debugDump() const {
    /*
    qCDebug(entities) << "   AtmospherePropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "       Atmosphere Center:" << getAtmosphereCenter();
    qCDebug(entities) << "       Atmosphere Inner Radius:" << getAtmosphereInnerRadius();
    qCDebug(entities) << "       Atmosphere Outer Radius:" << getAtmosphereOuterRadius();
    qCDebug(entities) << "       ... more ...";
    */
}

bool AtmospherePropertyGroup::appentToEditPacket(unsigned char* bufferOut, int sizeIn, int& sizeOut) {
    return true;
}

bool AtmospherePropertyGroup::decodeFromEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes) {
    return true;
}

void AtmospherePropertyGroup::markAllChanged() {
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
