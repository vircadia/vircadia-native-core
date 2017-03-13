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
  //, _averageUpdatesPerSecond(SAMPLES_PER_SECOND)
{
    
    _statCount = 0;
//    _octreeServerLabelsCount = 0;

//    for (int i = 0; i < MAX_VOXEL_SERVERS; i++) {
//        _octreeServerLables[i] = 0;
//    }

    // Setup stat items
//    _serverElements = AddStatItem("Elements on Servers");
    _localElements = AddStatItem("Local Elements");
    _localElementsMemory = AddStatItem("Elements Memory");
    _sendingMode = AddStatItem("Sending Mode");

    _processedPackets = AddStatItem("Incoming Entity Packets");
    _processedPacketsElements = AddStatItem("Processed Packets Elements");
    _processedPacketsEntities = AddStatItem("Processed Packets Entities");
    _processedPacketsTiming = AddStatItem("Processed Packets Timing");

    _outboundEditPackets = AddStatItem("Outbound Entity Packets");

    _entityUpdateTime = AddStatItem("Entity Update Time");
    _entityUpdates = AddStatItem("Entity Updates");
    
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


int OctreeStatsProvider::AddStatItem(const char* caption, unsigned colorRGBA) {
    const int STATS_LABEL_WIDTH = 600;
    
    _statCount++; // increment our current stat count
    
    if (colorRGBA == 0) {
        static unsigned rotatingColors[] = { GREENISH, YELLOWISH, GREYISH };
        colorRGBA = rotatingColors[_statCount % (sizeof(rotatingColors)/sizeof(rotatingColors[0]))];
    }
    //QLabel* label = _labels[_statCount] = new QLabel();

    // Set foreground color to 62.5% brightness of the meter (otherwise will be hard to read on the bright background)
    //QPalette palette = label->palette();
    
    // This goofiness came from the bandwidth meter code, it basically stores a color in an unsigned and extracts it
    unsigned rgb = colorRGBA >> 8;
    const unsigned colorpart1 = 0xfefefeu;
    const unsigned colorpart2 = 0xf8f8f8;
    rgb = ((rgb & colorpart1) >> 1) + ((rgb & colorpart2) >> 3);
//    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
//    label->setPalette(palette);
//    _form->addRow(QString(" %1:").arg(caption), label);
//    label->setFixedWidth(STATS_LABEL_WIDTH);
    
    return _statCount;
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
    

    // Only refresh our stats every once in a while, unless asked for realtime
    //if no realtime, then update once per second. Otherwise consider 60FPS update, ie 16ms interval
    int updateinterval = Menu::getInstance()->isOptionChecked(MenuOption::ShowRealtimeEntityStats) ? 16 : 1000;
    _updateTimer.start(updateinterval);

    const int FLOATING_POINT_PRECISION = 3;

    // Update labels

    //QLabel* label;
    QLocale locale(QLocale::English);
    std::stringstream statsValue;
//    statsValue.precision(4);

    // Octree Elements Memory Usage
    //label = _labels[_localElementsMemory];
//    statsValue.str("");
//    statsValue << "Elements RAM: " << OctreeElement::getTotalMemoryUsage() / 1000000.0f << "MB ";
    //label->setText(statsValue.str().c_str());
    m_localElementsMemory = QString("Elements RAM: %1MB").arg(OctreeElement::getTotalMemoryUsage() / 1000000.0f, 5, 'f', 4);
    emit localElementsMemoryChanged(m_localElementsMemory);
    // Local Elements
    //label = _labels[_localElements];
//    unsigned long localTotal = OctreeElement::getNodeCount();
//    unsigned long localInternal = OctreeElement::getInternalNodeCount();
//    unsigned long localLeaves = OctreeElement::getLeafNodeCount();
//    QString localTotalString = locale.toString((uint)localTotal); // consider adding: .rightJustified(10, ' ');
//    QString localInternalString = locale.toString((uint)localInternal);
//    QString localLeavesString = locale.toString((uint)localLeaves);

//    statsValue.str("");
//    statsValue <<
//        "Total: " << qPrintable(localTotalString) << " / " <<
//        "Internal: " << qPrintable(localInternalString) << " / " <<
//        "Leaves: " << qPrintable(localLeavesString) << "";

    m_localElements = QString("Total: %1 / Internal: %2 / Leaves: %3").
            arg(OctreeElement::getNodeCount()).
            arg(OctreeElement::getInternalNodeCount()).
            arg(OctreeElement::getLeafNodeCount());
    emit localElementsChanged(m_localElements);

    //label->setText(statsValue.str().c_str());

    // iterate all the current octree stats, and list their sending modes, total their octree elements, etc...
    std::stringstream sendingMode("");

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
                sendingMode << ",";
            }
            if (stats.isMoving()) {
                sendingMode << "M";
                movingServerCount++;
            } else {
                sendingMode << "S";
            }
            if (stats.isFullScene()) {
                sendingMode << "F";
            } else {
                sendingMode << "p";
            }
        }
    });
    sendingMode << " - " << serverCount << " servers";
    if (movingServerCount > 0) {
        sendingMode << " <SCENE NOT STABLE>";
    } else {
        sendingMode << " <SCENE STABLE>";
    }

