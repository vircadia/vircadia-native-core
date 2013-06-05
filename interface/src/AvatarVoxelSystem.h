//
//  AvatarVoxelSystem.h
//  interface
//
//  Created by Andrzej Kapolka on 5/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AvatarVoxelSystem__
#define __interface__AvatarVoxelSystem__

#include <QObject>

#include "VoxelSystem.h"

const int BONE_ELEMENTS_PER_VERTEX = 4;
typedef GLubyte BoneIndices[BONE_ELEMENTS_PER_VERTEX];

class QNetworkReply;
class QUrl;

class Avatar;

class AvatarVoxelSystem : public QObject, public VoxelSystem {
    Q_OBJECT

public:
        
    AvatarVoxelSystem(Avatar* avatar);
    virtual ~AvatarVoxelSystem();
    
    virtual void init();

    virtual void removeOutOfView();
    
    void loadVoxelsFromURL(const QUrl& url);
    
protected:
    
    virtual void updateNodeInArrays(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                    float voxelScale, const nodeColor& color);
    virtual void copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void applyScaleAndBindProgram(bool texture);
    virtual void removeScaleAndReleaseProgram(bool texture);

private slots:
    
    void handleVoxelDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleVoxelReplyError();    
    
private:
    
    void computeBoneIndicesAndWeights(const glm::vec3& vertex, BoneIndices& indices, glm::vec4& weights) const;
    
    Avatar* _avatar;
    
    GLubyte* _readBoneIndicesArray;
    GLfloat* _readBoneWeightsArray;
    GLubyte* _writeBoneIndicesArray;
    GLfloat* _writeBoneWeightsArray;
    
    GLuint _vboBoneIndicesID;
    GLuint _vboBoneWeightsID;
    
    QNetworkReply* _voxelReply;
    
    static ProgramObject* _skinProgram;
    static int _boneMatricesLocation;
    static int _boneIndicesLocation;
    static int _boneWeightsLocation;
};

#endif /* defined(__interface__AvatarVoxelSystem__) */
