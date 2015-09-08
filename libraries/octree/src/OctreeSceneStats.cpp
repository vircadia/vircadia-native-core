//
//  OctreeSceneStats.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>
#include <QString>
#include <QStringList>

#include <LogHandler.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>

#include "OctreePacketData.h"
#include "OctreeElement.h"
#include "OctreeLogging.h"
#include "OctreeSceneStats.h"


const int samples = 100;
OctreeSceneStats::OctreeSceneStats() :
    _isReadyToSend(false),
    _isStarted(false),
    _lastFullElapsed(0),
    _lastFullTotalEncodeTime(0),
    _lastFullTotalPackets(0),
    _lastFullTotalBytes(0),
    _elapsedAverage(samples),
    _bitsPerOctreeAverage(samples),
    _incomingPacket(0),
    _incomingBytes(0),
    _incomingWastedBytes(0),
    _incomingOctreeSequenceNumberStats(),
    _incomingFlightTimeAverage(samples),
    _jurisdictionRoot(NULL)
{
    reset();
}

// copy constructor
OctreeSceneStats::OctreeSceneStats(const OctreeSceneStats& other) :
_jurisdictionRoot(NULL) {
    copyFromOther(other);
}

// copy assignment
OctreeSceneStats& OctreeSceneStats::operator=(const OctreeSceneStats& other) {
    copyFromOther(other);
    return *this;
}

void OctreeSceneStats::copyFromOther(const OctreeSceneStats& other) {
    _totalEncodeTime = other._totalEncodeTime;
    _elapsed = other._elapsed;
    _lastFullElapsed = other._lastFullElapsed;
    _lastFullTotalEncodeTime = other._lastFullTotalEncodeTime;
    _lastFullTotalPackets = other._lastFullTotalPackets;
    _lastFullTotalBytes = other._lastFullTotalBytes;
    _encodeStart = other._encodeStart;

    _packets = other._packets;
    _bytes = other._bytes;
    _passes = other._passes;

    _totalElements = other._totalElements;
    _totalInternal = other._totalInternal;
    _totalLeaves = other._totalLeaves;

    _traversed = other._traversed;
    _internal = other._internal;
    _leaves = other._leaves;

    _skippedDistance = other._skippedDistance;
    _internalSkippedDistance = other._internalSkippedDistance;
    _leavesSkippedDistance = other._leavesSkippedDistance;

    _skippedOutOfView = other._skippedOutOfView;
    _internalSkippedOutOfView = other._internalSkippedOutOfView;
    _leavesSkippedOutOfView = other._leavesSkippedOutOfView;

    _skippedWasInView = other._skippedWasInView;
    _internalSkippedWasInView = other._internalSkippedWasInView;
    _leavesSkippedWasInView = other._leavesSkippedWasInView;

    _skippedNoChange = other._skippedNoChange;
    _internalSkippedNoChange = other._internalSkippedNoChange;
    _leavesSkippedNoChange = other._leavesSkippedNoChange;

    _skippedOccluded = other._skippedOccluded;
    _internalSkippedOccluded = other._internalSkippedOccluded;
    _leavesSkippedOccluded = other._leavesSkippedOccluded;

    _colorSent = other._colorSent;
    _internalColorSent = other._internalColorSent;
    _leavesColorSent = other._leavesColorSent;

    _didntFit = other._didntFit;
    _internalDidntFit = other._internalDidntFit;
    _leavesDidntFit = other._leavesDidntFit;

    _colorBitsWritten = other._colorBitsWritten;
    _existsBitsWritten = other._existsBitsWritten;
    _existsInPacketBitsWritten = other._existsInPacketBitsWritten;
    _treesRemoved = other._treesRemoved;

    // before copying the jurisdictions, delete any current values...
    if (_jurisdictionRoot) {
        delete[] _jurisdictionRoot;
        _jurisdictionRoot = NULL;
    }
    for (size_t i = 0; i < _jurisdictionEndNodes.size(); i++) {
        if (_jurisdictionEndNodes[i]) {
            delete[] _jurisdictionEndNodes[i];
        }
    }
    _jurisdictionEndNodes.clear();

    // Now copy the values from the other
    if (other._jurisdictionRoot) {
        int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(other._jurisdictionRoot));
        _jurisdictionRoot = new unsigned char[bytes];
        memcpy(_jurisdictionRoot, other._jurisdictionRoot, bytes);
    }
    for (size_t i = 0; i < other._jurisdictionEndNodes.size(); i++) {
        unsigned char* endNodeCode = other._jurisdictionEndNodes[i];
        if (endNodeCode) {
            int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(endNodeCode));
            unsigned char* endNodeCodeCopy = new unsigned char[bytes];
            memcpy(endNodeCodeCopy, endNodeCode, bytes);
            _jurisdictionEndNodes.push_back(endNodeCodeCopy);
        }
    }

    _incomingPacket = other._incomingPacket;
    _incomingBytes = other._incomingBytes;
    _incomingWastedBytes = other._incomingWastedBytes;

    _incomingOctreeSequenceNumberStats = other._incomingOctreeSequenceNumberStats;
}


