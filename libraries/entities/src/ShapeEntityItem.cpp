//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <glm/gtx/transform.hpp>

#include <QtCore/QDebug>

#include <GeometryUtil.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ShapeEntityItem.h"

namespace entity {
    static const std::array<QString, Shape::NUM_SHAPES> shapeStrings { {
        "Triangle", 
        "Quad", 
        "Hexagon",
        "Octagon",
        "Circle",
        "Cube", 
        "Sphere", 
        "Tetrahedron", 
        "Octahedron", 
        "Dodecahedron", 
        "Icosahedron", 
        "Torus",
        "Cone", 
        "Cylinder" 
    } };

    Shape shapeFromString(const ::QString& shapeString) {
        for (size_t i = 0; i < shapeStrings.size(); ++i) {
            if (shapeString.toLower() == shapeStrings[i].toLower()) {
                return static_cast<Shape>(i);
            }
        }
        return Shape::Sphere;
    }

    ::QString stringFromShape(Shape shape) {
        return shapeStrings[shape];
    }
}

ShapeEntityItem::Pointer ShapeEntityItem::baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity { new ShapeEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

EntityItemPointer ShapeEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return baseFactory(entityID, properties);
}

EntityItemPointer ShapeEntityItem::boxFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    auto result = baseFactory(entityID, properties);
    result->setShape(entity::Shape::Cube);
    return result;
}

EntityItemPointer ShapeEntityItem::sphereFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    auto result = baseFactory(entityID, properties);
    result->setShape(entity::Shape::Sphere);
    return result;
}

// our non-pure virtual subclass for now...
ShapeEntityItem::ShapeEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Shape;
    _volumeMultiplier *= PI / 6.0f;
}

EntityItemProperties ShapeEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    properties.setColor(getXColor());
    properties.setShape(entity::stringFromShape(getShape()));
    return properties;
}

void ShapeEntityItem::setShape(const entity::Shape& shape) {
    _shape = shape;
    switch (_shape) {
        case entity::Shape::Cube:
            _type = EntityTypes::Box;
            break;
        case entity::Shape::Sphere:
            _type = EntityTypes::Sphere;
            break;
        default:
            _type = EntityTypes::Shape;
            break;
    }
}

bool ShapeEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alpha, setAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shape, setShape);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ShapeEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
    return somethingChanged;
}

int ShapeEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_SHAPE, QString, setShape);
    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags ShapeEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_SHAPE;
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ALPHA;
    return requestedProperties;
}

void ShapeEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_SHAPE, entity::stringFromShape(getShape()));
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA, getAlpha());
}

void ShapeEntityItem::setColor(const rgbColor& value) {
    memcpy(_color, value, sizeof(rgbColor));
}

xColor ShapeEntityItem::getXColor() const {
    return xColor { _color[0], _color[1], _color[2] };
}

void ShapeEntityItem::setColor(const xColor& value) {
    setColor(rgbColor { value.red, value.green, value.blue });
}

QColor ShapeEntityItem::getQColor() const {
    auto& color = getColor();
    return QColor(color[0], color[1], color[2], (int)(getAlpha() * 255));
}

void ShapeEntityItem::setColor(const QColor& value) {
    setColor(rgbColor { (uint8_t)value.red(), (uint8_t)value.green(), (uint8_t)value.blue() });
    setAlpha(value.alpha());
}

bool ShapeEntityItem::supportsDetailedRayIntersection() const {
    return _shape == entity::Sphere;
}

bool ShapeEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                   bool& keepSearching, OctreeElementPointer& element,
                                                   float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                                   void** intersectedObject, bool precisionPicking) const {
    // determine the ray in the frame of the entity transformed from a unit sphere
    glm::mat4 entityToWorldMatrix = getEntityToWorldMatrix();
    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);
    glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 entityFrameDirection = glm::normalize(glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f)));

    float localDistance;
    // NOTE: unit sphere has center of 0,0,0 and radius of 0.5
    if (findRaySphereIntersection(entityFrameOrigin, entityFrameDirection, glm::vec3(0.0f), 0.5f, localDistance)) {
        // determine where on the unit sphere the hit point occured
        glm::vec3 entityFrameHitAt = entityFrameOrigin + (entityFrameDirection * localDistance);
        // then translate back to work coordinates
        glm::vec3 hitAt = glm::vec3(entityToWorldMatrix * glm::vec4(entityFrameHitAt, 1.0f));
        distance = glm::distance(origin, hitAt);
        bool success;
        surfaceNormal = glm::normalize(hitAt - getCenterPosition(success));
        if (!success) {
            return false;
        }
        return true;
    }
    return false;
}

void ShapeEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "SHAPE EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               name:" << _name;
    qCDebug(entities) << "              shape:" << stringFromShape(_shape) << " (EnumId: " << _shape << " )";
    qCDebug(entities) << "              color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "           position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "         dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "      getLastEdited:" << debugTime(getLastEdited(), now);
	qCDebug(entities) << "SHAPE EntityItem Ptr:" << this;
}

void ShapeEntityItem::computeShapeInfo(ShapeInfo& info) {

    // This will be called whenever DIRTY_SHAPE flag (set by dimension change, etc)
    // is set.

    const glm::vec3 entityDimensions = getDimensions();

    switch (_shape) {
        case entity::Shape::Quad:
        case entity::Shape::Cube:
            {
                _collisionShapeType = SHAPE_TYPE_BOX;
            }
            break;
        case entity::Shape::Sphere:
            {

                float diameter = entityDimensions.x;
                const float MIN_DIAMETER = 0.001f;
                const float MIN_RELATIVE_SPHERICAL_ERROR = 0.001f;
                if (diameter > MIN_DIAMETER
                    && fabsf(diameter - entityDimensions.y) / diameter < MIN_RELATIVE_SPHERICAL_ERROR
                    && fabsf(diameter - entityDimensions.z) / diameter < MIN_RELATIVE_SPHERICAL_ERROR) {

                    _collisionShapeType = SHAPE_TYPE_SPHERE;
                } else {
                    _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
                }
            }
            break;
        case entity::Shape::Cylinder:
            {
                _collisionShapeType = SHAPE_TYPE_CYLINDER_Y;
                // TODO WL21389: determine if rotation is axis-aligned
                //const Transform::Quat & rot = _transform.getRotation();

                // TODO WL21389: some way to tell apart SHAPE_TYPE_CYLINDER_Y, _X, _Z based on rotation and
                //       hull ( or dimensions, need circular cross section)
                // Should allow for minor variance along axes?

            }
            break;
        case entity::Shape::Triangle:
        case entity::Shape::Hexagon:
        case entity::Shape::Octagon:
        case entity::Shape::Circle:
        case entity::Shape::Tetrahedron:
        case entity::Shape::Octahedron:
        case entity::Shape::Dodecahedron:
        case entity::Shape::Icosahedron:
        case entity::Shape::Cone:
            {
                //TODO WL21389: SHAPE_TYPE_SIMPLE_HULL and pointCollection (later)
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
            break;
        case entity::Shape::Torus:
            {
                // Not in GeometryCache::buildShapes, unsupported.
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
                //TODO WL21389: SHAPE_TYPE_SIMPLE_HULL and pointCollection (later if desired support)
            }
            break;
        default:
            {
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
            break;
    }

    EntityItem::computeShapeInfo(info);
}

// This value specifes how the shape should be treated by physics calculations.
ShapeType ShapeEntityItem::getShapeType() const {
    return _collisionShapeType;
}

