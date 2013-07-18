//
//  VoxelSceneStats.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//
//

#ifndef __hifi__VoxelSceneStats__
#define __hifi__VoxelSceneStats__


class VoxelSceneStats {
public:
    VoxelSceneStats();
    ~VoxelSceneStats();
    void reset();
    void sceneStarted();
    void sceneCompleted();
    void printDebugDetails();
    
    // scene timing data in usecs
    uint64_t start;
    uint64_t end;
    uint64_t elapsed;
    
    // scene voxel related data
    unsigned long total;
    unsigned long traversed;
    unsigned long internal;
    unsigned long internalOutOfView;
    unsigned long internalOccluded;
    unsigned long internalDirty;
    unsigned long leaves;
    unsigned long leavesOutOfView;
    unsigned long leavesOccluded;
    unsigned long leavesDirty;
    
    // scene network related data
    unsigned int  packets;
    unsigned int  bytes;
    unsigned int  passes;
    
    // features related items
    bool wasFinished;
    bool wasMoving;
    bool hadDeltaView;
    bool hadOcclusionCulling;
};

#endif /* defined(__hifi__VoxelSceneStats__) */
