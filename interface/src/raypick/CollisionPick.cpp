//
//  Created by Sabrina Shanman 2018/07/16
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CollisionPick.h"

#include <QtCore/QDebug>

#include <glm/gtx/transform.hpp>

#include "PhysicsCollisionGroups.h"
#include "ScriptEngineLogging.h"
#include "UUIDHasher.h"

PickResultPointer CollisionPickResult::compareAndProcessNewResult(const PickResultPointer& newRes) {
    const std::shared_ptr<CollisionPickResult> newCollisionResult = std::static_pointer_cast<CollisionPickResult>(newRes);

    if (entityIntersections.size()) {
        entityIntersections.insert(entityIntersections.cend(), newCollisionResult->entityIntersections.begin(), newCollisionResult->entityIntersections.end());
    } else {
        entityIntersections = newCollisionResult->entityIntersections;
    }

    if (avatarIntersections.size()) {
        avatarIntersections.insert(avatarIntersections.cend(), newCollisionResult->avatarIntersections.begin(), newCollisionResult->avatarIntersections.end());
    } else {
        avatarIntersections = newCollisionResult->avatarIntersections;
    }

    intersects = entityIntersections.size() || avatarIntersections.size();

    return std::make_shared<CollisionPickResult>(*this);
}

void buildObjectIntersectionsMap(IntersectionType intersectionType, const std::vector<ContactTestResult>& objectIntersections, std::unordered_map<QUuid, QVariantMap>& intersections, std::unordered_map<QUuid, QVariantList>& collisionPointPairs) {
    for (auto& objectIntersection : objectIntersections) {
        auto at = intersections.find(objectIntersection.foundID);
        if (at == intersections.end()) {
            QVariantMap intersectingObject;
            intersectingObject["id"] = objectIntersection.foundID;
            intersectingObject["type"] = intersectionType;
            intersections[objectIntersection.foundID] = intersectingObject;

            collisionPointPairs[objectIntersection.foundID] = QVariantList();
        }

        QVariantMap collisionPointPair;
        collisionPointPair["pointOnPick"] = vec3toVariant(objectIntersection.testCollisionPoint);
        collisionPointPair["pointOnObject"] = vec3toVariant(objectIntersection.foundCollisionPoint);
        collisionPointPair["normalOnPick"] = vec3toVariant(objectIntersection.collisionNormal);

        collisionPointPairs[objectIntersection.foundID].append(collisionPointPair);
    }
}

/*@jsdoc
 * An intersection result for a collision pick.
 *
 * @typedef {object} CollisionPickResult
 * @property {boolean} intersects - <code>true</code> if there is at least one intersection, <code>false</code> if there isn't.
 * @property {IntersectingObject[]} intersectingObjects - All objects which intersect with the <code>collisionRegion</code>.
 * @property {CollisionRegion} collisionRegion - The collision region that was used. Valid even if there was no intersection.
 */

/*@jsdoc
 * Information about a {@link CollisionPick}'s intersection with an object.
 *
 * @typedef {object} IntersectingObject
 * @property {Uuid} id - The ID of the object.
 * @property {IntersectionType} type - The type of the object, either <code>1</code> for INTERSECTED_ENTITY or <code>3</code> 
 *     for INTERSECTED_AVATAR.
 * @property {CollisionContact[]} collisionContacts - Information on the penetration between the pick and the object.
 */

/*@jsdoc
 * A pair of points that represents part of an overlap between a {@link CollisionPick} and an object in the physics engine. 
 * Points which are further apart represent deeper overlap.
 *
 * @typedef {object} CollisionContact
 * @property {Vec3} pointOnPick - A point representing a penetration of the object's surface into the volume of the pick, in 
 *     world coordinates.
 * @property {Vec3} pointOnObject - A point representing a penetration of the pick's surface into the volume of the object, in 
 *     world coordinates.
 * @property {Vec3} normalOnPick - The normal vector pointing away from the pick, representing the direction of collision.
 */

