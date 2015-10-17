//
//  LightEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LightEntityItem_h
#define hifi_LightEntityItem_h

#include "EntityItem.h"

class LightEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    LightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    ALLOW_INSTANTIATION // This class can be instantiated

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    virtual void setDimensions(const glm::vec3& value);
    
    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const;
    virtual bool setProperties(const EntityItemProperties& properties);

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged);

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const {
        xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color;
    }

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }

    bool getIsSpotlight() const { return _isSpotlight; }
    void setIsSpotlight(bool value);

    void setIgnoredColor(const rgbColor& value) { }
    void setIgnoredAttenuation(float value) { }

    float getIntensity() const { return _intensity; }
    void setIntensity(float value) { _intensity = value; }

    float getExponent() const { return _exponent; }
    void setExponent(float value) { _exponent = value; }

    float getCutoff() const { return _cutoff; }
    void setCutoff(float value);
    
    static bool getLightsArePickable() { return _lightsArePickable; }
    static void setLightsArePickable(bool value) { _lightsArePickable = value; }
    
protected:

    // properties of a light
    rgbColor _color;
    bool _isSpotlight;
    float _intensity;
    float _exponent;
    float _cutoff;

    static bool _lightsArePickable;
};

#endif // hifi_LightEntityItem_h
