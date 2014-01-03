//
//  OctreeSendThread.cpp
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <NodeList.h>
#include <PacketHeaders.h>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "OctreeSendThread.h"
#include "OctreeServer.h"
#include "OctreeServerConsts.h"

uint64_t startSceneSleepTime = 0;
uint64_t endSceneSleepTime = 0;

OctreeSendThread::OctreeSendThread(const QUuid& nodeUUID, OctreeServer* myServer) :
    _nodeUUID(nodeUUID),
    _myServer(myServer),
    _packetData()
{
}

bool OctreeSendThread::process() {
    uint64_t  start = usecTimestampNow();
    bool gotLock = false;
    
    // don't do any send processing until the initial load of the octree is complete...
    if (_myServer->isInitialLoadComplete()) {
        Node* node = NodeList::getInstance()->nodeWithUUID(_nodeUUID);
    
        if (node) {
            // make sure the node list doesn't kill our node while we're using it
            if (node->trylock()) {
                gotLock = true;
                OctreeQueryNode* nodeData = NULL;
    
                nodeData = (OctreeQueryNode*) node->getLinkedData();
    
                int packetsSent = 0;

                // Sometimes the node data has not yet been linked, in which case we can't really do anything
                if (nodeData) {
                    bool viewFrustumChanged = nodeData->updateCurrentViewFrustum();
                    if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                        printf("nodeData->updateCurrentViewFrustum() changed=%s\n", debug::valueOf(viewFrustumChanged));
                    }
                    packetsSent = packetDistributor(node, nodeData, viewFrustumChanged);
                }
    
                node->unlock(); // we're done with this node for now.
            }
        }
    } else {
        if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
            qDebug("OctreeSendThread::process() waiting for isInitialLoadComplete()\n");
        }
    }
     
    // Only sleep if we're still running and we got the lock last time we tried, otherwise try to get the lock asap
    if (isStillRunning() && gotLock) {
        // dynamically sleep until we need to fire off the next set of octree elements
        int elapsed = (usecTimestampNow() - start);
        int usecToSleep =  OCTREE_SEND_INTERVAL_USECS - elapsed;

        if (usecToSleep > 0) {
            PerformanceWarning warn(false,"OctreeSendThread... usleep()",false,&_usleepTime,&_usleepCalls);
            usleep(usecToSleep);
        } else {
            if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                std::cout << "Last send took too much time, not sleeping!\n";
            }
        }
    }

    return isStillRunning();  // keep running till they terminate us
}

uint64_t OctreeSendThread::_usleepTime = 0;
uint64_t OctreeSendThread::_usleepCalls = 0;

uint64_t OctreeSendThread::_totalBytes = 0;
uint64_t OctreeSendThread::_totalWastedBytes = 0;
uint64_t OctreeSendThread::_totalPackets = 0;

