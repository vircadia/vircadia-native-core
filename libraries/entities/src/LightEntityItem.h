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
    static const bool DEFAULT_IS_SPOTLIGHT;
    static const float DEFAULT_INTENSITY;
    static const float DEFAULT_FALLOFF_RADIUS;
    static const float DEFAULT_EXPONENT;
    static const float MIN_CUTOFF;
    static const float MAX_CUTOFF;
    static const float DEFAULT_CUTOFF;

    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    LightEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    virtual void setUnscaledDimensions(const glm::vec3& value) override;

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    virtual bool setSubClassProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    glm::u8vec3 getColor() const;
    void setColor(const glm::u8vec3& value);

    bool getIsSpotlight() const;
    void setIsSpotlight(bool value);

    float getIntensity() const;
    void setIntensity(float value);
    float getFalloffRadius() const;
    void setFalloffRadius(float value);

    float getExponent() const;
    void setExponent(float value);

    float getCutoff() const;
    void setCutoff(float value);
    
    static bool getLightsArePickable() { return _lightsArePickable; }
    static void setLightsArePickable(bool value) { _lightsArePickable = value; }

    virtual bool supportsDetailedIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                            const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& distance,
                            BoxFace& face, glm::vec3& surfaceNormal,
                            QVariantMap& extraInfo, bool precisionPicking) const override;
    virtual bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                            const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                            float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
                            QVariantMap& extraInfo, bool precisionPicking) const override;

private:
    // properties of a light
    glm::u8vec3 _color;
    bool _isSpotlight { DEFAULT_IS_SPOTLIGHT };
    float _intensity { DEFAULT_INTENSITY };
    float _falloffRadius { DEFAULT_FALLOFF_RADIUS };
    float _exponent { DEFAULT_EXPONENT };
    float _cutoff { DEFAULT_CUTOFF };

    static bool _lightsArePickable;
};

#endif // hifi_LightEntityItem_h
