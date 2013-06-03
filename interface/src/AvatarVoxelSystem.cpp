//
//  AvatarVoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cstring>

#include <QtDebug>

#include "Avatar.h"
#include "AvatarVoxelSystem.h"
#include "renderer/ProgramObject.h"

const float AVATAR_TREE_SCALE = 1.0f;
const int MAX_VOXELS_PER_AVATAR = 2000;
const int BONE_ELEMENTS_PER_VOXEL = BONE_ELEMENTS_PER_VERTEX * VERTICES_PER_VOXEL;

AvatarVoxelSystem::AvatarVoxelSystem(Avatar* avatar) :
    VoxelSystem(AVATAR_TREE_SCALE, MAX_VOXELS_PER_AVATAR),
    _avatar(avatar) {
}

AvatarVoxelSystem::~AvatarVoxelSystem() {   
    delete[] _readBoneIndicesArray;
    delete[] _readBoneWeightsArray;
    delete[] _writeBoneIndicesArray;
    delete[] _writeBoneWeightsArray;
}

ProgramObject* AvatarVoxelSystem::_skinProgram = 0;
int AvatarVoxelSystem::_boneMatricesLocation;
int AvatarVoxelSystem::_boneIndicesLocation;
int AvatarVoxelSystem::_boneWeightsLocation;

void AvatarVoxelSystem::init() {
    VoxelSystem::init();
    
    // prep the data structures for incoming voxel data
    _writeBoneIndicesArray = new GLubyte[BONE_ELEMENTS_PER_VOXEL * _maxVoxels];
    _readBoneIndicesArray = new GLubyte[BONE_ELEMENTS_PER_VOXEL * _maxVoxels];

    _writeBoneWeightsArray = new GLfloat[BONE_ELEMENTS_PER_VOXEL * _maxVoxels];
    _readBoneWeightsArray = new GLfloat[BONE_ELEMENTS_PER_VOXEL * _maxVoxels];
    
    // VBO for the boneIndicesArray
    glGenBuffers(1, &_vboBoneIndicesID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneIndicesID);
    glBufferData(GL_ARRAY_BUFFER, BONE_ELEMENTS_PER_VOXEL * sizeof(GLubyte) * _maxVoxels, NULL, GL_DYNAMIC_DRAW);
    
    // VBO for the boneWeightsArray
    glGenBuffers(1, &_vboBoneWeightsID);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneWeightsID);
    glBufferData(GL_ARRAY_BUFFER, BONE_ELEMENTS_PER_VOXEL * sizeof(GLfloat) * _maxVoxels, NULL, GL_DYNAMIC_DRAW);
    
    for (int i = 0; i < 150; i++) {
        int power = pow(2, randIntInRange(6, 8));
        float size = 1.0f / power;
        _tree->createVoxel(
            randIntInRange(0, power - 1) * size,
            randIntInRange(0, power - 1) * size,
            randIntInRange(0, power - 1) * size, size, 255, 0, 255, true);
    }
    setupNewVoxelsForDrawing();
    
    // load our skin program if this is the first avatar system to initialize
    if (_skinProgram != 0) {
        return;
    }
    _skinProgram = new ProgramObject();
    _skinProgram->addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/skin_voxels.vert");
    _skinProgram->link();
    
    _boneMatricesLocation = _skinProgram->uniformLocation("boneMatrices");
    _boneIndicesLocation = _skinProgram->attributeLocation("boneIndices");
    _boneWeightsLocation = _skinProgram->attributeLocation("boneWeights");
}

void AvatarVoxelSystem::render(bool texture) {
    VoxelSystem::render(texture);
}

void AvatarVoxelSystem::updateNodeInArrays(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                           float voxelScale, const nodeColor& color) {
    VoxelSystem::updateNodeInArrays(nodeIndex, startVertex, voxelScale, color);
    
    GLubyte* writeBoneIndicesAt = _writeBoneIndicesArray + (nodeIndex * BONE_ELEMENTS_PER_VOXEL);
    GLfloat* writeBoneWeightsAt = _writeBoneWeightsArray + (nodeIndex * BONE_ELEMENTS_PER_VOXEL);  
    for (int i = 0; i < VERTICES_PER_VOXEL; i++) {
        BoneIndices boneIndices;
        glm::vec4 boneWeights;
        computeBoneIndicesAndWeights(computeVoxelVertex(startVertex, voxelScale, i), boneIndices, boneWeights);
        for (int j = 0; j < BONE_ELEMENTS_PER_VERTEX; j++) {
            *(writeBoneIndicesAt + i * BONE_ELEMENTS_PER_VERTEX + j) = boneIndices[j];
            *(writeBoneWeightsAt + i * BONE_ELEMENTS_PER_VERTEX + j) = boneWeights[j];
        }
    }
}

void AvatarVoxelSystem::copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    VoxelSystem::copyWrittenDataSegmentToReadArrays(segmentStart, segmentEnd);
    
    int segmentLength = (segmentEnd - segmentStart) + 1;
    GLintptr   segmentStartAt   = segmentStart * BONE_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLsizeiptr segmentSizeBytes = segmentLength * BONE_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLubyte* readBoneIndicesAt     = _readBoneIndicesArray  + (segmentStart * BONE_ELEMENTS_PER_VOXEL);
    GLubyte* writeBoneIndicesAt    = _writeBoneIndicesArray + (segmentStart * BONE_ELEMENTS_PER_VOXEL);
    memcpy(readBoneIndicesAt, writeBoneIndicesAt, segmentSizeBytes);

    segmentStartAt          = segmentStart * BONE_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    segmentSizeBytes        = segmentLength * BONE_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    GLfloat* readBoneWeightsAt   = _readBoneWeightsArray   + (segmentStart * BONE_ELEMENTS_PER_VOXEL);
    GLfloat* writeBoneWeightsAt  = _writeBoneWeightsArray  + (segmentStart * BONE_ELEMENTS_PER_VOXEL);
    memcpy(readBoneWeightsAt, writeBoneWeightsAt, segmentSizeBytes);
}

