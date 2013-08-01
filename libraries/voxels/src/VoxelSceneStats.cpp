//
//  VoxelSceneStats.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "VoxelNode.h"
#include "VoxelSceneStats.h"


const int samples = 100;
VoxelSceneStats::VoxelSceneStats() : 
    _elapsedAverage(samples), 
    _bitsPerVoxelAverage(samples) 
{
    reset();
    _isReadyToSend = false;
    _isStarted = false;
}

void VoxelSceneStats::sceneStarted(bool isFullScene, bool isMoving, VoxelNode* root) {
    reset(); // resets packet and voxel stats
    _isStarted = true;
    _start = usecTimestampNow();
    _totalVoxels   = root->getSubTreeNodeCount();
    _totalInternal = root->getSubTreeInternalNodeCount();
    _totalLeaves   = root->getSubTreeLeafNodeCount();
    
    _isFullScene = isFullScene;
    _isMoving = isMoving;
}

void VoxelSceneStats::sceneCompleted() {
    if (_isStarted) {
        _end = usecTimestampNow();
        _elapsed = _end - _start;
        _elapsedAverage.updateAverage((float)_elapsed);

        _statsMessageLength = packIntoMessage(_statsMessage, sizeof(_statsMessage));
        _isReadyToSend = true;
        _isStarted = false;
    }
}

void VoxelSceneStats::encodeStarted() {
    _encodeStart = usecTimestampNow();
}

void VoxelSceneStats::encodeStopped() {
    _totalEncodeTime += (usecTimestampNow() - _encodeStart);
}

