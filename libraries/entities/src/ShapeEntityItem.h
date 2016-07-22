//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeEntityItem_h
#define hifi_ShapeEntityItem_h

#include "EntityItem.h"

namespace entity {
    enum Shape {
        Triangle,
        Quad,
        Hexagon,
        Octagon,
        Circle,
        Cube,
        Sphere,
        Tetrahedron,
        Octahedron,
        Dodecahedron,
        Icosahedron,
        Torus,
        Cone,
        Cylinder,
        NUM_SHAPES,
    };

    Shape shapeFromString(const ::QString& shapeString);
    ::QString stringFromShape(Shape shape);
}


class ShapeEntityItem : public EntityItem {
    using Pointer = std::shared_ptr<ShapeEntityItem>;
    static Pointer baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer sphereFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer boxFactory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ShapeEntityItem(const EntityItemID& entityItemID);

    void pureVirtualFunctionPlaceHolder() override { };
    // Triggers warnings on OSX
    //ALLOW_INSTANTIATION 
    
    // methods for getting/setting all properties of an entity
    EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    bool setProperties(const EntityItemProperties& properties) override;

    EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const override;

    int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    entity::Shape getShape() const { return _shape; }
    void setShape(const entity::Shape& shape);
    void setShape(const QString& shape) { setShape(entity::shapeFromString(shape)); }

    float getAlpha() const { return _alpha; };
    void setAlpha(float alpha) { _alpha = alpha; }

    const rgbColor& getColor() const { return _color; }
    void setColor(const rgbColor& value);

    xColor getXColor() const;
    void setColor(const xColor& value);

    QColor getQColor() const;
    void setColor(const QColor& value);

    ShapeType getShapeType() const override;
    bool shouldBePhysical() const override { return !isDead(); }
    
    bool supportsDetailedRayIntersection() const override;
    bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                bool& keepSearching, OctreeElementPointer& element, float& distance,
                                                BoxFace& face, glm::vec3& surfaceNormal,
                                                void** intersectedObject, bool precisionPicking) const override;

    void debugDump() const override;

protected:

    float _alpha { 1 };
    rgbColor _color;
    entity::Shape _shape { entity::Shape::Sphere };
};

#endif // hifi_ShapeEntityItem_h
