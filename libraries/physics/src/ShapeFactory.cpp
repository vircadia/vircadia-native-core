//
//  ShapeFactory.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.12.01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include <SharedUtil.h> // for MILLIMETERS_PER_METER

#include "ShapeFactory.h"
#include "BulletUtil.h"

// These are the same normalized directions used by the btShapeHull class.
// 12 points for the face centers of a duodecohedron plus another 30 points
// for the midpoints the edges, for a total of 42.
const uint32_t NUM_UNIT_SPHERE_DIRECTIONS = 42;
static const btVector3 _unitSphereDirections[NUM_UNIT_SPHERE_DIRECTIONS] = {
    btVector3(btScalar(0.000000) , btScalar(-0.000000),btScalar(-1.000000)),
    btVector3(btScalar(0.723608) , btScalar(-0.525725),btScalar(-0.447219)),
    btVector3(btScalar(-0.276388) , btScalar(-0.850649),btScalar(-0.447219)),
    btVector3(btScalar(-0.894426) , btScalar(-0.000000),btScalar(-0.447216)),
    btVector3(btScalar(-0.276388) , btScalar(0.850649),btScalar(-0.447220)),
    btVector3(btScalar(0.723608) , btScalar(0.525725),btScalar(-0.447219)),
    btVector3(btScalar(0.276388) , btScalar(-0.850649),btScalar(0.447220)),
    btVector3(btScalar(-0.723608) , btScalar(-0.525725),btScalar(0.447219)),
    btVector3(btScalar(-0.723608) , btScalar(0.525725),btScalar(0.447219)),
    btVector3(btScalar(0.276388) , btScalar(0.850649),btScalar(0.447219)),
    btVector3(btScalar(0.894426) , btScalar(0.000000),btScalar(0.447216)),
    btVector3(btScalar(-0.000000) , btScalar(0.000000),btScalar(1.000000)),
    btVector3(btScalar(0.425323) , btScalar(-0.309011),btScalar(-0.850654)),
    btVector3(btScalar(-0.162456) , btScalar(-0.499995),btScalar(-0.850654)),
    btVector3(btScalar(0.262869) , btScalar(-0.809012),btScalar(-0.525738)),
    btVector3(btScalar(0.425323) , btScalar(0.309011),btScalar(-0.850654)),
    btVector3(btScalar(0.850648) , btScalar(-0.000000),btScalar(-0.525736)),
    btVector3(btScalar(-0.525730) , btScalar(-0.000000),btScalar(-0.850652)),
    btVector3(btScalar(-0.688190) , btScalar(-0.499997),btScalar(-0.525736)),
    btVector3(btScalar(-0.162456) , btScalar(0.499995),btScalar(-0.850654)),
    btVector3(btScalar(-0.688190) , btScalar(0.499997),btScalar(-0.525736)),
    btVector3(btScalar(0.262869) , btScalar(0.809012),btScalar(-0.525738)),
    btVector3(btScalar(0.951058) , btScalar(0.309013),btScalar(0.000000)),
    btVector3(btScalar(0.951058) , btScalar(-0.309013),btScalar(0.000000)),
    btVector3(btScalar(0.587786) , btScalar(-0.809017),btScalar(0.000000)),
    btVector3(btScalar(0.000000) , btScalar(-1.000000),btScalar(0.000000)),
    btVector3(btScalar(-0.587786) , btScalar(-0.809017),btScalar(0.000000)),
    btVector3(btScalar(-0.951058) , btScalar(-0.309013),btScalar(-0.000000)),
    btVector3(btScalar(-0.951058) , btScalar(0.309013),btScalar(-0.000000)),
    btVector3(btScalar(-0.587786) , btScalar(0.809017),btScalar(-0.000000)),
    btVector3(btScalar(-0.000000) , btScalar(1.000000),btScalar(-0.000000)),
    btVector3(btScalar(0.587786) , btScalar(0.809017),btScalar(-0.000000)),
    btVector3(btScalar(0.688190) , btScalar(-0.499997),btScalar(0.525736)),
    btVector3(btScalar(-0.262869) , btScalar(-0.809012),btScalar(0.525738)),
    btVector3(btScalar(-0.850648) , btScalar(0.000000),btScalar(0.525736)),
    btVector3(btScalar(-0.262869) , btScalar(0.809012),btScalar(0.525738)),
    btVector3(btScalar(0.688190) , btScalar(0.499997),btScalar(0.525736)),
    btVector3(btScalar(0.525730) , btScalar(0.000000),btScalar(0.850652)),
    btVector3(btScalar(0.162456) , btScalar(-0.499995),btScalar(0.850654)),
    btVector3(btScalar(-0.425323) , btScalar(-0.309011),btScalar(0.850654)),
    btVector3(btScalar(-0.425323) , btScalar(0.309011),btScalar(0.850654)),
    btVector3(btScalar(0.162456) , btScalar(0.499995),btScalar(0.850654))
};


