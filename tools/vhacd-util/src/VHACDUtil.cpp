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

#include <QVector>
#include "VHACDUtil.h"


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
    std::cout << "Reading FBX.....\n";

    QByteArray fbxContents = fbx.readAll();

    if (filename.toLower().endsWith(".obj")) {
        result = OBJReader().readOBJ(fbxContents, QVariantHash());
    } else if (filename.toLower().endsWith(".fbx")) {
        result = readFBX(fbxContents, QVariantHash());
    } else {
        qDebug() << "unknown file extension";
        return false;
    }

    reSortFBXGeometryMeshes(result);

    return true;
}


unsigned int getTrianglesInMeshPart(const FBXMeshPart &meshPart, std::vector<int>& triangles) {
    // append all the triangles (and converted quads) from this mesh-part to triangles
    std::vector<int> meshPartTriangles = meshPart.triangleIndices.toStdVector();
    triangles.insert(triangles.end(), meshPartTriangles.begin(), meshPartTriangles.end());

    // convert quads to triangles
    unsigned int triangleCount = meshPart.triangleIndices.size() / 3;
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
        triangleCount += 2;
    }

    return triangleCount;
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

    unsigned int triangleCount = triangles.size() / 3;
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

        float dropAmount = 0;
        dropAmount = glm::max(glm::length(p1 - p0), dropAmount);
        dropAmount = glm::max(glm::length(p2 - p1), dropAmount);
        dropAmount = glm::max(glm::length(p0 - p2), dropAmount);

        glm::vec3 p3 = av - glm::vec3(0, dropAmount, 0);  // a point 1 meter below the average of this triangle's points
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
    for (unsigned int i = 0; i < triangleCount; i++) {
        glm::vec3 p0 = mesh.vertices[meshPart.triangleIndices[i * 3]];
        glm::vec3 p1 = mesh.vertices[meshPart.triangleIndices[i * 3 + 1]];
        glm::vec3 p2 = mesh.vertices[meshPart.triangleIndices[i * 3 + 2]];
        aaBox += p0;
        aaBox += p1;
        aaBox += p2;
    }

    unsigned int quadCount = meshPart.quadIndices.size() / 4;
    for (unsigned int i = 0; i < quadCount; i++) {
        unsigned int p0Index = meshPart.quadIndices[i * 4];
        unsigned int p1Index = meshPart.quadIndices[i * 4 + 1];
        unsigned int p2Index = meshPart.quadIndices[i * 4 + 2];
        unsigned int p3Index = meshPart.quadIndices[i * 4 + 3];
        glm::vec3 p0 = mesh.vertices[p0Index];
        glm::vec3 p1 = mesh.vertices[p1Index + 1];
        glm::vec3 p2 = mesh.vertices[p2Index + 2];
        glm::vec3 p3 = mesh.vertices[p3Index + 3];
        aaBox += p0;
        aaBox += p1;
        aaBox += p2;
        aaBox += p3;
    }

    return aaBox;
}



bool vhacd::VHACDUtil::computeVHACD(FBXGeometry& geometry,
                                    VHACD::IVHACD::Parameters params,
                                    FBXGeometry& result,
                                    int startMeshIndex,
                                    int endMeshIndex,
                                    float minimumMeshSize, float maximumMeshSize) {
    // count the mesh-parts
    int meshCount = 0;
    foreach (const FBXMesh& mesh, geometry.meshes) {
        meshCount += mesh.parts.size();
    }

    VHACD::IVHACD * interfaceVHACD = VHACD::CreateVHACD();

    if (startMeshIndex < 0) {
        startMeshIndex = 0;
    }
    if (endMeshIndex < 0) {
        endMeshIndex = meshCount;
    }

    std::cout << "Performing V-HACD computation on " << endMeshIndex - startMeshIndex << " meshes ..... " << std::endl;

    result.meshExtents.reset();
    result.meshes.append(FBXMesh());
    FBXMesh &resultMesh = result.meshes.last();

    int count = 0;
    foreach (const FBXMesh& mesh, geometry.meshes) {

        // each mesh has its own transform to move it to model-space
        std::vector<glm::vec3> vertices;
        foreach (glm::vec3 vertex, mesh.vertices) {
            vertices.push_back(glm::vec3(mesh.modelTransform * glm::vec4(vertex, 1.0f)));
        }

        foreach (const FBXMeshPart &meshPart, mesh.parts) {

            if (count < startMeshIndex || count >= endMeshIndex) {
                count ++;
                continue;
            }

            qDebug() << "--------------------";

            std::vector<int> triangles;
            unsigned int triangleCount = getTrianglesInMeshPart(meshPart, triangles);

            // only process meshes with triangles
            if (triangles.size() <= 0) {
                qDebug() << " Skipping (no triangles)...";
                count++;
                continue;
            }

            int nPoints = vertices.size();
            AABox aaBox = getAABoxForMeshPart(mesh, meshPart);
            const float largestDimension = aaBox.getLargestDimension();

            qDebug() << "Mesh " << count << " -- " << nPoints << " points, " << triangleCount << " triangles, "
                     << "size =" << largestDimension;

            if (largestDimension < minimumMeshSize) {
                qDebug() << " Skipping (too small)...";
                count++;
                continue;
            }

            if (maximumMeshSize > 0.0f && largestDimension > maximumMeshSize) {
                qDebug() << " Skipping (too large)...";
                count++;
                continue;
            }


            // compute approximate convex decomposition
            bool res = interfaceVHACD->Compute(&vertices[0].x, 3, nPoints, &triangles[0], 3, triangleCount, params);
            if (!res){
                qDebug() << "V-HACD computation failed for Mesh : " << count;
                count++;
                continue;
            }

            // Number of hulls for this input meshPart
            unsigned int nConvexHulls = interfaceVHACD->GetNConvexHulls();

            // create an output meshPart for each convex hull
            for (unsigned int j = 0; j < nConvexHulls; j++) {
                VHACD::IVHACD::ConvexHull hull;
                interfaceVHACD->GetConvexHull(j, hull);

                resultMesh.parts.append(FBXMeshPart());
                FBXMeshPart &resultMeshPart = resultMesh.parts.last();

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
            }

            count++;
        }
    }

    //release memory
    interfaceVHACD->Clean();
    interfaceVHACD->Release();

    if (count > 0){
        return true;
    }        
    else{
        return false;
    }
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

    if (progress < 10){
        std::cout << "\b\b";
    }
    else{
        std::cout << "\b\b\b";
    }

    std::cout << progress << "%";

    if (progress >= 100){
        std::cout << std::endl;
    }
}

vhacd::ProgressCallback::ProgressCallback(void){}
vhacd::ProgressCallback::~ProgressCallback(){}
