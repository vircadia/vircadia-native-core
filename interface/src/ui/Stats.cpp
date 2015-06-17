//
//  Stats.cpp
//  interface/src/ui
//
//  Created by Lucas Crisman on 22/03/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <gpu/GPUConfig.h>

#include "Stats.h"

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <avatar/AvatarManager.h>
#include <Application.h>
#include <GeometryCache.h>
#include <GLCanvas.h>
#include <LODManager.h>
#include <PerfStat.h>

#include "BandwidthRecorder.h"
#include "InterfaceConfig.h"
#include "Menu.h"
#include "Util.h"
#include "SequenceNumberStats.h"

HIFI_QML_DEF(Stats)

using namespace std;

static Stats* INSTANCE{ nullptr };

Stats* Stats::getInstance() {
    if (!INSTANCE) {
        Stats::registerType();
        Stats::show();
        //Stats::toggle();
        Q_ASSERT(INSTANCE);
    }
    return INSTANCE;
}

Stats::Stats(QQuickItem* parent) :  QQuickItem(parent) {
    INSTANCE = this;
}

bool Stats::includeTimingRecord(const QString& name) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails)) {
        if (name.startsWith("/idle/update/")) {
            if (name.startsWith("/idle/update/myAvatar/")) {
                if (name.startsWith("/idle/update/myAvatar/simulate/")) {
                    return Menu::getInstance()->isOptionChecked(MenuOption::ExpandMyAvatarSimulateTiming);
                }
                return Menu::getInstance()->isOptionChecked(MenuOption::ExpandMyAvatarTiming);
            } else if (name.startsWith("/idle/update/otherAvatars/")) {
                return Menu::getInstance()->isOptionChecked(MenuOption::ExpandOtherAvatarTiming);
            }
            return Menu::getInstance()->isOptionChecked(MenuOption::ExpandUpdateTiming);
        } else if (name.startsWith("/idle/updateGL/paintGL/")) {
            return Menu::getInstance()->isOptionChecked(MenuOption::ExpandPaintGLTiming);
        } else if (name.startsWith("/paintGL/")) {
            return Menu::getInstance()->isOptionChecked(MenuOption::ExpandPaintGLTiming);
        }
        return true;
    }
    return false;
}

#define STAT_UPDATE(name, src) \
    { \
        auto val = src; \
        if (_##name != val) { \
            _##name = val; \
            emit name##Changed(); \
        } \
    }

