//
//  AtmospherePropertyGroup.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AtmospherePropertyGroup_h
#define hifi_AtmospherePropertyGroup_h

#include <QtScript/QScriptEngine>

#include "PropertyGroup.h"
#include "EntityItemPropertiesMacros.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;

#include <stdint.h>
#include <glm/glm.hpp>


/*

#include <glm/gtx/extented_min_max.hpp>

#include <QtCore/QObject>
#include <QVector>
#include <QString>

#include <AACube.h>
#include <FBXReader.h> // for SittingPoint
#include <PropertyFlags.h>
#include <OctreeConstants.h>
#include <ShapeInfo.h>

#include "EntityItemID.h"
#include "AtmospherePropertyGroupMacros.h"
#include "EntityTypes.h"
*/


class AtmospherePropertyGroup : public PropertyGroup {
public:
    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings);
    virtual void debugDump() const;

    virtual bool appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;

    virtual bool decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes);
    virtual void markAllChanged();
    virtual EntityPropertyFlags getChangedProperties() const;

    // EntityItem related helpers
    // methods for getting/setting all properties of an entity
    virtual void getProperties(EntityItemProperties& propertiesOut) const;
    
    /// returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties);

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;
        
    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData, 
                                                bool& somethingChanged);

    
    static const glm::vec3 DEFAULT_CENTER;
    static const float DEFAULT_INNER_RADIUS;
    static const float DEFAULT_OUTER_RADIUS;
    static const float DEFAULT_RAYLEIGH_SCATTERING;
    static const float DEFAULT_MIE_SCATTERING;
    static const glm::vec3 DEFAULT_SCATTERING_WAVELENGTHS;
    static const bool DEFAULT_HAS_STARS;
    
    DEFINE_PROPERTY_REF(PROP_ATMOSPHERE_CENTER, Center, center, glm::vec3, DEFAULT_CENTER);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, InnerRadius, innerRadius, float, DEFAULT_INNER_RADIUS);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, OuterRadius, outerRadius, float, DEFAULT_OUTER_RADIUS);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, MieScattering, mieScattering, float, DEFAULT_MIE_SCATTERING);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, RayleighScattering, rayleighScattering, float, DEFAULT_RAYLEIGH_SCATTERING);
    DEFINE_PROPERTY_REF(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, ScatteringWavelengths, scatteringWavelengths, glm::vec3, DEFAULT_SCATTERING_WAVELENGTHS);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_HAS_STARS, HasStars, hasStars, bool, DEFAULT_HAS_STARS);
};

#endif // hifi_AtmospherePropertyGroup_h
