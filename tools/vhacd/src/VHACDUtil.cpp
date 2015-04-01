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


//Read all the meshes from provided FBX file
bool vhacd::VHACDUtil::loadFBX(const QString filename, FBXGeometry& result) {

    // open the fbx file
    QFile fbx(filename);
    if (!fbx.open(QIODevice::ReadOnly)) {
        return false;
    }
    std::cout << "Reading FBX.....\n";

    QByteArray fbxContents = fbx.readAll();

    if (filename.toLower().endsWith(".obj")) {
        result = readOBJ(fbxContents, QVariantHash());
    } else if (filename.toLower().endsWith(".fbx")) {
        result = readFBX(fbxContents, QVariantHash());
    } else {
        qDebug() << "unknown file extension";
        return false;
    }

    return true;
}


// void vhacd::VHACDUtil::combineMeshes(FBXGeometryvhacd::LoadFBXResults *meshes, vhacd::LoadFBXResults *results) const {
//     float largestDimension = 0;
//     int indexStart = 0;

//     QVector<glm::vec3> emptyVertices;
//     QVector<int> emptyTriangles;
//     results->perMeshVertices.append(emptyVertices);
//     results->perMeshTriangleIndices.append(emptyTriangles);
//     results->perMeshLargestDimension.append(largestDimension);

//     for (int i = 0; i < meshes->meshCount; i++) {
//         QVector<glm::vec3> vertices = meshes->perMeshVertices.at(i);
//         QVector<int> triangles = meshes->perMeshTriangleIndices.at(i);
//         const float largestDimension = meshes->perMeshLargestDimension.at(i);

//         for (int j = 0; j < triangles.size(); j++) {
//             triangles[ j ] += indexStart;
//         }
//         indexStart += vertices.size();

//         results->perMeshVertices[0] << vertices;
//         results->perMeshTriangleIndices[0] << triangles;
//         if (results->perMeshLargestDimension[0] < largestDimension) {
//             results->perMeshLargestDimension[0] = largestDimension;
//         }
//     }

//     results->meshCount = 1;
// }


// void vhacd::VHACDUtil::fattenMeshes(vhacd::LoadFBXResults *meshes, vhacd::LoadFBXResults *results) const {

//     for (int i = 0; i < meshes->meshCount; i++) {
//         QVector<glm::vec3> vertices = meshes->perMeshVertices.at(i);
//         QVector<int> triangles = meshes->perMeshTriangleIndices.at(i);
//         const float largestDimension = meshes->perMeshLargestDimension.at(i);

//         results->perMeshVertices.append(vertices);
//         results->perMeshTriangleIndices.append(triangles);
//         results->perMeshLargestDimension.append(largestDimension);

//         for (int j = 0; j < triangles.size(); j += 3) {
//             auto p0 = vertices[triangles[j]];
//             auto p1 = vertices[triangles[j+1]];
//             auto p2 = vertices[triangles[j+2]];

//             auto d0 = p1 - p0;
//             auto d1 = p2 - p0;

//             auto cp = glm::cross(d0, d1);
//             cp = -2.0f * glm::normalize(cp);

//             auto p3 = p0 + cp;
            
//             auto n = results->perMeshVertices.size();
//             results->perMeshVertices[i] << p3;

//             results->perMeshTriangleIndices[i] << triangles[j] << n << triangles[j + 1];
//             results->perMeshTriangleIndices[i] << triangles[j + 1] << n << triangles[j + 2];
//             results->perMeshTriangleIndices[i] << triangles[j + 2] << n << triangles[j];
//         }

//         results->meshCount++;
//     }
// }



bool vhacd::VHACDUtil::computeVHACD(FBXGeometry& geometry,
                                    VHACD::IVHACD::Parameters params,
                                    FBXGeometry& result,
                                    int startMeshIndex,
                                    int endMeshIndex, float minimumMeshSize,
                                    bool fattenFaces) {

    // vhacd::LoadFBXResults *meshes = new vhacd::LoadFBXResults;
    // combineMeshes(inMeshes, meshes);

    // vhacd::LoadFBXResults *meshes = new vhacd::LoadFBXResults;

    // if (fattenFaces) {
    //     fattenMeshes(inMeshes, meshes);
    // } else {
    //     meshes = inMeshes;
    // }


    // count the mesh-parts
    QVector<FBXMeshPart> meshParts;
    int meshCount = 0;
    foreach (FBXMesh mesh, geometry.meshes) {
        meshCount += mesh.parts.size();
    }


    VHACD::IVHACD * interfaceVHACD = VHACD::CreateVHACD();

    if (startMeshIndex < 0) {
        startMeshIndex = 0;
    }
    if (endMeshIndex < 0) {
        endMeshIndex = meshCount;
    }

    // for (int i = 0; i < meshCount; i++) {
    //     std::cout << meshes->perMeshTriangleIndices.at(i).size() << " ";
    // }
    // std::cout << "\n";


    std::cout << "Performing V-HACD computation on " << endMeshIndex - startMeshIndex << " meshes ..... " << std::endl;

    result.meshExtents.reset();
    result.meshes.append(FBXMesh());
    FBXMesh &resultMesh = result.meshes.last();

    int count = 0;
    foreach (const FBXMesh& mesh, geometry.meshes) {
        foreach (const FBXMeshPart &meshPart, mesh.parts) {
            qDebug() << "--------------------";
            std::vector<glm::vec3> vertices = mesh.vertices.toStdVector();
            std::vector<int> triangles = meshPart.triangleIndices.toStdVector();

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

            // convert quads to triangles
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
                // split each quad into two triangles
                triangles.push_back(p0Index);
                triangles.push_back(p1Index);
                triangles.push_back(p2Index);
                triangles.push_back(p0Index);
                triangles.push_back(p2Index);
                triangles.push_back(p3Index);
                triangleCount += 2;
            }

            // only process meshes with triangles
            if (triangles.size() <= 0) {
                continue;
            }

            int nPoints = vertices.size();
            const float largestDimension = aaBox.getLargestDimension();

            qDebug() << "Mesh " << count << " -- " << nPoints << " points, " << triangleCount << " triangles, "
                     << "size =" << largestDimension;

            if (largestDimension < minimumMeshSize /* || largestDimension > 1000 */) {
                qDebug() << " Skipping...";
                continue;
            }

            // compute approximate convex decomposition
            bool res = interfaceVHACD->Compute(&vertices[0].x, 3, nPoints, &triangles[0], 3, triangleCount, params);
            if (!res){
                qDebug() << "V-HACD computation failed for Mesh : " << count;
                continue;
            }

            // Number of hulls for this input meshPart
            unsigned int nConvexHulls = interfaceVHACD->GetNConvexHulls();

            // create an output meshPart for each convex hull
            for (unsigned int j = 0; j < nConvexHulls; j++) {
                VHACD::IVHACD::ConvexHull hull;
                interfaceVHACD->GetConvexHull(j, hull);
                // each convex-hull is a mesh-part
                resultMesh.parts.append(FBXMeshPart());
                FBXMeshPart &resultMeshPart = resultMesh.parts.last();

                int hullIndexStart = resultMesh.vertices.size();

                qDebug() << "j =" << j << "points =" << hull.m_nPoints << "tris ="  << hull.m_nTriangles;

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
void vhacd::ProgressCallback::Update(const double  overallProgress, const double  stageProgress, const double operationProgress,
    const char * const stage, const char * const operation){
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
