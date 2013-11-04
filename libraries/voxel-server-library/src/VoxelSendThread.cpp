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
    _myServer(myServer) {
}

bool VoxelSendThread::process() {
    uint64_t  start = usecTimestampNow();
    
    Node* node = NodeList::getInstance()->nodeWithUUID(_nodeUUID);
    VoxelNodeData* nodeData = NULL;
    
    if (node) {
        nodeData = (VoxelNodeData*) node->getLinkedData();
    }
    
    int packetsSent = 0;

    // Sometimes the node data has not yet been linked, in which case we can't really do anything
    if (nodeData) {
        bool viewFrustumChanged = nodeData->updateCurrentViewFrustum();
        if (_myServer->wantsDebugVoxelSending()) {
            printf("nodeData->updateCurrentViewFrustum() changed=%s\n", debug::valueOf(viewFrustumChanged));
        }
        packetsSent = deepestLevelVoxelDistributor(node, nodeData, viewFrustumChanged);
    }
    
    // dynamically sleep until we need to fire off the next set of voxels
    int elapsed = (usecTimestampNow() - start);
    int usecToSleep =  VOXEL_SEND_INTERVAL_USECS - elapsed;
    
    if (usecToSleep > 0) {
        usleep(usecToSleep);
    } else {
        if (_myServer->wantsDebugVoxelSending()) {
            std::cout << "Last send took too much time, not sleeping!\n";
        }
    }
    return isStillRunning();  // keep running till they terminate us
}


int VoxelSendThread::handlePacketSend(Node* node, VoxelNodeData* nodeData, int& trueBytesSent, int& truePacketsSent) {

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

            // actually send it
            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(), statsMessage, statsMessageLength);
        } else {
            // not enough room in the packet, send two packets
            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(), statsMessage, statsMessageLength);
            trueBytesSent += statsMessageLength;
            truePacketsSent++;
            packetsSent++;

            NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(),
                                            nodeData->getPacket(), nodeData->getPacketLength());
        }
    } else {
        // just send the voxel packet
        NodeList::getInstance()->getNodeSocket()->send(node->getActiveSocket(),
                                                       nodeData->getPacket(), nodeData->getPacketLength());
    }
    // remember to track our stats
    nodeData->stats.packetSent(nodeData->getPacketLength());
    trueBytesSent += nodeData->getPacketLength();
    truePacketsSent++;
    packetsSent++;
    nodeData->resetVoxelPacket();
    return packetsSent;
}

/// Version of voxel distributor that sends the deepest LOD level at once
int VoxelSendThread::deepestLevelVoxelDistributor(Node* node, VoxelNodeData* nodeData, bool viewFrustumChanged) {

    _myServer->lockTree();

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
            if (_myServer->wantsDebugVoxelSending()) {
                printf("wantColor=%s --- SENDING PARTIAL PACKET! nodeData->getCurrentPacketIsColor()=%s\n", 
                       debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()));
            }

            packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
        } else {
            if (_myServer->wantsDebugVoxelSending()) {
                printf("wantColor=%s --- FIXING HEADER! nodeData->getCurrentPacketIsColor()=%s\n", 
                       debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()));
            }
            nodeData->resetVoxelPacket();
        }
    }
    
    if (_myServer->wantsDebugVoxelSending()) {
        printf("wantColor=%s getCurrentPacketIsColor()=%s, viewFrustumChanged=%s, getWantLowResMoving()=%s\n", 
               debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()),
               debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->getWantLowResMoving()));
    }

    const ViewFrustum* lastViewFrustum =  wantDelta ? &nodeData->getLastKnownViewFrustum() : NULL;

    if (_myServer->wantsDebugVoxelSending()) {
        printf("deepestLevelVoxelDistributor() viewFrustumChanged=%s, nodeBag.isEmpty=%s, viewSent=%s\n",
                debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->nodeBag.isEmpty()), 
                debug::valueOf(nodeData->getViewSent())
            );
    }
    
    // If the current view frustum has changed OR we have nothing to send, then search against 
    // the current view frustum for things to send.
    if (viewFrustumChanged || nodeData->nodeBag.isEmpty()) {
        uint64_t now = usecTimestampNow();
        if (_myServer->wantsDebugVoxelSending()) {
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
        
        nodeData->stats.sceneCompleted();
        
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
        
        if (_myServer->wantsDebugVoxelSending()) {
            printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                nodeData->getMaxVoxelPacketsPerSecond(), clientMaxPacketsPerInterval);
        }
        while (somethingToSend && packetsSentThisInterval < maxPacketsPerInterval - (shouldSendEnvironments ? 1 : 0)) {
            if (_myServer->wantsDebugVoxelSending()) {
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
                if (_myServer->wantsDebugVoxelSending()) {
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
                      
                nodeData->stats.encodeStarted();
                bytesWritten = _myServer->getServerTree().encodeTreeBitstream(subTree, _tempOutputBuffer, MAX_VOXEL_PACKET_SIZE - 1,
                                                              nodeData->nodeBag, params);
                nodeData->stats.encodeStopped();

                if (nodeData->getAvailable() >= bytesWritten) {
                    nodeData->writeToPacket(_tempOutputBuffer, bytesWritten);
                } else {
                    packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                    nodeData->writeToPacket(_tempOutputBuffer, bytesWritten);
                }
            } else {
                if (nodeData->isPacketWaiting()) {
                    packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                }
                //packetsSentThisInterval = _myServer->getPacketsPerClientPerInterval(); // done for now, no nodes left
                somethingToSend = false;
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
        } else if (_myServer->wantsDebugVoxelSending()) {
            printf("packetLoop() took %d milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                    elapsedmsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
        }
        
        // if after sending packets we've emptied our bag, then we want to remember that we've sent all 
        // the voxels from the current view frustum
        if (nodeData->nodeBag.isEmpty()) {
            nodeData->updateLastKnownViewFrustum();
            nodeData->setViewSent(true);
            if (_myServer->wantsDebugVoxelSending()) {
                nodeData->map.printStats();
            }
            nodeData->map.erase(); // It would be nice if we could save this, and only reset it when the view frustum changes
        }

        if (_myServer->wantsDebugVoxelSending()) {
            printf("truePacketsSent=%d packetsSentThisInterval=%d maxPacketsPerInterval=%d server PPI=%d nodePPS=%d nodePPI=%d\n", 
                truePacketsSent, packetsSentThisInterval, maxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval(), 
                nodeData->getMaxVoxelPacketsPerSecond(), clientMaxPacketsPerInterval);
        }
        
    } // end if bag wasn't empty, and so we sent stuff...

    _myServer->unlockTree();

    return truePacketsSent;
}