//    label = _labels[_sendingMode];
//    label->setText(sendingMode.str().c_str());
    
    // Server Elements
    m_serverElements = QString("Total: %1/ Internal: %2/ Leaves: %3").
            arg(totalNodes).arg(totalInternal).arg(totalLeaves);
    emit serverElementsChanged(m_serverElements);
//    QString serversTotalString = locale.toString((uint)totalNodes); // consider adding: .rightJustified(10, ' ');
//    QString serversInternalString = locale.toString((uint)totalInternal);
//    QString serversLeavesString = locale.toString((uint)totalLeaves);
////    label = _labels[_serverElements];
//    statsValue.str("");
//    statsValue <<
//        "Total: " << qPrintable(serversTotalString) << " / " <<
//        "Internal: " << qPrintable(serversInternalString) << " / " <<
//        "Leaves: " << qPrintable(serversLeavesString) << "";
////    label->setText(statsValue.str().c_str());

    // Processed Packets Elements
    auto averageElementsPerPacket = entities->getAverageElementsPerPacket();
    auto averageEntitiesPerPacket = entities->getAverageEntitiesPerPacket();

    auto averageElementsPerSecond = entities->getAverageElementsPerSecond();
    auto averageEntitiesPerSecond = entities->getAverageEntitiesPerSecond();

    auto averageWaitLockPerPacket = entities->getAverageWaitLockPerPacket();
    auto averageUncompressPerPacket = entities->getAverageUncompressPerPacket();
    auto averageReadBitstreamPerPacket = entities->getAverageReadBitstreamPerPacket();

    QString averageElementsPerPacketString = locale.toString(averageElementsPerPacket, 'f', FLOATING_POINT_PRECISION);
    QString averageEntitiesPerPacketString = locale.toString(averageEntitiesPerPacket, 'f', FLOATING_POINT_PRECISION);

    QString averageElementsPerSecondString = locale.toString(averageElementsPerSecond, 'f', FLOATING_POINT_PRECISION);
    QString averageEntitiesPerSecondString = locale.toString(averageEntitiesPerSecond, 'f', FLOATING_POINT_PRECISION);

    QString averageWaitLockPerPacketString = locale.toString(averageWaitLockPerPacket);
    QString averageUncompressPerPacketString = locale.toString(averageUncompressPerPacket);
    QString averageReadBitstreamPerPacketString = locale.toString(averageReadBitstreamPerPacket);

//    label = _labels[_processedPackets];
    const OctreePacketProcessor& entitiesPacketProcessor =  qApp->getOctreePacketProcessor();

    auto incomingPacketsDepth = entitiesPacketProcessor.packetsToProcessCount();
    auto incomingPPS = entitiesPacketProcessor.getIncomingPPS();
    auto processedPPS = entitiesPacketProcessor.getProcessedPPS();
    auto treeProcessedPPS = entities->getAveragePacketsPerSecond();

    QString incomingPPSString = locale.toString(incomingPPS, 'f', FLOATING_POINT_PRECISION);
    QString processedPPSString = locale.toString(processedPPS, 'f', FLOATING_POINT_PRECISION);
    QString treeProcessedPPSString = locale.toString(treeProcessedPPS, 'f', FLOATING_POINT_PRECISION);

    statsValue.str("");
    statsValue << 
        "Queue Size: " << incomingPacketsDepth << " Packets / " <<
        "Network IN: " << qPrintable(incomingPPSString) << " PPS / " <<
        "Queue OUT: " << qPrintable(processedPPSString) << " PPS / " <<
        "Tree IN: " << qPrintable(treeProcessedPPSString) << " PPS";
        
