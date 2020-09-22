//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GridEntityItem_h
#define hifi_GridEntityItem_h

#include "EntityItem.h"

#include "PulsePropertyGroup.h"

class GridEntityItem : public EntityItem {
    using Pointer = std::shared_ptr<GridEntityItem>;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    GridEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    virtual void setUnscaledDimensions(const glm::vec3& value) override;

    // methods for getting/setting all properties of an entity
    EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    bool setSubClassProperties(const EntityItemProperties& properties) override;

    EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                            EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                            EntityPropertyFlags& requestedProperties,
                            EntityPropertyFlags& propertyFlags,
                            EntityPropertyFlags& propertiesDidntFit,
                            int& propertyCount,
                            OctreeElement::AppendState& appendState) const override;

    int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                         ReadBitstreamToTreeParams& args,
                                         EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                         bool& somethingChanged) override;

    static const uint32_t DEFAULT_MAJOR_GRID_EVERY;
    static const float DEFAULT_MINOR_GRID_EVERY;

    void setColor(const glm::u8vec3& color);
    glm::u8vec3 getColor() const;

    void setAlpha(float alpha);
    float getAlpha() const;

    void setFollowCamera(bool followCamera);
    bool getFollowCamera() const;

    void setMajorGridEvery(uint32_t majorGridEvery);
    uint32_t getMajorGridEvery() const;

    void setMinorGridEvery(float minorGridEvery);
    float getMinorGridEvery() const;

    PulsePropertyGroup getPulseProperties() const;

protected:
    glm::u8vec3 _color;
    float _alpha;
    PulsePropertyGroup _pulseProperties;

    bool _followCamera { true };
    uint32_t _majorGridEvery { DEFAULT_MAJOR_GRID_EVERY };
    float _minorGridEvery { DEFAULT_MINOR_GRID_EVERY };

};

#endif // hifi_GridEntityItem_h
