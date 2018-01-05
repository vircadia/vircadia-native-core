//
//  OctreeStatsProvider.cpp
//  interface/src/ui
//
//  Created by Vlad Stelmahovsky on 3/12/17.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include "../octree/OctreePacketProcessor.h"
#include "ui/OctreeStatsProvider.h"

OctreeStatsProvider::OctreeStatsProvider(QObject* parent, NodeToOctreeSceneStats* model) :
    QObject(parent),
    _model(model)
  , _statCount(0)
  , _averageUpdatesPerSecond(SAMPLES_PER_SECOND)
{
    //schedule updates
    connect(&_updateTimer, &QTimer::timeout, this, &OctreeStatsProvider::updateOctreeStatsData);
    _updateTimer.setInterval(100);
    //timer will be rescheduled on each new timeout
    _updateTimer.setSingleShot(true);
}

/*
 * Start updates statistics
*/
void OctreeStatsProvider::startUpdates() {
    _updateTimer.start();
}

/*
 * Stop updates statistics
*/
void OctreeStatsProvider::stopUpdates() {
    _updateTimer.stop();
}

QColor OctreeStatsProvider::getColor() const {
    static int statIndex = 1;
    static quint32 rotatingColors[] = { GREENISH, YELLOWISH, GREYISH };
    quint32 colorRGBA = rotatingColors[statIndex % (sizeof(rotatingColors)/sizeof(rotatingColors[0]))];
    quint32 rgb = colorRGBA >> 8;
    const quint32 colorpart1 = 0xfefefeu;
    const quint32 colorpart2 = 0xf8f8f8;
    rgb = ((rgb & colorpart1) >> 1) + ((rgb & colorpart2) >> 3);
    statIndex++;
    return QColor::fromRgb(rgb);
}

OctreeStatsProvider::~OctreeStatsProvider() {
    _updateTimer.stop();
}

int OctreeStatsProvider::serversNum() const {
    return m_serversNum;
}

