//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShapeEntityItem.h"

#include <glm/gtx/transform.hpp>

#include <QtCore/QDebug>

#include <GeometryUtil.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

namespace entity {

    /*@jsdoc
     * <p>A <code>"Shape"</code>, <code>"Box"</code>, or <code>"Sphere"</code> {@link Entities.EntityType|EntityType} may 
     * display as one of the following geometrical shapes:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Dimensions</th><th>Notes</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>"Circle"</code></td><td>2D</td><td>A circle oriented in 3D.</td></tr>
     *     <tr><td><code>"Cone"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Cube"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Cylinder"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Dodecahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Hexagon"</code></td><td>3D</td><td>A hexagonal prism.</td></tr>
     *     <tr><td><code>"Icosahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Octagon"</code></td><td>3D</td><td>An octagonal prism.</td></tr>
     *     <tr><td><code>"Octahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Quad"</code></td><td>2D</td><td>A square oriented in 3D.</td></tr>
     *     <tr><td><code>"Sphere"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Tetrahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Torus"</code></td><td>3D</td><td><em>Not implemented.</em></td></tr>
     *     <tr><td><code>"Triangle"</code></td><td>3D</td><td>A triangular prism.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {string} Entities.Shape
     */
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
        "Torus",  // Not implemented yet.
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

    QString stringFromShape(Shape shape) {
        return shapeStrings[shape];
    }
}

// hullShapeCalculator is a hook for external code that knows how to configure a ShapeInfo
// for given entity::Shape and dimensions
ShapeEntityItem::ShapeInfoCalculator hullShapeCalculator = nullptr;

void ShapeEntityItem::setShapeInfoCalulator(ShapeEntityItem::ShapeInfoCalculator callback) {
    hullShapeCalculator = callback;
}

ShapeEntityItem::Pointer ShapeEntityItem::baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity(new ShapeEntityItem(entityID), [](ShapeEntityItem* ptr) { ptr->deleteLater(); });
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

EntityItemProperties ShapeEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alpha, getAlpha);
    withReadLock([&] {
        _pulseProperties.getProperties(properties);
    });
    properties.setShape(entity::stringFromShape(getShape()));
    properties._shapeChanged = false;

    return properties;
}

void ShapeEntityItem::setShape(const entity::Shape& shape) {
    switch (shape) {
        case entity::Shape::Cube:
            _type = EntityTypes::Box;
            break;
        case entity::Shape::Sphere:
            _type = EntityTypes::Sphere;
            break;
        case entity::Shape::Circle:
            // Circle is implicitly flat so we enforce flat dimensions
            setUnscaledDimensions(getUnscaledDimensions());
            break;
        case entity::Shape::Quad:
            // Quad is implicitly flat so we enforce flat dimensions
            setUnscaledDimensions(getUnscaledDimensions());
            break;
        default:
            _type = EntityTypes::Shape;
            break;
    }

    if (shape != getShape()) {
        // Internally grabs writeLock
        markDirtyFlags(Simulation::DIRTY_SHAPE);
        withWriteLock([&] {
            _needsRenderUpdate = true;
            _shape = shape;
        });
    }
}

entity::Shape ShapeEntityItem::getShape() const {
    return resultWithReadLock<entity::Shape>([&] {
        return _shape;
    });
}

bool ShapeEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alpha, setAlpha);
    withWriteLock([&] {
        bool pulsePropertiesChanged = _pulseProperties.setProperties(properties);
        somethingChanged |= pulsePropertiesChanged;
        _needsRenderUpdate |= pulsePropertiesChanged;
    });
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shape, setShape);

    return somethingChanged;
}

int ShapeEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, glm::u8vec3, setColor);
    READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);
    withWriteLock([&] {
        int bytesFromPulse = _pulseProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData,
            somethingChanged);
        bytesRead += bytesFromPulse;
        dataAt += bytesFromPulse;
    });
    READ_ENTITY_PROPERTY(PROP_SHAPE, QString, setShape);

    return bytesRead;
}

EntityPropertyFlags ShapeEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ALPHA;
    requestedProperties += _pulseProperties.getEntityProperties(params);
    requestedProperties += PROP_SHAPE;
    return requestedProperties;
}

void ShapeEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA, getAlpha());
    withReadLock([&] {
        _pulseProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
            propertyFlags, propertiesDidntFit, propertyCount, appendState);
    });
    APPEND_ENTITY_PROPERTY(PROP_SHAPE, entity::stringFromShape(getShape()));
}

void ShapeEntityItem::setColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _color != value;
        _color = value;
    });
}

glm::u8vec3 ShapeEntityItem::getColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _color;
    });
}

void ShapeEntityItem::setAlpha(float alpha) {
    withWriteLock([&] {
        _needsRenderUpdate |= _alpha != alpha;
        _alpha = alpha;
    });
}

float ShapeEntityItem::getAlpha() const {
    return resultWithReadLock<float>([&] {
        return _alpha;
    });
}

void ShapeEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    const float MAX_FLAT_DIMENSION = 0.0001f;
    const auto shape = getShape();
    if ((shape == entity::Shape::Circle || shape == entity::Shape::Quad) && value.y > MAX_FLAT_DIMENSION) {
        // enforce flatness in Y
        glm::vec3 newDimensions = value;
        newDimensions.y = MAX_FLAT_DIMENSION;
        EntityItem::setUnscaledDimensions(newDimensions);
    } else {
        EntityItem::setUnscaledDimensions(value);
    }
}

bool ShapeEntityItem::supportsDetailedIntersection() const {
    return getShape() == entity::Sphere;
}

bool ShapeEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                  const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                                                  float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                                  QVariantMap& extraInfo, bool precisionPicking) const {
    glm::vec3 dimensions = getScaledDimensions();
    BillboardMode billboardMode = getBillboardMode();
    glm::quat rotation = billboardMode == BillboardMode::NONE ? getWorldOrientation() : getLocalOrientation();
    glm::vec3 position = getWorldPosition() + rotation * (dimensions * (ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint()));
    rotation = BillboardModeHelpers::getBillboardRotation(position, rotation, billboardMode, viewFrustumPos);

    // determine the ray in the frame of the entity transformed from a unit sphere
    glm::mat4 entityToWorldMatrix = glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(dimensions);
    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);
    glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 entityFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

    // NOTE: unit sphere has center of 0,0,0 and radius of 0.5
    if (findRaySphereIntersection(entityFrameOrigin, entityFrameDirection, glm::vec3(0.0f), 0.5f, distance)) {
        bool success;
        glm::vec3 center = getCenterPosition(success);
        if (success) {
            // FIXME: this is only correct for uniformly scaled spheres
            // determine where on the unit sphere the hit point occured
            glm::vec3 hitAt = origin + (direction * distance);
            surfaceNormal = glm::normalize(hitAt - center);
        } else {
            return false;
        }
        return true;
    }
    return false;
}

bool ShapeEntityItem::findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                                       const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& parabolicDistance,
                                                       BoxFace& face, glm::vec3& surfaceNormal,
                                                       QVariantMap& extraInfo, bool precisionPicking) const {
    glm::vec3 dimensions = getScaledDimensions();
    BillboardMode billboardMode = getBillboardMode();
    glm::quat rotation = billboardMode == BillboardMode::NONE ? getWorldOrientation() : getLocalOrientation();
    glm::vec3 position = getWorldPosition() + rotation * (dimensions * (ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint()));
    rotation = BillboardModeHelpers::getBillboardRotation(position, rotation, billboardMode, viewFrustumPos);

    // determine the parabola in the frame of the entity transformed from a unit sphere
    glm::mat4 entityToWorldMatrix = glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(dimensions);
    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);
    glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 entityFrameVelocity = glm::vec3(worldToEntityMatrix * glm::vec4(velocity, 0.0f));
    glm::vec3 entityFrameAcceleration = glm::vec3(worldToEntityMatrix * glm::vec4(acceleration, 0.0f));

    // NOTE: unit sphere has center of 0,0,0 and radius of 0.5
    if (findParabolaSphereIntersection(entityFrameOrigin, entityFrameVelocity, entityFrameAcceleration, glm::vec3(0.0f), 0.5f, parabolicDistance)) {
        bool success;
        glm::vec3 center = getCenterPosition(success);
        if (success) {
            // FIXME: this is only correct for uniformly scaled spheres
            surfaceNormal = glm::normalize((origin + velocity * parabolicDistance + 0.5f * acceleration * parabolicDistance * parabolicDistance) - center);
        } else {
            return false;
        }
        return true;
    }
    return false;
}