int OctreeSendThread::handlePacketSend(Node* node, OctreeQueryNode* nodeData, int& trueBytesSent, int& truePacketsSent) {
    bool debug = _myServer->wantsDebugSending();
    uint64_t now = usecTimestampNow();

    bool packetSent = false; // did we send a packet?
    int packetsSent = 0;
    // Here's where we check to see if this packet is a duplicate of the last packet. If it is, we will silently
    // obscure the packet and not send it. This allows the callers and upper level logic to not need to know about
    // this rate control savings.
    if (nodeData->shouldSuppressDuplicatePacket()) {
        nodeData->resetOctreePacket(true); // we still need to reset it though!
        return packetsSent; // without sending...
    }
    
    const unsigned char* messageData = nodeData->getPacket();
    int numBytesPacketHeader = numBytesForPacketHeader(messageData);
    const unsigned char* dataAt = messageData + numBytesPacketHeader;
    dataAt += sizeof(OCTREE_PACKET_FLAGS);
    OCTREE_PACKET_SEQUENCE sequence = (*(OCTREE_PACKET_SEQUENCE*)dataAt);
    dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
    

    // If we've got a stats message ready to send, then see if we can piggyback them together
    if (nodeData->stats.isReadyToSend()) {
        // Send the stats message to the client
        unsigned char* statsMessage = nodeData->stats.getStatsMessage();
        int statsMessageLength = nodeData->stats.getStatsMessageLength();

        // If the size of the stats message and the voxel message will fit in a packet, then piggyback them
        if (nodeData->getPacketLength() + statsMessageLength < MAX_PACKET_SIZE) {

            // copy voxel message to back of stats message
            memcpy(statsMessage + statsMessageLength, nodeData->getPacket(), nodeData->getPacketLength());
            statsMessageLength += nodeData->getPacketLength();

            // since a stats message is only included on end of scene, don't consider any of these bytes "wasted", since
            // there was nothing else to send.
            int thisWastedBytes = 0;
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++; 
            if (debug) {
                qDebug() << "Adding stats to packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence << 
                        " size: " << nodeData->getPacketLength() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]\n";
            }
            
            // actually send it
            NodeList::getInstance()->getNodeSocket().writeDatagram((char*) statsMessage, statsMessageLength,
                                                                   node->getActiveSocket()->getAddress(),
                                                                   node->getActiveSocket()->getPort());
            packetSent = true;
        } else {
            // not enough room in the packet, send two packets
            NodeList::getInstance()->getNodeSocket().writeDatagram((char*) statsMessage, statsMessageLength,
                                                                   node->getActiveSocket()->getAddress(),
                                                                   node->getActiveSocket()->getPort());

            // since a stats message is only included on end of scene, don't consider any of these bytes "wasted", since
            // there was nothing else to send.
            int thisWastedBytes = 0;
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += statsMessageLength;
            _totalPackets++;
            if (debug) {
                qDebug() << "Sending separate stats packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence << 
                        " size: " << statsMessageLength << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]\n";
            }

            trueBytesSent += statsMessageLength;
            truePacketsSent++;
            packetsSent++;

            NodeList::getInstance()->getNodeSocket().writeDatagram((char*) nodeData->getPacket(), nodeData->getPacketLength(),
                                                                   node->getActiveSocket()->getAddress(),
                                                                   node->getActiveSocket()->getPort());

            packetSent = true;

            thisWastedBytes = MAX_PACKET_SIZE - nodeData->getPacketLength();
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++;
            if (debug) {
                qDebug() << "Sending packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence << 
                        " size: " << nodeData->getPacketLength() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]\n";
            }
        }
        nodeData->stats.markAsSent();
    } else {
        // If there's actually a packet waiting, then send it.
        if (nodeData->isPacketWaiting()) {
            // just send the voxel packet
            NodeList::getInstance()->getNodeSocket().writeDatagram((char*) nodeData->getPacket(), nodeData->getPacketLength(),
                                                                   node->getActiveSocket()->getAddress(),
                                                                   node->getActiveSocket()->getPort());
            packetSent = true;

            int thisWastedBytes = MAX_PACKET_SIZE - nodeData->getPacketLength();
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++;
            if (debug) {
                qDebug() << "Sending packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence << 
                        " size: " << nodeData->getPacketLength() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]\n";
            }
        }
    }
    // remember to track our stats
    if (packetSent) {
        nodeData->stats.packetSent(nodeData->getPacketLength());
        trueBytesSent += nodeData->getPacketLength();
        truePacketsSent++;
        packetsSent++;
        nodeData->resetOctreePacket();
    }
    
    return packetsSent;
}