//    label->setText(statsValue.str().c_str());

//    label = _labels[_processedPacketsElements];
    statsValue.str("");
    statsValue << 
        "" << qPrintable(averageElementsPerPacketString) << " per packet / " <<
        "" << qPrintable(averageElementsPerSecondString) << " per second";
        
//    label->setText(statsValue.str().c_str());

//    label = _labels[_processedPacketsEntities];
    statsValue.str("");
    statsValue << 
        "" << qPrintable(averageEntitiesPerPacketString) << " per packet / " <<
        "" << qPrintable(averageEntitiesPerSecondString) << " per second";
        
//    label->setText(statsValue.str().c_str());

//    label = _labels[_processedPacketsTiming];
    statsValue.str("");
    statsValue << 
        "Lock Wait: " << qPrintable(averageWaitLockPerPacketString) << " (usecs) / " <<
        "Uncompress: " << qPrintable(averageUncompressPerPacketString) << " (usecs) / " <<
        "Process: " << qPrintable(averageReadBitstreamPerPacketString) << " (usecs)";
        
    //label->setText(statsValue.str().c_str());

    auto entitiesEditPacketSender = qApp->getEntityEditPacketSender();

    auto outboundPacketsDepth = entitiesEditPacketSender->packetsToSendCount();
    auto outboundQueuedPPS = entitiesEditPacketSender->getLifetimePPSQueued();
    auto outboundSentPPS = entitiesEditPacketSender->getLifetimePPS();

    QString outboundQueuedPPSString = locale.toString(outboundQueuedPPS, 'f', FLOATING_POINT_PRECISION);
    QString outboundSentPPSString = locale.toString(outboundSentPPS, 'f', FLOATING_POINT_PRECISION);

    //label = _labels[_outboundEditPackets];
    statsValue.str("");
    statsValue <<
        "Queue Size: " << outboundPacketsDepth << " packets / " <<
        "Queued IN: " << qPrintable(outboundQueuedPPSString) << " PPS / " <<
        "Sent OUT: " << qPrintable(outboundSentPPSString) << " PPS";

    //label->setText(statsValue.str().c_str());

    
    // Entity Edits update time
    //label = _labels[_entityUpdateTime];
    auto averageEditDelta = entitiesTree->getAverageEditDeltas();
    auto maxEditDelta = entitiesTree->getMaxEditDelta();

    QString averageEditDeltaString = locale.toString((uint)averageEditDelta);
    QString maxEditDeltaString = locale.toString((uint)maxEditDelta);

    statsValue.str("");
    statsValue << 
        "Average: " << qPrintable(averageEditDeltaString) << " (usecs) / " <<
        "Max: " << qPrintable(maxEditDeltaString) << " (usecs)";
        
    //label->setText(statsValue.str().c_str());

    // Entity Edits
    //label = _labels[_entityUpdates];
    auto bytesPerEdit = entitiesTree->getAverageEditBytes();
    
    auto updatesPerSecond = _averageUpdatesPerSecond.getAverage();
    if (updatesPerSecond < 1) {
        updatesPerSecond = 0; // we don't really care about small updates per second so suppress those
    }

    QString totalTrackedEditsString = locale.toString((uint)totalTrackedEdits);
    QString updatesPerSecondString = locale.toString(updatesPerSecond, 'f', FLOATING_POINT_PRECISION);
    QString bytesPerEditString = locale.toString(bytesPerEdit);

    statsValue.str("");
    statsValue << 
        "" << qPrintable(updatesPerSecondString) << " updates per second / " <<
        "" << qPrintable(totalTrackedEditsString) << " total updates / " <<
        "Average Size: " << qPrintable(bytesPerEditString) << " bytes ";
        
    //label->setText(statsValue.str().c_str());

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


