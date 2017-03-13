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

#include <sstream>

#include <QPalette>
#include <QColor>

#include "Application.h"

#include "../octree/OctreePacketProcessor.h"
#include "ui/OctreeStatsProvider.h"

OctreeStatsProvider::OctreeStatsProvider(QObject* parent, NodeToOctreeSceneStats* model) :
    QObject(parent),
    _model(model)
  , _averageUpdatesPerSecond(SAMPLES_PER_SECOND)
  , _statCount(0)
{
    //schedule updates
    connect(&_updateTimer, &QTimer::timeout, this, &OctreeStatsProvider::updateOctreeStatsData);
    _updateTimer.setInterval(100);
    //timer will be rescheduled on each new timeout
    _updateTimer.setSingleShot(true);
}

void OctreeStatsProvider::moreless(const QString& link) {
    QStringList linkDetails = link.split("-");
    const int COMMAND_ITEM = 0;
    const int SERVER_NUMBER_ITEM = 1;
    QString serverNumberString = linkDetails[SERVER_NUMBER_ITEM];
    QString command = linkDetails[COMMAND_ITEM];
    int serverNumber = serverNumberString.toInt();
    
//    if (command == "more") {
//        _extraServerDetails[serverNumber-1] = MORE;
//    } else if (command == "most") {
//        _extraServerDetails[serverNumber-1] = MOST;
//    } else {
//        _extraServerDetails[serverNumber-1] = LESS;
//    }
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


//int OctreeStatsProvider::AddStatItem(const char* caption, unsigned colorRGBA) {
//    const int STATS_LABEL_WIDTH = 600;
    
//    _statCount++; // increment our current stat count
    
//    if (colorRGBA == 0) {
//        static unsigned rotatingColors[] = { GREENISH, YELLOWISH, GREYISH };
//        colorRGBA = rotatingColors[_statCount % (sizeof(rotatingColors)/sizeof(rotatingColors[0]))];
//    }
//    //QLabel* label = _labels[_statCount] = new QLabel();

//    // Set foreground color to 62.5% brightness of the meter (otherwise will be hard to read on the bright background)
//    //QPalette palette = label->palette();
    
//    // This goofiness came from the bandwidth meter code, it basically stores a color in an unsigned and extracts it
//    unsigned rgb = colorRGBA >> 8;
//    const unsigned colorpart1 = 0xfefefeu;
//    const unsigned colorpart2 = 0xf8f8f8;
//    rgb = ((rgb & colorpart1) >> 1) + ((rgb & colorpart2) >> 3);
//    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
//    label->setPalette(palette);
//    _form->addRow(QString(" %1:").arg(caption), label);
//    label->setFixedWidth(STATS_LABEL_WIDTH);
    
//    return _statCount;
//}

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
    _updateTimer.start(SAMPLING_WINDOW/1000);

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
    //qDebug() << "vladest: octree servers:" << serverCount;
    if (m_serversNum != serverCount) {
        m_serversNum = serverCount;
        emit serversNumChanged(m_serversNum);
    }
}