void OctreeStatsProvider::updateOctreeStatsData() {

    // Processed Entities Related stats
    auto entities = qApp->getEntities();
    auto entitiesTree = entities->getTree();

    // Do this ever paint event... even if we don't update
    auto totalTrackedEdits = entitiesTree->getTotalTrackedEdits();
    
    const quint64 SAMPLING_WINDOW = USECS_PER_SECOND / SAMPLES_PER_SECOND;
    quint64 now = usecTimestampNow();
    quint64 sinceLastWindow = now - _lastWindowAt;
    auto editsInLastWindow = totalTrackedEdits - _lastKnownTrackedEdits;
    float sinceLastWindowInSeconds = (float)sinceLastWindow / (float)USECS_PER_SECOND;
    float recentUpdatesPerSecond = (float)editsInLastWindow / sinceLastWindowInSeconds;
    if (sinceLastWindow > SAMPLING_WINDOW) {
        _averageUpdatesPerSecond.updateAverage(recentUpdatesPerSecond);
        _lastWindowAt = now;
        _lastKnownTrackedEdits = totalTrackedEdits;
    }

    // Only refresh our stats every once in a while, unless asked for realtime
    quint64 REFRESH_AFTER = Menu::getInstance()->isOptionChecked(MenuOption::ShowRealtimeEntityStats) ? 0 : USECS_PER_SECOND;
    quint64 sinceLastRefresh = now - _lastRefresh;
    if (sinceLastRefresh < REFRESH_AFTER) {
        _updateTimer.start((REFRESH_AFTER - sinceLastRefresh)/1000);
        return;
    }
    // Only refresh our stats every once in a while, unless asked for realtime
    //if no realtime, then update once per second. Otherwise consider 60FPS update, ie 16ms interval
    //int updateinterval = Menu::getInstance()->isOptionChecked(MenuOption::ShowRealtimeEntityStats) ? 16 : 1000;
    _updateTimer.start(REFRESH_AFTER/1000);

    const int FLOATING_POINT_PRECISION = 3;

    m_localElementsMemory = QString("Elements RAM: %1MB").arg(OctreeElement::getTotalMemoryUsage() / 1000000.0f, 5, 'f', 4);
    emit localElementsMemoryChanged(m_localElementsMemory);

    // Local Elements
    m_localElements = QString("Total: %1 / Internal: %2 / Leaves: %3").
            arg(OctreeElement::getNodeCount()).
            arg(OctreeElement::getInternalNodeCount()).
            arg(OctreeElement::getLeafNodeCount());
    emit localElementsChanged(m_localElements);

    // iterate all the current octree stats, and list their sending modes, total their octree elements, etc...
    int serverCount = 0;
    int movingServerCount = 0;
    unsigned long totalNodes = 0;
    unsigned long totalInternal = 0;
    unsigned long totalLeaves = 0;

    m_sendingMode.clear();
    NodeToOctreeSceneStats* sceneStats = qApp->getOcteeSceneStats();
    sceneStats->withReadLock([&] {
        for (NodeToOctreeSceneStatsIterator i = sceneStats->begin(); i != sceneStats->end(); i++) {
            //const QUuid& uuid = i->first;
            OctreeSceneStats& stats = i->second;
            serverCount++;

            // calculate server node totals
            totalNodes += stats.getTotalElements();
            totalInternal += stats.getTotalInternal();
            totalLeaves += stats.getTotalLeaves();

            // Sending mode
            if (serverCount > 1) {
                m_sendingMode += ",";
            }
            if (stats.isMoving()) {
                m_sendingMode += "M";
                movingServerCount++;
            } else {
                m_sendingMode += "S";
            }
            if (stats.isFullScene()) {
                m_sendingMode += "F";
            } else {
                m_sendingMode += "p";
            }
        }
    });
    m_sendingMode += QString(" - %1 servers").arg(serverCount);
    if (movingServerCount > 0) {
        m_sendingMode += " <SCENE NOT STABLE> ";
    } else {
        m_sendingMode += " <SCENE STABLE> ";
    }

    emit sendingModeChanged(m_sendingMode);
    
    // Server Elements
    m_serverElements = QString("Total: %1 / Internal: %2 / Leaves: %3").
            arg(totalNodes).arg(totalInternal).arg(totalLeaves);
    emit serverElementsChanged(m_serverElements);


    // Processed Packets Elements
    auto averageElementsPerPacket = entities->getAverageElementsPerPacket();
    auto averageEntitiesPerPacket = entities->getAverageEntitiesPerPacket();

    auto averageElementsPerSecond = entities->getAverageElementsPerSecond();
    auto averageEntitiesPerSecond = entities->getAverageEntitiesPerSecond();

    auto averageWaitLockPerPacket = entities->getAverageWaitLockPerPacket();
    auto averageUncompressPerPacket = entities->getAverageUncompressPerPacket();
    auto averageReadBitstreamPerPacket = entities->getAverageReadBitstreamPerPacket();

    const OctreePacketProcessor& entitiesPacketProcessor =  qApp->getOctreePacketProcessor();

    auto incomingPacketsDepth = entitiesPacketProcessor.packetsToProcessCount();
    auto incomingPPS = entitiesPacketProcessor.getIncomingPPS();
    auto processedPPS = entitiesPacketProcessor.getProcessedPPS();
    auto treeProcessedPPS = entities->getAveragePacketsPerSecond();

    m_processedPackets = QString("Queue Size: %1 Packets / Network IN: %2 PPS / Queue OUT: %3 PPS / Tree IN: %4 PPS")
            .arg(incomingPacketsDepth)
            .arg(incomingPPS, 5, 'f', FLOATING_POINT_PRECISION)
            .arg(processedPPS, 5, 'f', FLOATING_POINT_PRECISION)
            .arg(treeProcessedPPS, 5, 'f', FLOATING_POINT_PRECISION);
    emit processedPacketsChanged(m_processedPackets);

    m_processedPacketsElements = QString("%1 per packet / %2 per second")
            .arg(averageElementsPerPacket, 5, 'f', FLOATING_POINT_PRECISION)
            .arg(averageElementsPerSecond, 5, 'f', FLOATING_POINT_PRECISION);
    emit processedPacketsElementsChanged(m_processedPacketsElements);

    m_processedPacketsEntities = QString("%1 per packet / %2 per second")
            .arg(averageEntitiesPerPacket, 5, 'f', FLOATING_POINT_PRECISION)
            .arg(averageEntitiesPerSecond, 5, 'f', FLOATING_POINT_PRECISION);
    emit processedPacketsEntitiesChanged(m_processedPacketsEntities);

    m_processedPacketsTiming = QString("Lock Wait: %1 (usecs) / Uncompress: %2 (usecs) / Process: %3 (usecs)")
            .arg(averageWaitLockPerPacket)
            .arg(averageUncompressPerPacket)
            .arg(averageReadBitstreamPerPacket);
    emit processedPacketsTimingChanged(m_processedPacketsTiming);

    auto entitiesEditPacketSender = qApp->getEntityEditPacketSender();
    auto outboundPacketsDepth = entitiesEditPacketSender->packetsToSendCount();
    auto outboundQueuedPPS = entitiesEditPacketSender->getLifetimePPSQueued();
    auto outboundSentPPS = entitiesEditPacketSender->getLifetimePPS();

    m_outboundEditPackets = QString("Queue Size: %1 packets / Queued IN: %2 PPS / Sent OUT: %3 PPS")
            .arg(outboundPacketsDepth)
            .arg(outboundQueuedPPS, 5, 'f', FLOATING_POINT_PRECISION)
            .arg(outboundSentPPS, 5, 'f', FLOATING_POINT_PRECISION);
    emit outboundEditPacketsChanged(m_outboundEditPackets);
    
    // Entity Edits update time
    auto averageEditDelta = entitiesTree->getAverageEditDeltas();
    auto maxEditDelta = entitiesTree->getMaxEditDelta();

    m_entityUpdateTime = QString("Average: %1 (usecs) / Max: %2 (usecs)")
            .arg(averageEditDelta)
            .arg(maxEditDelta);
    emit entityUpdateTimeChanged(m_entityUpdateTime);

    // Entity Edits
    auto bytesPerEdit = entitiesTree->getAverageEditBytes();
    
    auto updatesPerSecond = _averageUpdatesPerSecond.getAverage();
    if (updatesPerSecond < 1) {
        updatesPerSecond = 0; // we don't really care about small updates per second so suppress those
    }

    m_entityUpdates = QString("%1 updates per second / %2 total updates / Average Size: %3 bytes")
            .arg(updatesPerSecond, 5, 'f', FLOATING_POINT_PRECISION)
            .arg(totalTrackedEdits)
            .arg(bytesPerEdit);
    emit entityUpdatesChanged(m_entityUpdates);

    updateOctreeServers();
}

