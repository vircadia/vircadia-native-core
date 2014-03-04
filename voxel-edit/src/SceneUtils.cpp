//
//  SceneUtils.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 5/7/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <algorithm>

#include <glm/gtc/noise.hpp>

#include "SceneUtils.h"

void addCornersAndAxisLines(VoxelTree* tree) {
    // We want our corner voxels to be about 1/2 meter high, and our TREE_SCALE is in meters, so...    
    float voxelSize = 0.5f / TREE_SCALE;
    
    // Now some more examples... a little more complex
    printf("creating corner points...\n");
    tree->createVoxel(0              , 0              , 0              , voxelSize, 255, 255 ,255);
    tree->createVoxel(1.0 - voxelSize, 0              , 0              , voxelSize, 255, 0   ,0  );
    tree->createVoxel(0              , 1.0 - voxelSize, 0              , voxelSize, 0  , 255 ,0  );
    tree->createVoxel(0              , 0              , 1.0 - voxelSize, voxelSize, 0  , 0   ,255);
    tree->createVoxel(1.0 - voxelSize, 0              , 1.0 - voxelSize, voxelSize, 255, 0   ,255);
    tree->createVoxel(0              , 1.0 - voxelSize, 1.0 - voxelSize, voxelSize, 0  , 255 ,255);
    tree->createVoxel(1.0 - voxelSize, 1.0 - voxelSize, 0              , voxelSize, 255, 255 ,0  );
    tree->createVoxel(1.0 - voxelSize, 1.0 - voxelSize, 1.0 - voxelSize, voxelSize, 255, 255 ,255);
    printf("DONE creating corner points...\n");
}

void addSurfaceScene(VoxelTree * tree) {
    printf("adding surface scene...\n");
    float voxelSize = 1.f / (8 * TREE_SCALE);
   
    // color 1= blue, color 2=green
    unsigned char r1, g1, b1, r2, g2, b2, red, green, blue;
    r1 = r2 = b2 = g1 = 0;
    b1 = g2 = 255;
    
    for (float x = 0.0; x < 1.0; x += voxelSize) {
        for (float z = 0.0; z < 1.0; z += voxelSize) {

            glm::vec2 position = glm::vec2(x, z);
            float perlin = glm::perlin(position) + .25f * glm::perlin(position * 4.f) + .125f * glm::perlin(position * 16.f);
            float gradient = (1.0f + perlin)/ 2.0f;
            red   = (unsigned char)std::min(255, std::max(0, (int)(r1 + ((r2 - r1) * gradient))));
            green = (unsigned char)std::min(255, std::max(0, (int)(g1 + ((g2 - g1) * gradient))));
            blue  = (unsigned char)std::min(255, std::max(0, (int)(b1 + ((b2 - b1) * gradient))));

            int height = (4 * gradient)+1; // make it at least 4 thick, so we get some averaging
            for (int i = 0; i < height; i++) {
                tree->createVoxel(x, ((i+1) * voxelSize) , z, voxelSize, red, green, blue);
            }
        }
    }
    printf("DONE adding surface scene...\n");
}
