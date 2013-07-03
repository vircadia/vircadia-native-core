//
//  VoxelHelper.h
//  hifi
//
//  Created by Stojce Slavkovski on 6/30/13.
//
//

#ifndef __hifi__VoxelHelper__
#define __hifi__VoxelHelper__

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "VoxelTree.h"

namespace voxelImport {
    
    using namespace std;
    using namespace glm;

    class VoxelHelper {
    public:
        static void createVoxels(VoxelTree* tree, vector<vec3> voxels, float voxelSize, rgbColor voxelColor);
    };
}

#endif /* defined(__hifi__VoxelHelper__) */