#define STAT_UPDATE_FLOAT(name, src, epsilon) \
    { \
        float val = src; \
        if (abs(_##name - val) >= epsilon) { \
            _##name = val; \
            emit name##Changed(); \
        } \
    }


void Stats::updateStats() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
        if (isVisible()) {
            setVisible(false);
        }
        return;
    } else {
        if (!isVisible()) {
            setVisible(true);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    auto avatarManager = DependencyManager::get<AvatarManager>();
    // we need to take one avatar out so we don't include ourselves
    STAT_UPDATE(avatarCount, avatarManager->size() - 1);
    STAT_UPDATE(serverCount, nodeList->size());
    STAT_UPDATE(framerate, (int)qApp->getFps());

    auto bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    STAT_UPDATE(packetInCount, bandwidthRecorder->getCachedTotalAverageInputPacketsPerSecond());
    STAT_UPDATE(packetOutCount, bandwidthRecorder->getCachedTotalAverageOutputPacketsPerSecond());
    STAT_UPDATE_FLOAT(mbpsIn, (float)bandwidthRecorder->getCachedTotalAverageOutputKilobitsPerSecond() / 1000.0f, 0.01f);
    STAT_UPDATE_FLOAT(mbpsOut, (float)bandwidthRecorder->getCachedTotalAverageInputKilobitsPerSecond() / 1000.0f, 0.01f);

    // Second column: ping
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
        SharedNodePointer avatarMixerNode = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        STAT_UPDATE(audioPing, audioMixerNode ? audioMixerNode->getPingMs() : -1);
        STAT_UPDATE(avatarPing, avatarMixerNode ? avatarMixerNode->getPingMs() : -1);

        //// Now handle voxel servers, since there could be more than one, we average their ping times
        unsigned long totalPingOctree = 0;
        int octreeServerCount = 0;
        int pingOctreeMax = 0;
        int pingVoxel;
        nodeList->eachNode([&](const SharedNodePointer& node) {
            // TODO: this should also support entities
            if (node->getType() == NodeType::EntityServer) {
                totalPingOctree += node->getPingMs();
                octreeServerCount++;
                if (pingOctreeMax < node->getPingMs()) {
                    pingOctreeMax = node->getPingMs();
                }
            }
        });

        if (octreeServerCount) {
            pingVoxel = totalPingOctree / octreeServerCount;
        }

        STAT_UPDATE(entitiesPing, pingVoxel);
        //if (_expanded) {
        //    QString voxelMaxPing;
        //    if (pingVoxel >= 0) {  // Average is only meaningful if pingVoxel is valid.
        //        voxelMaxPing = QString("Voxel max ping: %1").arg(pingOctreeMax);
        //    } else {
        //        voxelMaxPing = QString("Voxel max ping: --");
        //    }
    } else {
        // -2 causes the QML to hide the ping column
        STAT_UPDATE(audioPing, -2);
    }
    

    // Third column, avatar stats
    MyAvatar* myAvatar = avatarManager->getMyAvatar();
    glm::vec3 avatarPos = myAvatar->getPosition();
    STAT_UPDATE(position, QVector3D(avatarPos.x, avatarPos.y, avatarPos.z));
    STAT_UPDATE_FLOAT(velocity, glm::length(myAvatar->getVelocity()), 0.1f);
    STAT_UPDATE_FLOAT(yaw, myAvatar->getBodyYaw(), 0.1f);
    if (_expanded) {
        SharedNodePointer avatarMixer = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        if (avatarMixer) {
            STAT_UPDATE(avatarMixerKbps, roundf(
                bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AvatarMixer) +
                bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AvatarMixer)));
            STAT_UPDATE(avatarMixerPps, roundf(
                bandwidthRecorder->getAverageInputPacketsPerSecond(NodeType::AvatarMixer) +
                bandwidthRecorder->getAverageOutputPacketsPerSecond(NodeType::AvatarMixer)));
        } else {
            STAT_UPDATE(avatarMixerKbps, -1);
            STAT_UPDATE(avatarMixerPps, -1);
        }
        SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
        if (audioMixerNode) {
            STAT_UPDATE(audioMixerKbps, roundf(
                bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AudioMixer) +
                bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AudioMixer)));
            STAT_UPDATE(audioMixerPps, roundf(
                bandwidthRecorder->getAverageInputPacketsPerSecond(NodeType::AudioMixer) +
                bandwidthRecorder->getAverageOutputPacketsPerSecond(NodeType::AudioMixer)));
        } else {
            STAT_UPDATE(audioMixerKbps, -1);
            STAT_UPDATE(audioMixerPps, -1);
        }

        STAT_UPDATE(downloads, ResourceCache::getLoadingRequests().size());
        STAT_UPDATE(downloadsPending, ResourceCache::getPendingRequestCount());
        // TODO fix to match original behavior
        //stringstream downloads;
        //downloads << "Downloads: ";
        //foreach(Resource* resource, ) {
        //    downloads << (int)(resource->getProgress() * 100.0f) << "% ";
        //}
        //downloads << "(" <<  << " pending)";
    } // expanded avatar column

    // Fourth column, octree stats
}

