//
//  Created by Bradley Austin Davis 2015/06/17
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Stats.h"

#include <sstream>
#include <QFontDatabase>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <avatar/AvatarManager.h>
#include <Application.h>
#include <GeometryCache.h>
#include <LODManager.h>
#include <PerfStat.h>

#include "BandwidthRecorder.h"
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
        Q_ASSERT(INSTANCE);
    }
    return INSTANCE;
}

Stats::Stats(QQuickItem* parent) :  QQuickItem(parent) {
    INSTANCE = this;
    const QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    _monospaceFont = font.family();
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
        if (fabs(_##name - val) >= epsilon) { \
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

    bool shouldDisplayTimingDetail = Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails) &&
        Menu::getInstance()->isOptionChecked(MenuOption::Stats) && isExpanded();
    if (shouldDisplayTimingDetail != PerformanceTimer::isActive()) {
        PerformanceTimer::setActive(shouldDisplayTimingDetail);
    }


    auto nodeList = DependencyManager::get<NodeList>();
    auto avatarManager = DependencyManager::get<AvatarManager>();
    // we need to take one avatar out so we don't include ourselves
    STAT_UPDATE(avatarCount, avatarManager->size() - 1);
    STAT_UPDATE(serverCount, nodeList->size());
    STAT_UPDATE(framerate, (int)qApp->getFps());
    STAT_UPDATE(simrate, (int)Application::getInstance()->getAverageSimsPerSecond());

    auto bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    STAT_UPDATE(packetInCount, bandwidthRecorder->getCachedTotalAverageInputPacketsPerSecond());
    STAT_UPDATE(packetOutCount, bandwidthRecorder->getCachedTotalAverageOutputPacketsPerSecond());
    STAT_UPDATE_FLOAT(mbpsIn, (float)bandwidthRecorder->getCachedTotalAverageInputKilobitsPerSecond() / 1000.0f, 0.01f);
    STAT_UPDATE_FLOAT(mbpsOut, (float)bandwidthRecorder->getCachedTotalAverageOutputKilobitsPerSecond() / 1000.0f, 0.01f);

    // Second column: ping
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
        SharedNodePointer avatarMixerNode = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        SharedNodePointer assetServerNode = nodeList->soloNodeOfType(NodeType::AssetServer);
        STAT_UPDATE(audioPing, audioMixerNode ? audioMixerNode->getPingMs() : -1);
        STAT_UPDATE(avatarPing, avatarMixerNode ? avatarMixerNode->getPingMs() : -1);
        STAT_UPDATE(assetPing, assetServerNode ? assetServerNode->getPingMs() : -1);

        //// Now handle entity servers, since there could be more than one, we average their ping times
        int totalPingOctree = 0;
        int octreeServerCount = 0;
        int pingOctreeMax = 0;
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
        
        // update the entities ping with the average for all connected entity servers
        STAT_UPDATE(entitiesPing, octreeServerCount ? totalPingOctree / octreeServerCount : -1);
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
    int serverCount = 0;
    int movingServerCount = 0;
    unsigned long totalNodes = 0;
    unsigned long totalInternal = 0;
    unsigned long totalLeaves = 0;
    std::stringstream sendingModeStream("");
    sendingModeStream << "[";
    NodeToOctreeSceneStats* octreeServerSceneStats = Application::getInstance()->getOcteeSceneStats();
    for (NodeToOctreeSceneStatsIterator i = octreeServerSceneStats->begin(); i != octreeServerSceneStats->end(); i++) {
        //const QUuid& uuid = i->first;
        OctreeSceneStats& stats = i->second;
        serverCount++;
        if (_expanded) {
            if (serverCount > 1) {
                sendingModeStream << ",";
            }
            if (stats.isMoving()) {
                sendingModeStream << "M";
                movingServerCount++;
            } else {
                sendingModeStream << "S";
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
            sendingModeStream << "---";
        }
        sendingModeStream << "] " << serverCount << " servers";
        if (movingServerCount > 0) {
            sendingModeStream << " <SCENE NOT STABLE>";
        } else {
            sendingModeStream << " <SCENE STABLE>";
        }
        QString sendingModeResult = sendingModeStream.str().c_str();
        STAT_UPDATE(sendingMode, sendingModeResult);
    }

    // Incoming packets
    QLocale locale(QLocale::English);
    auto voxelPacketsToProcess = qApp->getOctreePacketProcessor().packetsToProcessCount();
    if (_expanded) {
        std::stringstream octreeStats;
        QString packetsString = locale.toString((int)voxelPacketsToProcess);
        QString maxString = locale.toString((int)_recentMaxPackets);
        octreeStats << "Octree Packets to Process: " << qPrintable(packetsString)
            << " [Recent Max: " << qPrintable(maxString) << "]";
        QString str = octreeStats.str().c_str();
        STAT_UPDATE(packetStats, str);
        // drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
    }

    if (_resetRecentMaxPacketsSoon && voxelPacketsToProcess > 0) {
        _recentMaxPackets = 0;
        _resetRecentMaxPacketsSoon = false;
    }
    if (voxelPacketsToProcess == 0) {
        _resetRecentMaxPacketsSoon = true;
    } else if (voxelPacketsToProcess > _recentMaxPackets) {
        _recentMaxPackets = voxelPacketsToProcess;
    }

    // Server Octree Elements
    STAT_UPDATE(serverElements, (int)totalNodes);
    STAT_UPDATE(localElements, (int)OctreeElement::getNodeCount());

    if (_expanded) {
        STAT_UPDATE(serverInternal, (int)totalInternal);
        STAT_UPDATE(serverLeaves, (int)totalLeaves);
        // Local Voxels
        STAT_UPDATE(localInternal, (int)OctreeElement::getInternalNodeCount());
        STAT_UPDATE(localLeaves, (int)OctreeElement::getLeafNodeCount());
        // LOD Details
        STAT_UPDATE(lodStatus, "You can see " + DependencyManager::get<LODManager>()->getLODFeedbackText());
    }

    bool performanceTimerIsActive = PerformanceTimer::isActive();
    bool displayPerf = _expanded && Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails);
    if (displayPerf && performanceTimerIsActive) {
        if (!_timingExpanded) {
            _timingExpanded = true;
            emit timingExpandedChanged();
        }
        PerformanceTimer::tallyAllTimerRecords(); // do this even if we're not displaying them, so they don't stack up

        // we will also include room for 1 line per timing record and a header of 4 lines
        // Timing details...

        // First iterate all the records, and for the ones that should be included, insert them into 
        // a new Map sorted by average time...
        bool onlyDisplayTopTen = Menu::getInstance()->isOptionChecked(MenuOption::OnlyDisplayTopTen);
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
        QString perfLines;
        while (j.hasPrevious()) {
            j.previous();
            static const QChar noBreakingSpace = QChar::Nbsp;
            QString functionName = j.value();
            const PerformanceTimerRecord& record = allRecords.value(functionName);
            perfLines += QString("%1: %2 [%3]\n").
                arg(QString(qPrintable(functionName)), 90, noBreakingSpace).
                arg((float)record.getMovingAverage() / (float)USECS_PER_MSEC, 8, 'f', 3, noBreakingSpace).
                arg((int)record.getCount(), 6, 10, noBreakingSpace);
            linesDisplayed++;
            if (onlyDisplayTopTen && linesDisplayed == 10) {
                break;
            }
        }
        _timingStats = perfLines;
        emit timingStatsChanged();
    } else if (_timingExpanded) {
        _timingExpanded = false;
        emit timingExpandedChanged();
    }
}

void Stats::setRenderDetails(const RenderDetails& details) {
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

}


*/
