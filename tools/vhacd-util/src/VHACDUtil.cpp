//
//  VHACDUtil.cpp
//  tools/vhacd/src
//
//  Created by Virendra Singh on 2/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VHACDUtil.h"

#include <unordered_map>
#include <QVector>

#include <NumericalConstants.h>


// FBXReader jumbles the order of the meshes by reading them back out of a hashtable.  This will put
// them back in the order in which they appeared in the file.
bool FBXGeometryLessThan(const FBXMesh& e1, const FBXMesh& e2) {
    return e1.meshIndex < e2.meshIndex;
}
void reSortFBXGeometryMeshes(FBXGeometry& geometry) {
    qSort(geometry.meshes.begin(), geometry.meshes.end(), FBXGeometryLessThan);
}


// Read all the meshes from provided FBX file
bool vhacd::VHACDUtil::loadFBX(const QString filename, FBXGeometry& result) {
    if (_verbose) {
        qDebug() << "reading FBX file =" << filename << "...";
    }

    // open the fbx file
    QFile fbx(filename);
    if (!fbx.open(QIODevice::ReadOnly)) {
        qWarning() << "unable to open FBX file =" << filename;
        return false;
    }
    try {
        QByteArray fbxContents = fbx.readAll();
        FBXGeometry* geom;
        if (filename.toLower().endsWith(".obj")) {
            bool combineParts = false;
            geom = OBJReader().readOBJ(fbxContents, QVariantHash(), combineParts);
        } else if (filename.toLower().endsWith(".fbx")) {
            geom = readFBX(fbxContents, QVariantHash(), filename);
        } else {
            qWarning() << "file has unknown extension" << filename;
            return false;
        }
        result = *geom;
        delete geom;

        reSortFBXGeometryMeshes(result);
    } catch (const QString& error) {
        qWarning() << "error reading" << filename << ":" << error;
        return false;
    }

    return true;
}


void getTrianglesInMeshPart(const FBXMeshPart &meshPart, std::vector<int>& triangleIndices) {
    // append triangle indices
    triangleIndices.reserve(triangleIndices.size() + (size_t)meshPart.triangleIndices.size());
    for (auto index : meshPart.triangleIndices) {
        triangleIndices.push_back(index);
    }

    // convert quads to triangles
    const uint32_t QUAD_STRIDE = 4;
    uint32_t numIndices = (uint32_t)meshPart.quadIndices.size();
    for (uint32_t i = 0; i < numIndices; i += QUAD_STRIDE) {
        uint32_t p0Index = meshPart.quadIndices[i];
        uint32_t p1Index = meshPart.quadIndices[i + 1];
        uint32_t p2Index = meshPart.quadIndices[i + 2];
        uint32_t p3Index = meshPart.quadIndices[i + 3];
        // split each quad into two triangles
        triangleIndices.push_back(p0Index);
        triangleIndices.push_back(p1Index);
        triangleIndices.push_back(p2Index);
        triangleIndices.push_back(p0Index);
        triangleIndices.push_back(p2Index);
        triangleIndices.push_back(p3Index);
    }
}

