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
    FBXGeometry geometry = readFBX(fbxContents, QVariantHash());
    //results->meshCount = geometry.meshes.count();

    int count = 0;
    foreach(FBXMesh mesh, geometry.meshes){
        //get vertices for each mesh		
        QVector<glm::vec3> vertices = mesh.vertices;

        //get the triangle indices for each mesh
        QVector<int> triangles;
        foreach(FBXMeshPart part, mesh.parts){
            QVector<int> indices = part.triangleIndices;
            triangles += indices;
        }

        //only read meshes with triangles
		if (triangles.count() <= 0){
			continue;
		}           
        results->perMeshVertices.append(vertices);
        results->perMeshTriangleIndices.append(triangles);
        count++;
    }

    results->meshCount = count;
    return true;
}

bool vhacd::VHACDUtil::computeVHACD(vhacd::LoadFBXResults *meshes, VHACD::IVHACD::Parameters params, vhacd::ComputeResults *results)const{
    VHACD::IVHACD * interfaceVHACD = VHACD::CreateVHACD();
    int meshCount = meshes->meshCount;
    int count = 0;
    std::cout << "Performing V-HACD computation on " << meshCount << " meshes ..... " << std::endl;

    for (int i = 0; i < meshCount; i++){

        std::vector<glm::vec3> vertices = meshes->perMeshVertices.at(i).toStdVector();
        std::vector<int> triangles = meshes->perMeshTriangleIndices.at(i).toStdVector();
        int nPoints = (unsigned int)vertices.size();
        int nTriangles = (unsigned int)triangles.size() / 3;
        std::cout << "Mesh " << i + 1 << " :    ";
        // compute approximate convex decomposition
        bool res = interfaceVHACD->Compute(&vertices[0].x, 3, nPoints, &triangles[0], 3, nTriangles, params);
        if (!res){
            std::cout << "V-HACD computation failed for Mesh : " << i + 1 << std::endl;
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
