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

#include <SphereShape.h>
#include "EntityItem.h"

class LightEntityItem : public EntityItem {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    LightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    ALLOW_INSTANTIATION // This class can be instantiated

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    virtual void setDimensions(const glm::vec3& value);
    
    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
    virtual bool setProperties(const EntityItemProperties& properties, bool forceCopy = false);

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
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    const rgbColor& getAmbientColor() const { return _ambientColor; }
    xColor getAmbientXColor() const { 
        xColor color = { _ambientColor[RED_INDEX], _ambientColor[GREEN_INDEX], _ambientColor[BLUE_INDEX] }; return color; 
    }

    void setAmbientColor(const rgbColor& value) { memcpy(_ambientColor, value, sizeof(_ambientColor)); }
    void setAmbientColor(const xColor& value) {
            _ambientColor[RED_INDEX] = value.red;
            _ambientColor[GREEN_INDEX] = value.green;
            _ambientColor[BLUE_INDEX] = value.blue;
    }

    const rgbColor& getDiffuseColor() const { return _diffuseColor; }
    xColor getDiffuseXColor() const { 
        xColor color = { _diffuseColor[RED_INDEX], _diffuseColor[GREEN_INDEX], _diffuseColor[BLUE_INDEX] }; return color; 
    }

    void setDiffuseColor(const rgbColor& value) { memcpy(_diffuseColor, value, sizeof(_diffuseColor)); }
    void setDiffuseColor(const xColor& value) {
            _diffuseColor[RED_INDEX] = value.red;
            _diffuseColor[GREEN_INDEX] = value.green;
            _diffuseColor[BLUE_INDEX] = value.blue;
    }

    const rgbColor& getSpecularColor() const { return _specularColor; }
    xColor getSpecularXColor() const { 
        xColor color = { _specularColor[RED_INDEX], _specularColor[GREEN_INDEX], _specularColor[BLUE_INDEX] }; return color; 
    }

    void setSpecularColor(const rgbColor& value) { memcpy(_specularColor, value, sizeof(_specularColor)); }
    void setSpecularColor(const xColor& value) {
            _specularColor[RED_INDEX] = value.red;
            _specularColor[GREEN_INDEX] = value.green;
            _specularColor[BLUE_INDEX] = value.blue;
    }

    bool getIsSpotlight() const { return _isSpotlight; }
    void setIsSpotlight(bool value) { _isSpotlight = value; }

    float getConstantAttenuation() const { return _constantAttenuation; }
    void setConstantAttenuation(float value) { _constantAttenuation = value; }

    float getLinearAttenuation() const { return _linearAttenuation; }
    void setLinearAttenuation(float value) { _linearAttenuation = value; }

    float getQuadraticAttenuation() const { return _quadraticAttenuation; }
    void setQuadraticAttenuation(float value) { _quadraticAttenuation = value; }

    float getExponent() const { return _exponent; }
    void setExponent(float value) { _exponent = value; }

    float getCutoff() const { return _cutoff; }
    void setCutoff(float value) { _cutoff = value; }
    
    virtual const Shape& getCollisionShapeInMeters() const { return _emptyShape; }
    
protected:
    virtual void recalculateCollisionShape() { /* nothing to do */ }

    // properties of a light
    rgbColor _ambientColor;
    rgbColor _diffuseColor;
    rgbColor _specularColor;
    bool _isSpotlight;
    float _constantAttenuation;
    float _linearAttenuation; 
    float _quadraticAttenuation;
    float _exponent;
    float _cutoff;

    // used for collision detection
    SphereShape _emptyShape;
};

#endif // hifi_LightEntityItem_h