OctreeSceneStats::~OctreeSceneStats() {
    reset();
}

void OctreeSceneStats::sceneStarted(bool isFullScene, bool isMoving, OctreeElement* root, JurisdictionMap* jurisdictionMap) {
    reset(); // resets packet and octree stats
    _isStarted = true;
    _start = usecTimestampNow();

    _totalElements = OctreeElement::getNodeCount();
    _totalInternal = OctreeElement::getInternalNodeCount();
    _totalLeaves   = OctreeElement::getLeafNodeCount();

    _isFullScene = isFullScene;
    _isMoving = isMoving;

    if (_jurisdictionRoot) {
        delete[] _jurisdictionRoot;
        _jurisdictionRoot = NULL;
    }
    // clear existing endNodes before copying new ones...
    for (size_t i=0; i < _jurisdictionEndNodes.size(); i++) {
        if (_jurisdictionEndNodes[i]) {
            delete[] _jurisdictionEndNodes[i];
        }
    }
    _jurisdictionEndNodes.clear();

    // setup jurisdictions
    if (jurisdictionMap) {
        unsigned char* jurisdictionRoot = jurisdictionMap->getRootOctalCode();
        if (jurisdictionRoot) {
            int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(jurisdictionRoot));
            _jurisdictionRoot = new unsigned char[bytes];
            memcpy(_jurisdictionRoot, jurisdictionRoot, bytes);
        }

        // copy new endNodes...
        for (int i = 0; i < jurisdictionMap->getEndNodeCount(); i++) {
            unsigned char* endNodeCode = jurisdictionMap->getEndNodeOctalCode(i);
            if (endNodeCode) {
                int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(endNodeCode));
                unsigned char* endNodeCodeCopy = new unsigned char[bytes];
                memcpy(endNodeCodeCopy, endNodeCode, bytes);
                _jurisdictionEndNodes.push_back(endNodeCodeCopy);
            }
        }
    }
}

void OctreeSceneStats::sceneCompleted() {
    if (_isStarted) {
        _end = usecTimestampNow();
        _elapsed = _end - _start;
        _elapsedAverage.updateAverage((float)_elapsed);

        if (_isFullScene) {
            _lastFullElapsed = _elapsed;
            _lastFullTotalEncodeTime = _totalEncodeTime;
        }

        packIntoPacket();
        _isReadyToSend = true;
        _isStarted = false;
    }
}

void OctreeSceneStats::encodeStarted() {
    _encodeStart = usecTimestampNow();
}

void OctreeSceneStats::encodeStopped() {
    _totalEncodeTime += (usecTimestampNow() - _encodeStart);
}

void OctreeSceneStats::reset() {
    _totalEncodeTime = 0;
    _encodeStart = 0;

    _packets = 0;
    _bytes = 0;
    _passes = 0;

    _totalElements = 0;
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

    if (_jurisdictionRoot) {
        delete[] _jurisdictionRoot;
        _jurisdictionRoot = NULL;
    }
    for (size_t i = 0; i < _jurisdictionEndNodes.size(); i++) {
        if (_jurisdictionEndNodes[i]) {
            delete[] _jurisdictionEndNodes[i];
        }
    }
    _jurisdictionEndNodes.clear();
}

void OctreeSceneStats::packetSent(int bytes) {
    _packets++;
    _bytes += bytes;
}