void OctreeStatsProvider::showOctreeServersOfType(int& serverCount, NodeType_t serverType, const char* serverTypeName,
                                                NodeToJurisdictionMap& serverJurisdictions) {
                                                
    QLocale locale(QLocale::English);
    
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->eachNode([&](const SharedNodePointer& node){
        
        // only send to the NodeTypes that are NodeType_t_VOXEL_SERVER
        if (node->getType() == serverType) {
            serverCount++;
            
//            if (serverCount > _octreeServerLabelsCount) {
//                QString label = QString("%1 Server %2").arg(serverTypeName).arg(serverCount);
//                //int thisServerRow = _octreeServerLables[serverCount-1] = AddStatItem(label.toUtf8().constData());
////                _labels[thisServerRow]->setTextFormat(Qt::RichText);
////                _labels[thisServerRow]->setTextInteractionFlags(Qt::TextBrowserInteraction);
////                connect(_labels[thisServerRow], SIGNAL(linkActivated(const QString&)), this, SLOT(moreless(const QString&)));
//                _octreeServerLabelsCount++;
//            }
            
            std::stringstream serverDetails("");
            std::stringstream extraDetails("");
            std::stringstream linkDetails("");
            
            if (node->getActiveSocket()) {
                serverDetails << "active ";
            } else {
                serverDetails << "inactive ";
            }
            
            QUuid nodeUUID = node->getUUID();
            
            // lookup our nodeUUID in the jurisdiction map, if it's missing then we're
            // missing at least one jurisdiction
            serverJurisdictions.withReadLock([&] {
                if (serverJurisdictions.find(nodeUUID) == serverJurisdictions.end()) {
                    serverDetails << " unknown jurisdiction ";
                    return;
                } 
                const JurisdictionMap& map = serverJurisdictions[nodeUUID];

                auto rootCode = map.getRootOctalCode();

                if (rootCode) {
                    QString rootCodeHex = octalCodeToHexString(rootCode.get());

                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode.get(), rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);
                    serverDetails << " jurisdiction: "
                        << qPrintable(rootCodeHex)
                        << " ["
                        << rootDetails.x << ", "
                        << rootDetails.y << ", "
                        << rootDetails.z << ": "
                        << rootDetails.s << "] ";
                } else {
                    serverDetails << " jurisdiction has no rootCode";
                } // root code
            });
            
            // now lookup stats details for this server...
            if (/*_extraServerDetails[serverCount-1] != LESS*/true) {
                NodeToOctreeSceneStats* sceneStats = qApp->getOcteeSceneStats();
                sceneStats->withReadLock([&] {
                    if (sceneStats->find(nodeUUID) != sceneStats->end()) {
                        OctreeSceneStats& stats = sceneStats->at(nodeUUID);
/*
                        switch (_extraServerDetails[serverCount - 1]) {
                        case MOST: {
                            extraDetails << "<br/>";

                            float lastFullEncode = stats.getLastFullTotalEncodeTime() / USECS_PER_MSEC;
                            float lastFullSend = stats.getLastFullElapsedTime() / USECS_PER_MSEC;
                            float lastFullSendInSeconds = stats.getLastFullElapsedTime() / USECS_PER_SECOND;
                            float lastFullPackets = stats.getLastFullTotalPackets();
                            float lastFullPPS = lastFullPackets;
                            if (lastFullSendInSeconds > 0) {
                                lastFullPPS = lastFullPackets / lastFullSendInSeconds;
                            }

                            QString lastFullEncodeString = locale.toString(lastFullEncode);
                            QString lastFullSendString = locale.toString(lastFullSend);
                            QString lastFullPacketsString = locale.toString(lastFullPackets);
                            QString lastFullBytesString = locale.toString((uint)stats.getLastFullTotalBytes());
                            QString lastFullPPSString = locale.toString(lastFullPPS);

                            extraDetails << "<br/>" << "Last Full Scene... " <<
                                "Encode: " << qPrintable(lastFullEncodeString) << " ms " <<
                                "Send: " << qPrintable(lastFullSendString) << " ms " <<
                                "Packets: " << qPrintable(lastFullPacketsString) << " " <<
                                "Bytes: " << qPrintable(lastFullBytesString) << " " <<
                                "Rate: " << qPrintable(lastFullPPSString) << " PPS";

                            for (int i = 0; i < OctreeSceneStats::ITEM_COUNT; i++) {
                                OctreeSceneStats::Item item = (OctreeSceneStats::Item)(i);
                                OctreeSceneStats::ItemInfo& itemInfo = stats.getItemInfo(item);
                                extraDetails << "<br/>" << itemInfo.caption << " " << stats.getItemValue(item);
                            }
                        } // fall through... since MOST has all of MORE
                        case MORE: {
                            QString totalString = locale.toString((uint)stats.getTotalElements());
                            QString internalString = locale.toString((uint)stats.getTotalInternal());
                            QString leavesString = locale.toString((uint)stats.getTotalLeaves());

                            serverDetails << "<br/>" << "Node UUID: " << qPrintable(nodeUUID.toString()) << " ";

                            serverDetails << "<br/>" << "Elements: " <<
                                qPrintable(totalString) << " total " <<
                                qPrintable(internalString) << " internal " <<
                                qPrintable(leavesString) << " leaves ";

                            QString incomingPacketsString = locale.toString((uint)stats.getIncomingPackets());
                            QString incomingBytesString = locale.toString((uint)stats.getIncomingBytes());
                            QString incomingWastedBytesString = locale.toString((uint)stats.getIncomingWastedBytes());
                            const SequenceNumberStats& seqStats = stats.getIncomingOctreeSequenceNumberStats();
                            QString incomingOutOfOrderString = locale.toString((uint)seqStats.getOutOfOrder());
                            QString incomingLateString = locale.toString((uint)seqStats.getLate());
                            QString incomingUnreasonableString = locale.toString((uint)seqStats.getUnreasonable());
                            QString incomingEarlyString = locale.toString((uint)seqStats.getEarly());
                            QString incomingLikelyLostString = locale.toString((uint)seqStats.getLost());
                            QString incomingRecovered = locale.toString((uint)seqStats.getRecovered());

                            qint64 clockSkewInUsecs = node->getClockSkewUsec();
                            QString formattedClockSkewString = formatUsecTime(clockSkewInUsecs);
                            qint64 clockSkewInMS = clockSkewInUsecs / (qint64)USECS_PER_MSEC;
                            QString incomingFlightTimeString = locale.toString((int)stats.getIncomingFlightTimeAverage());
                            QString incomingPingTimeString = locale.toString(node->getPingMs());
                            QString incomingClockSkewString = locale.toString(clockSkewInMS);

                            serverDetails << "<br/>" << "Incoming Packets: " << qPrintable(incomingPacketsString) <<
                                "/ Lost: " << qPrintable(incomingLikelyLostString) <<
                                "/ Recovered: " << qPrintable(incomingRecovered);

                            serverDetails << "<br/>" << " Out of Order: " << qPrintable(incomingOutOfOrderString) <<
                                "/ Early: " << qPrintable(incomingEarlyString) <<
                                "/ Late: " << qPrintable(incomingLateString) <<
                                "/ Unreasonable: " << qPrintable(incomingUnreasonableString);

                            serverDetails << "<br/>" <<
                                " Average Flight Time: " << qPrintable(incomingFlightTimeString) << " msecs";

                            serverDetails << "<br/>" <<
                                " Average Ping Time: " << qPrintable(incomingPingTimeString) << " msecs";

                            serverDetails << "<br/>" <<
                                " Average Clock Skew: " << qPrintable(incomingClockSkewString) << " msecs" << 
                                " [" << qPrintable(formattedClockSkewString) << "]";


                            serverDetails << "<br/>" << "Incoming" <<
                                " Bytes: " << qPrintable(incomingBytesString) <<
                                " Wasted Bytes: " << qPrintable(incomingWastedBytesString);

                            serverDetails << extraDetails.str();
                            if (_extraServerDetails[serverCount - 1] == MORE) {
                                linkDetails << "   " << " [<a href='most-" << serverCount << "'>most...</a>]";
                                linkDetails << "   " << " [<a href='less-" << serverCount << "'>less...</a>]";
                            } else {
                                linkDetails << "   " << " [<a href='more-" << serverCount << "'>less...</a>]";
                                linkDetails << "   " << " [<a href='less-" << serverCount << "'>least...</a>]";
                            }

                        } break;
                        case LESS: {
                            // nothing
                        } break;
                        }*/
                    }
                });
            } else {
                linkDetails << "   " << " [<a href='more-" << serverCount << "'>more...</a>]";
                linkDetails << "   " << " [<a href='most-" << serverCount << "'>most...</a>]";
            }
            serverDetails << linkDetails.str();
            //_labels[_octreeServerLables[serverCount - 1]]->setText(serverDetails.str().c_str());
        } // is VOXEL_SERVER
    });
}