void OctreeStatsProvider::updateOctreeServers() {
    int serverCount = 0;

    showOctreeServersOfType(serverCount, NodeType::EntityServer, "Entity",
                            qApp->getEntityServerJurisdictions());
    if (m_serversNum != serverCount) {
        m_serversNum = serverCount;
        emit serversNumChanged(m_serversNum);
    }
}

void OctreeStatsProvider::showOctreeServersOfType(int& serverCount, NodeType_t serverType, const char* serverTypeName,
                                                  NodeToJurisdictionMap& serverJurisdictions) {

    m_servers.clear();

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->eachNode([&](const SharedNodePointer& node) {
        
        // only send to the NodeTypes that are NodeType_t_VOXEL_SERVER
        if (node->getType() == serverType) {
            serverCount++;
            
            QString lesserDetails;
            QString moreDetails;
            QString mostDetails;
            
            if (node->getActiveSocket()) {
                lesserDetails += "active ";
            } else {
                lesserDetails += "inactive ";
            }
            
            QUuid nodeUUID = node->getUUID();
            
            // lookup our nodeUUID in the jurisdiction map, if it's missing then we're
            // missing at least one jurisdiction
            serverJurisdictions.withReadLock([&] {
                if (serverJurisdictions.find(nodeUUID) == serverJurisdictions.end()) {
                    lesserDetails += " unknown jurisdiction ";
                    return;
                }
                const JurisdictionMap& map = serverJurisdictions[nodeUUID];

                auto rootCode = map.getRootOctalCode();

                if (rootCode) {
                    QString rootCodeHex = octalCodeToHexString(rootCode.get());

                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode.get(), rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);
                    lesserDetails += QString(" jurisdiction: %1 [%2, %3, %4: %5]")
                            .arg(rootCodeHex)
                            .arg(rootDetails.x)
                            .arg(rootDetails.y)
                            .arg(rootDetails.z)
                            .arg(rootDetails.s);
                } else {
                    lesserDetails += " jurisdiction has no rootCode";
                } // root code
            });
            
            // now lookup stats details for this server...
            NodeToOctreeSceneStats* sceneStats = qApp->getOcteeSceneStats();
            sceneStats->withReadLock([&] {
                if (sceneStats->find(nodeUUID) != sceneStats->end()) {
                    OctreeSceneStats& stats = sceneStats->at(nodeUUID);

                    float lastFullEncode = stats.getLastFullTotalEncodeTime() / USECS_PER_MSEC;
                    float lastFullSend = stats.getLastFullElapsedTime() / USECS_PER_MSEC;
                    float lastFullSendInSeconds = stats.getLastFullElapsedTime() / USECS_PER_SECOND;
                    float lastFullPackets = stats.getLastFullTotalPackets();
                    float lastFullPPS = lastFullPackets;
                    if (lastFullSendInSeconds > 0) {
                        lastFullPPS = lastFullPackets / lastFullSendInSeconds;
                    }

                    mostDetails += QString("<br/><br/>Last Full Scene... Encode: %1 ms Send: %2 ms Packets: %3 Bytes: %4 Rate: %5 PPS")
                            .arg(lastFullEncode)
                            .arg(lastFullSend)
                            .arg(lastFullPackets)
                            .arg(stats.getLastFullTotalBytes())
                            .arg(lastFullPPS);

                    for (int i = 0; i < OctreeSceneStats::ITEM_COUNT; i++) {
                        OctreeSceneStats::Item item = (OctreeSceneStats::Item)(i);
                        OctreeSceneStats::ItemInfo& itemInfo = stats.getItemInfo(item);
                        mostDetails += QString("<br/> %1 %2")
                                .arg(itemInfo.caption).arg(stats.getItemValue(item));
                    }

                    moreDetails += "<br/>Node UUID: " +nodeUUID.toString() + " ";

                    moreDetails += QString("<br/>Elements: %1 total %2 internal %3 leaves ")
                            .arg(stats.getTotalElements())
                            .arg(stats.getTotalInternal())
                            .arg(stats.getTotalLeaves());

                    const SequenceNumberStats& seqStats = stats.getIncomingOctreeSequenceNumberStats();
                    qint64 clockSkewInUsecs = node->getClockSkewUsec();
                    qint64 clockSkewInMS = clockSkewInUsecs / (qint64)USECS_PER_MSEC;

                    moreDetails += QString("<br/>Incoming Packets: %1/ Lost: %2/ Recovered: %3")
                            .arg(stats.getIncomingPackets())
                            .arg(seqStats.getLost())
                            .arg(seqStats.getRecovered());

                    moreDetails += QString("<br/> Out of Order: %1/ Early: %2/ Late: %3/ Unreasonable: %4")
                            .arg(seqStats.getOutOfOrder())
                            .arg(seqStats.getEarly())
                            .arg(seqStats.getLate())
                            .arg(seqStats.getUnreasonable());

                    moreDetails += QString("<br/> Average Flight Time: %1 msecs")
                            .arg(stats.getIncomingFlightTimeAverage());

                    moreDetails += QString("<br/> Average Ping Time: %1 msecs")
                            .arg(node->getPingMs());

                    moreDetails += QString("<br/> Average Clock Skew: %1 msecs [%2]")
                            .arg(clockSkewInMS)
                            .arg(formatUsecTime(clockSkewInUsecs));


                    moreDetails += QString("<br/>Incoming Bytes: %1 Wasted Bytes: %2")
                            .arg(stats.getIncomingBytes())
                            .arg(stats.getIncomingWastedBytes());
                }
            });
            m_servers.append(lesserDetails);
            m_servers.append(moreDetails);
            m_servers.append(mostDetails);
        } // is VOXEL_SERVER
    });
    emit serversChanged(m_servers);
}


