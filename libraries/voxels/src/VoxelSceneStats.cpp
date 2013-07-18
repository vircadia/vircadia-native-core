//
//  VoxelSceneStats.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//
//

#include "VoxelSceneStats.h"

VoxelSceneStats::VoxelSceneStats() :
    start(0),
    end(0),
    elapsed(0),
    total(0),
    traversed(0),
    internal(0),
    internalOutOfView(0),
    internalOccluded(0),
    internalDirty(0),
    leaves(0),
    leavesOutOfView(0),
    leavesOccluded(0),
    leavesDirty(0),
    packets(0),
    bytes(0),
    passes(0),
    elapsedUsecsToSend(0),
    wasFinished(false),
    wasMoving(false),
    hadDeltaView(false),
    hadOcclusionCulling(false)
{
}

VoxelSceneStats::~VoxelSceneStats() {
}

void VoxelSceneStats::sceneStarted() {
    start = usecTimestampNow();
}

void VoxelSceneStats::sceneCompleted() {
    end = usecTimestampNow();
    elapsed = end - start;
}
    
void VoxelSceneStats::reset() {
    start = 0;
    end= 0;
    elapsed= 0;


    total = 0;
    traversed = 0;
    internal = 0;
    internalOutOfView = 0;
    internalOccluded = 0;
    internalDirty = 0;
    leaves = 0;
    leavesOutOfView = 0;
    leavesOccluded = 0;
    leavesDirty = 0;
    packets = 0;
    bytes = 0;
    passes = 0;
    wasFinished = false;
    wasMoving = false;
    hadDeltaView = false;
    hadOcclusionCulling = false;
}


void VoxelSceneStats::printDebugDetails() {
    qDebug("VoxelSceneStats: start: %llu, end: %llu, elapsed: %llu \n", start, end, elapsed);
}