QVariantMap CollisionPickResult::toVariantMap() const {
    QVariantMap variantMap;

    variantMap["intersects"] = intersects;

    std::unordered_map<QUuid, QVariantMap> intersections;
    std::unordered_map<QUuid, QVariantList> collisionPointPairs;

    buildObjectIntersectionsMap(ENTITY, entityIntersections, intersections, collisionPointPairs);
    buildObjectIntersectionsMap(AVATAR, avatarIntersections, intersections, collisionPointPairs);

    QVariantList qIntersectingObjects;
    for (auto& intersectionKeyVal : intersections) {
        const QUuid& id = intersectionKeyVal.first;
        QVariantMap& intersection = intersectionKeyVal.second;

        intersection["collisionContacts"] = collisionPointPairs[id];
        qIntersectingObjects.append(intersection);
    }

    variantMap["intersectingObjects"] = qIntersectingObjects;
    variantMap["collisionRegion"] = pickVariant;

    return variantMap;
}

bool CollisionPick::isLoaded() const {
    return !_mathPick.shouldComputeShapeInfo() || (_cachedResource && _cachedResource->isLoaded());
}

bool CollisionPick::getShapeInfoReady(const CollisionRegion& pick) {
    if (_mathPick.shouldComputeShapeInfo()) {
        if (_cachedResource && _cachedResource->isLoaded()) {
            computeShapeInfo(pick, *_mathPick.shapeInfo, _cachedResource);
            _mathPick.loaded = true;
        } else {
            _mathPick.loaded = false;
        }
    } else {
        computeShapeInfoDimensionsOnly(pick, *_mathPick.shapeInfo, _cachedResource);
        _mathPick.loaded = true;
    }

    return _mathPick.loaded;
}

void CollisionPick::computeShapeInfoDimensionsOnly(const CollisionRegion& pick, ShapeInfo& shapeInfo, QSharedPointer<GeometryResource> resource) {
    ShapeType type = shapeInfo.getType();
    glm::vec3 dimensions = pick.transform.getScale();
    QString modelURL = (resource ? resource->getURL().toString() : "");
    if (type == SHAPE_TYPE_COMPOUND) {
        shapeInfo.setParams(type, dimensions, modelURL);
    } else if (type >= SHAPE_TYPE_SIMPLE_HULL && type <= SHAPE_TYPE_STATIC_MESH) {
        shapeInfo.setParams(type, 0.5f * dimensions, modelURL);
    } else {
        shapeInfo.setParams(type, 0.5f * dimensions, modelURL);
    }
}