void OctreeSceneStats::traversed(const OctreeElement* element) {
    _traversed++;
    if (element->isLeaf()) {
        _leaves++;
    } else {
        _internal++;
    }
}

void OctreeSceneStats::skippedDistance(const OctreeElement* element) {
    _skippedDistance++;
    if (element->isLeaf()) {
        _leavesSkippedDistance++;
    } else {
        _internalSkippedDistance++;
    }
}

void OctreeSceneStats::skippedOutOfView(const OctreeElement* element) {
    _skippedOutOfView++;
    if (element->isLeaf()) {
        _leavesSkippedOutOfView++;
    } else {
        _internalSkippedOutOfView++;
    }
}

void OctreeSceneStats::skippedWasInView(const OctreeElement* element) {
    _skippedWasInView++;
    if (element->isLeaf()) {
        _leavesSkippedWasInView++;
    } else {
        _internalSkippedWasInView++;
    }
}

void OctreeSceneStats::skippedNoChange(const OctreeElement* element) {
    _skippedNoChange++;
    if (element->isLeaf()) {
        _leavesSkippedNoChange++;
    } else {
        _internalSkippedNoChange++;
    }
}

void OctreeSceneStats::skippedOccluded(const OctreeElement* element) {
    _skippedOccluded++;
    if (element->isLeaf()) {
        _leavesSkippedOccluded++;
    } else {
        _internalSkippedOccluded++;
    }
}

void OctreeSceneStats::colorSent(const OctreeElement* element) {
    _colorSent++;
    if (element->isLeaf()) {
        _leavesColorSent++;
    } else {
        _internalColorSent++;
    }
}

void OctreeSceneStats::didntFit(const OctreeElement* element) {
    _didntFit++;
    if (element->isLeaf()) {
        _leavesDidntFit++;
    } else {
        _internalDidntFit++;
    }
}

void OctreeSceneStats::colorBitsWritten() {
    _colorBitsWritten++;
}

void OctreeSceneStats::existsBitsWritten() {
    _existsBitsWritten++;
}

void OctreeSceneStats::existsInPacketBitsWritten() {
    _existsInPacketBitsWritten++;
}

void OctreeSceneStats::childBitsRemoved(bool includesExistsBits, bool includesColors) {
    _existsInPacketBitsWritten--;
    if (includesExistsBits) {
        _existsBitsWritten--;
    }
    if (includesColors) {
        _colorBitsWritten--;
    }
    _treesRemoved++;
}

int OctreeSceneStats::packIntoPacket() {
    _statsPacket->reset();

    _statsPacket->writePrimitive(_start);
    _statsPacket->writePrimitive(_end);
    _statsPacket->writePrimitive(_elapsed);
    _statsPacket->writePrimitive(_totalEncodeTime);
    _statsPacket->writePrimitive(_isFullScene);
    _statsPacket->writePrimitive(_isMoving);
    _statsPacket->writePrimitive(_packets);
    _statsPacket->writePrimitive(_bytes);

    _statsPacket->writePrimitive(_totalInternal);
    _statsPacket->writePrimitive(_totalLeaves);
    _statsPacket->writePrimitive(_internal);
    _statsPacket->writePrimitive(_leaves);
    _statsPacket->writePrimitive(_internalSkippedDistance);
    _statsPacket->writePrimitive(_leavesSkippedDistance);
    _statsPacket->writePrimitive(_internalSkippedOutOfView);
    _statsPacket->writePrimitive(_leavesSkippedOutOfView);
    _statsPacket->writePrimitive(_internalSkippedWasInView);
    _statsPacket->writePrimitive(_leavesSkippedWasInView);
    _statsPacket->writePrimitive(_internalSkippedNoChange);
    _statsPacket->writePrimitive(_leavesSkippedNoChange);
    _statsPacket->writePrimitive(_internalSkippedOccluded);
    _statsPacket->writePrimitive(_leavesSkippedOccluded);
    _statsPacket->writePrimitive(_internalColorSent);
    _statsPacket->writePrimitive(_leavesColorSent);
    _statsPacket->writePrimitive(_internalDidntFit);
    _statsPacket->writePrimitive(_leavesDidntFit);
    _statsPacket->writePrimitive(_colorBitsWritten);
    _statsPacket->writePrimitive(_existsBitsWritten);
    _statsPacket->writePrimitive(_existsInPacketBitsWritten);
    _statsPacket->writePrimitive(_treesRemoved);

    // add the root jurisdiction
    if (_jurisdictionRoot) {
        // copy the
        int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(_jurisdictionRoot));
        _statsPacket->writePrimitive(bytes);
        _statsPacket->write(reinterpret_cast<char*>(_jurisdictionRoot), bytes);

        // if and only if there's a root jurisdiction, also include the end elements
        int endNodeCount = _jurisdictionEndNodes.size();

        _statsPacket->writePrimitive(endNodeCount);

        for (int i=0; i < endNodeCount; i++) {
            unsigned char* endNodeCode = _jurisdictionEndNodes[i];
            int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(endNodeCode));
            _statsPacket->writePrimitive(bytes);
            _statsPacket->write(reinterpret_cast<char*>(endNodeCode), bytes);
        }
    } else {
        int bytes = 0;
        _statsPacket->writePrimitive(bytes);
    }

    return _statsPacket->getPayloadSize();
}

