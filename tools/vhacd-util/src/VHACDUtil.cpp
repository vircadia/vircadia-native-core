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

#include <unordered_map>
#include <QVector>
#include "VHACDUtil.h"

const float COLLISION_TETRAHEDRON_SCALE = 0.25f;


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

    // open the fbx file
    QFile fbx(filename);
    if (!fbx.open(QIODevice::ReadOnly)) {
        return false;
    }
    qDebug() << "reading FBX file =" << filename << "...";
    try {
        QByteArray fbxContents = fbx.readAll();
        FBXGeometry* geom;
        if (filename.toLower().endsWith(".obj")) {
            geom = OBJReader().readOBJ(fbxContents, QVariantHash());
        } else if (filename.toLower().endsWith(".fbx")) {
            geom = readFBX(fbxContents, QVariantHash(), filename);
        } else {
            qDebug() << "unknown file extension";
            return false;
        }
        result = *geom;

        reSortFBXGeometryMeshes(result);
    } catch (const QString& error) {
        qDebug() << "Error reading" << filename << ":" << error;
        return false;
    }

    return true;
}


void getTrianglesInMeshPart(const FBXMeshPart &meshPart, std::vector<int>& triangles) {
    // append all the triangles (and converted quads) from this mesh-part to triangles
    std::vector<int> meshPartTriangles = meshPart.triangleIndices.toStdVector();
    triangles.insert(triangles.end(), meshPartTriangles.begin(), meshPartTriangles.end());

    // convert quads to triangles
    unsigned int quadCount = meshPart.quadIndices.size() / 4;
    for (unsigned int i = 0; i < quadCount; i++) {
        unsigned int p0Index = meshPart.quadIndices[i * 4];
        unsigned int p1Index = meshPart.quadIndices[i * 4 + 1];
        unsigned int p2Index = meshPart.quadIndices[i * 4 + 2];
        unsigned int p3Index = meshPart.quadIndices[i * 4 + 3];
        // split each quad into two triangles
        triangles.push_back(p0Index);
        triangles.push_back(p1Index);
        triangles.push_back(p2Index);
        triangles.push_back(p0Index);
        triangles.push_back(p2Index);
        triangles.push_back(p3Index);
    }
}


