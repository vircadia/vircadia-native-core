//
//  AvatarVoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cstring>

#include <QNetworkReply>

#include <GeometryUtil.h>

#include "Application.h"
#include "Avatar.h"
#include "AvatarVoxelSystem.h"
#include "renderer/ProgramObject.h"

const float AVATAR_TREE_SCALE = 1.0f;
const int MAX_VOXELS_PER_AVATAR = 10000;
const int BONE_ELEMENTS_PER_VOXEL = BONE_ELEMENTS_PER_VERTEX * VERTICES_PER_VOXEL;

AvatarVoxelSystem::AvatarVoxelSystem(Avatar* avatar) :
    VoxelSystem(AVATAR_TREE_SCALE, MAX_VOXELS_PER_AVATAR),
    _mode(0), _avatar(avatar), _voxelReply(0) {

    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
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

void AvatarVoxelSystem::removeOutOfView() {
    // no-op for now
}

class Mode {
public:
    
    bool bindVoxelsTogether;
    int maxBonesPerBind;
    bool includeBonesOutsideBindRadius;
};

const Mode MODES[] = {
    { false, BONE_ELEMENTS_PER_VERTEX, false }, // original
    { false, 1, true }, // one bone per vertex
    { true, 1, true }, // one bone per voxel
    { true, BONE_ELEMENTS_PER_VERTEX, false } }; // four bones per voxel

void AvatarVoxelSystem::cycleMode() {
    _mode = (_mode + 1) % (sizeof(MODES) / sizeof(MODES[0]));
    qDebug("Voxeltar bind mode %d.\n", _mode);
    
    // rebind
    QUrl url = _voxelURL;
    setVoxelURL(QUrl());
    setVoxelURL(url);
}

void AvatarVoxelSystem::setVoxelURL(const QUrl& url) {
    // don't restart the download if it's the same URL
    if (_voxelURL == url) {
        return;
    }

    // cancel any current download
    if (_voxelReply != 0) {
        delete _voxelReply;
        _voxelReply = 0;
    }
    
    killLocalVoxels();
    
    // remember the URL
    _voxelURL = url;
    
    // handle "file://" urls...
    if (url.isLocalFile()) {
        QString pathString = url.path();
        QByteArray pathAsAscii = pathString.toLocal8Bit();
        const char* path = pathAsAscii.data();
        readFromSVOFile(path);
        return;        
    }
    
    // load the URL data asynchronously
    if (!url.isValid()) {
        return;
    }
    _voxelReply = Application::getInstance()->getNetworkAccessManager()->get(QNetworkRequest(url));
    connect(_voxelReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleVoxelDownloadProgress(qint64,qint64)));
    connect(_voxelReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleVoxelReplyError()));
}

void AvatarVoxelSystem::updateNodeInArrays(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                           float voxelScale, const nodeColor& color) {
    VoxelSystem::updateNodeInArrays(nodeIndex, startVertex, voxelScale, color);
    
    GLubyte* writeBoneIndicesAt = _writeBoneIndicesArray + (nodeIndex * BONE_ELEMENTS_PER_VOXEL);
    GLfloat* writeBoneWeightsAt = _writeBoneWeightsArray + (nodeIndex * BONE_ELEMENTS_PER_VOXEL);
    
    if (MODES[_mode].bindVoxelsTogether) {
        BoneIndices boneIndices;
        glm::vec4 boneWeights;
        computeBoneIndicesAndWeights(startVertex + glm::vec3(voxelScale, voxelScale, voxelScale) * 0.5f,
            boneIndices, boneWeights);
        for (int i = 0; i < VERTICES_PER_VOXEL; i++) {
            for (int j = 0; j < BONE_ELEMENTS_PER_VERTEX; j++) {
                *(writeBoneIndicesAt + i * BONE_ELEMENTS_PER_VERTEX + j) = boneIndices[j];
                *(writeBoneWeightsAt + i * BONE_ELEMENTS_PER_VERTEX + j) = boneWeights[j];
            }
        }
    } else {
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
        orientation = orientation * glm::inverse(_avatar->getSkeleton().joint[i].absoluteBindPoseRotation);
        boneMatrices[i].rotate(QQuaternion(orientation.w, orientation.x, orientation.y, orientation.z));
        const glm::vec3& bindPosition = _avatar->getSkeleton().joint[i].absoluteBindPosePosition;
        boneMatrices[i].translate(-bindPosition.x, -bindPosition.y, -bindPosition.z);
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

void AvatarVoxelSystem::handleVoxelDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    // for now, just wait until we have the full business
    if (bytesReceived < bytesTotal) {
        return;
    }

    QByteArray entirety = _voxelReply->readAll();
    _voxelReply->disconnect(this);
    _voxelReply->deleteLater();
    _voxelReply = 0;
    
    _tree->readBitstreamToTree((unsigned char*)entirety.data(), entirety.size(), WANT_COLOR, NO_EXISTS_BITS);
    setupNewVoxelsForDrawing();
}

void AvatarVoxelSystem::handleVoxelReplyError() {
    qDebug("%s\n", _voxelReply->errorString().toLocal8Bit().constData());
    
    _voxelReply->disconnect(this);
    _voxelReply->deleteLater();
    _voxelReply = 0;
}

class IndexDistance {
public:
    IndexDistance(GLubyte index = AVATAR_JOINT_PELVIS, float distance = FLT_MAX) : index(index), distance(distance) { }

    GLubyte index;
    float distance;
};

void AvatarVoxelSystem::computeBoneIndicesAndWeights(const glm::vec3& vertex, BoneIndices& indices, glm::vec4& weights) const {
    // transform into joint space
    glm::vec3 jointVertex = (vertex - glm::vec3(0.5f, 0.5f, 0.5f)) * AVATAR_TREE_SCALE;
    
    // find the nearest four joints (TODO: use a better data structure for the pose positions to speed this up)
    IndexDistance nearest[BONE_ELEMENTS_PER_VERTEX];
    const Skeleton& skeleton = _avatar->getSkeleton();
    for (int i = 0; i < NUM_AVATAR_JOINTS; i++) {
        AvatarJointID parent = skeleton.joint[i].parent;
        float distance = glm::length(computeVectorFromPointToSegment(jointVertex,
            skeleton.joint[parent == AVATAR_JOINT_NULL ? i : parent].absoluteBindPosePosition,
            skeleton.joint[i].absoluteBindPosePosition));
        if (!MODES[_mode].includeBonesOutsideBindRadius && distance > skeleton.joint[i].bindRadius) {
            continue;
        }
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
    for (int i = 0; i < MODES[_mode].maxBonesPerBind; i++) {
        indices[i] = nearest[i].index;
        if (nearest[i].distance != FLT_MAX) {
            weights[i] = 1.0f / glm::max(nearest[i].distance, EPSILON);
            totalWeight += weights[i];
        
        } else {
            weights[i] = 0.0f;
        }
    }
    
    // if it's not attached to anything, consider it attached to the hip
    if (totalWeight == 0.0f) {
        weights[0] = 1.0f;
        return;
    }
    
    // ortherwise, normalize the weights
    for (int i = 0; i < BONE_ELEMENTS_PER_VERTEX; i++) {
        weights[i] /= totalWeight;
    }
}