void CollisionPick::computeShapeInfo(const CollisionRegion& pick, ShapeInfo& shapeInfo, QSharedPointer<GeometryResource> resource) {
    // This code was copied and modified from RenderableModelEntityItem::computeShapeInfo
    // TODO: Move to some shared code area (in entities-renderer? model-networking?)
    // after we verify this is working and do a diff comparison with RenderableModelEntityItem::computeShapeInfo
    // to consolidate the code.
    // We may also want to make computeShapeInfo always abstract away from the gpu model mesh, like it does here.
    const uint32_t TRIANGLE_STRIDE = 3;
    const uint32_t QUAD_STRIDE = 4;

    ShapeType type = shapeInfo.getType();
    glm::vec3 dimensions = pick.transform.getScale();
    if (type == SHAPE_TYPE_COMPOUND) {
        // should never fall in here when collision model not fully loaded
        // TODO: assert that all geometries exist and are loaded
        //assert(_model && _model->isLoaded() && _compoundShapeResource && _compoundShapeResource->isLoaded());
        const HFMModel& collisionModel = resource->getHFMModel();

        ShapeInfo::PointCollection& pointCollection = shapeInfo.getPointCollection();
        pointCollection.clear();
        uint32_t i = 0;

        // the way OBJ files get read, each section under a "g" line is its own meshPart.  We only expect
        // to find one actual "mesh" (with one or more meshParts in it), but we loop over the meshes, just in case.
        foreach (const HFMMesh& mesh, collisionModel.meshes) {
            // each meshPart is a convex hull
            foreach (const HFMMeshPart &meshPart, mesh.parts) {
                pointCollection.push_back(QVector<glm::vec3>());
                ShapeInfo::PointList& pointsInPart = pointCollection[i];

                // run through all the triangles and (uniquely) add each point to the hull
                uint32_t numIndices = (uint32_t)meshPart.triangleIndices.size();
                // TODO: assert rather than workaround after we start sanitizing HFMMesh higher up
                //assert(numIndices % TRIANGLE_STRIDE == 0);
                numIndices -= numIndices % TRIANGLE_STRIDE; // WORKAROUND lack of sanity checking in FBXSerializer

                for (uint32_t j = 0; j < numIndices; j += TRIANGLE_STRIDE) {
                    glm::vec3 p0 = mesh.vertices[meshPart.triangleIndices[j]];
                    glm::vec3 p1 = mesh.vertices[meshPart.triangleIndices[j + 1]];
                    glm::vec3 p2 = mesh.vertices[meshPart.triangleIndices[j + 2]];
                    if (!pointsInPart.contains(p0)) {
                        pointsInPart << p0;
                    }
                    if (!pointsInPart.contains(p1)) {
                        pointsInPart << p1;
                    }
                    if (!pointsInPart.contains(p2)) {
                        pointsInPart << p2;
                    }
                }

                // run through all the quads and (uniquely) add each point to the hull
                numIndices = (uint32_t)meshPart.quadIndices.size();
                // TODO: assert rather than workaround after we start sanitizing HFMMesh higher up
                //assert(numIndices % QUAD_STRIDE == 0);
                numIndices -= numIndices % QUAD_STRIDE; // WORKAROUND lack of sanity checking in FBXSerializer

                for (uint32_t j = 0; j < numIndices; j += QUAD_STRIDE) {
                    glm::vec3 p0 = mesh.vertices[meshPart.quadIndices[j]];
                    glm::vec3 p1 = mesh.vertices[meshPart.quadIndices[j + 1]];
                    glm::vec3 p2 = mesh.vertices[meshPart.quadIndices[j + 2]];
                    glm::vec3 p3 = mesh.vertices[meshPart.quadIndices[j + 3]];
                    if (!pointsInPart.contains(p0)) {
                        pointsInPart << p0;
                    }
                    if (!pointsInPart.contains(p1)) {
                        pointsInPart << p1;
                    }
                    if (!pointsInPart.contains(p2)) {
                        pointsInPart << p2;
                    }
                    if (!pointsInPart.contains(p3)) {
                        pointsInPart << p3;
                    }
                }

                if (pointsInPart.size() == 0) {
                    qCDebug(scriptengine) << "Warning -- meshPart has no faces";
                    pointCollection.pop_back();
                    continue;
                }
                ++i;
            }
        }

        // We expect that the collision model will have the same units and will be displaced
        // from its origin in the same way the visual model is.  The visual model has
        // been centered and probably scaled.  We take the scaling and offset which were applied
        // to the visual model and apply them to the collision model (without regard for the
        // collision model's extents).

        glm::vec3 scaleToFit = dimensions / resource->getHFMModel().getUnscaledMeshExtents().size();
        // multiply each point by scale
        for (int32_t i = 0; i < pointCollection.size(); i++) {
            for (int32_t j = 0; j < pointCollection[i].size(); j++) {
                // back compensate for registration so we can apply that offset to the shapeInfo later
                pointCollection[i][j] = scaleToFit * pointCollection[i][j];
            }
        }
        shapeInfo.setParams(type, dimensions, resource->getURL().toString());
    } else if (type >= SHAPE_TYPE_SIMPLE_HULL && type <= SHAPE_TYPE_STATIC_MESH) {
        const HFMModel& hfmModel = resource->getHFMModel();
        int numHFMMeshes = hfmModel.meshes.size();
        int totalNumVertices = 0;
        for (int i = 0; i < numHFMMeshes; i++) {
            const HFMMesh& mesh = hfmModel.meshes.at(i);
            totalNumVertices += mesh.vertices.size();
        }
        const int32_t MAX_VERTICES_PER_STATIC_MESH = 1e6;
        if (totalNumVertices > MAX_VERTICES_PER_STATIC_MESH) {
            qWarning() << "model" << "has too many vertices" << totalNumVertices << "and will collide as a box.";
            shapeInfo.setParams(SHAPE_TYPE_BOX, 0.5f * dimensions);
            return;
        }

        auto& meshes = resource->getHFMModel().meshes;
        int32_t numMeshes = (int32_t)(meshes.size());

        const int MAX_ALLOWED_MESH_COUNT = 1000;
        if (numMeshes > MAX_ALLOWED_MESH_COUNT) {
            // too many will cause the deadlock timer to throw...
            shapeInfo.setParams(SHAPE_TYPE_BOX, 0.5f * dimensions);
            return;
        }

        ShapeInfo::PointCollection& pointCollection = shapeInfo.getPointCollection();
        pointCollection.clear();
        if (type == SHAPE_TYPE_SIMPLE_COMPOUND) {
            pointCollection.resize(numMeshes);
        } else {
            pointCollection.resize(1);
        }

        ShapeInfo::TriangleIndices& triangleIndices = shapeInfo.getTriangleIndices();
        triangleIndices.clear();

        Extents extents;
        int32_t meshCount = 0;
        int32_t pointListIndex = 0;
        for (auto& mesh : meshes) {
            if (!mesh.vertices.size()) {
                continue;
            }
            QVector<glm::vec3> vertices = mesh.vertices;

            ShapeInfo::PointList& points = pointCollection[pointListIndex];

            // reserve room
            int32_t sizeToReserve = (int32_t)(vertices.count());
            if (type == SHAPE_TYPE_SIMPLE_COMPOUND) {
                // a list of points for each mesh
                pointListIndex++;
            } else {
                // only one list of points
                sizeToReserve += (int32_t)points.size();
            }
            points.reserve(sizeToReserve);

            // copy points
            const glm::vec3* vertexItr = vertices.cbegin();
            while (vertexItr != vertices.cend()) {
                glm::vec3 point = *vertexItr;
                points.push_back(point);
                extents.addPoint(point);
                ++vertexItr;
            }

            if (type == SHAPE_TYPE_STATIC_MESH) {
                // copy into triangleIndices
                size_t triangleIndicesCount = 0;
                for (const HFMMeshPart& meshPart : mesh.parts) {
                    triangleIndicesCount += meshPart.triangleIndices.count();
                }
                triangleIndices.reserve((int)triangleIndicesCount);

                for (const HFMMeshPart& meshPart : mesh.parts) {
                    const int* indexItr = meshPart.triangleIndices.cbegin();
                    while (indexItr != meshPart.triangleIndices.cend()) {
                        triangleIndices.push_back(*indexItr);
                        ++indexItr;
                    }
                }
            } else if (type == SHAPE_TYPE_SIMPLE_COMPOUND) {
                // for each mesh copy unique part indices, separated by special bogus (flag) index values
                for (const HFMMeshPart& meshPart : mesh.parts) {
                    // collect unique list of indices for this part
                    std::set<int32_t> uniqueIndices;
                    auto numIndices = meshPart.triangleIndices.count();
                    // TODO: assert rather than workaround after we start sanitizing HFMMesh higher up
                    //assert(numIndices% TRIANGLE_STRIDE == 0);
                    numIndices -= numIndices % TRIANGLE_STRIDE; // WORKAROUND lack of sanity checking in FBXSerializer

                    auto indexItr = meshPart.triangleIndices.cbegin();
                    while (indexItr != meshPart.triangleIndices.cend()) {
                        uniqueIndices.insert(*indexItr);
                        ++indexItr;
                    }

                    // store uniqueIndices in triangleIndices
                    triangleIndices.reserve(triangleIndices.size() + (int32_t)uniqueIndices.size());
                    for (auto index : uniqueIndices) {
                        triangleIndices.push_back(index);
                    }
                    // flag end of part
                    triangleIndices.push_back(END_OF_MESH_PART);
                }
                // flag end of mesh
                triangleIndices.push_back(END_OF_MESH);
            }
            ++meshCount;
        }

        // scale and shift
        glm::vec3 extentsSize = extents.size();
        glm::vec3 scaleToFit = dimensions / extentsSize;
        for (int32_t i = 0; i < 3; ++i) {
            if (extentsSize[i] < 1.0e-6f) {
                scaleToFit[i] = 1.0f;
            }
        }
        for (auto points : pointCollection) {
            for (int32_t i = 0; i < points.size(); ++i) {
                points[i] = (points[i] * scaleToFit);
            }
        }

        shapeInfo.setParams(type, 0.5f * dimensions, resource->getURL().toString());
    }
}

