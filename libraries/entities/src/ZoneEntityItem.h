//
//  ZoneEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ZoneEntityItem_h
#define hifi_ZoneEntityItem_h

#include "EntityItem.h" 

class ZoneEntityItem : public EntityItem {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    ALLOW_INSTANTIATION // This class can be instantiated
    
    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
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
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    const rgbColor& getColor() const { return _sunColor; }
    xColor getXColor() const { xColor color = { _sunColor[RED_INDEX], _sunColor[GREEN_INDEX], _sunColor[BLUE_INDEX] }; return color; }

    void setColor(const rgbColor& value) { memcpy(_sunColor, value, sizeof(_sunColor)); }
    void setColor(const xColor& value) {
        _sunColor[RED_INDEX] = value.red;
        _sunColor[GREEN_INDEX] = value.green;
        _sunColor[BLUE_INDEX] = value.blue;
    }
    
    virtual ShapeType getShapeType() const { return SHAPE_TYPE_BOX; }

    virtual void debugDump() const;

protected:
    rgbColor _sunColor;
};

#endif // hifi_ZoneEntityItem_h
