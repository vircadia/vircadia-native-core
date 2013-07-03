//
//  SketchUp.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 6/27/13.
//
//

#include "SketchUp.h"
#include "AABox.h"
#include "Voxelizer.h"
#include "VoxelHelper.h"

#include <slapi/slapi.h>
#include <slapi/unicodestring.h>
#include <slapi/geometry.h>
#include <slapi/initialize.h>
#include <slapi/color.h>
#include <slapi/model/material.h>
#include <slapi/model/model.h>
#include <slapi/model/entities.h>
#include <slapi/model/face.h>
#include <slapi/model/edge.h>
#include <slapi/model/vertex.h>
#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

namespace voxelImport {

    vector<SUFaceRef> getFacesFomModel(SUModelRef model) {
        // Get the entity container of the model.
        SUEntitiesRef entities = SU_INVALID;
        SUModelGetEntities(model, &entities);
        
        // Get all the faces from the entities object
        size_t faceCount = 0;
        SUEntitiesGetNumFaces(entities, &faceCount);
        vector<SUFaceRef> faces(faceCount);
        SUEntitiesGetFaces(entities, faceCount, &faces[0], &faceCount);
        
        return faces;
    }
    
    SUColor getMaterialColor(SUFaceRef face) {
        
        SUMaterialRef mat = SU_INVALID;
        SUFaceGetFrontMaterial(face, &mat);
        
        SUColor materialColor = SU_INVALID;
        SUMaterialGetColor(mat, &materialColor);
        
        return materialColor;
    }
    
    vector<vec3> getEdgeVerices(SUFaceRef face) {
        size_t edgeCount = 0;
        SUFaceGetNumEdges(face, &edgeCount);
        
        vector<SUEdgeRef> edges(edgeCount);
        SUFaceGetEdges(face, edgeCount, &edges[0], &edgeCount);
        
        
        vector<vec3> vertices(edgeCount);
        
        // Get the vertex positions for each edge
        for (size_t j = 0; j < edgeCount; j++) {
            
            SUVertexRef startVertex = SU_INVALID;
            SUVertexRef endVertex = SU_INVALID;
            
            SUEdgeGetStartVertex(edges[j], &startVertex);
            SUEdgeGetEndVertex(edges[j], &endVertex);
            SUPoint3D start;
            //                    SUPoint3D end;
            SUVertexGetPosition(startVertex, &start);
            //                    SUVertexGetPosition(endVertex, &end);
            
            vertices[j] = vec3(start.x, start.y, start.z);
        }
        return vertices;
    }
    
    bool SketchUp::importModel(const char* modelFileName, VoxelTree tree, float voxelSize, float modelVoxelSize) {
        
        // Always initialize the API before using it
        SUInitialize();
        // Load the model from a file
        SUModelRef model = SU_INVALID;
        SUResult res = SUModelCreateFromFile(&model, modelFileName);
        // It's best to always check the return code from each SU function call.
        // Only showing this check once to keep this example short.
        if (res != SU_ERROR_NONE) {
            printf("error loading model");
            return false;
        }
        
        vector<SUFaceRef> faces = getFacesFomModel(model);
        size_t faceCount = faces.size();
        unsigned long voxelVericesCount = 0;
        
        for (size_t i = 0; i < faceCount; i++) {
            
            SUFaceRef face = faces[i];
            SUColor materialColor = getMaterialColor(face);
            
            vector<vec3> vertices = getEdgeVerices(face);
            rgbColor voxelColor = {materialColor.red, materialColor.green, materialColor.blue};
            vector<vec3> voxelVerices;

            if (vertices.size() == 2) {
                voxelVerices = vertices;
            } else if (vertices.size() > 2) {
                Voxelizer *voxelizer = new Voxelizer();
                voxelVerices = voxelizer->Voxelize(vertices, modelVoxelSize);
            }
            voxelVericesCount += voxelVerices.size();
            VoxelHelper::createVoxels(&tree, voxelVerices, voxelSize, voxelColor);
        }
        
        printf("total voxelsVerices: %ld\n", voxelVericesCount);
        printf("total Voxels: %ld\n", tree.getVoxelCount());
        // Must release the model or there will be memory leaks
        SUModelRelease(&model);
        // Always terminate the API when done using it
        SUTerminate();
        return true;
    }
}