// util method
btConvexHullShape* createConvexHull(const ShapeInfo::PointList& points) {
    assert(points.size() > 0);

    btConvexHullShape* hull = new btConvexHullShape();
    glm::vec3 center = points[0];
    glm::vec3 maxCorner = center;
    glm::vec3 minCorner = center;
    for (int i = 1; i < points.size(); i++) {
        center += points[i];
        maxCorner = glm::max(maxCorner, points[i]);
        minCorner = glm::min(minCorner, points[i]);
    }
    center /= (float)(points.size());

    float margin = hull->getMargin();

    // Bullet puts "margins" around all the collision shapes.  This can cause objects that use ConvexHull shapes
    // to have visible gaps between them and the surface they touch.  One option is to reduce the size of the margin
    // but this can reduce the performance and stability of the simulation (e.g. the GJK algorithm will fail to provide
    // nearest contact points and narrow-phase collisions will fall into more expensive code paths).  Alternatively
    // one can shift the geometry of the shape to make the margin surface approximately close to the visible surface.
    // This is the strategy we try, but if the object is too small then we start to reduce the margin down to some minimum.

    const float MIN_MARGIN = 0.01f;
    glm::vec3 diagonal = maxCorner - minCorner;
    float smallestDimension = glm::min(diagonal[0], diagonal[1]);
    smallestDimension = glm::min(smallestDimension, diagonal[2]);
    const float MIN_DIMENSION = 2.0f * MIN_MARGIN + 0.001f;
    if (smallestDimension < MIN_DIMENSION) {
        for (int i = 0; i < 3; ++i) {
            if (diagonal[i] < MIN_DIMENSION) {
                diagonal[i] = MIN_DIMENSION;
            }
        }
        smallestDimension = MIN_DIMENSION;
    }
    margin = glm::min(glm::max(0.5f * smallestDimension, MIN_MARGIN), margin);
    hull->setMargin(margin);

    // add the points, correcting for margin
    glm::vec3 relativeScale = (diagonal - glm::vec3(2.0f * margin)) / diagonal;
    glm::vec3 correctedPoint;
    for (int i = 0; i < points.size(); ++i) {
        correctedPoint = (points[i] - center) * relativeScale + center;
        hull->addPoint(btVector3(correctedPoint[0], correctedPoint[1], correctedPoint[2]), false);
    }

    uint32_t numPoints = (uint32_t)hull->getNumPoints();
    if (numPoints > MAX_HULL_POINTS) {
        // we have too many points, so we compute point projections along canonical unit vectors
        // and keep the those that project the farthest
        btVector3 btCenter = glmToBullet(center);
        btVector3* shapePoints = hull->getUnscaledPoints();
        std::vector<uint32_t> finalIndices;
        finalIndices.reserve(NUM_UNIT_SPHERE_DIRECTIONS);
        for (uint32_t i = 0; i < NUM_UNIT_SPHERE_DIRECTIONS; ++i) {
            uint32_t bestIndex = 0;
            btScalar maxDistance = _unitSphereDirections[i].dot(shapePoints[0] - btCenter);
            for (uint32_t j = 1; j < numPoints; ++j) {
                btScalar distance = _unitSphereDirections[i].dot(shapePoints[j] - btCenter);
                if (distance > maxDistance) {
                    maxDistance = distance;
                    bestIndex = j;
                }
            }
            bool keep = true;
            for (uint32_t j = 0; j < finalIndices.size(); ++j) {
                if (finalIndices[j] == bestIndex) {
                    keep = false;
                    break;
                }
            }
            if (keep) {
                finalIndices.push_back(bestIndex);
            }
        }

        // we cannot copy Bullet shapes so we must create a new one...
        btConvexHullShape* newHull = new btConvexHullShape();
        for (uint32_t i = 0; i < finalIndices.size(); ++i) {
            newHull->addPoint(shapePoints[finalIndices[i]], false);
        }
        // ...and delete the old one
        delete hull;
        hull = newHull;
    }

    hull->recalcLocalAabb();
    return hull;
}

