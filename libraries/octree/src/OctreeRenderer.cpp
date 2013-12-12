//
//  OctreeRenderer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <glm/glm.hpp>
#include <stdint.h>

#include <SharedUtil.h>
#include <PerfStat.h>
#include "OctreeRenderer.h"

OctreeRenderer::OctreeRenderer() {
    _tree = NULL;
    _viewFrustum = NULL;
}

void OctreeRenderer::init() {
    _tree = createTree();
}

OctreeRenderer::~OctreeRenderer() {
}

void OctreeRenderer::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    bool showTimingDetails = false; // Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    bool extraDebugging = false; // Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging)
    PerformanceWarning warn(showTimingDetails, "OctreeRenderer::processDatagram()",showTimingDetails);
    
    const unsigned char* packetData = (const unsigned char*)dataByteArray.constData();
    int packetLength = dataByteArray.size();

    unsigned char command = *packetData;
    
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    PACKET_TYPE expectedType = getExpectedPacketType();
    
    if(command == expectedType) {
        PerformanceWarning warn(showTimingDetails, "OctreeRenderer::processDatagram expected PACKET_TYPE",showTimingDetails);
    
        const unsigned char* dataAt = packetData + numBytesPacketHeader;

        OCTREE_PACKET_FLAGS flags = (*(OCTREE_PACKET_FLAGS*)(dataAt));
        dataAt += sizeof(OCTREE_PACKET_FLAGS);
        OCTREE_PACKET_SEQUENCE sequence = (*(OCTREE_PACKET_SEQUENCE*)dataAt);
        dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
        
        OCTREE_PACKET_SENT_TIME sentAt = (*(OCTREE_PACKET_SENT_TIME*)dataAt);
        dataAt += sizeof(OCTREE_PACKET_SENT_TIME);
        
        bool packetIsColored = oneAtBit(flags, PACKET_IS_COLOR_BIT);
        bool packetIsCompressed = oneAtBit(flags, PACKET_IS_COMPRESSED_BIT);
        
        OCTREE_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
        int flightTime = arrivedAt - sentAt;
        
        OCTREE_PACKET_INTERNAL_SECTION_SIZE sectionLength = 0;
        int dataBytes = packetLength - OCTREE_PACKET_HEADER_SIZE;

        if (extraDebugging) {
            qDebug("OctreeRenderer::processDatagram() ... Got Packet Section"
                   " color:%s compressed:%s sequence: %u flight:%d usec size:%d data:%d"
                   "\n",
                debug::valueOf(packetIsColored), debug::valueOf(packetIsCompressed), 
                sequence, flightTime, packetLength, dataBytes);
        }
        
        int subsection = 1;
        while (dataBytes > 0) {
            if (packetIsCompressed) {
                if (dataBytes > sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE)) {
                    sectionLength = (*(OCTREE_PACKET_INTERNAL_SECTION_SIZE*)dataAt);
                    dataAt += sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                    dataBytes -= sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                } else {
                    sectionLength = 0;
                    dataBytes = 0; // stop looping something is wrong
                }
            } else {
                sectionLength = dataBytes;
            }
            
            if (sectionLength) {
                // ask the VoxelTree to read the bitstream into the tree
                ReadBitstreamToTreeParams args(packetIsColored ? WANT_COLOR : NO_COLOR, WANT_EXISTS_BITS, NULL, 
                                                getDataSourceUUID());
                _tree->lockForWrite();
                OctreePacketData packetData(packetIsCompressed);
                packetData.loadFinalizedContent(dataAt, sectionLength);
                if (extraDebugging) {
                    qDebug("OctreeRenderer::processDatagram() ... Got Packet Section"
                           " color:%s compressed:%s sequence: %u flight:%d usec size:%d data:%d"
                           " subsection:%d sectionLength:%d uncompressed:%d\n",
                        debug::valueOf(packetIsColored), debug::valueOf(packetIsCompressed), 
                        sequence, flightTime, packetLength, dataBytes, subsection, sectionLength, packetData.getUncompressedSize());
                }
                _tree->readBitstreamToTree(packetData.getUncompressedData(), packetData.getUncompressedSize(), args);
                _tree->unlock();
            
                dataBytes -= sectionLength;
                dataAt += sectionLength;
            }
        }
        subsection++;
    }
}

bool OctreeRenderer::renderOperation(OctreeElement* element, void* extraData) {
    RenderArgs* args = static_cast<RenderArgs*>(extraData);
    //if (true || element->isInView(*args->_viewFrustum)) {
    if (element->isInView(*args->_viewFrustum)) {
        if (element->hasContent()) {
            args->_renderer->renderElement(element, args);
        }
        return true;
    }
    // if not in view stop recursing
    return false;
}

void OctreeRenderer::render() {
    RenderArgs args = { 0, this, _viewFrustum };
    if (_tree) {
        _tree->lockForRead();
        _tree->recurseTreeWithOperation(renderOperation, &args);
        _tree->unlock();
    }
}
