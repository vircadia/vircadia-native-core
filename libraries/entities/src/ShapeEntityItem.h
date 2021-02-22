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

#include "PulsePropertyGroup.h"

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
    QString stringFromShape(Shape shape);
}

class ShapeEntityItem : public EntityItem {
    using Pointer = std::shared_ptr<ShapeEntityItem>;
    static Pointer baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer sphereFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer boxFactory(const EntityItemID& entityID, const EntityItemProperties& properties);

    using ShapeInfoCalculator = std::function<void( const ShapeEntityItem * const shapeEntity, ShapeInfo& info)>;
    static void setShapeInfoCalulator(ShapeInfoCalculator callback);

    ShapeEntityItem(const EntityItemID& entityItemID);

    void pureVirtualFunctionPlaceHolder() override { };
    // Triggers warnings on OSX
    //ALLOW_INSTANTIATION 
    
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

    entity::Shape getShape() const;
    void setShape(const entity::Shape& shape);
    void setShape(const QString& shape) { setShape(entity::shapeFromString(shape)); }

    float getAlpha() const;
    void setAlpha(float alpha);

    glm::u8vec3 getColor() const;
    void setColor(const glm::u8vec3& value);

    void setUnscaledDimensions(const glm::vec3& value) override;

    bool supportsDetailedIntersection() const override;
    bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                     const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                                     float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                     QVariantMap& extraInfo, bool precisionPicking) const override;
    bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                                          const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                                          float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
                                          QVariantMap& extraInfo, bool precisionPicking) const override;
    bool getRotateForPicking() const override;

    void debugDump() const override;

    virtual void computeShapeInfo(ShapeInfo& info) override;
    virtual ShapeType getShapeType() const override;

    PulsePropertyGroup getPulseProperties() const;

    void setUserData(const QString& value) override;

protected:
    glm::u8vec3 _color;
    float _alpha { 1.0f };
    PulsePropertyGroup _pulseProperties;
    entity::Shape _shape { entity::Shape::Sphere };

    //! This is SHAPE_TYPE_ELLIPSOID rather than SHAPE_TYPE_NONE to maintain
    //! prior functionality where new or unsupported shapes are treated as
    //! ellipsoids.
    ShapeType _collisionShapeType { ShapeType::SHAPE_TYPE_ELLIPSOID };
};

#endif // hifi_ShapeEntityItem_h
