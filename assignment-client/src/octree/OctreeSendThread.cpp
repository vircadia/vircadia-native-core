//
//  OctreeSendThread.cpp
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <chrono>
#include <thread>

#include <NodeList.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>

#include "OctreeQueryNode.h"
#include "OctreeSendThread.h"
#include "OctreeServer.h"
#include "OctreeServerConsts.h"
#include "OctreeLogging.h"

quint64 startSceneSleepTime = 0;
quint64 endSceneSleepTime = 0;

OctreeSendThread::OctreeSendThread(OctreeServer* myServer, const SharedNodePointer& node) :
    _myServer(myServer),
    _node(node),
    _nodeUuid(node->getUUID())
{
    QString safeServerName("Octree");

    // set our QThread object name so we can identify this thread while debugging
    setObjectName(QString("Octree Send Thread (%1)").arg(uuidStringWithoutCurlyBraces(_nodeUuid)));

    if (_myServer) {
        safeServerName = _myServer->getMyServerName();
    }

    qDebug() << qPrintable(safeServerName)  << "server [" << _myServer << "]: client connected "
                                            "- starting sending thread [" << this << "]";

    OctreeServer::clientConnected();
}

OctreeSendThread::~OctreeSendThread() {
    setIsShuttingDown();
    
    QString safeServerName("Octree");
    if (_myServer) {
        safeServerName = _myServer->getMyServerName();
    }

    qDebug() << qPrintable(safeServerName)  << "server [" << _myServer << "]: client disconnected "
                                            "- ending sending thread [" << this << "]";

    OctreeServer::clientDisconnected();
    OctreeServer::stopTrackingThread(this);
}

void OctreeSendThread::setIsShuttingDown() {
    _isShuttingDown = true;
}


bool OctreeSendThread::process() {
    if (_isShuttingDown) {
        return false; // exit early if we're shutting down
    }

    OctreeServer::didProcess(this);

    quint64  start = usecTimestampNow();

    // we'd better have a server at this point, or we're in trouble
    assert(_myServer);

    // don't do any send processing until the initial load of the octree is complete...
    if (_myServer->isInitialLoadComplete()) {
        if (auto node = _node.lock()) {
            _nodeMissingCount = 0;
            OctreeQueryNode* nodeData = static_cast<OctreeQueryNode*>(node->getLinkedData());

            // Sometimes the node data has not yet been linked, in which case we can't really do anything
            if (nodeData && !nodeData->isShuttingDown()) {
                bool viewFrustumChanged = nodeData->updateCurrentViewFrustum();
                packetDistributor(node, nodeData, viewFrustumChanged);
            }
        } else {
            return false; // exit early if we're shutting down
        }
    }

    if (_isShuttingDown) {
        return false; // exit early if we're shutting down
    }

    // Only sleep if we're still running and we got the lock last time we tried, otherwise try to get the lock asap
    if (isStillRunning()) {
        // dynamically sleep until we need to fire off the next set of octree elements
        int elapsed = (usecTimestampNow() - start);
        int usecToSleep =  OCTREE_SEND_INTERVAL_USECS - elapsed;

        if (usecToSleep <= 0) {
            const int MIN_USEC_TO_SLEEP = 1;
            usecToSleep = MIN_USEC_TO_SLEEP;
        }

        {
            PerformanceWarning warn(false,"OctreeSendThread... usleep()",false,&_usleepTime,&_usleepCalls);
            std::this_thread::sleep_for(std::chrono::microseconds(usecToSleep));
        }

    }

    return isStillRunning();  // keep running till they terminate us
}

AtomicUIntStat OctreeSendThread::_usleepTime { 0 };
AtomicUIntStat OctreeSendThread::_usleepCalls { 0 };
AtomicUIntStat OctreeSendThread::_totalBytes { 0 };
AtomicUIntStat OctreeSendThread::_totalWastedBytes { 0 };
AtomicUIntStat OctreeSendThread::_totalPackets { 0 };

AtomicUIntStat OctreeSendThread::_totalSpecialBytes { 0 };
AtomicUIntStat OctreeSendThread::_totalSpecialPackets { 0 };