int OctreeSceneStats::unpackFromPacket(NLPacket& packet) {
    packet.readPrimitive(&_start);
    packet.readPrimitive(&_end);
    packet.readPrimitive(&_elapsed);
    packet.readPrimitive(&_totalEncodeTime);

    packet.readPrimitive(&_isFullScene);
    packet.readPrimitive(&_isMoving);
    packet.readPrimitive(&_packets);
    packet.readPrimitive(&_bytes);

    if (_isFullScene) {
        _lastFullElapsed = _elapsed;
        _lastFullTotalEncodeTime = _totalEncodeTime;
        _lastFullTotalPackets = _packets;
        _lastFullTotalBytes = _bytes;
    }

    packet.readPrimitive(&_totalInternal);
    packet.readPrimitive(&_totalLeaves);
    _totalElements = _totalInternal + _totalLeaves;

    packet.readPrimitive(&_internal);
    packet.readPrimitive(&_leaves);
    _traversed = _internal + _leaves;

    packet.readPrimitive(&_internalSkippedDistance);
    packet.readPrimitive(&_leavesSkippedDistance);
    _skippedDistance = _internalSkippedDistance + _leavesSkippedDistance;

    packet.readPrimitive(&_internalSkippedOutOfView);
    packet.readPrimitive(&_leavesSkippedOutOfView);
    _skippedOutOfView = _internalSkippedOutOfView + _leavesSkippedOutOfView;

    packet.readPrimitive(&_internalSkippedWasInView);
    packet.readPrimitive(&_leavesSkippedWasInView);
    _skippedWasInView = _internalSkippedWasInView + _leavesSkippedWasInView;

    packet.readPrimitive(&_internalSkippedNoChange);
    packet.readPrimitive(&_leavesSkippedNoChange);
    _skippedNoChange = _internalSkippedNoChange + _leavesSkippedNoChange;

    packet.readPrimitive(&_internalSkippedOccluded);
    packet.readPrimitive(&_leavesSkippedOccluded);
    _skippedOccluded = _internalSkippedOccluded + _leavesSkippedOccluded;

    packet.readPrimitive(&_internalColorSent);
    packet.readPrimitive(&_leavesColorSent);
    _colorSent = _internalColorSent + _leavesColorSent;

    packet.readPrimitive(&_internalDidntFit);
    packet.readPrimitive(&_leavesDidntFit);
    _didntFit = _internalDidntFit + _leavesDidntFit;

    packet.readPrimitive(&_colorBitsWritten);
    packet.readPrimitive(&_existsBitsWritten);
    packet.readPrimitive(&_existsInPacketBitsWritten);
    packet.readPrimitive(&_treesRemoved);
    // before allocating new juridiction, clean up existing ones
    if (_jurisdictionRoot) {
        delete[] _jurisdictionRoot;
        _jurisdictionRoot = NULL;
    }
    
    // clear existing endNodes before copying new ones...
    for (size_t i = 0; i < _jurisdictionEndNodes.size(); i++) {
        if (_jurisdictionEndNodes[i]) {
            delete[] _jurisdictionEndNodes[i];
        }
    }
    _jurisdictionEndNodes.clear();

    // read the root jurisdiction
    int bytes = 0;
    packet.readPrimitive(&bytes);

    if (bytes == 0) {
        _jurisdictionRoot = NULL;
        _jurisdictionEndNodes.clear();
    } else {
        _jurisdictionRoot = new unsigned char[bytes];
        packet.read(reinterpret_cast<char*>(_jurisdictionRoot), bytes);

        // if and only if there's a root jurisdiction, also include the end elements
        _jurisdictionEndNodes.clear();
        
        int endNodeCount = 0;
        packet.readPrimitive(&endNodeCount);
        
        for (int i=0; i < endNodeCount; i++) {
            int bytes = 0;
            
            packet.readPrimitive(&bytes);
            
            unsigned char* endNodeCode = new unsigned char[bytes];
            packet.read(reinterpret_cast<char*>(endNodeCode), bytes);
            
            _jurisdictionEndNodes.push_back(endNodeCode);
        }
    }

    // running averages
    _elapsedAverage.updateAverage((float)_elapsed);
    unsigned long total = _existsInPacketBitsWritten + _colorSent;
    float calculatedBPV = total == 0 ? 0 : (_bytes * 8) / total;
    _bitsPerOctreeAverage.updateAverage(calculatedBPV);

    return packet.pos(); // excludes header!
}


