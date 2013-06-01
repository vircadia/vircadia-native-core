//
//  AvatarVoxelSystem.h
//  interface
//
//  Created by Andrzej Kapolka on 5/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AvatarVoxelSystem__
#define __interface__AvatarVoxelSystem__

#include "VoxelSystem.h"

class Avatar;

class AvatarVoxelSystem : public VoxelSystem {
public:
    
    AvatarVoxelSystem(Avatar* avatar);
    virtual ~AvatarVoxelSystem();
    
    virtual void init();
    virtual void render(bool texture);

protected:
    
    virtual void updateNodeInArrays(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                    float voxelScale, const nodeColor& color);
    virtual void copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void bindProgram(bool texture);
    virtual void releaseProgram(bool texture);
    
private:
    
    Avatar* _avatar;
    
    GLubyte* _readBoneIndicesArray;
    GLfloat* _readBoneWeightsArray;
    GLubyte* _writeBoneIndicesArray;
    GLfloat* _writeBoneWeightsArray;
    
    GLuint _vboBoneIndicesID;
    GLuint _vboBoneWeightsID;
    
    static ProgramObject* _skinProgram;
    static int _boneMatricesLocation;
    static int _boneIndicesLocation;
    static int _boneWeightsLocation;
};

#endif /* defined(__interface__AvatarVoxelSystem__) */