int OctreeSendThread::handlePacketSend(SharedNodePointer node, OctreeQueryNode* nodeData, int& trueBytesSent,
                                       int& truePacketsSent, bool dontSuppressDuplicate) {
    OctreeServer::didHandlePacketSend(this);

    // if we're shutting down, then exit early
    if (nodeData->isShuttingDown()) {
        return 0;
    }

    bool debug = _myServer->wantsDebugSending();
    quint64 now = usecTimestampNow();

    bool packetSent = false; // did we send a packet?
    int packetsSent = 0;

    // Here's where we check to see if this packet is a duplicate of the last packet. If it is, we will silently
    // obscure the packet and not send it. This allows the callers and upper level logic to not need to know about
    // this rate control savings.
    if (!dontSuppressDuplicate && nodeData->shouldSuppressDuplicatePacket()) {
        nodeData->resetOctreePacket(); // we still need to reset it though!
        return packetsSent; // without sending...
    }

    // If we've got a stats message ready to send, then see if we can piggyback them together
    if (nodeData->stats.isReadyToSend() && !nodeData->isShuttingDown()) {
        // Send the stats message to the client
        NLPacket& statsPacket = nodeData->stats.getStatsMessage();

        // If the size of the stats message and the octree message will fit in a packet, then piggyback them
        if (nodeData->getPacket().getDataSize() <= statsPacket.bytesAvailableForWrite()) {

            // copy octree message to back of stats message
            statsPacket.write(nodeData->getPacket().getData(), nodeData->getPacket().getDataSize());

            // since a stats message is only included on end of scene, don't consider any of these bytes "wasted", since
            // there was nothing else to send.
            int thisWastedBytes = 0;
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += statsPacket.getDataSize();
            _totalPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Adding stats to packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " statsMessageLength: " << statsPacket.getDataSize() <<
                        " original size: " << nodeData->getPacket().getDataSize() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }

            // actually send it
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(statsPacket, *node);
            packetSent = true;
        } else {
            // not enough room in the packet, send two packets
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(statsPacket, *node);

            // since a stats message is only included on end of scene, don't consider any of these bytes "wasted", since
            // there was nothing else to send.
            int thisWastedBytes = 0;
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += statsPacket.getDataSize();
            _totalPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Sending separate stats packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " size: " << statsPacket.getDataSize() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }

            trueBytesSent += statsPacket.getDataSize();
            truePacketsSent++;
            packetsSent++;

            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(nodeData->getPacket(), *node);
            packetSent = true;

            int packetSizeWithHeader = nodeData->getPacket().getDataSize();
            thisWastedBytes = udt::MAX_PACKET_SIZE - packetSizeWithHeader;
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += nodeData->getPacket().getDataSize();
            _totalPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Sending packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " size: " << nodeData->getPacket().getDataSize() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }
        }
        nodeData->stats.markAsSent();
    } else {
        // If there's actually a packet waiting, then send it.
        if (nodeData->isPacketWaiting() && !nodeData->isShuttingDown()) {
            // just send the octree packet
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(nodeData->getPacket(), *node);
            packetSent = true;

            int packetSizeWithHeader = nodeData->getPacket().getDataSize();
            int thisWastedBytes = udt::MAX_PACKET_SIZE - packetSizeWithHeader;
            _totalWastedBytes += thisWastedBytes;
            _totalBytes += packetSizeWithHeader;
            _totalPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Sending packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " size: " << packetSizeWithHeader << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }
        }
    }

    // remember to track our stats
    if (packetSent) {
        nodeData->stats.packetSent(nodeData->getPacket().getPayloadSize());
        trueBytesSent += nodeData->getPacket().getPayloadSize();
        truePacketsSent++;
        packetsSent++;
        nodeData->octreePacketSent();
        nodeData->resetOctreePacket();
    }

    return packetsSent;
}