void OctreeSceneStats::printDebugDetails() {
    qCDebug(octree) << "\n------------------------------";
    qCDebug(octree) << "OctreeSceneStats:";
    qCDebug(octree) << "start: " << _start;
    qCDebug(octree) << "end: " << _end;
    qCDebug(octree) << "elapsed: " << _elapsed;
    qCDebug(octree) << "encoding: " << _totalEncodeTime;
    qCDebug(octree);
    qCDebug(octree) << "full scene: " << debug::valueOf(_isFullScene);
    qCDebug(octree) << "moving: " << debug::valueOf(_isMoving);
    qCDebug(octree);
    qCDebug(octree) << "packets: " << _packets;
    qCDebug(octree) << "bytes: " << _bytes;
    qCDebug(octree);
    qCDebug(octree) << "total elements: " << _totalElements;
    qCDebug(octree) << "internal: " << _totalInternal;
    qCDebug(octree) << "leaves: " << _totalLeaves;
    qCDebug(octree) << "traversed: " << _traversed;
    qCDebug(octree) << "internal: " << _internal;
    qCDebug(octree) << "leaves: " << _leaves;
    qCDebug(octree) << "skipped distance: " << _skippedDistance;
    qCDebug(octree) << "internal: " << _internalSkippedDistance;
    qCDebug(octree) << "leaves: " << _leavesSkippedDistance;
    qCDebug(octree) << "skipped out of view: " << _skippedOutOfView;
    qCDebug(octree) << "internal: " << _internalSkippedOutOfView;
    qCDebug(octree) << "leaves: " << _leavesSkippedOutOfView;
    qCDebug(octree) << "skipped was in view: " << _skippedWasInView;
    qCDebug(octree) << "internal: " << _internalSkippedWasInView;
    qCDebug(octree) << "leaves: " << _leavesSkippedWasInView;
    qCDebug(octree) << "skipped no change: " << _skippedNoChange;
    qCDebug(octree) << "internal: " << _internalSkippedNoChange;
    qCDebug(octree) << "leaves: " << _leavesSkippedNoChange;
    qCDebug(octree) << "skipped occluded: " << _skippedOccluded;
    qCDebug(octree) << "internal: " << _internalSkippedOccluded;
    qCDebug(octree) << "leaves: " << _leavesSkippedOccluded;
    qCDebug(octree);
    qCDebug(octree) << "color sent: " << _colorSent;
    qCDebug(octree) << "internal: " << _internalColorSent;
    qCDebug(octree) << "leaves: " << _leavesColorSent;
    qCDebug(octree) << "Didn't Fit: " << _didntFit;
    qCDebug(octree) << "internal: " << _internalDidntFit;
    qCDebug(octree) << "leaves: " << _leavesDidntFit;
    qCDebug(octree) << "color bits: " << _colorBitsWritten;
    qCDebug(octree) << "exists bits: " << _existsBitsWritten;
    qCDebug(octree) << "in packet bit: " << _existsInPacketBitsWritten;
    qCDebug(octree) << "trees removed: " << _treesRemoved;
}

