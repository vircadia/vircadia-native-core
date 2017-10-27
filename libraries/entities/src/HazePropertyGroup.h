//
//  HazePropertyGroup.h
//  libraries/entities/src
//
//  Created by Nissim hadar on 9/21/17.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HazePropertyGroup_h
#define hifi_HazePropertyGroup_h

#include <stdint.h>

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include "PropertyGroup.h"
#include "EntityItemPropertiesMacros.h"

#include <model/HazeInit.h>

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;

class HazePropertyGroup : public PropertyGroup {
public:
    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties,
                                   QScriptEngine* engine, bool skipDefaults,
                                   EntityItemProperties& defaultEntityProperties) const override;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) override;

    void merge(const HazePropertyGroup& other);

    virtual void debugDump() const override;
    virtual void listChangedProperties(QList<QString>& out) override;

    virtual bool appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual bool decodeFromEditPacket(EntityPropertyFlags& propertyFlags,
                                      const unsigned char*& dataAt, int& processedBytes) override;
    virtual void markAllChanged() override;
    virtual EntityPropertyFlags getChangedProperties() const override;

    // EntityItem related helpers
    // methods for getting/setting all properties of an entity
    virtual void getProperties(EntityItemProperties& propertiesOut) const override;

    /// returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    // Range only parameters
    DEFINE_PROPERTY(PROP_HAZE_RANGE, HazeRange, hazeRange, float, model::initialHazeRange_m);
    DEFINE_PROPERTY_REF(PROP_HAZE_COLOR, HazeColor, hazeColor, xColor, model::initialHazeColorXcolor);
    DEFINE_PROPERTY_REF(PROP_HAZE_GLARE_COLOR, HazeGlareColor, hazeGlareColor, xColor, model::initialHazeGlareColorXcolor);
    DEFINE_PROPERTY(PROP_HAZE_ENABLE_GLARE, HazeEnableGlare, hazeEnableGlare, bool, false);
    DEFINE_PROPERTY_REF(PROP_HAZE_GLARE_ANGLE, HazeGlareAngle, hazeGlareAngle, float, model::initialGlareAngle_degs);

    // Altitude parameters
    DEFINE_PROPERTY(PROP_HAZE_ALTITUDE_EFFECT, HazeAltitudeEffect, hazeAltitudeEffect, bool, false);
    DEFINE_PROPERTY_REF(PROP_HAZE_CEILING, HazeCeiling, hazeCeiling, float, model::initialHazeBaseReference_m + model::initialHazeHeight_m);
    DEFINE_PROPERTY_REF(PROP_HAZE_BASE_REF, HazeBaseRef, hazeBaseRef, float, model::initialHazeBaseReference_m);

    // Background (skybox) blend value
    DEFINE_PROPERTY_REF(PROP_HAZE_BACKGROUND_BLEND, HazeBackgroundBlend, hazeBackgroundBlend, float, model::initialHazeBackgroundBlend);

    // hazeDirectional light attenuation
    DEFINE_PROPERTY(PROP_HAZE_ATTENUATE_KEYLIGHT, HazeAttenuateKeyLight, hazeAttenuateKeyLight, bool, false);
    DEFINE_PROPERTY_REF(PROP_HAZE_KEYLIGHT_RANGE, HazeKeyLightRange, hazeKeyLightRange, float, model::initialHazeKeyLightRange_m);
    DEFINE_PROPERTY_REF(PROP_HAZE_KEYLIGHT_ALTITUDE, HazeKeyLightAltitude, hazeKeyLightAltitude, float, model::initialHazeKeyLightAltitude_m);
};

#endif // hifi_HazePropertyGroup_h