// util method
btTriangleIndexVertexArray* createStaticMeshArray(const ShapeInfo& info) {
    assert(info.getType() == SHAPE_TYPE_STATIC_MESH); // should only get here for mesh shapes

    const ShapeInfo::PointCollection& pointCollection = info.getPointCollection();

    if (pointCollection.size() < 1) {
        // no lists of points to work with
        return nullptr;
    }

    // we only use the first point collection
    const ShapeInfo::PointList& pointList = pointCollection[0];
    if (pointList.size() < 3) {
        // not enough distinct points to make a non-degenerate triangle
        return nullptr;
    }

    const ShapeInfo::TriangleIndices& triangleIndices = info.getTriangleIndices();
    int32_t numIndices = triangleIndices.size();
    if (numIndices < 3) {
        // not enough indices to make a single triangle
        return nullptr;
    }

    // allocate mesh buffers
    btIndexedMesh mesh;
    const int32_t VERTICES_PER_TRIANGLE = 3;
    mesh.m_numTriangles = numIndices / VERTICES_PER_TRIANGLE;
    if (numIndices < INT16_MAX) {
        // small number of points so we can use 16-bit indices
        mesh.m_triangleIndexBase = new unsigned char[sizeof(int16_t) * (size_t)numIndices];
        mesh.m_indexType = PHY_SHORT;
        mesh.m_triangleIndexStride = VERTICES_PER_TRIANGLE * sizeof(int16_t);
    } else {
        mesh.m_triangleIndexBase = new unsigned char[sizeof(int32_t) * (size_t)numIndices];
        mesh.m_indexType = PHY_INTEGER;
        mesh.m_triangleIndexStride = VERTICES_PER_TRIANGLE * sizeof(int32_t);
    }
    mesh.m_numVertices = pointList.size();
    mesh.m_vertexBase = new unsigned char[VERTICES_PER_TRIANGLE * sizeof(btScalar) * (size_t)mesh.m_numVertices];
    mesh.m_vertexStride = VERTICES_PER_TRIANGLE * sizeof(btScalar);
    mesh.m_vertexType = PHY_FLOAT;

    // copy data into buffers
    btScalar* vertexData = static_cast<btScalar*>((void*)(mesh.m_vertexBase));
    for (int32_t i = 0; i < mesh.m_numVertices; ++i) {
        int32_t j = i * VERTICES_PER_TRIANGLE;
        const glm::vec3& point = pointList[i];
        vertexData[j] = point.x;
        vertexData[j + 1] = point.y;
        vertexData[j + 2] = point.z;
    }
    if (numIndices < INT16_MAX) {
        int16_t* indices = static_cast<int16_t*>((void*)(mesh.m_triangleIndexBase));
        for (int32_t i = 0; i < numIndices; ++i) {
            indices[i] = (int16_t)triangleIndices[i];
        }
    } else {
        int32_t* indices = static_cast<int32_t*>((void*)(mesh.m_triangleIndexBase));
        for (int32_t i = 0; i < numIndices; ++i) {
            indices[i] = triangleIndices[i];
        }
    }

    // store buffers in a new dataArray and return the pointer
    // (external StaticMeshShape will own all of the data that was allocated here)
    btTriangleIndexVertexArray* dataArray = new btTriangleIndexVertexArray;
    dataArray->addIndexedMesh(mesh, mesh.m_indexType);
    return dataArray;
}

// util method
void deleteStaticMeshArray(btTriangleIndexVertexArray* dataArray) {
    assert(dataArray);
    IndexedMeshArray& meshes = dataArray->getIndexedMeshArray();
    for (int32_t i = 0; i < meshes.size(); ++i) {
        btIndexedMesh mesh = meshes[i];
        mesh.m_numTriangles = 0;
        delete [] mesh.m_triangleIndexBase;
        mesh.m_triangleIndexBase = nullptr;
        mesh.m_numVertices = 0;
        delete [] mesh.m_vertexBase;
        mesh.m_vertexBase = nullptr;
    }
    meshes.clear();
    delete dataArray;
}

