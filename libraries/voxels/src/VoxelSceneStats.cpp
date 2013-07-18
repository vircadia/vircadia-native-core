//
//  VoxelSceneStats.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//
//

#include <SharedUtil.h>

#include "VoxelNode.h"
#include "VoxelSceneStats.h"

VoxelSceneStats::VoxelSceneStats() {
    reset();
}

VoxelSceneStats::~VoxelSceneStats() {
}

void VoxelSceneStats::sceneStarted(bool fullScene, bool moving) {
    _start = usecTimestampNow();
    reset(); // resets packet and voxel stats
    _fullSceneDraw = fullScene;
    _moving = moving;
}

void VoxelSceneStats::sceneCompleted() {
    _end = usecTimestampNow();
    _elapsed = _end - _start;
}
    
void VoxelSceneStats::reset() {
    _packets = 0;
    _bytes = 0;
    _passes = 0;

    _traversed = 0;
    _internal = 0;
    _leaves = 0;

    _skippedDistance = 0;
    _internalSkippedDistance = 0;
    _leavesSkippedDistance = 0;

    _skippedOutOfView = 0;
    _internalSkippedOutOfView = 0;
    _leavesSkippedOutOfView = 0;

    _skippedWasInView = 0;
    _internalSkippedWasInView = 0;
    _leavesSkippedWasInView = 0;

    _skippedNoChange = 0;
    _internalSkippedNoChange = 0;
    _leavesSkippedNoChange = 0;

    _skippedOccluded = 0;
    _internalSkippedOccluded = 0;
    _leavesSkippedOccluded = 0;

    _colorSent = 0;
    _internalColorSent = 0;
    _leavesColorSent = 0;

    _didntFit = 0;
    _internalDidntFit = 0;
    _leavesDidntFit = 0;

}

void VoxelSceneStats::packetSent(int bytes) {
    _packets++;
    _bytes += bytes;
}

void VoxelSceneStats::traversed(const VoxelNode* node) {
    _traversed++;
    if (node->isLeaf()) {
        _leaves++;
    } else {
        _internal++;
    }
}

void VoxelSceneStats::skippedDistance(const VoxelNode* node) {
    _skippedDistance++;
    if (node->isLeaf()) {
        _leavesSkippedDistance++;
    } else {
        _internalSkippedDistance++;
    }
}

void VoxelSceneStats::skippedOutOfView(const VoxelNode* node) {
    _skippedOutOfView++;
    if (node->isLeaf()) {
        _leavesSkippedOutOfView++;
    } else {
        _internalSkippedOutOfView++;
    }
}

void VoxelSceneStats::skippedWasInView(const VoxelNode* node) {
    _skippedWasInView++;
    if (node->isLeaf()) {
        _leavesSkippedWasInView++;
    } else {
        _internalSkippedWasInView++;
    }
}

void VoxelSceneStats::skippedNoChange(const VoxelNode* node) {
    _skippedNoChange++;
    if (node->isLeaf()) {
        _leavesSkippedNoChange++;
    } else {
        _internalSkippedNoChange++;
    }
}

void VoxelSceneStats::skippedOccluded(const VoxelNode* node) {
    _skippedOccluded++;
    if (node->isLeaf()) {
        _leavesSkippedOccluded++;
    } else {
        _internalSkippedOccluded++;
    }
}

void VoxelSceneStats::colorSent(const VoxelNode* node) {
    _colorSent++;
    if (node->isLeaf()) {
        _leavesColorSent++;
    } else {
        _internalColorSent++;
    }
}

void VoxelSceneStats::didntFit(const VoxelNode* node) {
    _didntFit++;
    if (node->isLeaf()) {
        _leavesDidntFit++;
    } else {
        _internalDidntFit++;
    }
}

void VoxelSceneStats::printDebugDetails() {
    qDebug("\n------------------------------\n");
    qDebug("VoxelSceneStats:\n");
    qDebug("    start  : %llu \n", _start);
    qDebug("    end    : %llu \n", _end);
    qDebug("    elapsed: %llu \n", _elapsed);
    qDebug("\n");
    qDebug("    full scene: %s\n", debug::valueOf(_fullSceneDraw));
    qDebug("    moving: %s\n", debug::valueOf(_moving));
    qDebug("\n");
    qDebug("    packets: %d\n", _packets);
    qDebug("    bytes  : %d\n", _bytes);
    qDebug("\n");
    qDebug("    traversed           : %lu\n", _traversed                );
    qDebug("        internal        : %lu\n", _internal                 );
    qDebug("        leaves          : %lu\n", _leaves                   );
    qDebug("    skipped distance    : %lu\n", _skippedDistance          );
    qDebug("        internal        : %lu\n", _internalSkippedDistance  );
    qDebug("        leaves          : %lu\n", _leavesSkippedDistance    );
    qDebug("    skipped out of view : %lu\n", _skippedOutOfView         );
    qDebug("        internal        : %lu\n", _internalSkippedOutOfView );
    qDebug("        leaves          : %lu\n", _leavesSkippedOutOfView   );
    qDebug("    skipped was in view : %lu\n", _skippedWasInView         );
    qDebug("        internal        : %lu\n", _internalSkippedWasInView );
    qDebug("        leaves          : %lu\n", _leavesSkippedWasInView   );
    qDebug("    skipped no change   : %lu\n", _skippedNoChange          );
    qDebug("        internal        : %lu\n", _internalSkippedNoChange  );
    qDebug("        leaves          : %lu\n", _leavesSkippedNoChange    );
    qDebug("    skipped occluded    : %lu\n", _skippedOccluded          );
    qDebug("        internal        : %lu\n", _internalSkippedOccluded  );
    qDebug("        leaves          : %lu\n", _leavesSkippedOccluded    );
    qDebug("    color sent          : %lu\n", _colorSent                );
    qDebug("        internal        : %lu\n", _internalColorSent        );
    qDebug("        leaves          : %lu\n", _leavesColorSent          );
    qDebug("    Didn't Fit          : %lu\n", _didntFit                 );
    qDebug("        internal        : %lu\n", _internalDidntFit         );
    qDebug("        leaves          : %lu\n", _leavesDidntFit           );
}
