//
//  VoxelSendThread.cpp
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel packet sender
//

#include <NodeList.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <EnvironmentData.h>
extern EnvironmentData environmentData[3];

#include "VoxelSendThread.h"
#include "VoxelServer.h"
#include "VoxelServerConsts.h"

VoxelSendThread::VoxelSendThread(const QUuid& nodeUUID, VoxelServer* myServer) :
    _nodeUUID(nodeUUID),
    _myServer(myServer),
    _encodedSomething(false) {
}

bool VoxelSendThread::process() {
    uint64_t  start = usecTimestampNow();
    bool gotLock = false;
    
    // don't do any send processing until the initial load of the voxels is complete...
    if (_myServer->isInitialLoadComplete()) {
        Node* node = NodeList::getInstance()->nodeWithUUID(_nodeUUID);
    
        if (node) {
            // make sure the node list doesn't kill our node while we're using it
            if (node->trylock()) {
                gotLock = true;
                VoxelNodeData* nodeData = NULL;
    
                nodeData = (VoxelNodeData*) node->getLinkedData();
    
                int packetsSent = 0;

                // Sometimes the node data has not yet been linked, in which case we can't really do anything
                if (nodeData) {
                    bool viewFrustumChanged = nodeData->updateCurrentViewFrustum();
                    if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                        printf("nodeData->updateCurrentViewFrustum() changed=%s\n", debug::valueOf(viewFrustumChanged));
                    }
                    packetsSent = deepestLevelVoxelDistributor(node, nodeData, viewFrustumChanged);
                }
    
                node->unlock(); // we're done with this node for now.
            }
        }
    } else {
        if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
            qDebug("VoxelSendThread::process() waiting for isInitialLoadComplete()\n");
        }
    }
     
    // Only sleep if we're still running and we got the lock last time we tried, otherwise try to get the lock asap
    if (isStillRunning() && gotLock) {
        // dynamically sleep until we need to fire off the next set of voxels
        int elapsed = (usecTimestampNow() - start);
        int usecToSleep =  VOXEL_SEND_INTERVAL_USECS - elapsed;

        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                std::cout << "Last send took too much time, not sleeping!\n";
            }
        }
    }

    return isStillRunning();  // keep running till they terminate us
}


uint64_t VoxelSendThread::_totalBytes = 0;
uint64_t VoxelSendThread::_totalWastedBytes = 0;
uint64_t VoxelSendThread::_totalPackets = 0;

int VoxelSendThread::handlePacketSend(Node* node, VoxelNodeData* nodeData, int& trueBytesSent, int& truePacketsSent) {
    bool debug = _myServer->wantsDebugVoxelSending();

    bool packetSent = false; // did we send a packet?
    int packetsSent = 0;
    // Here's where we check to see if this packet is a duplicate of the last packet. If it is, we will silently
    // obscure the packet and not send it. This allows the callers and upper level logic to not need to know about
    // this rate control savings.
    if (nodeData->shouldSuppressDuplicatePacket()) {
        nodeData->resetVoxelPacket(); // we still need to reset it though!
        return packetsSent; // without sending...
    }

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

            int thisWastedBytes = MAX_PACKET_SIZE - statsMessageLength; // the statsMessageLength at this point includes data
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++;
            if (debug) {
                qDebug("Adding stats to packet [%llu]: size:%d [%llu] wasted bytes:%d [%llu]\n",
                    _totalPackets,
                    nodeData->getPacketLength(), _totalBytes,
                    thisWastedBytes, _totalWastedBytes);
            }
            
            // actually send it
            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(), statsMessage, statsMessageLength);
            packetSent = true;
        } else {
            // not enough room in the packet, send two packets
            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(), statsMessage, statsMessageLength);

            int thisWastedBytes = MAX_PACKET_SIZE - statsMessageLength; // the statsMessageLength is only the stats
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++;
            if (debug) {
                qDebug("Sending separate stats packet [%llu]: size:%d [%llu] wasted bytes:%d [%llu]\n",
                    _totalPackets,
                    nodeData->getPacketLength(), _totalBytes,
                    thisWastedBytes, _totalWastedBytes);
            }

            trueBytesSent += statsMessageLength;
            truePacketsSent++;
            packetsSent++;

            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(),
                                            nodeData->getPacket(), nodeData->getPacketLength());

            packetSent = true;

            thisWastedBytes = MAX_PACKET_SIZE - nodeData->getPacketLength();
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++;
            if (debug) {
                qDebug("Sending packet [%llu]: size:%d [%llu] wasted bytes:%d [%llu]\n",
                    _totalPackets,
                    nodeData->getPacketLength(), _totalBytes,
                    thisWastedBytes, _totalWastedBytes);
            }
        }
        nodeData->stats.markAsSent();
    } else {
        // If there's actually a packet waiting, then send it.
        if (nodeData->isPacketWaiting()) {
            // just send the voxel packet
            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(),
                                                           nodeData->getPacket(), nodeData->getPacketLength());
            packetSent = true;

            int thisWastedBytes = MAX_PACKET_SIZE - nodeData->getPacketLength();
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacketLength();
            _totalPackets++;
            if (debug) {
                qDebug("Sending packet [%llu]: size:%d [%llu] wasted bytes:%d [%llu]\n",
                    _totalPackets,
                    nodeData->getPacketLength(), _totalBytes,
                    thisWastedBytes, _totalWastedBytes);
            }
        }
    }
    // remember to track our stats
    if (packetSent) {
        nodeData->stats.packetSent(nodeData->getPacketLength());
        trueBytesSent += nodeData->getPacketLength();
        truePacketsSent++;
        packetsSent++;
        nodeData->resetVoxelPacket();
    }
    
    return packetsSent;
}

