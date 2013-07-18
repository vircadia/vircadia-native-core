//
//  VoxelSceneStats.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//
//

#ifndef __hifi__VoxelSceneStats__
#define __hifi__VoxelSceneStats__

#include <stdint.h>

class VoxelNode;

class VoxelSceneStats {
public:
    VoxelSceneStats();
    ~VoxelSceneStats();
    void reset();
    void sceneStarted(bool fullScene, bool moving);
    void sceneCompleted();
    void printDebugDetails();
    void packetSent(int bytes);
    
    void traversed(const VoxelNode* node);
    void skippedDistance(const VoxelNode* node);
    void skippedOutOfView(const VoxelNode* node);
    void skippedWasInView(const VoxelNode* node);
    void skippedNoChange(const VoxelNode* node);
    void skippedOccluded(const VoxelNode* node);
    void colorSent(const VoxelNode* node);
    void didntFit(const VoxelNode* node);
    void colorBitsWritten(const VoxelNode* node);
    void existsBitsWritten(const VoxelNode* node);
    void existsInPacketBitsWritten(const VoxelNode* node);
    
private:
    // scene timing data in usecs
    uint64_t _start;
    uint64_t _end;
    uint64_t _elapsed;
    
    // scene voxel related data
    unsigned long _traversed;
    unsigned long _internal;
    unsigned long _leaves;
    
    unsigned long _skippedDistance;
    unsigned long _internalSkippedDistance;
    unsigned long _leavesSkippedDistance;

    unsigned long _skippedOutOfView;
    unsigned long _internalSkippedOutOfView;
    unsigned long _leavesSkippedOutOfView;

    unsigned long _skippedWasInView;
    unsigned long _internalSkippedWasInView;
    unsigned long _leavesSkippedWasInView;

    unsigned long _skippedNoChange;
    unsigned long _internalSkippedNoChange;
    unsigned long _leavesSkippedNoChange;

    unsigned long _skippedOccluded;
    unsigned long _internalSkippedOccluded;
    unsigned long _leavesSkippedOccluded;

    unsigned long _colorSent;
    unsigned long _internalColorSent;
    unsigned long _leavesColorSent;

    unsigned long _didntFit;
    unsigned long _internalDidntFit;
    unsigned long _leavesDidntFit;

    unsigned long total;
    unsigned long internalOutOfView;
    unsigned long internalOccluded;
    unsigned long internalDirty;
    unsigned long leavesOutOfView;
    unsigned long leavesOccluded;
    unsigned long leavesDirty;
    
    // scene network related data
    unsigned int  _packets;
    unsigned int  _bytes;
    unsigned int  _passes;
    
    // features related items
    bool _moving;
    bool _fullSceneDraw;
};

#endif /* defined(__hifi__VoxelSceneStats__) */