CollisionPick::CollisionPick(const PickFilter& filter, float maxDistance, bool enabled, bool scaleWithParent, CollisionRegion collisionRegion, PhysicsEnginePointer physicsEngine) :
    Pick(collisionRegion, filter, maxDistance, enabled),
    _scaleWithParent(scaleWithParent),
    _physicsEngine(physicsEngine) {
    if (collisionRegion.shouldComputeShapeInfo()) {
        _cachedResource = DependencyManager::get<ModelCache>()->getCollisionGeometryResource(collisionRegion.modelURL);
    }
    _mathPick.loaded = isLoaded();
}

CollisionRegion CollisionPick::getMathematicalPick() const {
    CollisionRegion mathPick = _mathPick;
    mathPick.loaded = isLoaded();
    if (parentTransform) {
        Transform parentTransformValue = parentTransform->getTransform();
        mathPick.transform = parentTransformValue.worldTransform(mathPick.transform);

        if (_scaleWithParent) {
            glm::vec3 scale = parentTransformValue.getScale();
            float largestDimension = glm::max(glm::max(scale.x, scale.y), scale.z);
            mathPick.threshold *= largestDimension;
        } else {
            // We need to undo parent scaling after-the-fact because the parent's scale was needed to calculate this mathPick's position
            mathPick.transform.setScale(_mathPick.transform.getScale());
        }
    }
    return mathPick;
}