OctreeSceneStats::ItemInfo OctreeSceneStats::_ITEMS[] = {
    { "Elapsed", GREENISH, 2, "Elapsed,fps" },
    { "Encode", YELLOWISH, 2, "Time,fps" },
    { "Network", GREYISH, 3, "Packets,Bytes,KBPS" },
    { "Octrees on Server", GREENISH, 3, "Total,Internal,Leaves" },
    { "Octrees Sent", YELLOWISH, 5, "Total,Bits/Octree,Avg Bits/Octree,Internal,Leaves" },
    { "Colors Sent", GREYISH, 3, "Total,Internal,Leaves" },
    { "Bitmasks Sent", GREENISH, 3, "Colors,Exists,In Packets" },
    { "Traversed", YELLOWISH, 3, "Total,Internal,Leaves" },
    { "Skipped - Total", GREYISH, 3, "Total,Internal,Leaves" },
    { "Skipped - Distance", GREENISH, 3, "Total,Internal,Leaves" },
    { "Skipped - Out of View", YELLOWISH, 3, "Total,Internal,Leaves" },
    { "Skipped - Was in View", GREYISH, 3, "Total,Internal,Leaves" },
    { "Skipped - No Change", GREENISH, 3, "Total,Internal,Leaves" },
    { "Skipped - Occluded", YELLOWISH, 3, "Total,Internal,Leaves" },
    { "Didn't fit in packet", GREYISH, 4, "Total,Internal,Leaves,Removed" },
    { "Mode", GREENISH, 4, "Moving,Stationary,Partial,Full" },
};

const char* OctreeSceneStats::getItemValue(Item item) {
    const quint64 USECS_PER_SECOND = 1000 * 1000;
    int calcFPS, calcAverageFPS, calculatedKBPS;
    switch(item) {
        case ITEM_ELAPSED: {
            calcFPS = (float)USECS_PER_SECOND / (float)_elapsed;
            float elapsedAverage = _elapsedAverage.getAverage();
            calcAverageFPS = (float)USECS_PER_SECOND / (float)elapsedAverage;

            sprintf(_itemValueBuffer, "%llu usecs (%d fps) Average: %.0f usecs (%d fps)",
                    (long long unsigned int)_elapsed, calcFPS, (double)elapsedAverage, calcAverageFPS);
            break;
        }
        case ITEM_ENCODE:
            calcFPS = (float)USECS_PER_SECOND / (float)_totalEncodeTime;
            sprintf(_itemValueBuffer, "%llu usecs (%d fps)", (long long unsigned int)_totalEncodeTime, calcFPS);
            break;
        case ITEM_PACKETS: {
            float elapsedSecs = ((float)_elapsed / (float)USECS_PER_SECOND);
            calculatedKBPS = elapsedSecs == 0 ? 0 : ((_bytes * 8) / elapsedSecs) / 1000;
            sprintf(_itemValueBuffer, "%d packets %lu bytes (%d kbps)", _packets, (long unsigned int)_bytes, calculatedKBPS);
            break;
        }
        case ITEM_VOXELS_SERVER: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_totalElements,
                    (long unsigned int)_totalInternal,
                    (long unsigned int)_totalLeaves);
            break;
        }
        case ITEM_VOXELS: {
            unsigned long total = _existsInPacketBitsWritten + _colorSent;
            float calculatedBPV = total == 0 ? 0 : (_bytes * 8) / total;
            float averageBPV = _bitsPerOctreeAverage.getAverage();
            sprintf(_itemValueBuffer, "%lu (%.2f bits/octree Average: %.2f bits/octree) %lu internal %lu leaves",
                    total, (double)calculatedBPV, (double)averageBPV,
                    (long unsigned int)_existsInPacketBitsWritten,
                    (long unsigned int)_colorSent);
            break;
        }
        case ITEM_TRAVERSED: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_traversed, (long unsigned int)_internal, (long unsigned int)_leaves);
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
                    (long unsigned int)_skippedDistance,
                    (long unsigned int)_internalSkippedDistance,
                    (long unsigned int)_leavesSkippedDistance);
            break;
        }
        case ITEM_SKIPPED_OUT_OF_VIEW: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_skippedOutOfView,
                    (long unsigned int)_internalSkippedOutOfView,
                    (long unsigned int)_leavesSkippedOutOfView);
            break;
        }
        case ITEM_SKIPPED_WAS_IN_VIEW: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_skippedWasInView,
                    (long unsigned int)_internalSkippedWasInView,
                    (long unsigned int)_leavesSkippedWasInView);
            break;
        }
        case ITEM_SKIPPED_NO_CHANGE: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_skippedNoChange,
                    (long unsigned int)_internalSkippedNoChange,
                    (long unsigned int)_leavesSkippedNoChange);
            break;
        }
        case ITEM_SKIPPED_OCCLUDED: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_skippedOccluded,
                    (long unsigned int)_internalSkippedOccluded,
                    (long unsigned int)_leavesSkippedOccluded);
            break;
        }
        case ITEM_COLORS: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves",
                    (long unsigned int)_colorSent,
                    (long unsigned int)_internalColorSent,
                    (long unsigned int)_leavesColorSent);
            break;
        }
        case ITEM_DIDNT_FIT: {
            sprintf(_itemValueBuffer, "%lu total %lu internal %lu leaves (removed: %lu)",
                    (long unsigned int)_didntFit,
                    (long unsigned int)_internalDidntFit,
                    (long unsigned int)_leavesDidntFit,
                    (long unsigned int)_treesRemoved);
            break;
        }
        case ITEM_BITS: {
            sprintf(_itemValueBuffer, "colors: %lu, exists: %lu, in packets: %lu",
                    (long unsigned int)_colorBitsWritten,
                    (long unsigned int)_existsBitsWritten,
                    (long unsigned int)_existsInPacketBitsWritten);
            break;
        }
        case ITEM_MODE: {
            sprintf(_itemValueBuffer, "%s - %s", (_isFullScene ? "Full Scene" : "Partial Scene"),
                    (_isMoving ? "Moving" : "Stationary"));
            break;
        }
        default:
            break;
    }
    return _itemValueBuffer;
}

