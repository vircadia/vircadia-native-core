//
//  ImageEntityItem.h
//  libraries/entities/src
//
//  Created by Elisa Lupin-Jimenez on 1/3/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ImageEntityItem_h
#define hifi_ImageEntityItem_h

#include "EntityItem.h"
#include "ShapeEntityItem.h"

class ImageEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ImageEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
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

    void setUnscaledDimensions(const glm::vec3& value) override;

    static const QString DEFAULT_IMAGE_URL;
    virtual void setImageURL(const QString& value);
    QString getImageURL() const;

    //virtual void computeShapeInfo(ShapeInfo& info) override;
    virtual ShapeType getShapeType() const override;

protected:
    QString _imageURL;

    entity::Shape _shape{ entity::Shape::Quad };
    ShapeType _collisionShapeType{ ShapeType::SHAPE_TYPE_BOX };

};

#endif // hifi_ImageEntityItem_h
