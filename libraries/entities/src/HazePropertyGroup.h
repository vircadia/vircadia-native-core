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

    static const uint8_t DEFAULT_HAZE_MODE;

    static const float DEFAULT_HAZE_RANGE;
    static const xColor DEFAULT_HAZE_BLEND_IN_COLOR;
    static const xColor DEFAULT_HAZE_BLEND_OUT_COLOR;
    static const float DEFAULT_LIGHT_BLEND_ANGLE;

    static const float DEFAULT_HAZE_ALTITUDE;
    static const float DEFAULT_HAZE_BASE_REF;

    static const float DEFAULT_HAZE_BACKGROUND_BLEND;

    static const float DEFAULT_HAZE_KEYLIGHT_RANGE;
    static const float DEFAULT_HAZE_KEYLIGHT_ALTITUDE;

    // Selects whether haze inherits the mode, is off or the mode (range only, range & altitude...) 
    DEFINE_PROPERTY(PROP_HAZE_MODE, HazeMode, hazeMode, uint8_t, DEFAULT_HAZE_MODE);

    // Range only parameters
    DEFINE_PROPERTY(PROP_HAZE_RANGE, HazeRange, hazeRange, float, DEFAULT_HAZE_RANGE);
    DEFINE_PROPERTY_REF(PROP_HAZE_BLEND_IN_COLOR, HazeBlendInColor, hazeBlendInColor, xColor, DEFAULT_HAZE_BLEND_IN_COLOR);
    DEFINE_PROPERTY_REF(PROP_HAZE_BLEND_OUT_COLOR, HazeBlendOutColor, hazeBlendOutColor, xColor, DEFAULT_HAZE_BLEND_OUT_COLOR);
    DEFINE_PROPERTY(PROP_HAZE_LIGHT_BLEND_ANGLE, HazeLightBlendAngle, hazeLightBlendAngle, float, DEFAULT_LIGHT_BLEND_ANGLE);

    // Range & Altitude parameters
    DEFINE_PROPERTY(PROP_HAZE_ALTITUDE, HazeAltitude, hazeAltitude, float, DEFAULT_HAZE_ALTITUDE);
    DEFINE_PROPERTY(PROP_HAZE_BASE_REF, HazeBaseRef, hazeBaseRef, float, DEFAULT_HAZE_BASE_REF);

    // Background (skybox) blend value
    DEFINE_PROPERTY(PROP_HAZE_BACKGROUND_BLEND, HazeBackgroundBlend, hazeBackgroundBlend, float, DEFAULT_HAZE_BACKGROUND_BLEND);

    // Directional light attenuation
    DEFINE_PROPERTY(PROP_HAZE_KEYLIGHT_RANGE, HazeKeyLightRange, hazeKeyLightRange, float, DEFAULT_HAZE_KEYLIGHT_RANGE);
    DEFINE_PROPERTY(PROP_HAZE_KEYLIGHT_ALTITUDE, HazeKeyLightAltitude, hazeKeyLightAltitude, float, DEFAULT_HAZE_KEYLIGHT_ALTITUDE);
};

#endif // hifi_HazePropertyGroup_h