void vhacd::VHACDUtil::fattenMeshes(const FBXMesh& mesh, FBXMesh& result,
                                    unsigned int& meshPartCount,
                                    unsigned int startMeshIndex, unsigned int endMeshIndex) const {
    // this is used to make meshes generated from a highfield collidable.  each triangle
    // is converted into a tetrahedron and made into its own mesh-part.

    std::vector<int> triangles;
    foreach (const FBXMeshPart &meshPart, mesh.parts) {
        if (meshPartCount < startMeshIndex || meshPartCount >= endMeshIndex) {
            meshPartCount++;
            continue;
        }
        getTrianglesInMeshPart(meshPart, triangles);
    }

    auto triangleCount = triangles.size() / 3;
    if (triangleCount == 0) {
        return;
    }

    int indexStartOffset = result.vertices.size();

    // new mesh gets the transformed points from the original
    for (int i = 0; i < mesh.vertices.size(); i++) {
        // apply the source mesh's transform to the points
        glm::vec4 v = mesh.modelTransform * glm::vec4(mesh.vertices[i], 1.0f);
        result.vertices += glm::vec3(v);
    }

    // turn each triangle into a tetrahedron

    for (unsigned int i = 0; i < triangleCount; i++) {
        int index0 = triangles[i * 3] + indexStartOffset;
        int index1 = triangles[i * 3 + 1] + indexStartOffset;
        int index2 = triangles[i * 3 + 2] + indexStartOffset;

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
    unsigned int triangleCount = meshPart.triangleIndices.size() / 3;
    for (unsigned int i = 0; i < triangleCount; ++i) {
        aaBox += mesh.vertices[meshPart.triangleIndices[i * 3]];
        aaBox += mesh.vertices[meshPart.triangleIndices[i * 3 + 1]];
        aaBox += mesh.vertices[meshPart.triangleIndices[i * 3 + 2]];
    }

    unsigned int quadCount = meshPart.quadIndices.size() / 4;
    for (unsigned int i = 0; i < quadCount; ++i) {
        aaBox += mesh.vertices[meshPart.quadIndices[i * 4]];
        aaBox += mesh.vertices[meshPart.quadIndices[i * 4 + 1]];
        aaBox += mesh.vertices[meshPart.quadIndices[i * 4 + 2]];
        aaBox += mesh.vertices[meshPart.quadIndices[i * 4 + 3]];
    }

    return aaBox;
}

struct TriangleEdge {
    int indexA { -1 };
    int indexB { -1 };
    TriangleEdge() {}
    TriangleEdge(int A, int B) : indexA(A), indexB(B) {}
    bool operator==(const TriangleEdge& other) const {
        return indexA == other.indexA && indexB == other.indexB;
    }
    void sortIndices() {
        if (indexB < indexA) {
            int t = indexA;
            indexA = indexB;
            indexB = t;
        }
    }
};

namespace std {
    template <>
    struct hash<TriangleEdge> {
        std::size_t operator()(const TriangleEdge& edge) const {
            return (hash<int>()(edge.indexA) ^ (hash<int>()(edge.indexB) << 1));
        }
    };
}

// returns false if any edge has only one adjacent triangle
bool isClosedManifold(const std::vector<int>& triangles) {
    using EdgeList = std::unordered_map<TriangleEdge, int>;
    EdgeList edges;

    // count the triangles for each edge
    for (size_t i = 0; i < triangles.size(); i += 3) {
        TriangleEdge edge;
        for (int j = 0; j < 3; ++j) {
            edge.indexA = triangles[(int)i + j];
            edge.indexB = triangles[i + ((j + 1) % 3)];
            edge.sortIndices();

            EdgeList::iterator edgeEntry = edges.find(edge);
            if (edgeEntry == edges.end()) {
                edges.insert(std::pair<TriangleEdge, int>(edge, 1));
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

void getConvexResults(VHACD::IVHACD* convexifier, FBXMesh& resultMesh) {
    // Number of hulls for this input meshPart
    unsigned int numHulls = convexifier->GetNConvexHulls();
    qDebug() << "  hulls =" << numHulls;

    // create an output meshPart for each convex hull
    for (unsigned int j = 0; j < numHulls; j++) {
        VHACD::IVHACD::ConvexHull hull;
        convexifier->GetConvexHull(j, hull);

        resultMesh.parts.append(FBXMeshPart());
        FBXMeshPart& resultMeshPart = resultMesh.parts.last();

        int hullIndexStart = resultMesh.vertices.size();
        for (unsigned int i = 0; i < hull.m_nPoints; i++) {
            float x = hull.m_points[i * 3];
            float y = hull.m_points[i * 3 + 1];
            float z = hull.m_points[i * 3 + 2];
            resultMesh.vertices.append(glm::vec3(x, y, z));
        }

        for (unsigned int i = 0; i < hull.m_nTriangles; i++) {
            int index0 = hull.m_triangles[i * 3] + hullIndexStart;
            int index1 = hull.m_triangles[i * 3 + 1] + hullIndexStart;
            int index2 = hull.m_triangles[i * 3 + 2] + hullIndexStart;
            resultMeshPart.triangleIndices.append(index0);
            resultMeshPart.triangleIndices.append(index1);
            resultMeshPart.triangleIndices.append(index2);
        }
        qDebug() << "    hull" << j << " vertices =" << hull.m_nPoints
            << " triangles =" << hull.m_nTriangles
            << " FBXMeshVertices =" << resultMesh.vertices.size();
    }
}

float computeDt(uint64_t start) {
    return (float)(usecTimestampNow() - start) / 1.0e6f;
}

bool vhacd::VHACDUtil::computeVHACD(FBXGeometry& geometry,
                                    VHACD::IVHACD::Parameters params,
                                    FBXGeometry& result,
                                    float minimumMeshSize, float maximumMeshSize) {
    qDebug() << "meshes =" << geometry.meshes.size();

    // count the mesh-parts
    int numParts = 0;
    foreach (const FBXMesh& mesh, geometry.meshes) {
        numParts += mesh.parts.size();
    }
    qDebug() << "total parts =" << numParts;

    VHACD::IVHACD * convexifier = VHACD::CreateVHACD();

    result.meshExtents.reset();
    result.meshes.append(FBXMesh());
    FBXMesh &resultMesh = result.meshes.last();

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
                float distance = glm::distance(mesh.vertices[i], mesh.vertices[j]);
                if (distance < 0.0001f) {
                    dupeIndexMap[i] = j;
                    ++numDupes;
                    break;
                }
            }
        }

        // each mesh has its own transform to move it to model-space
        std::vector<glm::vec3> vertices;
        foreach (glm::vec3 vertex, mesh.vertices) {
            vertices.push_back(glm::vec3(mesh.modelTransform * glm::vec4(vertex, 1.0f)));
        }
        auto numVertices = vertices.size();

        qDebug() << "mesh" << meshIndex << ": "
            << " parts =" << mesh.parts.size() << " clusters =" << mesh.clusters.size()
            << " vertices =" << numVertices;
        ++meshIndex;

        std::vector<int> openParts;

        int partIndex = 0;
        foreach (const FBXMeshPart &meshPart, mesh.parts) {
            std::vector<int> triangles;
            getTrianglesInMeshPart(meshPart, triangles);

            // only process meshes with triangles
            if (triangles.size() <= 0) {
                qDebug() << "  skip part" << partIndex << "(zero triangles)";
                ++partIndex;
                continue;
            }

            // collapse dupe indices
            for (auto& i : triangles) {
                i = dupeIndexMap[i];
            }

            AABox aaBox = getAABoxForMeshPart(mesh, meshPart);
            const float largestDimension = aaBox.getLargestDimension();

            if (largestDimension < minimumMeshSize) {
                qDebug() << "  skip part" << partIndex << ":  dimension =" << largestDimension << "(too small)";
                ++partIndex;
                continue;
            }

            if (maximumMeshSize > 0.0f && largestDimension > maximumMeshSize) {
                qDebug() << "  skip part" << partIndex << ":  dimension =" << largestDimension << "(too large)";
                ++partIndex;
                continue;
            }

            // figure out if the mesh is a closed manifold or not
            bool closed = isClosedManifold(triangles);
            if (closed) {
                unsigned int triangleCount = triangles.size() / 3;
                qDebug() << "  process closed part" << partIndex << ": "
                    << " triangles =" << triangleCount;

                // compute approximate convex decomposition
                bool success = convexifier->Compute(&vertices[0].x, 3, (uint)numVertices, &triangles[0], 3, triangleCount, params);
                if (success) {
                    getConvexResults(convexifier, resultMesh);
                } else {
                    qDebug() << "  failed to convexify";
                }
            } else {
                qDebug() << "  postpone open part" << partIndex;
                openParts.push_back(partIndex);
            }
            ++partIndex;
            ++validPartsFound;
        }
        if (! openParts.empty()) {
            // combine open meshes in an attempt to produce a closed mesh

            std::vector<int> triangles;
            for (auto index : openParts) {
                const FBXMeshPart &meshPart = mesh.parts[index];
                getTrianglesInMeshPart(meshPart, triangles);
            }

            // collapse dupe indices
            for (auto& i : triangles) {
                i = dupeIndexMap[i];
            }

            // this time we don't care if the parts are close or not
            unsigned int triangleCount = triangles.size() / 3;
            qDebug() << "  process remaining open parts =" << openParts.size() << ": "
                << " triangles =" << triangleCount;

            // compute approximate convex decomposition
            bool success = convexifier->Compute(&vertices[0].x, 3, (uint)numVertices, &triangles[0], 3, triangleCount, params);
            if (success) {
                getConvexResults(convexifier, resultMesh);
            } else {
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
    std::cout << progress << "%";
    if (progress >= 100) {
        std::cout << std::endl;
    }
}

vhacd::ProgressCallback::ProgressCallback(void){}
vhacd::ProgressCallback::~ProgressCallback(){}
