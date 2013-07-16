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

    // Now some more examples... creating some lines using the line primitive
    printf("creating voxel lines...\n");
    // We want our line voxels to be about 1/32 meter high, and our TREE_SCALE is in meters, so...    
    float lineVoxelSize = 1.f / (32 * TREE_SCALE);
    rgbColor red   = {255, 0, 0};
    rgbColor green = {0, 255, 0};
    rgbColor blue  = {0, 0, 255};
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), lineVoxelSize, blue);
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), lineVoxelSize, red);
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), lineVoxelSize, green);
    printf("DONE creating lines...\n");
}

void addSphereScene(VoxelTree * tree) {
    printf("adding sphere scene...\n");

    // Now some more examples... creating some spheres using the sphere primitive
    // We want the smallest unit of our spheres to be about 1/16th of a meter tall
    float sphereVoxelSize = 1.f / (8 * TREE_SCALE);
    printf("creating spheres... sphereVoxelSize=%f\n",sphereVoxelSize);

    tree->createSphere(0.030625, 0.5, 0.5, (0.25 - 0.06125), sphereVoxelSize, true, NATURAL);
    printf("1 spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.030625, (0.75 - 0.030625), (0.75 - 0.030625), (0.75 - 0.06125), sphereVoxelSize, true, GRADIENT);
    printf("2 spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.030625, (0.75 - 0.030625), (0.75 - 0.030625), 0.06125, sphereVoxelSize, true, RANDOM);
    printf("3 spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.030625, (0.75 - 0.030625), 0.06125, (0.75 - 0.06125), sphereVoxelSize, true, GRADIENT);
    printf("4 spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.06125, 0.125, 0.125, (0.75 - 0.125), sphereVoxelSize, true, GRADIENT);

/**
    float radius = 0.0125f;
    printf("5 spheres added...\n");
    tree->createSphere(radius, 0.25, radius * 5.0f, 0.25, sphereVoxelSize, true, GRADIENT);
    printf("6 spheres added...\n");
    tree->createSphere(radius, 0.125, radius * 5.0f, 0.25, sphereVoxelSize, true, RANDOM);
    printf("7 spheres added...\n");
    tree->createSphere(radius, 0.075, radius * 5.0f, 0.25, sphereVoxelSize, true, GRADIENT);
    printf("8 spheres added...\n");
    tree->createSphere(radius, 0.05, radius * 5.0f, 0.25, sphereVoxelSize, true, RANDOM);
    printf("9 spheres added...\n");
    tree->createSphere(radius, 0.025, radius * 5.0f, 0.25, sphereVoxelSize, true, GRADIENT);
    printf("10 spheres added...\n");
*/
    float largeRadius = 0.1875f;
    tree->createSphere(largeRadius, 0.5, 0.5, 0.5, sphereVoxelSize, true, NATURAL);
    printf("11 - last large sphere added... largeRadius=%f sphereVoxelSize=%f\n", largeRadius, sphereVoxelSize);

    printf("DONE adding scene of spheres...\n");
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
