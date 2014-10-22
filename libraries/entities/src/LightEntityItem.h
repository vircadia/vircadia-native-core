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

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }

    bool getIsSpotlight() const { return _isSpotlight; }
    void setIsSpotlight(bool value) { _isSpotlight = value; }

    virtual const Shape& getCollisionShapeInMeters() const { return _emptyShape; }
    
protected:
    virtual void recalculateCollisionShape() { /* nothing to do */ }

    rgbColor _color;
    bool _isSpotlight;
    SphereShape _emptyShape;
};

#endif // hifi_LightEntityItem_h