void CollisionPick::filterIntersections(std::vector<ContactTestResult>& intersections) const {
    const QVector<QUuid>& ignoreItems = getIgnoreItems();
    const QVector<QUuid>& includeItems = getIncludeItems();
    bool isWhitelist = !includeItems.empty();

    if (!isWhitelist && ignoreItems.empty()) {
        return;
    }

    std::vector<ContactTestResult> filteredIntersections;

    int n = (int)intersections.size();
    for (int i = 0; i < n; i++) {
        auto& intersection = intersections[i];
        const QUuid& id = intersection.foundID;
        if (!ignoreItems.contains(id) && (!isWhitelist || includeItems.contains(id))) {
            filteredIntersections.push_back(intersection);
        }
    }

    intersections = filteredIntersections;
}

PickResultPointer CollisionPick::getEntityIntersection(const CollisionRegion& pick) {
    if (!pick.loaded) {
        // Cannot compute result
        return std::make_shared<CollisionPickResult>(pick.toVariantMap(), std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
    }
    getShapeInfoReady(pick);
    
    auto entityIntersections = _physicsEngine->contactTest(USER_COLLISION_MASK_ENTITIES, *_mathPick.shapeInfo, pick.transform, pick.collisionGroup, pick.threshold);
    filterIntersections(entityIntersections);
    return std::make_shared<CollisionPickResult>(pick, entityIntersections, std::vector<ContactTestResult>());
}

PickResultPointer CollisionPick::getAvatarIntersection(const CollisionRegion& pick) {
    if (!pick.loaded) {
        // Cannot compute result
        return std::make_shared<CollisionPickResult>(pick, std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
    }
    getShapeInfoReady(pick);
    
    auto avatarIntersections = _physicsEngine->contactTest(USER_COLLISION_MASK_AVATARS, *_mathPick.shapeInfo, pick.transform, pick.collisionGroup, pick.threshold);
    filterIntersections(avatarIntersections);
    return std::make_shared<CollisionPickResult>(pick, std::vector<ContactTestResult>(), avatarIntersections);
}

PickResultPointer CollisionPick::getHUDIntersection(const CollisionRegion& pick) {
    return std::make_shared<CollisionPickResult>(pick, std::vector<ContactTestResult>(), std::vector<ContactTestResult>());
}

Transform CollisionPick::getResultTransform() const {
    Transform transform;
    transform.setTranslation(_mathPick.transform.getTranslation());
    return transform;
}