void OctreeSceneStats::trackIncomingOctreePacket(NLPacket& packet, bool wasStatsPacket, int nodeClockSkewUsec) {
    const bool wantExtraDebugging = false;

    // skip past the flags
    packet.seek(sizeof(OCTREE_PACKET_FLAGS));
    
    OCTREE_PACKET_SEQUENCE sequence;
    packet.readPrimitive(&sequence);

    OCTREE_PACKET_SENT_TIME sentAt;
    packet.readPrimitive(&sentAt);

    //bool packetIsColored = oneAtBit(flags, PACKET_IS_COLOR_BIT);
    //bool packetIsCompressed = oneAtBit(flags, PACKET_IS_COMPRESSED_BIT);

    OCTREE_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
    qint64 flightTime = arrivedAt - sentAt + nodeClockSkewUsec;

    if (wantExtraDebugging) {
        qCDebug(octree) << "sentAt:" << sentAt << " usecs";
        qCDebug(octree) << "arrivedAt:" << arrivedAt << " usecs";
        qCDebug(octree) << "nodeClockSkewUsec:" << nodeClockSkewUsec << " usecs";
        qCDebug(octree) << "flightTime:" << flightTime << " usecs";
    }

    // Guard against possible corrupted packets... with bad timestamps
    const int MAX_RESONABLE_FLIGHT_TIME = 200 * USECS_PER_SECOND; // 200 seconds is more than enough time for a packet to arrive
    const int MIN_RESONABLE_FLIGHT_TIME = 0;
    if (flightTime > MAX_RESONABLE_FLIGHT_TIME || flightTime < MIN_RESONABLE_FLIGHT_TIME) {
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(
                    "ignoring unreasonable packet... flightTime: -?\\d+ nodeClockSkewUsec: -?\\d+ usecs");

        qCDebug(octree) << "ignoring unreasonable packet... flightTime:" << flightTime
                    << "nodeClockSkewUsec:" << nodeClockSkewUsec << "usecs";;
        return; // ignore any packets that are unreasonable
    }

    _incomingOctreeSequenceNumberStats.sequenceNumberReceived(sequence);

    // track packets here...
    _incomingPacket++;
    _incomingBytes += packet.getDataSize();
    if (!wasStatsPacket) {
        _incomingWastedBytes += (udt::MAX_PACKET_SIZE - packet.getDataSize());
    }
}