/// Version of voxel distributor that sends the deepest LOD level at once
int VoxelSendThread::deepestLevelVoxelDistributor(Node* node, VoxelNodeData* nodeData, bool viewFrustumChanged) {

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
    bool wantColor = LOW_RES_MONO && nodeData->getWantLowResMoving() && viewFrustumChanged ? false : nodeData->getWantColor();

    // If we have a packet waiting, and our desired want color, doesn't match the current waiting packets color
    // then let's just send that waiting packet.    
    if (wantColor != nodeData->getCurrentPacketIsColor()) {
    
        if (nodeData->isPacketWaiting()) {
            if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                printf("wantColor=%s --- SENDING PARTIAL PACKET! nodeData->getCurrentPacketIsColor()=%s\n", 
                       debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()));
            }

            packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
        } else {
            if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                printf("wantColor=%s --- FIXING HEADER! nodeData->getCurrentPacketIsColor()=%s\n", 
                       debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()));
            }
            nodeData->resetVoxelPacket();
        }
    }
    
    if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
        printf("wantColor=%s getCurrentPacketIsColor()=%s, viewFrustumChanged=%s, getWantLowResMoving()=%s\n", 
               debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()),
               debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->getWantLowResMoving()));
    }

    const ViewFrustum* lastViewFrustum =  wantDelta ? &nodeData->getLastKnownViewFrustum() : NULL;

    if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
        printf("deepestLevelVoxelDistributor() viewFrustumChanged=%s, nodeBag.isEmpty=%s, viewSent=%s\n",
                debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->nodeBag.isEmpty()), 
                debug::valueOf(nodeData->getViewSent())
            );
    }
    
    // If the current view frustum has changed OR we have nothing to send, then search against 
    // the current view frustum for things to send.
    if (viewFrustumChanged || nodeData->nodeBag.isEmpty()) {
        uint64_t now = usecTimestampNow();
        if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
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
            if (_myServer->wantDumpVoxelsOnMove() || nodeData->moveShouldDump() || nodeData->hasLodChanged()) {
                nodeData->dumpOutOfView();
            }
            nodeData->map.erase();
        } 
        
        if (!viewFrustumChanged && !nodeData->getWantDelta()) {
            // only set our last sent time if we weren't resetting due to frustum change
            uint64_t now = usecTimestampNow();
            nodeData->setLastTimeBagEmpty(now);
        }
        if (_encodedSomething) {
            nodeData->stats.sceneCompleted();
            packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
        }
        
        if (_myServer->wantDisplayVoxelStats()) {
            nodeData->stats.printDebugDetails();
        }
        
        // start tracking our stats
        bool isFullScene = ((!viewFrustumChanged || !nodeData->getWantDelta()) 
                                && nodeData->getViewFrustumJustStoppedChanging()) || nodeData->hasLodChanged();
        
        // If we're starting a full scene, then definitely we want to empty the nodeBag
        if (isFullScene) {
            nodeData->nodeBag.deleteAll();
        }

        nodeData->stats.sceneStarted(isFullScene, viewFrustumChanged, _myServer->getServerTree().rootNode, _myServer->getJurisdiction());

        // This is the start of "resending" the scene.
        bool dontRestartSceneOnMove = false; // this is experimental
        if (dontRestartSceneOnMove) {
            if (nodeData->nodeBag.isEmpty()) {
                nodeData->nodeBag.insert(_myServer->getServerTree().rootNode); // only in case of empty 
            }
        } else {
            nodeData->nodeBag.insert(_myServer->getServerTree().rootNode); // original behavior, reset on move or empty
        }
    }

    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!nodeData->nodeBag.isEmpty()) {
        int bytesWritten = 0;
        uint64_t start = usecTimestampNow();

        bool shouldSendEnvironments = _myServer->wantSendEnvironments() && shouldDo(ENVIRONMENT_SEND_INTERVAL_USECS, VOXEL_SEND_INTERVAL_USECS);

        int clientMaxPacketsPerInterval = std::max(1,(nodeData->getMaxVoxelPacketsPerSecond() / INTERVALS_PER_SECOND));
        int maxPacketsPerInterval = std::min(clientMaxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval());
        
        if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
            printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                nodeData->getMaxVoxelPacketsPerSecond(), clientMaxPacketsPerInterval);
        }
        
        while (somethingToSend && packetsSentThisInterval < maxPacketsPerInterval - (shouldSendEnvironments ? 1 : 0)) {
            if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                    truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                    nodeData->getMaxVoxelPacketsPerSecond(), clientMaxPacketsPerInterval);
            }


            // Check to see if we're taking too long, and if so bail early...
            uint64_t now = usecTimestampNow();
            long elapsedUsec = (now - start);
            long elapsedUsecPerPacket = (truePacketsSent == 0) ? 0 : (elapsedUsec / truePacketsSent);
            long usecRemaining = (VOXEL_SEND_INTERVAL_USECS - elapsedUsec);
            
            if (elapsedUsecPerPacket + SENDING_TIME_TO_SPARE > usecRemaining) {
                if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                    printf("packetLoop() usecRemaining=%ld bailing early took %ld usecs to generate %d bytes in %d packets (%ld usec avg), %d nodes still to send\n",
                            usecRemaining, elapsedUsec, trueBytesSent, truePacketsSent, elapsedUsecPerPacket,
                            nodeData->nodeBag.count());
                }
                break;
            }   
            
            if (!nodeData->nodeBag.isEmpty()) {
                VoxelNode* subTree = nodeData->nodeBag.extract();
                bool wantOcclusionCulling = nodeData->getWantOcclusionCulling();
                CoverageMap* coverageMap = wantOcclusionCulling ? &nodeData->map : IGNORE_COVERAGE_MAP;

                float voxelSizeScale = nodeData->getVoxelSizeScale();
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
                      

                _myServer->getServerTree().lockForRead();
                nodeData->stats.encodeStarted();

                //printf("calling nodeBag.count()=%d encode()... subTree=%p \n", nodeData->nodeBag.count(), subTree);
                //printOctalCode(subTree->getOctalCode());

                bytesWritten = _myServer->getServerTree().encodeTreeBitstream(subTree, &_tempPacket, nodeData->nodeBag, params);


                //printf("called encode()... bytesWritten=%d compressedSize=%d uncompressedSize=%d\n",bytesWritten, _tempPacket.getFinalizedSize(), _tempPacket.getUncompressedSize());

                if (bytesWritten > 0) {
                    _encodedSomething = true;
                }
                nodeData->stats.encodeStopped();
                _myServer->getServerTree().unlock();
            } else {
                // If the bag was empty then we didn't even attempt to encode, and so we know the bytesWritten were 0
                bytesWritten = 0;
                somethingToSend = false; // this will cause us to drop out of the loop...
            }
            
            // We only consider sending anything if there is something in the _tempPacket to send... But
            // if bytesWritten == 0 it means either the subTree couldn't fit or we had an empty bag... Both cases
            // mean we should send the previous packet contents and reset it. 
            bool sendNow = (bytesWritten == 0);
            if (_tempPacket.hasContent() && sendNow) {
                printf("calling writeToPacket() compressedSize=%d uncompressedSize=%d\n",_tempPacket.getFinalizedSize(), _tempPacket.getUncompressedSize());
                nodeData->writeToPacket(_tempPacket.getFinalizedData(), _tempPacket.getFinalizedSize());
                packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                _tempPacket.reset();
            }
        }
        
        // send the environment packet
        if (shouldSendEnvironments) {
            int numBytesPacketHeader = populateTypeAndVersion(_tempOutputBuffer, PACKET_TYPE_ENVIRONMENT_DATA);
            int envPacketLength = numBytesPacketHeader;
            int environmentsToSend = _myServer->getSendMinimalEnvironment() ? 1 : _myServer->getEnvironmentDataCount();
            
            for (int i = 0; i < environmentsToSend; i++) {
                envPacketLength += _myServer->getEnvironmentData(i)->getBroadcastData(_tempOutputBuffer + envPacketLength);
            }
            
            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(), _tempOutputBuffer, envPacketLength);
            trueBytesSent += envPacketLength;
            truePacketsSent++;
            packetsSentThisInterval++;
        }
        
        uint64_t end = usecTimestampNow();
        int elapsedmsec = (end - start)/1000;
        if (elapsedmsec > 100) {
            if (elapsedmsec > 1000) {
                int elapsedsec = (end - start)/1000000;
                printf("WARNING! packetLoop() took %d seconds to generate %d bytes in %d packets %d nodes still to send\n",
                        elapsedsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
            } else {
                printf("WARNING! packetLoop() took %d milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                        elapsedmsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
            }
        } else if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
            printf("packetLoop() took %d milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                    elapsedmsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
        }
        
        // if after sending packets we've emptied our bag, then we want to remember that we've sent all 
        // the voxels from the current view frustum
        if (nodeData->nodeBag.isEmpty()) {
            nodeData->updateLastKnownViewFrustum();
            nodeData->setViewSent(true);
            if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
                nodeData->map.printStats();
            }
            nodeData->map.erase(); // It would be nice if we could save this, and only reset it when the view frustum changes
        }

        if (_myServer->wantsDebugVoxelSending() && _myServer->wantsVerboseDebug()) {
            printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                nodeData->getMaxVoxelPacketsPerSecond(), clientMaxPacketsPerInterval);
        }
        
    } // end if bag wasn't empty, and so we sent stuff...

    return truePacketsSent;
}