void vhacd::VHACDUtil::fattenMesh(const FBXMesh& mesh, const glm::mat4& geometryOffset, FBXMesh& result) const {
    // this is used to make meshes generated from a highfield collidable.  each triangle
    // is converted into a tetrahedron and made into its own mesh-part.

    std::vector<int> triangleIndices;
    foreach (const FBXMeshPart &meshPart, mesh.parts) {
        getTrianglesInMeshPart(meshPart, triangleIndices);
    }

    if (triangleIndices.size() == 0) {
        return;
    }

    int indexStartOffset = result.vertices.size();

    // new mesh gets the transformed points from the original
    glm::mat4 totalTransform = geometryOffset * mesh.modelTransform;
    for (int i = 0; i < mesh.vertices.size(); i++) {
        // apply the source mesh's transform to the points
        glm::vec4 v = totalTransform * glm::vec4(mesh.vertices[i], 1.0f);
        result.vertices += glm::vec3(v);
    }

    // turn each triangle into a tetrahedron

    const uint32_t TRIANGLE_STRIDE = 3;
    const float COLLISION_TETRAHEDRON_SCALE = 0.25f;
    for (uint32_t i = 0; i < triangleIndices.size(); i += TRIANGLE_STRIDE) {
        int index0 = triangleIndices[i] + indexStartOffset;
        int index1 = triangleIndices[i + 1] + indexStartOffset;
        int index2 = triangleIndices[i + 2] + indexStartOffset;

        // TODO: skip triangles with a normal that points more negative-y than positive-y

        glm::vec3 p0 = result.vertices[index0];
        glm::vec3 p1 = result.vertices[index1];
        glm::vec3 p2 = result.vertices[index2];
        glm::vec3 av = (p0 + p1 + p2) / 3.0f; // center of the triangular face

        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        float threshold = 1.0f / sqrtf(3.0f);
        if (normal.y > -threshold && normal.y < threshold) {
            // this triangle is more a wall than a floor, skip it.
            continue;
        }

        // from the middle of the triangle, pull a point down to form a tetrahedron.
        float dropAmount = 0;
        dropAmount = glm::max(glm::length(p1 - p0), dropAmount);
        dropAmount = glm::max(glm::length(p2 - p1), dropAmount);
        dropAmount = glm::max(glm::length(p0 - p2), dropAmount);
        dropAmount *= COLLISION_TETRAHEDRON_SCALE;

        glm::vec3 p3 = av - glm::vec3(0.0f, dropAmount, 0.0f);
        int index3 = result.vertices.size();
        result.vertices << p3; // add the new point to the result mesh

        FBXMeshPart newMeshPart;
        setMeshPartDefaults(newMeshPart, "unknown");
        newMeshPart.triangleIndices << index0 << index1 << index2;
        newMeshPart.triangleIndices << index0 << index3 << index1;
        newMeshPart.triangleIndices << index1 << index3 << index2;
        newMeshPart.triangleIndices << index2 << index3 << index0;
        result.parts.append(newMeshPart);
    }
}

AABox getAABoxForMeshPart(const FBXMesh& mesh, const FBXMeshPart &meshPart) {
    AABox aaBox;
    const int TRIANGLE_STRIDE = 3;
    for (int i = 0; i < meshPart.triangleIndices.size(); i += TRIANGLE_STRIDE) {
        aaBox += mesh.vertices[meshPart.triangleIndices[i]];
        aaBox += mesh.vertices[meshPart.triangleIndices[i + 1]];
        aaBox += mesh.vertices[meshPart.triangleIndices[i + 2]];
    }

    const int QUAD_STRIDE = 4;
    for (int i = 0; i < meshPart.quadIndices.size(); i += QUAD_STRIDE) {
        aaBox += mesh.vertices[meshPart.quadIndices[i]];
        aaBox += mesh.vertices[meshPart.quadIndices[i + 1]];
        aaBox += mesh.vertices[meshPart.quadIndices[i + 2]];
        aaBox += mesh.vertices[meshPart.quadIndices[i + 3]];
    }

    return aaBox;
}

class TriangleEdge {
public:
    TriangleEdge() {}
    TriangleEdge(uint32_t A, uint32_t B) {
        setIndices(A, B);
    }
    void setIndices(uint32_t A, uint32_t B) {
        if (A < B) {
            _indexA = A;
            _indexB = B;
        } else {
            _indexA = B;
            _indexB = A;
        }
    }
    bool operator==(const TriangleEdge& other) const {
        return _indexA == other._indexA && _indexB == other._indexB;
    }

    uint32_t getIndexA() const { return _indexA; }
    uint32_t getIndexB() const { return _indexB; }
private:
    uint32_t _indexA { (uint32_t)(-1) };
    uint32_t _indexB { (uint32_t)(-1) };
};

namespace std {
    template <>
    struct hash<TriangleEdge> {
        std::size_t operator()(const TriangleEdge& edge) const {
            // use Cantor's pairing function to generate a hash of ZxZ --> Z
            uint32_t ab = edge.getIndexA() + edge.getIndexB();
            return hash<int>()((ab * (ab + 1)) / 2 + edge.getIndexB());
        }
    };
}

// returns false if any edge has only one adjacent triangle
bool isClosedManifold(const std::vector<int>& triangleIndices) {
    using EdgeList = std::unordered_map<TriangleEdge, int>;
    EdgeList edges;

    // count the triangles for each edge
    const uint32_t TRIANGLE_STRIDE = 3;
    for (uint32_t i = 0; i < triangleIndices.size(); i += TRIANGLE_STRIDE) {
        TriangleEdge edge;
        // the triangles indices are stored in sequential order
        for (uint32_t j = 0; j < 3; ++j) {
            edge.setIndices(triangleIndices[i + j], triangleIndices[i + ((j + 1) % 3)]);

            EdgeList::iterator edgeEntry = edges.find(edge);
            if (edgeEntry == edges.end()) {
                edges.insert(std::pair<TriangleEdge, uint32_t>(edge, 1));
            } else {
                edgeEntry->second += 1;
            }
        }
    }
    // scan for outside edge
    for (auto& edgeEntry : edges) {
        if (edgeEntry.second == 1) {
             return false;
        }
    }
    return true;
}

