//
//  OctreeRenderer.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>
#include <stdint.h>

#include <NumericalConstants.h>
#include <PerfStat.h>
#include <RenderArgs.h>
#include <SharedUtil.h>

#include "OctreeLogging.h"
#include "OctreeRenderer.h"

OctreeRenderer::OctreeRenderer() :
    _tree(NULL),
    _managedTree(false),
    _viewFrustum(NULL)
{
}

void OctreeRenderer::init() {
    if (!_tree) {
        _tree = createTree();
        _managedTree = true;
    }
}

OctreeRenderer::~OctreeRenderer() {
    if (_tree && _managedTree) {
        delete _tree;
    }
}

void OctreeRenderer::setTree(Octree* newTree) {
    if (_tree && _managedTree) {
        delete _tree;
        _managedTree = false;
    }
    _tree = newTree;
}

void OctreeRenderer::processDatagram(NLPacket& packet, SharedNodePointer sourceNode) {
    bool extraDebugging = false;
    
    if (extraDebugging) {
        qCDebug(octree) << "OctreeRenderer::processDatagram()";
    }

    if (!_tree) {
        qCDebug(octree) << "OctreeRenderer::processDatagram() called before init, calling init()...";
        this->init();
    }

    bool showTimingDetails = false; // Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showTimingDetails, "OctreeRenderer::processDatagram()", showTimingDetails);
    
    if (packet.getType() == getExpectedPacketType()) {
        PerformanceWarning warn(showTimingDetails, "OctreeRenderer::processDatagram expected PacketType", showTimingDetails);
        // if we are getting inbound packets, then our tree is also viewing, and we should remember that fact.
        _tree->setIsViewing(true);
        
        OCTREE_PACKET_FLAGS flags;
        packet.readPrimitive(&flags);
        
        OCTREE_PACKET_SEQUENCE sequence;
        packet.readPrimitive(&sequence);

        OCTREE_PACKET_SENT_TIME sentAt;
        packet.readPrimitive(&sentAt);

        bool packetIsColored = oneAtBit(flags, PACKET_IS_COLOR_BIT);
        bool packetIsCompressed = oneAtBit(flags, PACKET_IS_COMPRESSED_BIT);
        
        OCTREE_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
        int clockSkew = sourceNode ? sourceNode->getClockSkewUsec() : 0;
        int flightTime = arrivedAt - sentAt + clockSkew;

        OCTREE_PACKET_INTERNAL_SECTION_SIZE sectionLength = 0;

        if (extraDebugging) {
            qCDebug(octree, "OctreeRenderer::processDatagram() ... Got Packet Section"
                   " color:%s compressed:%s sequence: %u flight:%d usec size:%lld data:%lld",
                   debug::valueOf(packetIsColored), debug::valueOf(packetIsCompressed),
                   sequence, flightTime, packet.getDataSize(), packet.bytesLeftToRead());
        }
        
        _packetsInLastWindow++;
        
        int elementsPerPacket = 0;
        int entitiesPerPacket = 0;
        
        quint64 totalWaitingForLock = 0;
        quint64 totalUncompress = 0;
        quint64 totalReadBitsteam = 0;

        const QUuid& sourceUUID = packet.getSourceID();
        
        int subsection = 1;
        
        bool error = false;
        
        while (packet.bytesLeftToRead() > 0 && !error) {
            if (packetIsCompressed) {
                if (packet.bytesLeftToRead() > (qint64) sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE)) {
                    packet.readPrimitive(&sectionLength);
                } else {
                    sectionLength = 0;
                    error = true;
                }
            } else {
                sectionLength = packet.bytesLeftToRead();
            }
            
            if (sectionLength) {
                // ask the VoxelTree to read the bitstream into the tree
                ReadBitstreamToTreeParams args(packetIsColored ? WANT_COLOR : NO_COLOR, WANT_EXISTS_BITS, NULL,
                                                sourceUUID, sourceNode, false, packet.getVersion());
                quint64 startLock = usecTimestampNow();

                // FIXME STUTTER - there may be an opportunity to bump this lock outside of the
                // loop to reduce the amount of locking/unlocking we're doing
                _tree->lockForWrite();
                quint64 startUncompress = usecTimestampNow();
                
                OctreePacketData packetData(packetIsCompressed);
                packetData.loadFinalizedContent(reinterpret_cast<unsigned char*>(packet.getPayload() + packet.pos()),
                                                sectionLength);
                if (extraDebugging) {
                    qCDebug(octree, "OctreeRenderer::processDatagram() ... Got Packet Section"
                           " color:%s compressed:%s sequence: %u flight:%d usec size:%lld data:%lld"
                           " subsection:%d sectionLength:%d uncompressed:%d",
                           debug::valueOf(packetIsColored), debug::valueOf(packetIsCompressed),
                           sequence, flightTime, packet.getDataSize(), packet.bytesLeftToRead(), subsection, sectionLength,
                           packetData.getUncompressedSize());
                }
                
                if (extraDebugging) {
                    qCDebug(octree) << "OctreeRenderer::processDatagram() ******* START _tree->readBitstreamToTree()...";
                }
                quint64 startReadBitsteam = usecTimestampNow();
                _tree->readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
                quint64 endReadBitsteam = usecTimestampNow();
                if (extraDebugging) {
                    qCDebug(octree) << "OctreeRenderer::processDatagram() ******* END _tree->readBitstreamToTree()...";
                }
                _tree->unlock();
                
                // seek forwards in packet
                packet.seek(packet.pos() + sectionLength);

                elementsPerPacket += args.elementsPerPacket;
                entitiesPerPacket += args.entitiesPerPacket;

                _elementsInLastWindow += args.elementsPerPacket;
                _entitiesInLastWindow += args.entitiesPerPacket;

                totalWaitingForLock += (startUncompress - startLock);
                totalUncompress += (startReadBitsteam - startUncompress);
                totalReadBitsteam += (endReadBitsteam - startReadBitsteam);

            }
            subsection++;
        }
        _elementsPerPacket.updateAverage(elementsPerPacket);
        _entitiesPerPacket.updateAverage(entitiesPerPacket);

        _waitLockPerPacket.updateAverage(totalWaitingForLock);
        _uncompressPerPacket.updateAverage(totalUncompress);
        _readBitstreamPerPacket.updateAverage(totalReadBitsteam);
        
        quint64 now = usecTimestampNow();
        if (_lastWindowAt == 0) {
            _lastWindowAt = now;
        }
        quint64 sinceLastWindow = now - _lastWindowAt;
        
        if (sinceLastWindow > USECS_PER_SECOND) {
            float packetsPerSecondInWindow = (float)_packetsInLastWindow / (float)(sinceLastWindow / USECS_PER_SECOND);
            float elementsPerSecondInWindow = (float)_elementsInLastWindow / (float)(sinceLastWindow / USECS_PER_SECOND);
            float entitiesPerSecondInWindow = (float)_entitiesInLastWindow / (float)(sinceLastWindow / USECS_PER_SECOND);
            _packetsPerSecond.updateAverage(packetsPerSecondInWindow);
            _elementsPerSecond.updateAverage(elementsPerSecondInWindow);
            _entitiesPerSecond.updateAverage(entitiesPerSecondInWindow);

            _lastWindowAt = now;
            _packetsInLastWindow = 0;
            _elementsInLastWindow = 0;
            _entitiesInLastWindow = 0;
        }
    }
}

bool OctreeRenderer::renderOperation(OctreeElement* element, void* extraData) {
    RenderArgs* args = static_cast<RenderArgs*>(extraData);
    if (element->isInView(*args->_viewFrustum)) {
        if (element->hasContent()) {
            if (element->calculateShouldRender(args->_viewFrustum, args->_sizeScale, args->_boundaryLevelAdjust)) {
                args->_renderer->renderElement(element, args);
            } else {
                return false; // if we shouldn't render, then we also should stop recursing.
            }
        }
        return true; // continue recursing
    }
    // if not in view stop recursing
    return false;
}

void OctreeRenderer::render(RenderArgs* renderArgs) {
    if (_tree) {
        renderArgs->_renderer = this;
        _tree->lockForRead();
        _tree->recurseTreeWithOperation(renderOperation, renderArgs);
        _tree->unlock();
    }
}

void OctreeRenderer::clear() {
    if (_tree) {
        _tree->lockForWrite();
        _tree->eraseAllOctreeElements();
        _tree->unlock();
    }
}

