//
//  FlatImageEntity.h
//  libraries/entities/src
//
//  Created by Elisa Lupin-Jimenez on 1/3/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FlatImageEntity_h
#define hifi_FlatImageEntity_h

#include "EntityItem.h"

class FlatImageEntity : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    FlatImageEntity(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
        EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
        EntityPropertyFlags& requestedProperties,
        EntityPropertyFlags& propertyFlags,
        EntityPropertyFlags& propertiesDidntFit,
        int& propertyCount,
        OctreeElement::AppendState& appendState) const override;

    static const QString DEFAULT_IMAGE_URL;

};

#endif // hifi_FlatImageEntity_h