void VoxelSceneStats::reset() {
    _totalEncodeTime = 0;
    _encodeStart = 0;

    _packets = 0;
    _bytes = 0;
    _passes = 0;

    _totalVoxels = 0;
    _totalInternal = 0;
    _totalLeaves = 0;

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

    _colorBitsWritten = 0;
    _existsBitsWritten = 0;
    _existsInPacketBitsWritten = 0;
    _treesRemoved = 0;
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

void VoxelSceneStats::colorBitsWritten() {
    _colorBitsWritten++;
}

void VoxelSceneStats::existsBitsWritten() {
    _existsBitsWritten++;
}

void VoxelSceneStats::existsInPacketBitsWritten() {
    _existsInPacketBitsWritten++;
}

void VoxelSceneStats::childBitsRemoved(bool includesExistsBits, bool includesColors) {
    _existsInPacketBitsWritten--;
    if (includesExistsBits) {
        _existsBitsWritten--;
    }
    if (includesColors) {
        _colorBitsWritten--;
    }
    _treesRemoved++;
}

int VoxelSceneStats::packIntoMessage(unsigned char* destinationBuffer, int availableBytes) {
    unsigned char* bufferStart = destinationBuffer;
    
    int headerLength = populateTypeAndVersion(destinationBuffer, PACKET_TYPE_VOXEL_STATS);
    destinationBuffer += headerLength;
    
    memcpy(destinationBuffer, &_start, sizeof(_start));
    destinationBuffer += sizeof(_start);
    memcpy(destinationBuffer, &_end, sizeof(_end));
    destinationBuffer += sizeof(_end);
    memcpy(destinationBuffer, &_elapsed, sizeof(_elapsed));
    destinationBuffer += sizeof(_elapsed);
    memcpy(destinationBuffer, &_totalEncodeTime, sizeof(_totalEncodeTime));
    destinationBuffer += sizeof(_totalEncodeTime);
    memcpy(destinationBuffer, &_isFullScene, sizeof(_isFullScene));
    destinationBuffer += sizeof(_isFullScene);
    memcpy(destinationBuffer, &_isMoving, sizeof(_isMoving));
    destinationBuffer += sizeof(_isMoving);
    memcpy(destinationBuffer, &_packets, sizeof(_packets));
    destinationBuffer += sizeof(_packets);
    memcpy(destinationBuffer, &_bytes, sizeof(_bytes));
    destinationBuffer += sizeof(_bytes);

    memcpy(destinationBuffer, &_totalInternal, sizeof(_totalInternal));
    destinationBuffer += sizeof(_totalInternal);
    memcpy(destinationBuffer, &_totalLeaves, sizeof(_totalLeaves));
    destinationBuffer += sizeof(_totalLeaves);
    memcpy(destinationBuffer, &_internal, sizeof(_internal));
    destinationBuffer += sizeof(_internal);
    memcpy(destinationBuffer, &_leaves, sizeof(_leaves));
    destinationBuffer += sizeof(_leaves);
    memcpy(destinationBuffer, &_internalSkippedDistance, sizeof(_internalSkippedDistance));
    destinationBuffer += sizeof(_internalSkippedDistance);
    memcpy(destinationBuffer, &_leavesSkippedDistance, sizeof(_leavesSkippedDistance));
    destinationBuffer += sizeof(_leavesSkippedDistance);
    memcpy(destinationBuffer, &_internalSkippedOutOfView, sizeof(_internalSkippedOutOfView));
    destinationBuffer += sizeof(_internalSkippedOutOfView);
    memcpy(destinationBuffer, &_leavesSkippedOutOfView, sizeof(_leavesSkippedOutOfView));
    destinationBuffer += sizeof(_leavesSkippedOutOfView);
    memcpy(destinationBuffer, &_internalSkippedWasInView, sizeof(_internalSkippedWasInView));
    destinationBuffer += sizeof(_internalSkippedWasInView);
    memcpy(destinationBuffer, &_leavesSkippedWasInView, sizeof(_leavesSkippedWasInView));
    destinationBuffer += sizeof(_leavesSkippedWasInView);
    memcpy(destinationBuffer, &_internalSkippedNoChange, sizeof(_internalSkippedNoChange));
    destinationBuffer += sizeof(_internalSkippedNoChange);
    memcpy(destinationBuffer, &_leavesSkippedNoChange, sizeof(_leavesSkippedNoChange));
    destinationBuffer += sizeof(_leavesSkippedNoChange);
    memcpy(destinationBuffer, &_internalSkippedOccluded, sizeof(_internalSkippedOccluded));
    destinationBuffer += sizeof(_internalSkippedOccluded);
    memcpy(destinationBuffer, &_leavesSkippedOccluded, sizeof(_leavesSkippedOccluded));
    destinationBuffer += sizeof(_leavesSkippedOccluded);
    memcpy(destinationBuffer, &_internalColorSent, sizeof(_internalColorSent));
    destinationBuffer += sizeof(_internalColorSent);
    memcpy(destinationBuffer, &_leavesColorSent, sizeof(_leavesColorSent));
    destinationBuffer += sizeof(_leavesColorSent);
    memcpy(destinationBuffer, &_internalDidntFit, sizeof(_internalDidntFit));
    destinationBuffer += sizeof(_internalDidntFit);
    memcpy(destinationBuffer, &_leavesDidntFit, sizeof(_leavesDidntFit));
    destinationBuffer += sizeof(_leavesDidntFit);
    memcpy(destinationBuffer, &_colorBitsWritten, sizeof(_colorBitsWritten));
    destinationBuffer += sizeof(_colorBitsWritten);
    memcpy(destinationBuffer, &_existsBitsWritten, sizeof(_existsBitsWritten));
    destinationBuffer += sizeof(_existsBitsWritten);
    memcpy(destinationBuffer, &_existsInPacketBitsWritten, sizeof(_existsInPacketBitsWritten));
    destinationBuffer += sizeof(_existsInPacketBitsWritten);
    memcpy(destinationBuffer, &_treesRemoved, sizeof(_treesRemoved));
    destinationBuffer += sizeof(_treesRemoved);
    
    return destinationBuffer - bufferStart; // includes header!
}

int VoxelSceneStats::unpackFromMessage(unsigned char* sourceBuffer, int availableBytes) {
    unsigned char* startPosition = sourceBuffer;

    // increment to push past the packet header
    int numBytesPacketHeader = numBytesForPacketHeader(sourceBuffer);
    sourceBuffer += numBytesPacketHeader;
    
    memcpy(&_start, sourceBuffer, sizeof(_start));
    sourceBuffer += sizeof(_start);
    memcpy(&_end, sourceBuffer, sizeof(_end));
    sourceBuffer += sizeof(_end);
    memcpy(&_elapsed, sourceBuffer, sizeof(_elapsed));
    sourceBuffer += sizeof(_elapsed);
    memcpy(&_totalEncodeTime, sourceBuffer, sizeof(_totalEncodeTime));
    sourceBuffer += sizeof(_totalEncodeTime);
    memcpy(&_isFullScene, sourceBuffer, sizeof(_isFullScene));
    sourceBuffer += sizeof(_isFullScene);
    memcpy(&_isMoving, sourceBuffer, sizeof(_isMoving));
    sourceBuffer += sizeof(_isMoving);
    memcpy(&_packets, sourceBuffer, sizeof(_packets));
    sourceBuffer += sizeof(_packets);
    memcpy(&_bytes, sourceBuffer, sizeof(_bytes));
    sourceBuffer += sizeof(_bytes);

    memcpy(&_totalInternal, sourceBuffer, sizeof(_totalInternal));
    sourceBuffer += sizeof(_totalInternal);
    memcpy(&_totalLeaves, sourceBuffer, sizeof(_totalLeaves));
    sourceBuffer += sizeof(_totalLeaves);
    _totalVoxels = _totalInternal + _totalLeaves;

    memcpy(&_internal, sourceBuffer, sizeof(_internal));
    sourceBuffer += sizeof(_internal);
    memcpy(&_leaves, sourceBuffer, sizeof(_leaves));
    sourceBuffer += sizeof(_leaves);
    _traversed = _internal + _leaves;
  
    memcpy(&_internalSkippedDistance, sourceBuffer, sizeof(_internalSkippedDistance));
    sourceBuffer += sizeof(_internalSkippedDistance);
    memcpy(&_leavesSkippedDistance, sourceBuffer, sizeof(_leavesSkippedDistance));
    sourceBuffer += sizeof(_leavesSkippedDistance);
    _skippedDistance = _internalSkippedDistance + _leavesSkippedDistance;
    
    memcpy(&_internalSkippedOutOfView, sourceBuffer, sizeof(_internalSkippedOutOfView));
    sourceBuffer += sizeof(_internalSkippedOutOfView);
    memcpy(&_leavesSkippedOutOfView, sourceBuffer, sizeof(_leavesSkippedOutOfView));
    sourceBuffer += sizeof(_leavesSkippedOutOfView);
    _skippedOutOfView = _internalSkippedOutOfView + _leavesSkippedOutOfView;

    memcpy(&_internalSkippedWasInView, sourceBuffer, sizeof(_internalSkippedWasInView));
    sourceBuffer += sizeof(_internalSkippedWasInView);
    memcpy(&_leavesSkippedWasInView, sourceBuffer, sizeof(_leavesSkippedWasInView));
    sourceBuffer += sizeof(_leavesSkippedWasInView);
    _skippedWasInView = _internalSkippedWasInView + _leavesSkippedWasInView;

    memcpy(&_internalSkippedNoChange, sourceBuffer, sizeof(_internalSkippedNoChange));
    sourceBuffer += sizeof(_internalSkippedNoChange);
    memcpy(&_leavesSkippedNoChange, sourceBuffer, sizeof(_leavesSkippedNoChange));
    sourceBuffer += sizeof(_leavesSkippedNoChange);
    _skippedNoChange = _internalSkippedNoChange + _leavesSkippedNoChange;

    memcpy(&_internalSkippedOccluded, sourceBuffer, sizeof(_internalSkippedOccluded));
    sourceBuffer += sizeof(_internalSkippedOccluded);
    memcpy(&_leavesSkippedOccluded, sourceBuffer, sizeof(_leavesSkippedOccluded));
    sourceBuffer += sizeof(_leavesSkippedOccluded);
    _skippedOccluded = _internalSkippedOccluded + _leavesSkippedOccluded;

    memcpy(&_internalColorSent, sourceBuffer, sizeof(_internalColorSent));
    sourceBuffer += sizeof(_internalColorSent);
    memcpy(&_leavesColorSent, sourceBuffer, sizeof(_leavesColorSent));
    sourceBuffer += sizeof(_leavesColorSent);
    _colorSent = _internalColorSent + _leavesColorSent;

    memcpy(&_internalDidntFit, sourceBuffer, sizeof(_internalDidntFit));
    sourceBuffer += sizeof(_internalDidntFit);
    memcpy(&_leavesDidntFit, sourceBuffer, sizeof(_leavesDidntFit));
    sourceBuffer += sizeof(_leavesDidntFit);
    _didntFit = _internalDidntFit + _leavesDidntFit;

    memcpy(&_colorBitsWritten, sourceBuffer, sizeof(_colorBitsWritten));
    sourceBuffer += sizeof(_colorBitsWritten);
    memcpy(&_existsBitsWritten, sourceBuffer, sizeof(_existsBitsWritten));
    sourceBuffer += sizeof(_existsBitsWritten);
    memcpy(&_existsInPacketBitsWritten, sourceBuffer, sizeof(_existsInPacketBitsWritten));
    sourceBuffer += sizeof(_existsInPacketBitsWritten);
    memcpy(&_treesRemoved, sourceBuffer, sizeof(_treesRemoved));
    sourceBuffer += sizeof(_treesRemoved);
    
    // running averages
    _elapsedAverage.updateAverage((float)_elapsed);
    unsigned long total = _existsInPacketBitsWritten + _colorSent;
    float calculatedBPV = total == 0 ? 0 : (_bytes * 8) / total;
    _bitsPerVoxelAverage.updateAverage(calculatedBPV);


    return sourceBuffer - startPosition; // includes header!
}


void VoxelSceneStats::printDebugDetails() {
    qDebug("\n------------------------------\n");
    qDebug("VoxelSceneStats:\n");
    qDebug("    start    : %llu \n", (long long unsigned int)_start);
    qDebug("    end      : %llu \n", (long long unsigned int)_end);
    qDebug("    elapsed  : %llu \n", (long long unsigned int)_elapsed);
    qDebug("    encoding : %llu \n", (long long unsigned int)_totalEncodeTime);
    qDebug("\n");
    qDebug("    full scene: %s\n", debug::valueOf(_isFullScene));
    qDebug("    moving: %s\n", debug::valueOf(_isMoving));
    qDebug("\n");
    qDebug("    packets: %d\n", _packets);
    qDebug("    bytes  : %ld\n", _bytes);
    qDebug("\n");
    qDebug("    total voxels        : %lu\n", _totalVoxels              );
    qDebug("        internal        : %lu\n", _totalInternal            );
    qDebug("        leaves          : %lu\n", _totalLeaves              );
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

    qDebug("\n");
    qDebug("    color sent          : %lu\n", _colorSent                );
    qDebug("        internal        : %lu\n", _internalColorSent        );
    qDebug("        leaves          : %lu\n", _leavesColorSent          );
    qDebug("    Didn't Fit          : %lu\n", _didntFit                 );
    qDebug("        internal        : %lu\n", _internalDidntFit         );
    qDebug("        leaves          : %lu\n", _leavesDidntFit           );
    qDebug("    color bits          : %lu\n", _colorBitsWritten         );
    qDebug("    exists bits         : %lu\n", _existsBitsWritten        );
    qDebug("    in packet bit       : %lu\n", _existsInPacketBitsWritten);
    qDebug("    trees removed       : %lu\n", _treesRemoved             );
}

const unsigned greenish  = 0x40ff40d0;
const unsigned yellowish = 0xffef40c0;
const unsigned greyish   = 0xd0d0d0a0;

VoxelSceneStats::ItemInfo VoxelSceneStats::_ITEMS[] = {
    { "Elapsed"              , greenish  },
    { "Encode"               , yellowish },
    { "Network"              , greyish   },
    { "Voxels on Server"     , greenish  },
    { "Voxels Sent"          , yellowish },
    { "Colors Sent"          , greyish   },
    { "Bitmasks Sent"        , greenish  },
    { "Traversed"            , yellowish },
    { "Skipped - Total"      , greyish   },
    { "Skipped - Distance"   , greenish  },
    { "Skipped - Out of View", yellowish },
    { "Skipped - Was in View", greyish   },
    { "Skipped - No Change"  , greenish  },
    { "Skipped - Occluded"   , yellowish },
    { "Didn't fit in packet" , greyish   },
    { "Mode"                 , greenish  },
};

char* VoxelSceneStats::getItemValue(int item) {
    const uint64_t USECS_PER_SECOND = 1000 * 1000;
    int calcFPS, calcAverageFPS, calculatedKBPS;
    switch(item) {
        case ITEM_ELAPSED: {
            calcFPS = (float)USECS_PER_SECOND / (float)_elapsed;
            float elapsedAverage = _elapsedAverage.getAverage();
            calcAverageFPS = (float)USECS_PER_SECOND / (float)elapsedAverage;

            sprintf(_itemValueBuffer, "%llu usecs (%d fps) Average: %.0f usecs (%d fps)", 
                    (long long unsigned int)_elapsed, calcFPS, elapsedAverage, calcAverageFPS);
            break;
        }
        case ITEM_ENCODE:
            calcFPS = (float)USECS_PER_SECOND / (float)_totalEncodeTime;
            sprintf(_itemValueBuffer, "%llu usecs (%d fps)", (long long unsigned int)_totalEncodeTime, calcFPS);
            break;
        case ITEM_PACKETS: {
            float elapsedSecs = ((float)_elapsed / (float)USECS_PER_SECOND);
            calculatedKBPS = elapsedSecs == 0 ? 0 : ((_bytes * 8) / elapsedSecs) / 1000;
            sprintf(_itemValueBuffer, "%d packets %lu bytes (%d kbps)", _packets, _bytes, calculatedKBPS);
            break;
        }
        case ITEM_VOXELS_SERVER: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _totalVoxels, _totalInternal, _totalLeaves);
            break;
        }
        case ITEM_VOXELS: {
            unsigned long total = _existsInPacketBitsWritten + _colorSent;
            float calculatedBPV = total == 0 ? 0 : (_bytes * 8) / total;
            float averageBPV = _bitsPerVoxelAverage.getAverage();
            sprintf(_itemValueBuffer, "%lu (%.2f bits/voxel Average: %.2f bits/voxel) %lu internal %lu leaves", 
                    total, calculatedBPV, averageBPV, _existsInPacketBitsWritten, _colorSent);
            break;
        }
        case ITEM_TRAVERSED: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _traversed, _internal, _leaves);
            break;
        }
        case ITEM_SKIPPED: {
            unsigned long total    = _skippedDistance + _skippedOutOfView + 
                                     _skippedWasInView + _skippedNoChange + _skippedOccluded;
                                     
            unsigned long internal = _internalSkippedDistance + _internalSkippedOutOfView + 
                                     _internalSkippedWasInView + _internalSkippedNoChange + _internalSkippedOccluded;
                                     
            unsigned long leaves   = _leavesSkippedDistance + _leavesSkippedOutOfView + 
                                     _leavesSkippedWasInView + _leavesSkippedNoChange + _leavesSkippedOccluded;

            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    total, internal, leaves);
            break;
        }
        case ITEM_SKIPPED_DISTANCE: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _skippedDistance, _internalSkippedDistance, _leavesSkippedDistance);
            break;
        }
        case ITEM_SKIPPED_OUT_OF_VIEW: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _skippedOutOfView, _internalSkippedOutOfView, _leavesSkippedOutOfView);
            break;
        }
        case ITEM_SKIPPED_WAS_IN_VIEW: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _skippedWasInView, _internalSkippedWasInView, _leavesSkippedWasInView);
            break;
        }
        case ITEM_SKIPPED_NO_CHANGE: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _skippedNoChange, _internalSkippedNoChange, _leavesSkippedNoChange);
            break;
        }
        case ITEM_SKIPPED_OCCLUDED: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _skippedOccluded, _internalSkippedOccluded, _leavesSkippedOccluded);
            break;
        }
        case ITEM_COLORS: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves", 
                    _colorSent, _internalColorSent, _leavesColorSent);
            break;
        }
        case ITEM_DIDNT_FIT: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves (removed: %lu)", 
                    _didntFit, _internalDidntFit, _leavesDidntFit, _treesRemoved);
            break;
        }
        case ITEM_BITS: {
            sprintf(_itemValueBuffer, "colors: %lu, exists: %lu, in packets: %lu", 
                    _colorBitsWritten, _existsBitsWritten, _existsInPacketBitsWritten);
            break;
        }
        case ITEM_MODE: {
            sprintf(_itemValueBuffer, "%s - %s", (_isFullScene ? "Full Scene" : "Partial Scene"), 
                    (_isMoving ? "Moving" : "Stationary"));
            break;
        }
        default:
            sprintf(_itemValueBuffer, "");
            break;
    }
    return _itemValueBuffer;
}