/// Version of octree element distributor that sends the deepest LOD level at once
int OctreeSendThread::packetDistributor(SharedNodePointer node, OctreeQueryNode* nodeData, bool viewFrustumChanged) {

    OctreeServer::didPacketDistributor(this);

    // if shutting down, exit early
    if (nodeData->isShuttingDown()) {
        return 0;
    }

    // calculate max number of packets that can be sent during this interval
    int clientMaxPacketsPerInterval = std::max(1, (nodeData->getMaxQueryPacketsPerSecond() / INTERVALS_PER_SECOND));
    int maxPacketsPerInterval = std::min(clientMaxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval());

    int truePacketsSent = 0;
    int trueBytesSent = 0;
    int packetsSentThisInterval = 0;
    bool isFullScene = ((!viewFrustumChanged) && nodeData->getViewFrustumJustStoppedChanging())
                                || nodeData->hasLodChanged();

    bool somethingToSend = true; // assume we have something

    // If our packet already has content in it, then we must use the color choice of the waiting packet.
    // If we're starting a fresh packet, then...
    //     If we're moving, and the client asked for low res, then we force monochrome, otherwise, use
    //     the clients requested color state.

    // If we have a packet waiting, and our desired want color, doesn't match the current waiting packets color
    // then let's just send that waiting packet.
    if (nodeData->isPacketWaiting()) {
        packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
    } else {
        nodeData->resetOctreePacket();
    }
    int targetSize = MAX_OCTREE_PACKET_DATA_SIZE;
    targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);

    _packetData.changeSettings(true, targetSize); // FIXME - eventually support only compressed packets

    // If the current view frustum has changed OR we have nothing to send, then search against
    // the current view frustum for things to send.
    if (viewFrustumChanged || nodeData->elementBag.isEmpty()) {

        // if our view has changed, we need to reset these things...
        if (viewFrustumChanged) {
            if (nodeData->moveShouldDump() || nodeData->hasLodChanged()) {
                nodeData->dumpOutOfView();
            }
        }

        // track completed scenes and send out the stats packet accordingly
        nodeData->stats.sceneCompleted();
        nodeData->setLastRootTimestamp(_myServer->getOctree()->getRoot()->getLastChanged());
        _myServer->getOctree()->releaseSceneEncodeData(&nodeData->extraEncodeData);

        // TODO: add these to stats page
        //::endSceneSleepTime = _usleepTime;
        //unsigned long sleepTime = ::endSceneSleepTime - ::startSceneSleepTime;
        //unsigned long encodeTime = nodeData->stats.getTotalEncodeTime();
        //unsigned long elapsedTime = nodeData->stats.getElapsedTime();

        int packetsJustSent = handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent, isFullScene);
        packetsSentThisInterval += packetsJustSent;

        // If we're starting a full scene, then definitely we want to empty the elementBag
        if (isFullScene) {
            nodeData->elementBag.deleteAll();
        }

        // TODO: add these to stats page
        //::startSceneSleepTime = _usleepTime;

        nodeData->sceneStart(usecTimestampNow() - CHANGE_FUDGE);
        // start tracking our stats
        nodeData->stats.sceneStarted(isFullScene, viewFrustumChanged,
                                     _myServer->getOctree()->getRoot(), _myServer->getJurisdiction());

        // This is the start of "resending" the scene.
        bool dontRestartSceneOnMove = false; // this is experimental
        if (dontRestartSceneOnMove) {
            if (nodeData->elementBag.isEmpty()) {
                nodeData->elementBag.insert(_myServer->getOctree()->getRoot());
            }
        } else {
            nodeData->elementBag.insert(_myServer->getOctree()->getRoot());
        }
    }

    // If we have something in our elementBag, then turn them into packets and send them out...
    if (!nodeData->elementBag.isEmpty()) {
        int bytesWritten = 0;
        quint64 start = usecTimestampNow();

        // TODO: add these to stats page
        //quint64 startCompressTimeMsecs = OctreePacketData::getCompressContentTime() / 1000;
        //quint64 startCompressCalls = OctreePacketData::getCompressContentCalls();

        int extraPackingAttempts = 0;
        bool completedScene = false;

        while (somethingToSend && packetsSentThisInterval < maxPacketsPerInterval && !nodeData->isShuttingDown()) {
            float lockWaitElapsedUsec = OctreeServer::SKIP_TIME;
            float encodeElapsedUsec = OctreeServer::SKIP_TIME;
            float compressAndWriteElapsedUsec = OctreeServer::SKIP_TIME;
            float packetSendingElapsedUsec = OctreeServer::SKIP_TIME;

            quint64 startInside = usecTimestampNow();

            bool lastNodeDidntFit = false; // assume each node fits
            if (!nodeData->elementBag.isEmpty()) {

                quint64 lockWaitStart = usecTimestampNow();
                _myServer->getOctree()->withReadLock([&]{
                    quint64 lockWaitEnd = usecTimestampNow();
                    lockWaitElapsedUsec = (float)(lockWaitEnd - lockWaitStart);
                    quint64 encodeStart = usecTimestampNow();

                    OctreeElementPointer subTree = nodeData->elementBag.extract();
                    if (!subTree) {
                        return;
                    }

                    float octreeSizeScale = nodeData->getOctreeSizeScale();
                    int boundaryLevelAdjustClient = nodeData->getBoundaryLevelAdjust();

                    int boundaryLevelAdjust = boundaryLevelAdjustClient +
                                              (viewFrustumChanged ? LOW_RES_MOVING_ADJUST : NO_BOUNDARY_ADJUST);

                    EncodeBitstreamParams params(INT_MAX, WANT_EXISTS_BITS, DONT_CHOP,
                                                 viewFrustumChanged,
                                                 boundaryLevelAdjust, octreeSizeScale,
                                                 nodeData->getLastTimeBagEmpty(),
                                                 isFullScene, &nodeData->stats, _myServer->getJurisdiction(),
                                                 &nodeData->extraEncodeData);
                    nodeData->copyCurrentViewFrustum(params.viewFrustum);
                    if (viewFrustumChanged) {
                        nodeData->copyLastKnownViewFrustum(params.lastViewFrustum);
                    }

                    // Our trackSend() function is implemented by the server subclass, and will be called back
                    // during the encodeTreeBitstream() as new entities/data elements are sent
                    params.trackSend = [this, node](const QUuid& dataID, quint64 dataEdited) {
                        _myServer->trackSend(dataID, dataEdited, node->getUUID());
                    };

                    // TODO: should this include the lock time or not? This stat is sent down to the client,
                    // it seems like it may be a good idea to include the lock time as part of the encode time
                    // are reported to client. Since you can encode without the lock
                    nodeData->stats.encodeStarted();

                    bytesWritten = _myServer->getOctree()->encodeTreeBitstream(subTree, &_packetData, nodeData->elementBag, params);

                    quint64 encodeEnd = usecTimestampNow();
                    encodeElapsedUsec = (float)(encodeEnd - encodeStart);

                    // If after calling encodeTreeBitstream() there are no nodes left to send, then we know we've
                    // sent the entire scene. We want to know this below so we'll actually write this content into
                    // the packet and send it
                    completedScene = nodeData->elementBag.isEmpty();

                    if (params.stopReason == EncodeBitstreamParams::DIDNT_FIT) {
                        lastNodeDidntFit = true;
                        extraPackingAttempts++;
                    }

                    nodeData->stats.encodeStopped();
                });
            } else {
                // If the bag was empty then we didn't even attempt to encode, and so we know the bytesWritten were 0
                bytesWritten = 0;
                somethingToSend = false; // this will cause us to drop out of the loop...
            }

            // If the last node didn't fit, but we're in compressed mode, then we actually want to see if we can fit a
            // little bit more in this packet. To do this we write into the packet, but don't send it yet, we'll
            // keep attempting to write in compressed mode to add more compressed segments

            // We only consider sending anything if there is something in the _packetData to send... But
            // if bytesWritten == 0 it means either the subTree couldn't fit or we had an empty bag... Both cases
            // mean we should send the previous packet contents and reset it.
            if (completedScene || lastNodeDidntFit) {

                if (_packetData.hasContent()) {
                    quint64 compressAndWriteStart = usecTimestampNow();

                    // if for some reason the finalized size is greater than our available size, then probably the "compressed"
                    // form actually inflated beyond our padding, and in this case we will send the current packet, then
                    // write to out new packet...
                    unsigned int writtenSize = _packetData.getFinalizedSize() + sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);

                    if (writtenSize > nodeData->getAvailable()) {
                        packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                    }

                    nodeData->writeToPacket(_packetData.getFinalizedData(), _packetData.getFinalizedSize());
                    quint64 compressAndWriteEnd = usecTimestampNow();
                    compressAndWriteElapsedUsec = (float)(compressAndWriteEnd - compressAndWriteStart);
                }

                // If we're not running compressed, then we know we can just send now. Or if we're running compressed, but
                // the packet doesn't have enough space to bother attempting to pack more...
                bool sendNow = true;

                if (!completedScene && (nodeData->getAvailable() >= MINIMUM_ATTEMPT_MORE_PACKING &&
                                        extraPackingAttempts <= REASONABLE_NUMBER_OF_PACKING_ATTEMPTS)) {
                    sendNow = false; // try to pack more
                }

                int targetSize = MAX_OCTREE_PACKET_DATA_SIZE;
                if (sendNow) {
                    quint64 packetSendingStart = usecTimestampNow();
                    packetsSentThisInterval += handlePacketSend(node, nodeData, trueBytesSent, truePacketsSent);
                    quint64 packetSendingEnd = usecTimestampNow();
                    packetSendingElapsedUsec = (float)(packetSendingEnd - packetSendingStart);

                    targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                    extraPackingAttempts = 0;
                } else {
                    // If we're in compressed mode, then we want to see if we have room for more in this wire packet.
                    // but we've finalized the _packetData, so we want to start a new section, we will do that by
                    // resetting the packet settings with the max uncompressed size of our current available space
                    // in the wire packet. We also include room for our section header, and a little bit of padding
                    // to account for the fact that whenc compressing small amounts of data, we sometimes end up with
                    // a larger compressed size then uncompressed size
                    targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE) - COMPRESS_PADDING;
                }
                _packetData.changeSettings(true, targetSize); // will do reset - NOTE: Always compressed

            }
            OctreeServer::trackTreeWaitTime(lockWaitElapsedUsec);
            OctreeServer::trackEncodeTime(encodeElapsedUsec);
            OctreeServer::trackCompressAndWriteTime(compressAndWriteElapsedUsec);
            OctreeServer::trackPacketSendingTime(packetSendingElapsedUsec);

            quint64 endInside = usecTimestampNow();
            quint64 elapsedInsideUsecs = endInside - startInside;
            OctreeServer::trackInsideTime((float)elapsedInsideUsecs);
        }

        if (somethingToSend && _myServer->wantsVerboseDebug()) {
            qCDebug(octree) << "Hit PPS Limit, packetsSentThisInterval =" << packetsSentThisInterval
                            << "  maxPacketsPerInterval = " << maxPacketsPerInterval
                            << "  clientMaxPacketsPerInterval = " << clientMaxPacketsPerInterval;
        }

        // Here's where we can/should allow the server to send other data...
        // send the environment packet
        // TODO: should we turn this into a while loop to better handle sending multiple special packets
        if (_myServer->hasSpecialPacketsToSend(node) && !nodeData->isShuttingDown()) {
            int specialPacketsSent = 0;
            trueBytesSent += _myServer->sendSpecialPackets(node, nodeData, specialPacketsSent);
            nodeData->resetOctreePacket();   // because nodeData's _sequenceNumber has changed
            truePacketsSent += specialPacketsSent;
            packetsSentThisInterval += specialPacketsSent;

            _totalPackets += specialPacketsSent;
            _totalBytes += trueBytesSent;

            _totalSpecialPackets += specialPacketsSent;
            _totalSpecialBytes += trueBytesSent;
        }

        // Re-send packets that were nacked by the client
        while (nodeData->hasNextNackedPacket() && packetsSentThisInterval < maxPacketsPerInterval) {
            const NLPacket* packet = nodeData->getNextNackedPacket();
            if (packet) {
                DependencyManager::get<NodeList>()->sendUnreliablePacket(*packet, *node);
                truePacketsSent++;
                packetsSentThisInterval++;

                _totalBytes += packet->getDataSize();
                _totalPackets++;
                _totalWastedBytes += udt::MAX_PACKET_SIZE - packet->getDataSize();
            }
        }

        quint64 end = usecTimestampNow();
        int elapsedmsec = (end - start) / USECS_PER_MSEC;
        OctreeServer::trackLoopTime(elapsedmsec);

        // TODO: add these to stats page
        //quint64 endCompressCalls = OctreePacketData::getCompressContentCalls();
        //int elapsedCompressCalls = endCompressCalls - startCompressCalls;
        //quint64 endCompressTimeMsecs = OctreePacketData::getCompressContentTime() / 1000;
        //int elapsedCompressTimeMsecs = endCompressTimeMsecs - startCompressTimeMsecs;

        // if after sending packets we've emptied our bag, then we want to remember that we've sent all
        // the octree elements from the current view frustum
        if (nodeData->elementBag.isEmpty()) {
            nodeData->updateLastKnownViewFrustum();
            nodeData->setViewSent(true);

            // If this was a full scene then make sure we really send out a stats packet at this point so that
            // the clients will know the scene is stable
            if (isFullScene) {
                int thisTrueBytesSent = 0;
                int thisTruePacketsSent = 0;
                nodeData->stats.sceneCompleted();
                int packetsJustSent = handlePacketSend(node, nodeData, thisTrueBytesSent, thisTruePacketsSent, true);
                _totalBytes += thisTrueBytesSent;
                _totalPackets += thisTruePacketsSent;
                truePacketsSent += packetsJustSent;
            }
        }

    } // end if bag wasn't empty, and so we sent stuff...

    return truePacketsSent;
}
