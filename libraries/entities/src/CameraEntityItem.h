//
//  CameraEntityItem.h
//  libraries/entities/src
//
//  Created by Thijs Wenker on 11/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CameraEntityItem_h
#define hifi_CameraEntityItem_h

#include "EntityItem.h"

class CameraEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    CameraEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const;
    virtual bool setProperties(const EntityItemProperties& properties);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
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
        
};

#endif // hifi_CameraEntityItem_h
