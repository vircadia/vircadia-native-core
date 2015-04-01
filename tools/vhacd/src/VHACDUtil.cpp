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
bool vhacd::VHACDUtil::loadFBX(const QString filename, vhacd::LoadFBXResults *results) {

    // open the fbx file
    QFile fbx(filename);
    if (!fbx.open(QIODevice::ReadOnly)) {
        return false;
    }
    std::cout << "Reading FBX.....\n";

    QByteArray fbxContents = fbx.readAll();

    
    FBXGeometry geometry;

    if (filename.toLower().endsWith(".obj")) {
        geometry = readOBJ(fbxContents, QVariantHash());
    } else if (filename.toLower().endsWith(".fbx")) {
        geometry = readFBX(fbxContents, QVariantHash());
    } else {
        qDebug() << "unknown file extension";
        return false;
    }


    //results->meshCount = geometry.meshes.count();
    // qDebug() << "read in" << geometry.meshes.count() << "meshes";

    int count = 0;
    foreach(FBXMesh mesh, geometry.meshes) {
        //get vertices for each mesh		
        // QVector<glm::vec3> vertices = mesh.vertices;


        QVector<glm::vec3> vertices;
        foreach (glm::vec3 vertex, mesh.vertices) {
            vertices.append(glm::vec3(mesh.modelTransform * glm::vec4(vertex, 1.0f)));
        }

        //get the triangle indices for each mesh
        QVector<int> triangles;
        foreach(FBXMeshPart meshPart, mesh.parts){
            QVector<int> indices = meshPart.triangleIndices;
            triangles += indices;

            unsigned int quadCount = meshPart.quadIndices.size() / 4;
            for (unsigned int i = 0; i < quadCount; i++) {
                unsigned int p0Index = meshPart.quadIndices[i*4];
                unsigned int p1Index = meshPart.quadIndices[i*4+1];
                unsigned int p2Index = meshPart.quadIndices[i*4+2];
                unsigned int p3Index = meshPart.quadIndices[i*4+3];
                // split each quad into two triangles
                triangles.append(p0Index);
                triangles.append(p1Index);
                triangles.append(p2Index);
                triangles.append(p0Index);
                triangles.append(p2Index);
                triangles.append(p3Index);
            }
        }

        //only read meshes with triangles
        if (triangles.count() <= 0){
            continue;
        }           

        AABox aaBox;
        foreach (glm::vec3 p, vertices) {
            aaBox += p;
        }

        results->perMeshVertices.append(vertices);
        results->perMeshTriangleIndices.append(triangles);
        results->perMeshLargestDimension.append(aaBox.getLargestDimension());
        count++;
    }

    results->meshCount = count;
    return true;
}


void vhacd::VHACDUtil::combineMeshes(vhacd::LoadFBXResults *meshes, vhacd::LoadFBXResults *results) const {
    float largestDimension = 0;
    int indexStart = 0;

    QVector<glm::vec3> emptyVertices;
    QVector<int> emptyTriangles;
    results->perMeshVertices.append(emptyVertices);
    results->perMeshTriangleIndices.append(emptyTriangles);
    results->perMeshLargestDimension.append(largestDimension);

    for (int i = 0; i < meshes->meshCount; i++) {
        QVector<glm::vec3> vertices = meshes->perMeshVertices.at(i);
        QVector<int> triangles = meshes->perMeshTriangleIndices.at(i);
        const float largestDimension = meshes->perMeshLargestDimension.at(i);

        for (int j = 0; j < triangles.size(); j++) {
            triangles[ j ] += indexStart;
        }
        indexStart += vertices.size();

        results->perMeshVertices[0] << vertices;
        results->perMeshTriangleIndices[0] << triangles;
        if (results->perMeshLargestDimension[0] < largestDimension) {
            results->perMeshLargestDimension[0] = largestDimension;
        }
    }

    results->meshCount = 1;
}


