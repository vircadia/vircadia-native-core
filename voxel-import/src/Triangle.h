//
//  Triangle.h
//  hifi
//
//  Created by Stojce Slavkovski on 6/27/13.
//
//

#ifndef __hifi__Triangle__
#define __hifi__Triangle__

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "AABox.h"
namespace voxelImport {
    using namespace std;
    using namespace glm;
    
    class Triangle {
        
    public:
        Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c);
        vector<glm::vec3> getCollidingVoxels(float voxelSize);
        
    private:
        vec3 _a;
        vec3 _b;
        vec3 _c;
        AABox* _boundingBox;
        
        bool isCollidingWithVoxel(vec3 voxelCenter, vec3 delta);
        bool testVoxelEdgeXYProjection(vec3 triVertex, vec3 delta, vec3 voxelEvalPoint, vec3 faceNormal, vec3 faceEdge);
        bool testVoxelEdgeYZProjection(vec3 triVertex, vec3 delta, vec3 voxelEvalPoint, vec3 faceNormal, vec3 faceEdge);
        bool testVoxelEdgeZXProjection(vec3 triVertex, vec3 delta, vec3 voxelEvalPoint, vec3 faceNormal, vec3 faceEdge);
    };
}
#endif /* defined(__hifi__Triangle__) */
