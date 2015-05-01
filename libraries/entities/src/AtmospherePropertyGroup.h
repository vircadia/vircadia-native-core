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

#include "EntityItemProperties.h"

#include "PropertyGroup.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;

/*
#include <stdint.h>

#include <glm/glm.hpp>
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
    AtmospherePropertyGroup() {}
    virtual ~AtmospherePropertyGroup() {}

    // EntityItemProperty related helpers
    virtual QScriptValue copyToScriptValue(QScriptEngine* engine, bool skipDefaults) const;
    virtual void copyFromScriptValue(const QScriptValue& object);
    virtual void debugDump() const;

    virtual bool appentToEditPacket(unsigned char* bufferOut, int sizeIn, int& sizeOut);
    virtual bool decodeFromEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes);
    virtual void markAllChanged();

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
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);


    DEFINE_PROPERTY_REF(PROP_ATMOSPHERE_CENTER, AtmosphereCenter, atmosphereCenter, glm::vec3);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_INNER_RADIUS, AtmosphereInnerRadius, atmosphereInnerRadius, float);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_OUTER_RADIUS, AtmosphereOuterRadius, atmosphereOuterRadius, float);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_MIE_SCATTERING, AtmosphereMieScattering, atmosphereMieScattering, float);
    DEFINE_PROPERTY(PROP_ATMOSPHERE_RAYLEIGH_SCATTERING, AtmosphereRayleighScattering, atmosphereRayleighScattering, float);
    DEFINE_PROPERTY_REF(PROP_ATMOSPHERE_SCATTERING_WAVELENGTHS, AtmosphereScatteringWavelengths, atmosphereScatteringWavelengths, glm::vec3);

};

#endif // hifi_AtmospherePropertyGroup_h