void vhacd::VHACDUtil::fattenMeshes(vhacd::LoadFBXResults *meshes, vhacd::LoadFBXResults *results) const {

    for (int i = 0; i < meshes->meshCount; i++) {
        QVector<glm::vec3> vertices = meshes->perMeshVertices.at(i);
        QVector<int> triangles = meshes->perMeshTriangleIndices.at(i);
        const float largestDimension = meshes->perMeshLargestDimension.at(i);

        results->perMeshVertices.append(vertices);
        results->perMeshTriangleIndices.append(triangles);
        results->perMeshLargestDimension.append(largestDimension);

        for (int j = 0; j < triangles.size(); j += 3) {
            auto p0 = vertices[triangles[j]];
            auto p1 = vertices[triangles[j+1]];
            auto p2 = vertices[triangles[j+2]];

            auto d0 = p1 - p0;
            auto d1 = p2 - p0;

            auto cp = glm::cross(d0, d1);
            cp = 5.0f * glm::normalize(cp);

            auto p3 = p0 + cp;
            auto p4 = p1 + cp;
            auto p5 = p2 + cp;
            
            auto n = results->perMeshVertices.size();
            results->perMeshVertices[i] << p3 << p4 << p5;
            results->perMeshTriangleIndices[i] << n << n+1 << n+2;
        }

        results->meshCount++;
    }
}



bool vhacd::VHACDUtil::computeVHACD(vhacd::LoadFBXResults *inMeshes, VHACD::IVHACD::Parameters params,
                                    vhacd::ComputeResults *results,
                                    int startMeshIndex, int endMeshIndex, float minimumMeshSize,
                                    bool fattenFaces) const {

    vhacd::LoadFBXResults *meshes = new vhacd::LoadFBXResults;
    // combineMeshes(inMeshes, meshes);

    // vhacd::LoadFBXResults *meshes = new vhacd::LoadFBXResults;

    if (fattenFaces) {
        fattenMeshes(inMeshes, meshes);
    } else {
        meshes = inMeshes;
    }


    VHACD::IVHACD * interfaceVHACD = VHACD::CreateVHACD();
    int meshCount = meshes->meshCount;
    int count = 0;

    if (startMeshIndex < 0) {
        startMeshIndex = 0;
    }
    if (endMeshIndex < 0) {
        endMeshIndex = meshCount;
    }

    std::cout << "Performing V-HACD computation on " << endMeshIndex - startMeshIndex << " meshes ..... " << std::endl;

    for (int i = startMeshIndex; i < endMeshIndex; i++){
        qDebug() << "--------------------";
        std::vector<glm::vec3> vertices = meshes->perMeshVertices.at(i).toStdVector();
        std::vector<int> triangles = meshes->perMeshTriangleIndices.at(i).toStdVector();
        int nPoints = (unsigned int)vertices.size();
        int nTriangles = (unsigned int)triangles.size() / 3;
        const float largestDimension = meshes->perMeshLargestDimension.at(i);

        qDebug() << "Mesh " << i << " -- " << nPoints << " points, " << nTriangles << " triangles, "
                 << "size =" << largestDimension;

        if (largestDimension < minimumMeshSize /* || largestDimension > 1000 */) {
            qDebug() << " Skipping...";
            continue;
        }
        // compute approximate convex decomposition
        bool res = interfaceVHACD->Compute(&vertices[0].x, 3, nPoints, &triangles[0], 3, nTriangles, params);
        if (!res){
            qDebug() << "V-HACD computation failed for Mesh : " << i;
            continue;
        }
        count++; //For counting number of successfull computations

        //Number of hulls for the mesh
        unsigned int nConvexHulls = interfaceVHACD->GetNConvexHulls();
        results->convexHullsCountList.append(nConvexHulls);

        //get all the convex hulls for this mesh
        QVector<VHACD::IVHACD::ConvexHull> convexHulls;
        for (unsigned int j = 0; j < nConvexHulls; j++){
            VHACD::IVHACD::ConvexHull hull;
            interfaceVHACD->GetConvexHull(j, hull);

            double *m_points_copy = new double[hull.m_nPoints * 3];
            // std::copy(std::begin(hull.m_points), std::end(hull.m_points), std::begin(m_points_copy));
            for (unsigned int i=0; i<hull.m_nPoints * 3; i++) {
                m_points_copy[ i ] = hull.m_points[ i ];
            }
            hull.m_points = m_points_copy;

            int *m_triangles_copy = new int[hull.m_nTriangles * 3];
            // std::copy(std::begin(hull.m_triangles), std::end(hull.m_triangles), std::begin(m_triangles_copy));
            for (unsigned int i=0; i<hull.m_nTriangles * 3; i++) {
                m_triangles_copy[ i ] = hull.m_triangles[ i ];
            }
            hull.m_triangles = m_triangles_copy;
            convexHulls.append(hull);
        }
        results->convexHullList.append(convexHulls);
    } //end of for loop

    results->meshCount = count;

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