void vhacd::VHACDUtil::getConvexResults(VHACD::IVHACD* convexifier, FBXMesh& resultMesh) const {
    // Number of hulls for this input meshPart
    uint32_t numHulls = convexifier->GetNConvexHulls();
    if (_verbose) {
        qDebug() << "  hulls =" << numHulls;
    }

    // create an output meshPart for each convex hull
    const uint32_t TRIANGLE_STRIDE = 3;
    const uint32_t POINT_STRIDE = 3;
    for (uint32_t j = 0; j < numHulls; j++) {
        VHACD::IVHACD::ConvexHull hull;
        convexifier->GetConvexHull(j, hull);

        resultMesh.parts.append(FBXMeshPart());
        FBXMeshPart& resultMeshPart = resultMesh.parts.last();

        int hullIndexStart = resultMesh.vertices.size();
        resultMesh.vertices.reserve(hullIndexStart + hull.m_nPoints);
        uint32_t numIndices = hull.m_nPoints * POINT_STRIDE;
        for (uint32_t i = 0; i < numIndices; i += POINT_STRIDE) {
            float x = hull.m_points[i];
            float y = hull.m_points[i + 1];
            float z = hull.m_points[i + 2];
            resultMesh.vertices.append(glm::vec3(x, y, z));
        }

        numIndices = hull.m_nTriangles * TRIANGLE_STRIDE;
        resultMeshPart.triangleIndices.reserve(resultMeshPart.triangleIndices.size() + numIndices);
        for (uint32_t i = 0; i < numIndices; i += TRIANGLE_STRIDE) {
            resultMeshPart.triangleIndices.append(hull.m_triangles[i] + hullIndexStart);
            resultMeshPart.triangleIndices.append(hull.m_triangles[i + 1] + hullIndexStart);
            resultMeshPart.triangleIndices.append(hull.m_triangles[i + 2] + hullIndexStart);
        }
        if (_verbose) {
            qDebug() << "    hull" << j << " vertices =" << hull.m_nPoints
                << " triangles =" << hull.m_nTriangles
                << " FBXMeshVertices =" << resultMesh.vertices.size();
        }
    }
}

float computeDt(uint64_t start) {
    return (float)(usecTimestampNow() - start) / (float)USECS_PER_SECOND;
}