void AvatarVoxelSystem::updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd) {
    VoxelSystem::updateVBOSegment(segmentStart, segmentEnd);
    
    int segmentLength = (segmentEnd - segmentStart) + 1;
    GLintptr   segmentStartAt   = segmentStart * BONE_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLsizeiptr segmentSizeBytes = segmentLength * BONE_ELEMENTS_PER_VOXEL * sizeof(GLubyte);
    GLubyte* readBoneIndicesFrom   = _readBoneIndicesArray + (segmentStart * BONE_ELEMENTS_PER_VOXEL);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneIndicesID);
    glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readBoneIndicesFrom);
    
    segmentStartAt   = segmentStart * BONE_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    segmentSizeBytes = segmentLength * BONE_ELEMENTS_PER_VOXEL * sizeof(GLfloat);
    GLfloat* readBoneWeightsFrom   = _readBoneWeightsArray + (segmentStart * BONE_ELEMENTS_PER_VOXEL);
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneWeightsID);
    glBufferSubData(GL_ARRAY_BUFFER, segmentStartAt, segmentSizeBytes, readBoneWeightsFrom);  
}

void AvatarVoxelSystem::applyScaleAndBindProgram(bool texture) {
    _skinProgram->bind();
    
    // the base matrix includes centering and scale
    QMatrix4x4 baseMatrix;
    baseMatrix.scale(_treeScale);
    baseMatrix.translate(-0.5f, -0.5f, -0.5f);
    
    // bone matrices include joint transforms
    QMatrix4x4 boneMatrices[NUM_AVATAR_JOINTS];
    for (int i = 0; i < NUM_AVATAR_JOINTS; i++) {
        glm::vec3 position;
        glm::quat orientation;
        _avatar->getBodyBallTransform((AvatarJointID)i, position, orientation);
        boneMatrices[i].translate(position.x, position.y, position.z);
        boneMatrices[i].rotate(QQuaternion(orientation.w, orientation.x, orientation.y, orientation.z));
        const glm::vec3& defaultPosition = _avatar->getSkeleton().joint[i].absoluteDefaultPosePosition;
        boneMatrices[i].translate(-defaultPosition.x, -defaultPosition.y, -defaultPosition.z);
        boneMatrices[i] *= baseMatrix;
    } 
    _skinProgram->setUniformValueArray(_boneMatricesLocation, boneMatrices, NUM_AVATAR_JOINTS);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneIndicesID);
    glVertexAttribPointer(_boneIndicesLocation, BONE_ELEMENTS_PER_VERTEX, GL_UNSIGNED_BYTE, false, 0, 0);
    _skinProgram->enableAttributeArray(_boneIndicesLocation);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboBoneWeightsID);
    _skinProgram->setAttributeBuffer(_boneWeightsLocation, GL_FLOAT, 0, BONE_ELEMENTS_PER_VERTEX);
    _skinProgram->enableAttributeArray(_boneWeightsLocation);
}

void AvatarVoxelSystem::removeScaleAndReleaseProgram(bool texture) {
    _skinProgram->release();
    _skinProgram->disableAttributeArray(_boneIndicesLocation);
    _skinProgram->disableAttributeArray(_boneWeightsLocation);
}

class IndexDistance {
public:
    IndexDistance(GLubyte index = 0, float distance = FLT_MAX) : index(index), distance(distance) { }

    GLubyte index;
    float distance;
};

void AvatarVoxelSystem::computeBoneIndicesAndWeights(const glm::vec3& vertex, BoneIndices& indices, glm::vec4& weights) const {
    // transform into joint space
    glm::vec3 jointVertex = (vertex - glm::vec3(0.5f, 0.5f, 0.5f)) * AVATAR_TREE_SCALE;
    
    // find the nearest four joints (TODO: use a better data structure for the pose positions to speed this up)
    IndexDistance nearest[BONE_ELEMENTS_PER_VERTEX];
    for (int i = 0; i < NUM_AVATAR_JOINTS; i++) {
        float distance = glm::distance(jointVertex, _avatar->getSkeleton().joint[i].absoluteDefaultPosePosition);
        for (int j = 0; j < BONE_ELEMENTS_PER_VERTEX; j++) {
            if (distance < nearest[j].distance) {
                // move the rest of the indices down
                for (int k = BONE_ELEMENTS_PER_VERTEX - 1; k > j; k--) {
                    nearest[k] = nearest[k - 1];
                }       
                nearest[j] = IndexDistance(i, distance);
                break;
            }
        }
    } 
    
    // compute the weights based on inverse distance
    float totalWeight = 0.0f;
    for (int i = 0; i < BONE_ELEMENTS_PER_VERTEX; i++) {
        indices[i] = nearest[i].index;
        weights[i] = (i == 0) ? 1.0f : 0.0f; // 1.0f / glm::max(nearest[i].distance, EPSILON);
        totalWeight += weights[i];
    }
    
    // normalize the weights
    for (int i = 0; i < BONE_ELEMENTS_PER_VERTEX; i++) {
        weights[i] /= totalWeight;
    }
}
