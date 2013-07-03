//
//  VoxelHelper.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 6/30/13.
//
//

#include "VoxelHelper.h"

namespace voxelImport {
    
    using namespace std;
    using namespace glm;
    
    void VoxelHelper::createVoxels(VoxelTree* tree, vector<vec3> voxels, float voxelSize, rgbColor voxelColor) {
        if (voxels.size() > 2) { //surface
            for (size_t v = 0; v < voxels.size(); v++) {
                tree->createVoxel(voxels[v].x * voxelSize, voxels[v].y * voxelSize, voxels[v].z * voxelSize, voxelSize, voxelColor[0], voxelColor[1], voxelColor[2]);
            }
        } else if (voxels.size() == 2) {
            tree->createLine(voxels[0], voxels[1], voxelSize, voxelColor);
        }
    }
}