bool ShapeEntityItem::getRotateForPicking() const {
    auto shape = getShape();
    return getBillboardMode() != BillboardMode::NONE && (shape < entity::Shape::Cube || shape > entity::Shape::Icosahedron);
}

void ShapeEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "SHAPE EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               name:" << _name;
    qCDebug(entities) << "              shape:" << stringFromShape(_shape) << " (EnumId: " << _shape << " )";
    qCDebug(entities) << " collisionShapeType:" << ShapeInfo::getNameForShapeType(getShapeType());
    qCDebug(entities) << "              color:" << _color;
    qCDebug(entities) << "           position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "         dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "      getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "SHAPE EntityItem Ptr:" << this;
}

void ShapeEntityItem::computeShapeInfo(ShapeInfo& info) {

    // This will be called whenever DIRTY_SHAPE flag (set by dimension change, etc)
    // is set.

    const glm::vec3 entityDimensions = getScaledDimensions();
    const auto shape = getShape();

    switch (shape){
        case entity::Shape::Quad:
            // Quads collide like flat Cubes
        case entity::Shape::Cube: {
            _collisionShapeType = SHAPE_TYPE_BOX;
        }
        break;
        case entity::Shape::Sphere: {
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
        case entity::Shape::Circle:
            // Circles collide like flat Cylinders
        case entity::Shape::Cylinder: {
            float diameter = entityDimensions.x;
            const float MIN_DIAMETER = 0.001f;
            const float MIN_RELATIVE_SPHERICAL_ERROR = 0.001f;
            if (diameter > MIN_DIAMETER
                && fabsf(diameter - entityDimensions.z) / diameter < MIN_RELATIVE_SPHERICAL_ERROR) {
                _collisionShapeType = SHAPE_TYPE_CYLINDER_Y;
            } else if (hullShapeCalculator) {
                hullShapeCalculator(this, info);
                _collisionShapeType = SHAPE_TYPE_SIMPLE_HULL;
            } else {
                // woops, someone forgot to hook up the hullShapeCalculator()!
                // final fallback is ellipsoid
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
        }
        break;
        case entity::Shape::Cone: {
            if (hullShapeCalculator) {
                hullShapeCalculator(this, info);
                _collisionShapeType = SHAPE_TYPE_SIMPLE_HULL;
            } else {
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
        }
        break;
        // gons, ones, & angles built via GeometryCache::extrudePolygon
        case entity::Shape::Triangle:
        case entity::Shape::Hexagon:
        case entity::Shape::Octagon: {
            if (hullShapeCalculator) {
                hullShapeCalculator(this, info);
                _collisionShapeType = SHAPE_TYPE_SIMPLE_HULL;
            } else {
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
        }
        break;
        // hedrons built via GeometryCache::setUpFlatShapes
        case entity::Shape::Tetrahedron:
        case entity::Shape::Octahedron:
        case entity::Shape::Dodecahedron:
        case entity::Shape::Icosahedron: {
            if ( hullShapeCalculator ) {
                hullShapeCalculator(this, info);
                _collisionShapeType = SHAPE_TYPE_SIMPLE_HULL;
            } else {
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
        }
        break;
        case entity::Shape::Torus: {
            // Not in GeometryCache::buildShapes, unsupported.
            _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            //TODO handle this shape more correctly
        }
        break;
        default: {
            _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
        }
        break;
    }

    EntityItem::computeShapeInfo(info);
}

// This value specifies how the shape should be treated by physics calculations.
ShapeType ShapeEntityItem::getShapeType() const {
    return _collisionShapeType;
}

PulsePropertyGroup ShapeEntityItem::getPulseProperties() const {
    return resultWithReadLock<PulsePropertyGroup>([&] {
        return _pulseProperties;
    });
}

void ShapeEntityItem::setUserData(const QString& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _userData != value;
        _userData = value;
    });
}