const btCollisionShape* ShapeFactory::createShapeFromInfo(const ShapeInfo& info) {
    btCollisionShape* shape = NULL;
    int type = info.getType();
    switch(type) {
        case SHAPE_TYPE_BOX: {
            shape = new btBoxShape(glmToBullet(info.getHalfExtents()));
        }
        break;
        case SHAPE_TYPE_SPHERE: {
            float radius = info.getHalfExtents().x;
            shape = new btSphereShape(radius);
        }
        break;
        case SHAPE_TYPE_CAPSULE_Y: {
            glm::vec3 halfExtents = info.getHalfExtents();
            float radius = halfExtents.x;
            float height = 2.0f * halfExtents.y;
            shape = new btCapsuleShape(radius, height);
        }
        break;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_SIMPLE_HULL: {
            const ShapeInfo::PointCollection& pointCollection = info.getPointCollection();
            uint32_t numSubShapes = info.getNumSubShapes();
            if (numSubShapes == 1) {
                shape = createConvexHull(pointCollection[0]);
            } else {
                auto compound = new btCompoundShape();
                btTransform trans;
                trans.setIdentity();
                foreach (const ShapeInfo::PointList& hullPoints, pointCollection) {
                    btConvexHullShape* hull = createConvexHull(hullPoints);
                    compound->addChildShape(trans, hull);
                }
                shape = compound;
            }
        }
        break;
        case SHAPE_TYPE_SIMPLE_COMPOUND: {
            const ShapeInfo::PointCollection& pointCollection = info.getPointCollection();
            const ShapeInfo::TriangleIndices& triangleIndices = info.getTriangleIndices();
            uint32_t numIndices = triangleIndices.size();
            uint32_t numMeshes = info.getNumSubShapes();
            const uint32_t MIN_NUM_SIMPLE_COMPOUND_INDICES = 2; // END_OF_MESH_PART + END_OF_MESH
            if (numMeshes > 0 && numIndices > MIN_NUM_SIMPLE_COMPOUND_INDICES) {
                uint32_t i = 0;
                std::vector<btConvexHullShape*> hulls;
                for (auto& points : pointCollection) {
                    // build a hull around each part
                    while (i < numIndices) {
                        ShapeInfo::PointList hullPoints;
                        hullPoints.reserve(points.size());
                        while (i < numIndices) {
                            int32_t j = triangleIndices[i];
                            ++i;
                            if (j == END_OF_MESH_PART) {
                                // end of part
                                break;
                            }
                            hullPoints.push_back(points[j]);
                        }
                        if (hullPoints.size() > 0) {
                            btConvexHullShape* hull = createConvexHull(hullPoints);
                            hulls.push_back(hull);
                        }

                        assert(i < numIndices);
                        if (triangleIndices[i] == END_OF_MESH) {
                            // end of mesh
                            ++i;
                            break;
                        }
                    }
                }
                uint32_t numHulls = (uint32_t)hulls.size();
                if (numHulls == 1) {
                    shape = hulls[0];
                } else {
                    auto compound = new btCompoundShape();
                    btTransform trans;
                    trans.setIdentity();
                    for (auto hull : hulls) {
                        compound->addChildShape(trans, hull);
                    }
                    shape = compound;
                }
            }
        }
        break;
        case SHAPE_TYPE_STATIC_MESH: {
            btTriangleIndexVertexArray* dataArray = createStaticMeshArray(info);
            if (dataArray) {
                shape = new StaticMeshShape(dataArray);
            }
        }
        break;
    }
    if (shape) {
        if (glm::length2(info.getOffset()) > MIN_SHAPE_OFFSET * MIN_SHAPE_OFFSET) {
            // this shape has an offset, which we support by wrapping the true shape
            // in a btCompoundShape with a local transform
            auto compound = new btCompoundShape();
            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(glmToBullet(info.getOffset()));
            compound->addChildShape(trans, shape);
            shape = compound;
        }
    }
    return shape;
}

void ShapeFactory::deleteShape(const btCollisionShape* shape) {
    assert(shape);
    // ShapeFactory is responsible for deleting all shapes, even the const ones that are stored
    // in the ShapeManager, so we must cast to non-const here when deleting.
    // so we cast to non-const here when deleting memory.
    btCollisionShape* nonConstShape = const_cast<btCollisionShape*>(shape);
    if (nonConstShape->getShapeType() == (int)COMPOUND_SHAPE_PROXYTYPE) {
        btCompoundShape* compoundShape = static_cast<btCompoundShape*>(nonConstShape);
        const int numChildShapes = compoundShape->getNumChildShapes();
        for (int i = 0; i < numChildShapes; i ++) {
            btCollisionShape* childShape = compoundShape->getChildShape(i);
            if (childShape->getShapeType() == (int)COMPOUND_SHAPE_PROXYTYPE) {
                // recurse
                ShapeFactory::deleteShape(childShape);
            } else {
                delete childShape;
            }
        }
    }
    delete nonConstShape;
}

// the dataArray must be created before we create the StaticMeshShape
ShapeFactory::StaticMeshShape::StaticMeshShape(btTriangleIndexVertexArray* dataArray)
:   btBvhTriangleMeshShape(dataArray, true), _dataArray(dataArray) {
    assert(dataArray);
}

ShapeFactory::StaticMeshShape::~StaticMeshShape() {
    deleteStaticMeshArray(_dataArray);
    _dataArray = nullptr;
}