void Stats::setRenderDetails(const RenderDetails& details) {
    //STATS_PROPERTY(int, triangles, 0)
    //STATS_PROPERTY(int, quads, 0)
    //STATS_PROPERTY(int, materialSwitches, 0)
    //STATS_PROPERTY(int, meshOpaque, 0)
    //STATS_PROPERTY(int, meshTranslucent, 0)
    //STATS_PROPERTY(int, opaqueConsidered, 0)
    //STATS_PROPERTY(int, opaqueOutOfView, 0)
    //STATS_PROPERTY(int, opaqueTooSmall, 0)
    //STATS_PROPERTY(int, translucentConsidered, 0)
    //STATS_PROPERTY(int, translucentOutOfView, 0)
    //STATS_PROPERTY(int, translucentTooSmall, 0)
    //STATS_PROPERTY(int, octreeElementsServer, 0)
    //STATS_PROPERTY(int, octreeElementsLocal, 0)
    STAT_UPDATE(triangles, details._trianglesRendered);
    STAT_UPDATE(quads, details._quadsRendered);
    STAT_UPDATE(materialSwitches, details._materialSwitches);
    if (_expanded) {
        STAT_UPDATE(meshOpaque, details._opaque._rendered);
        STAT_UPDATE(meshTranslucent, details._opaque._rendered);
        STAT_UPDATE(opaqueConsidered, details._opaque._considered);
        STAT_UPDATE(opaqueOutOfView, details._opaque._outOfView);
        STAT_UPDATE(opaqueTooSmall, details._opaque._tooSmall);
        STAT_UPDATE(translucentConsidered, details._translucent._considered);
        STAT_UPDATE(translucentOutOfView, details._translucent._outOfView);
        STAT_UPDATE(translucentTooSmall, details._translucent._tooSmall);
    }
}