bool vhacd::VHACDUtil::computeVHACD(FBXGeometry& geometry,
                                    VHACD::IVHACD::Parameters params,
                                    FBXGeometry& result,
                                    float minimumMeshSize, float maximumMeshSize) {
    if (_verbose) {
        qDebug() << "meshes =" << geometry.meshes.size();
    }

    // count the mesh-parts
    int numParts = 0;
    foreach (const FBXMesh& mesh, geometry.meshes) {
        numParts += mesh.parts.size();
    }
    if (_verbose) {
        qDebug() << "total parts =" << numParts;
    }

    VHACD::IVHACD * convexifier = VHACD::CreateVHACD();

    result.meshExtents.reset();
    result.meshes.append(FBXMesh());
    FBXMesh &resultMesh = result.meshes.last();

    const uint32_t POINT_STRIDE = 3;
    const uint32_t TRIANGLE_STRIDE = 3;

    int meshIndex = 0;
    int validPartsFound = 0;
    foreach (const FBXMesh& mesh, geometry.meshes) {

        // find duplicate points
        int numDupes = 0;
        std::vector<int> dupeIndexMap;
        dupeIndexMap.reserve(mesh.vertices.size());
        for (int i = 0; i < mesh.vertices.size(); ++i) {
            dupeIndexMap.push_back(i);
            for (int j = 0; j < i; ++j) {
                float distance = glm::distance2(mesh.vertices[i], mesh.vertices[j]);
                const float MAX_DUPE_DISTANCE_SQUARED = 0.000001f;
                if (distance < MAX_DUPE_DISTANCE_SQUARED) {
                    dupeIndexMap[i] = j;
                    ++numDupes;
                    break;
                }
            }
        }

        // each mesh has its own transform to move it to model-space
        std::vector<glm::vec3> vertices;
        glm::mat4 totalTransform = geometry.offset * mesh.modelTransform;
        foreach (glm::vec3 vertex, mesh.vertices) {
            vertices.push_back(glm::vec3(totalTransform * glm::vec4(vertex, 1.0f)));
        }
        uint32_t numVertices = (uint32_t)vertices.size();

        if (_verbose) {
            qDebug() << "mesh" << meshIndex << ": "
                << " parts =" << mesh.parts.size() << " clusters =" << mesh.clusters.size()
                << " vertices =" << numVertices;
        }
        ++meshIndex;

        std::vector<int> openParts;

        int partIndex = 0;
        std::vector<int> triangleIndices;
        foreach (const FBXMeshPart &meshPart, mesh.parts) {
            triangleIndices.clear();
            getTrianglesInMeshPart(meshPart, triangleIndices);

            // only process meshes with triangles
            if (triangleIndices.size() <= 0) {
                if (_verbose) {
                    qDebug() << "  skip part" << partIndex << "(zero triangles)";
                }
                ++partIndex;
                continue;
            }

            // collapse dupe indices
            for (auto& index : triangleIndices) {
                index = dupeIndexMap[index];
            }

            AABox aaBox = getAABoxForMeshPart(mesh, meshPart);
            const float largestDimension = aaBox.getLargestDimension();

            if (largestDimension < minimumMeshSize) {
                if (_verbose) {
                    qDebug() << "  skip part" << partIndex << ":  dimension =" << largestDimension << "(too small)";
                }
                ++partIndex;
                continue;
            }

            if (maximumMeshSize > 0.0f && largestDimension > maximumMeshSize) {
                if (_verbose) {
                    qDebug() << "  skip part" << partIndex << ":  dimension =" << largestDimension << "(too large)";
                }
                ++partIndex;
                continue;
            }

            // figure out if the mesh is a closed manifold or not
            bool closed = isClosedManifold(triangleIndices);
            if (closed) {
                uint32_t triangleCount = (uint32_t)(triangleIndices.size()) / TRIANGLE_STRIDE;
                if (_verbose) {
                    qDebug() << "  process closed part" << partIndex << ": " << " triangles =" << triangleCount;
                }

                // compute approximate convex decomposition
                bool success = convexifier->Compute(&vertices[0].x, POINT_STRIDE, numVertices,
                        &triangleIndices[0], TRIANGLE_STRIDE, triangleCount, params);
                if (success) {
                    getConvexResults(convexifier, resultMesh);
                } else if (_verbose) {
                    qDebug() << "  failed to convexify";
                }
            } else {
                if (_verbose) {
                    qDebug() << "  postpone open part" << partIndex;
                }
                openParts.push_back(partIndex);
            }
            ++partIndex;
            ++validPartsFound;
        }
        if (! openParts.empty()) {
            // combine open meshes in an attempt to produce a closed mesh

            triangleIndices.clear();
            for (auto index : openParts) {
                const FBXMeshPart &meshPart = mesh.parts[index];
                getTrianglesInMeshPart(meshPart, triangleIndices);
            }

            // collapse dupe indices
            for (auto& index : triangleIndices) {
                index = dupeIndexMap[index];
            }

            // this time we don't care if the parts are closed or not
            uint32_t triangleCount = (uint32_t)(triangleIndices.size()) / TRIANGLE_STRIDE;
            if (_verbose) {
                qDebug() << "  process remaining open parts =" << openParts.size() << ": "
                    << " triangles =" << triangleCount;
            }

            // compute approximate convex decomposition
            bool success = convexifier->Compute(&vertices[0].x, POINT_STRIDE, numVertices,
                    &triangleIndices[0], TRIANGLE_STRIDE, triangleCount, params);
            if (success) {
                getConvexResults(convexifier, resultMesh);
            } else if (_verbose) {
                qDebug() << "  failed to convexify";
            }
        }
    }

    //release memory
    convexifier->Clean();
    convexifier->Release();

    return validPartsFound > 0;
}

vhacd::VHACDUtil:: ~VHACDUtil(){
    //nothing to be cleaned
}

//ProgressClaback implementation
void vhacd::ProgressCallback::Update(const double overallProgress,
                                     const double stageProgress,
                                     const double operationProgress,
                                     const char* const stage,
                                     const char* const operation) {
    int progress = (int)(overallProgress + 0.5);

    std::cout << "\b\b\b";
    std::cout << progress << "%" << std::flush;
    if (progress >= 100) {
        std::cout << std::endl;
    }
}

vhacd::ProgressCallback::ProgressCallback(void){}
vhacd::ProgressCallback::~ProgressCallback(){}
