//
//  AvatarVoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cstring>

#include "AvatarVoxelSystem.h"

const float AVATAR_TREE_SCALE = 1.0f;
const int MAX_VOXELS_PER_AVATAR = 2000;
const int BONE_INDEX_ELEMENTS_PER_VOXEL = 4 * VERTICES_PER_VOXEL;
const int BONE_WEIGHT_ELEMENTS_PER_VOXEL = 4 * VERTICES_PER_VOXEL;

AvatarVoxelSystem::AvatarVoxelSystem() : VoxelSystem(AVATAR_TREE_SCALE, MAX_VOXELS_PER_AVATAR) {
}

AvatarVoxelSystem::~AvatarVoxelSystem() {   
    delete[] _readBoneIndicesArray;
    delete[] _readBoneWeightsArray;
    delete[] _writeBoneIndicesArray;
    delete[] _writeBoneWeightsArray;
}

void AvatarVoxelSystem::init() {
    VoxelSystem::init();
    
    // prep the data structures for incoming voxel data
    _writeBoneIndicesArray = new GLubyte[BONE_INDEX_ELEMENTS_PER_VOXEL * _maxVoxels];
    _readBoneIndicesArray = new GLubyte[BONE_INDEX_ELEMENTS_PER_VOXEL * _maxVoxels];

    _writeBoneWeightsArray = new GLfloat[BONE_WEIGHT_ELEMENTS_PER_VOXEL * _maxVoxels];
    _readBoneWeightsArray = new GLfloat[BONE_WEIGHT_ELEMENTS_PER_VOXEL * _maxVoxels];
    
    // VBO for the boneIndicesArray
    glGenBuffers(1, &_vboBoneIndicesID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneIndicesID);
    glBufferData(GL_ARRAY_BUFFER, BONE_INDEX_ELEMENTS_PER_VOXEL * sizeof(GLubyte) * _maxVoxels, NULL, GL_DYNAMIC_DRAW);
    
    // VBO for the boneWeightsArray
    glGenBuffers(1, &_vboBoneWeightsID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneWeightsID);
    glBufferData(GL_ARRAY_BUFFER, BONE_WEIGHT_ELEMENTS_PER_VOXEL * sizeof(GLfloat) * _maxVoxels, NULL, GL_DYNAMIC_DRAW);
}

void AvatarVoxelSystem::render(bool texture) {
    VoxelSystem::render(texture);
}

void AvatarVoxelSystem::updateNodeInArrays(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                           float voxelScale, const nodeColor& color) {
    VoxelSystem::updateNodeInArrays(nodeIndex, startVertex, voxelScale, color);
    
    for (int j = 0; j < BONE_INDEX_ELEMENTS_PER_VOXEL; j++) {    
        GLubyte* writeBoneIndicesAt = _writeBoneIndicesArray + (nodeIndex * BONE_INDEX_ELEMENTS_PER_VOXEL);
        GLfloat* writeBoneWeightsAt = _writeBoneWeightsArray + (nodeIndex * BONE_WEIGHT_ELEMENTS_PER_VOXEL);
        *(writeBoneIndicesAt + j) = 0;
        *(writeBoneWeightsAt + j) = 0.0f;
    }
}

void AvatarVoxelSystem::copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    VoxelSystem::copyWrittenDataSegmentToReadArrays(segmentStart, segmentEnd);
    
    int segmentLength = (segmentEnd - segmentStart) + 1;
    GLintptr   segmentStartAt   = segmentStart * BONE_INDEX_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLsizeiptr segmentSizeBytes = segmentLength * BONE_INDEX_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLubyte* readBoneIndicesAt     = _readBoneIndicesArray  + (segmentStart * BONE_INDEX_ELEMENTS_PER_VOXEL);
    GLubyte* writeBoneIndicesAt    = _writeBoneIndicesArray + (segmentStart * BONE_INDEX_ELEMENTS_PER_VOXEL);
    memcpy(readBoneIndicesAt, writeBoneIndicesAt, segmentSizeBytes);

    segmentStartAt          = segmentStart * BONE_WEIGHT_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    segmentSizeBytes        = segmentLength * BONE_WEIGHT_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    GLfloat* readBoneWeightsAt   = _readBoneWeightsArray   + (segmentStart * BONE_WEIGHT_ELEMENTS_PER_VOXEL);
    GLfloat* writeBoneWeightsAt  = _writeBoneWeightsArray  + (segmentStart * BONE_WEIGHT_ELEMENTS_PER_VOXEL);
    memcpy(readBoneWeightsAt, writeBoneWeightsAt, segmentSizeBytes);
}

void AvatarVoxelSystem::updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    VoxelSystem::updateVBOSegment(segmentStart, segmentEnd);
    
    int segmentLength = (segmentEnd - segmentStart) + 1;
    GLintptr   segmentStartAt   = segmentStart * BONE_INDEX_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLsizeiptr segmentSizeBytes = segmentLength * BONE_INDEX_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLubyte* readBoneIndicesFrom   = _readBoneIndicesArray + (segmentStart * BONE_INDEX_ELEMENTS_PER_VOXEL);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneIndicesID);
    glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readBoneIndicesFrom);
    
    segmentStartAt   = segmentStart * BONE_WEIGHT_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    segmentSizeBytes = segmentLength * BONE_WEIGHT_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    GLfloat* readBoneWeightsFrom   = _readBoneWeightsArray + (segmentStart * BONE_WEIGHT_ELEMENTS_PER_VOXEL);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneWeightsID);
    glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readBoneWeightsFrom);  
}