/*
// display expanded or contracted stats
void Stats::display(
        int voxelPacketsToProcess) 
{
    // iterate all the current voxel stats, and list their sending modes, and total voxel counts
    std::stringstream sendingMode("");
    sendingMode << "Octree Sending Mode: [";
    int serverCount = 0;
    int movingServerCount = 0;
    unsigned long totalNodes = 0;
    unsigned long totalInternal = 0;
    unsigned long totalLeaves = 0;
    NodeToOctreeSceneStats* octreeServerSceneStats = Application::getInstance()->getOcteeSceneStats();
    for(NodeToOctreeSceneStatsIterator i = octreeServerSceneStats->begin(); i != octreeServerSceneStats->end(); i++) {
        //const QUuid& uuid = i->first;
        OctreeSceneStats& stats = i->second;
        serverCount++;
        if (_expanded) {
            if (serverCount > 1) {
                sendingMode << ",";
            }
            if (stats.isMoving()) {
                sendingMode << "M";
                movingServerCount++;
            } else {
                sendingMode << "S";
            }
        }

        // calculate server node totals
        totalNodes += stats.getTotalElements();
        if (_expanded) {
            totalInternal += stats.getTotalInternal();
            totalLeaves += stats.getTotalLeaves();                
        }
    }
    if (_expanded) {
        if (serverCount == 0) {
            sendingMode << "---";
        }
        sendingMode << "] " << serverCount << " servers";
        if (movingServerCount > 0) {
            sendingMode << " <SCENE NOT STABLE>";
        } else {
            sendingMode << " <SCENE STABLE>";
        }
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)sendingMode.str().c_str(), color);
    }

    // Incoming packets
    if (_expanded) {
        octreeStats.str("");
        QString packetsString = locale.toString((int)voxelPacketsToProcess);
        QString maxString = locale.toString((int)_recentMaxPackets);
        octreeStats << "Octree Packets to Process: " << qPrintable(packetsString)
                    << " [Recent Max: " << qPrintable(maxString) << "]";        
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
    }

    if (_resetRecentMaxPacketsSoon && voxelPacketsToProcess > 0) {
        _recentMaxPackets = 0;
        _resetRecentMaxPacketsSoon = false;
    }
    if (voxelPacketsToProcess == 0) {
        _resetRecentMaxPacketsSoon = true;
    } else {
        if (voxelPacketsToProcess > _recentMaxPackets) {
            _recentMaxPackets = voxelPacketsToProcess;
        }
    }

    QString serversTotalString = locale.toString((uint)totalNodes); // consider adding: .rightJustified(10, ' ');
    unsigned long localTotal = OctreeElement::getNodeCount();
    QString localTotalString = locale.toString((uint)localTotal); // consider adding: .rightJustified(10, ' ');

    // Server Octree Elements
    if (!_expanded) {
        octreeStats.str("");
        octreeStats << "Octree Elements Server: " << qPrintable(serversTotalString)
                        << " Local:" << qPrintable(localTotalString);
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
    }

    if (_expanded) {
        octreeStats.str("");
        octreeStats << "Octree Elements -";
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);

        QString serversInternalString = locale.toString((uint)totalInternal);
        QString serversLeavesString = locale.toString((uint)totalLeaves);

        octreeStats.str("");
        octreeStats << "  Server: " << qPrintable(serversTotalString) <<
            " Internal: " << qPrintable(serversInternalString) <<
            " Leaves: " << qPrintable(serversLeavesString);
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);

        // Local Voxels
        unsigned long localInternal = OctreeElement::getInternalNodeCount();
        unsigned long localLeaves = OctreeElement::getLeafNodeCount();
        QString localInternalString = locale.toString((uint)localInternal);
        QString localLeavesString = locale.toString((uint)localLeaves);

        octreeStats.str("");
        octreeStats << "  Local: " << qPrintable(serversTotalString) <<
            " Internal: " << qPrintable(localInternalString) <<
            " Leaves: " << qPrintable(localLeavesString) << "";
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
    }

    // LOD Details
    octreeStats.str("");
    QString displayLODDetails = DependencyManager::get<LODManager>()->getLODFeedbackText();
    octreeStats << "LOD: You can see " << qPrintable(displayLODDetails.trimmed());
    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
}


//// Performance timer


    bool performanceTimerIsActive = PerformanceTimer::isActive();
    bool displayPerf = _expanded && Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails);
    if (displayPerf && performanceTimerIsActive) {
        PerformanceTimer::tallyAllTimerRecords(); // do this even if we're not displaying them, so they don't stack up
        columnOneWidth = _generalStatsWidth + _pingStatsWidth + _geoStatsWidth; // 3 columns wide...
        // we will also include room for 1 line per timing record and a header of 4 lines
        lines += 4;

        const QMap<QString, PerformanceTimerRecord>& allRecords = PerformanceTimer::getAllTimerRecords();
        QMapIterator<QString, PerformanceTimerRecord> i(allRecords);
        bool onlyDisplayTopTen = Menu::getInstance()->isOptionChecked(MenuOption::OnlyDisplayTopTen);
        int statsLines = 0;
        while (i.hasNext()) {
            i.next();
            if (includeTimingRecord(i.key())) {
                lines++;
                statsLines++;
                if (onlyDisplayTopTen && statsLines == 10) {
                    break;
                }
            }
        }
    }


    // TODO: the display of these timing details should all be moved to JavaScript
    if (displayPerf && performanceTimerIsActive) {
        bool onlyDisplayTopTen = Menu::getInstance()->isOptionChecked(MenuOption::OnlyDisplayTopTen);
        // Timing details...
        verticalOffset += STATS_PELS_PER_LINE * 4; // skip 3 lines to be under the other columns
        drawText(columnOneHorizontalOffset, verticalOffset, scale, rotation, font, 
                "-------------------------------------------------------- Function "
                "------------------------------------------------------- --msecs- -calls--", color);

        // First iterate all the records, and for the ones that should be included, insert them into 
        // a new Map sorted by average time...
        QMap<float, QString> sortedRecords;
        const QMap<QString, PerformanceTimerRecord>& allRecords = PerformanceTimer::getAllTimerRecords();
        QMapIterator<QString, PerformanceTimerRecord> i(allRecords);

        while (i.hasNext()) {
            i.next();
            if (includeTimingRecord(i.key())) {
                float averageTime = (float)i.value().getMovingAverage() / (float)USECS_PER_MSEC;
                sortedRecords.insertMulti(averageTime, i.key());
            }
        }

        int linesDisplayed = 0;
        QMapIterator<float, QString> j(sortedRecords);
        j.toBack();
        while (j.hasPrevious()) {
            j.previous();
            QChar noBreakingSpace = QChar::Nbsp;
            QString functionName = j.value(); 
            const PerformanceTimerRecord& record = allRecords.value(functionName);

            QString perfLine = QString("%1: %2 [%3]").
                arg(QString(qPrintable(functionName)), 120, noBreakingSpace).
                arg((float)record.getMovingAverage() / (float)USECS_PER_MSEC, 8, 'f', 3, noBreakingSpace).
                arg((int)record.getCount(), 6, 10, noBreakingSpace);

            verticalOffset += STATS_PELS_PER_LINE;
            drawText(columnOneHorizontalOffset, verticalOffset, scale, rotation, font, perfLine.toUtf8().constData(), color);
            linesDisplayed++;
            if (onlyDisplayTopTen && linesDisplayed == 10) {
                break;
            }
        }
    }



*/