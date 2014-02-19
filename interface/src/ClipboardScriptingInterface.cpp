//
//  ClipboardScriptingInterface.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "Application.h"
#include "ClipboardScriptingInterface.h"

ClipboardScriptingInterface::ClipboardScriptingInterface() {
}

void ClipboardScriptingInterface::cutVoxels(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };
    Application::getInstance()->cutVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::copyVoxels(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };
    Application::getInstance()->copyVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::pasteVoxels(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE, 
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };

    Application::getInstance()->pasteVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::deleteVoxels(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };
    Application::getInstance()->deleteVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::exportVoxels(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };

    Application::getInstance()->exportVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::importVoxels() {
    Application::getInstance()->importVoxels();
}