/// Version of voxel distributor that sends the deepest LOD level at once
int OctreeSendThread::packetDistributor(Node* node, OctreeQueryNode* nodeData, bool viewFrustumChanged) {

    int truePacketsSent = 0;
    int trueBytesSent = 0;
    int packetsSentThisInterval = 0;
    bool somethingToSend = true; // assume we have something

    // FOR NOW... node tells us if it wants to receive only view frustum deltas
    bool wantDelta = viewFrustumChanged && nodeData->getWantDelta();

    // If our packet already has content in it, then we must use the color choice of the waiting packet.    
    // If we're starting a fresh packet, then... 
    //     If we're moving, and the client asked for low res, then we force monochrome, otherwise, use 
    //     the clients requested color state.
    bool wantColor = nodeData->getWantColor();
    bool wantCompression = nodeData->getWantCompression();

    // If we have a packet waiting, and our desired want color, doesn't match the current waiting packets color
    // then let's just send that waiting packet.    
    if (!nodeData->getCurrentPacketFormatMatches()) {
        if (nodeData->isPacketWaiting()) {
            if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                printf("wantColor=%s wantCompression=%s SENDING PARTIAL PACKET! currentPacketIsColor=%s currentPacketIsCompressed=%s\n", 
                        debug::valueOf(wantColor), debug::valueOf(wantCompression),
                        debug::valueOf(nodeData->getCurrentPacketIsColor()), 
                        debug::valueOf(nodeData->getCurrentPacketIsCompressed()) );
            }
            packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
        } else {
            if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                printf("wantColor=%s wantCompression=%s FIXING HEADER! currentPacketIsColor=%s currentPacketIsCompressed=%s\n", 
                        debug::valueOf(wantColor), debug::valueOf(wantCompression),
                        debug::valueOf(nodeData->getCurrentPacketIsColor()), 
                        debug::valueOf(nodeData->getCurrentPacketIsCompressed()) );
            }
            nodeData->resetOctreePacket();
        }
        int targetSize = MAX_OCTREE_PACKET_DATA_SIZE;
        if (wantCompression) {
            targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
        }
        if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
            printf("line:%d _packetData.changeSettings() wantCompression=%s targetSize=%d\n",__LINE__,
                debug::valueOf(wantCompression), targetSize);
        }
            
        _packetData.changeSettings(wantCompression, targetSize);
    }
    
    if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
        printf("wantColor/isColor=%s/%s wantCompression/isCompressed=%s/%s viewFrustumChanged=%s, getWantLowResMoving()=%s\n", 
                debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()), 
                debug::valueOf(wantCompression), debug::valueOf(nodeData->getCurrentPacketIsCompressed()),
                debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->getWantLowResMoving()));
    }

    const ViewFrustum* lastViewFrustum =  wantDelta ? &nodeData->getLastKnownViewFrustum() : NULL;

    if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
        printf("packetDistributor() viewFrustumChanged=%s, nodeBag.isEmpty=%s, viewSent=%s\n",
                debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->nodeBag.isEmpty()), 
                debug::valueOf(nodeData->getViewSent())
            );
    }
    
    // If the current view frustum has changed OR we have nothing to send, then search against 
    // the current view frustum for things to send.
    if (viewFrustumChanged || nodeData->nodeBag.isEmpty()) {
        uint64_t now = usecTimestampNow();
        if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
            printf("(viewFrustumChanged=%s || nodeData->nodeBag.isEmpty() =%s)...\n",
                   debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->nodeBag.isEmpty()));
            if (nodeData->getLastTimeBagEmpty() > 0) {
                float elapsedSceneSend = (now - nodeData->getLastTimeBagEmpty()) / 1000000.0f;
                if (viewFrustumChanged) {
                    printf("viewFrustumChanged resetting after elapsed time to send scene = %f seconds", elapsedSceneSend);
                } else {
                    printf("elapsed time to send scene = %f seconds", elapsedSceneSend);
                }
                printf(" [occlusionCulling:%s, wantDelta:%s, wantColor:%s ]\n", 
                       debug::valueOf(nodeData->getWantOcclusionCulling()), debug::valueOf(wantDelta), 
                       debug::valueOf(wantColor));
            }
        }
                
        // if our view has changed, we need to reset these things...
        if (viewFrustumChanged) {
            if (nodeData->moveShouldDump() || nodeData->hasLodChanged()) {
                nodeData->dumpOutOfView();
            }
            nodeData->map.erase();
        } 
        
        if (!viewFrustumChanged && !nodeData->getWantDelta()) {
            // only set our last sent time if we weren't resetting due to frustum change
            uint64_t now = usecTimestampNow();
            nodeData->setLastTimeBagEmpty(now);
        }

        // track completed scenes and send out the stats packet accordingly
        nodeData->stats.sceneCompleted();
        ::endSceneSleepTime = _usleepTime;
        unsigned long sleepTime = ::endSceneSleepTime - ::startSceneSleepTime;
        
        unsigned long encodeTime = nodeData->stats.getTotalEncodeTime();
        unsigned long elapsedTime = nodeData->stats.getElapsedTime();
        packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);

        if (_myServer->wantsDebugSending()) {
            qDebug() << "Scene completed at " << usecTimestampNow() 
                << "encodeTime:" << encodeTime 
                << " sleepTime:" << sleepTime 
                << " elapsed:" << elapsedTime 
                << " Packets:" << _totalPackets 
                << " Bytes:" << _totalBytes 
                << " Wasted:" << _totalWastedBytes << "\n";
        }
        
        // start tracking our stats
        bool isFullScene = ((!viewFrustumChanged || !nodeData->getWantDelta()) 
                                && nodeData->getViewFrustumJustStoppedChanging()) || nodeData->hasLodChanged();
        
        // If we're starting a full scene, then definitely we want to empty the nodeBag
        if (isFullScene) {
            nodeData->nodeBag.deleteAll();
        }

        if (_myServer->wantsDebugSending()) {
            qDebug() << "Scene started at " << usecTimestampNow() 
                << " Packets:" << _totalPackets 
                << " Bytes:" << _totalBytes 
                << " Wasted:" << _totalWastedBytes << "\n";
        }

        ::startSceneSleepTime = _usleepTime;
        nodeData->stats.sceneStarted(isFullScene, viewFrustumChanged, _myServer->getOctree()->getRoot(), _myServer->getJurisdiction());

        // This is the start of "resending" the scene.
        bool dontRestartSceneOnMove = false; // this is experimental
        if (dontRestartSceneOnMove) {
            if (nodeData->nodeBag.isEmpty()) {
                nodeData->nodeBag.insert(_myServer->getOctree()->getRoot()); // only in case of empty
            }
        } else {
            nodeData->nodeBag.insert(_myServer->getOctree()->getRoot()); // original behavior, reset on move or empty
        }
    }

    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!nodeData->nodeBag.isEmpty()) {
        int bytesWritten = 0;
        uint64_t start = usecTimestampNow();
        uint64_t startCompressTimeMsecs = OctreePacketData::getCompressContentTime() / 1000;
        uint64_t startCompressCalls = OctreePacketData::getCompressContentCalls();

        int clientMaxPacketsPerInterval = std::max(1,(nodeData->getMaxOctreePacketsPerSecond() / INTERVALS_PER_SECOND));
        int maxPacketsPerInterval = std::min(clientMaxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval());
        
        if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
            printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                nodeData->getMaxOctreePacketsPerSecond(), clientMaxPacketsPerInterval);
        }
        
        int extraPackingAttempts = 0;
        while (somethingToSend && packetsSentThisInterval < maxPacketsPerInterval) {
            if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                    truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                    nodeData->getMaxOctreePacketsPerSecond(), clientMaxPacketsPerInterval);
            }

            bool lastNodeDidntFit = false; // assume each node fits
            if (!nodeData->nodeBag.isEmpty()) {
                OctreeElement* subTree = nodeData->nodeBag.extract();
                bool wantOcclusionCulling = nodeData->getWantOcclusionCulling();
                CoverageMap* coverageMap = wantOcclusionCulling ? &nodeData->map : IGNORE_COVERAGE_MAP;

                float voxelSizeScale = nodeData->getOctreeSizeScale();
                int boundaryLevelAdjustClient = nodeData->getBoundaryLevelAdjust();

                int boundaryLevelAdjust = boundaryLevelAdjustClient + (viewFrustumChanged && nodeData->getWantLowResMoving() 
                                                                              ? LOW_RES_MOVING_ADJUST : NO_BOUNDARY_ADJUST);


                bool isFullScene = ((!viewFrustumChanged || !nodeData->getWantDelta()) && 
                                 nodeData->getViewFrustumJustStoppedChanging()) || nodeData->hasLodChanged();
                
                EncodeBitstreamParams params(INT_MAX, &nodeData->getCurrentViewFrustum(), wantColor, 
                                             WANT_EXISTS_BITS, DONT_CHOP, wantDelta, lastViewFrustum,
                                             wantOcclusionCulling, coverageMap, boundaryLevelAdjust, voxelSizeScale,
                                             nodeData->getLastTimeBagEmpty(),
                                             isFullScene, &nodeData->stats, _myServer->getJurisdiction());
                      

                _myServer->getOctree()->lockForRead();
                nodeData->stats.encodeStarted();
                bytesWritten = _myServer->getOctree()->encodeTreeBitstream(subTree, &_packetData, nodeData->nodeBag, params);
                
                // if we're trying to fill a full size packet, then we use this logic to determine if we have a DIDNT_FIT case.
                if (_packetData.getTargetSize() == MAX_OCTREE_PACKET_DATA_SIZE) {
                    if (_packetData.hasContent() && bytesWritten == 0 && 
                            params.stopReason == EncodeBitstreamParams::DIDNT_FIT) {
                        lastNodeDidntFit = true;
                    }
                } else {
                    // in compressed mode and we are trying to pack more... and we don't care if the _packetData has
                    // content or not... because in this case even if we were unable to pack any data, we want to drop
                    // below to our sendNow logic, but we do want to track that we attempted to pack extra
                    extraPackingAttempts++;
                    if (bytesWritten == 0 && params.stopReason == EncodeBitstreamParams::DIDNT_FIT) {
                        lastNodeDidntFit = true;
                    }
                }

                nodeData->stats.encodeStopped();
                _myServer->getOctree()->unlock();
            } else {
                // If the bag was empty then we didn't even attempt to encode, and so we know the bytesWritten were 0
                bytesWritten = 0;
                somethingToSend = false; // this will cause us to drop out of the loop...
            }
            
            // If the last node didn't fit, but we're in compressed mode, then we actually want to see if we can fit a
            // little bit more in this packet. To do this we 
            
            // We only consider sending anything if there is something in the _packetData to send... But
            // if bytesWritten == 0 it means either the subTree couldn't fit or we had an empty bag... Both cases
            // mean we should send the previous packet contents and reset it. 
            if (lastNodeDidntFit) {
                if (_packetData.hasContent()) {
                    // if for some reason the finalized size is greater than our available size, then probably the "compressed"
                    // form actually inflated beyond our padding, and in this case we will send the current packet, then
                    // write to out new packet...
                    int writtenSize = _packetData.getFinalizedSize() 
                            + (nodeData->getCurrentPacketIsCompressed() ? sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE) : 0);
                    
                    
                    if (writtenSize > nodeData->getAvailable()) {
                        if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                            printf("writtenSize[%d] > available[%d] too big, sending packet as is.\n",
                                writtenSize, nodeData->getAvailable());
                        }
                        packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                    }

                    if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                        printf("calling writeToPacket() available=%d compressedSize=%d uncompressedSize=%d target=%d\n",
                                nodeData->getAvailable(), _packetData.getFinalizedSize(), 
                                _packetData.getUncompressedSize(), _packetData.getTargetSize());
                    }
                    nodeData->writeToPacket(_packetData.getFinalizedData(), _packetData.getFinalizedSize());
                    extraPackingAttempts = 0;
                }
                
                // If we're not running compressed, the we know we can just send now. Or if we're running compressed, but
                // the packet doesn't have enough space to bother attempting to pack more...
                bool sendNow = true;
                
                if (nodeData->getCurrentPacketIsCompressed() && 
                    nodeData->getAvailable() >= MINIMUM_ATTEMPT_MORE_PACKING &&
                    extraPackingAttempts <= REASONABLE_NUMBER_OF_PACKING_ATTEMPTS) {
                    sendNow = false; // try to pack more
                }
                
                int targetSize = MAX_OCTREE_PACKET_DATA_SIZE;
                if (sendNow) {
                    packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                    if (wantCompression) {
                        targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                    }
                } else {
                    // If we're in compressed mode, then we want to see if we have room for more in this wire packet.
                    // but we've finalized the _packetData, so we want to start a new section, we will do that by
                    // resetting the packet settings with the max uncompressed size of our current available space
                    // in the wire packet. We also include room for our section header, and a little bit of padding
                    // to account for the fact that whenc compressing small amounts of data, we sometimes end up with
                    // a larger compressed size then uncompressed size
                    targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE) - COMPRESS_PADDING;
                }
                if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                    printf("line:%d _packetData.changeSettings() wantCompression=%s targetSize=%d\n",__LINE__,
                        debug::valueOf(nodeData->getWantCompression()), targetSize);
                }
                _packetData.changeSettings(nodeData->getWantCompression(), targetSize); // will do reset
            }
        }
        
        
        // Here's where we can/should allow the server to send other data...
        // send the environment packet
        if (_myServer->hasSpecialPacketToSend()) {
            trueBytesSent += _myServer->sendSpecialPacket(node);
            truePacketsSent++;
            packetsSentThisInterval++;
        }
        
        
        uint64_t end = usecTimestampNow();
        int elapsedmsec = (end - start)/1000;

        uint64_t endCompressCalls = OctreePacketData::getCompressContentCalls();
        int elapsedCompressCalls = endCompressCalls - startCompressCalls;
    
        uint64_t endCompressTimeMsecs = OctreePacketData::getCompressContentTime() / 1000;
        int elapsedCompressTimeMsecs = endCompressTimeMsecs - startCompressTimeMsecs;


        if (elapsedmsec > 100) {
            if (elapsedmsec > 1000) {
                int elapsedsec = (end - start)/1000000;
                printf("WARNING! packetLoop() took %d seconds [%d milliseconds %d calls in compress] to generate %d bytes in %d packets %d nodes still to send\n",
                        elapsedsec, elapsedCompressTimeMsecs, elapsedCompressCalls, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
            } else {
                printf("WARNING! packetLoop() took %d milliseconds [%d milliseconds %d calls in compress] to generate %d bytes in %d packets, %d nodes still to send\n",
                        elapsedmsec, elapsedCompressTimeMsecs, elapsedCompressCalls, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
            }
        } else if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
            printf("packetLoop() took %d milliseconds [%d milliseconds %d calls in compress] to generate %d bytes in %d packets, %d nodes still to send\n",
                    elapsedmsec, elapsedCompressTimeMsecs, elapsedCompressCalls, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
        }
        
        // if after sending packets we've emptied our bag, then we want to remember that we've sent all 
        // the voxels from the current view frustum
        if (nodeData->nodeBag.isEmpty()) {
            nodeData->updateLastKnownViewFrustum();
            nodeData->setViewSent(true);
            if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
                nodeData->map.printStats();
            }
            nodeData->map.erase(); // It would be nice if we could save this, and only reset it when the view frustum changes
        }

        if (_myServer->wantsDebugSending() && _myServer->wantsVerboseDebug()) {
            printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                nodeData->getMaxOctreePacketsPerSecond(), clientMaxPacketsPerInterval);
        }
        
    } // end if bag wasn't empty, and so we sent stuff...

    return truePacketsSent;
}

