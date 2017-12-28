//
//  Created by Bradley Austin Davis 2015/06/17
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Stats.h"

#include <queue>
#include <sstream>
#include <QFontDatabase>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <render/Args.h>
#include <avatar/AvatarManager.h>
#include <Application.h>
#include <AudioClient.h>
#include <GeometryCache.h>
#include <LODManager.h>
#include <OffscreenUi.h>
#include <PerfStat.h>
#include <plugins/DisplayPlugin.h>

#include <gl/Context.h>

#include "BandwidthRecorder.h"
#include "Menu.h"
#include "Util.h"
#include "SequenceNumberStats.h"
#include "StatTracker.h"


HIFI_QML_DEF(Stats)

using namespace std;

static Stats* INSTANCE{ nullptr };

QString getTextureMemoryPressureModeString();

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
    _audioStats = &DependencyManager::get<AudioClient>()->getStats();
}

bool Stats::includeTimingRecord(const QString& name) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails)) {
        if (name.startsWith("/idle/update/")) {
            if (name.startsWith("/idle/update/physics/")) {
                return Menu::getInstance()->isOptionChecked(MenuOption::ExpandPhysicsSimulationTiming);
            } else if (name.startsWith("/idle/update/myAvatar/")) {
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
        } else if (name.startsWith("step/")) {
            return Menu::getInstance()->isOptionChecked(MenuOption::ExpandPhysicsSimulationTiming);
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

extern std::atomic<size_t> DECIMATED_TEXTURE_COUNT;
extern std::atomic<size_t> RECTIFIED_TEXTURE_COUNT;

void Stats::updateStats(bool force) {
    QQuickItem* parent = parentItem();
    if (!force) {
        if (!Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
            if (parent->isVisible()) {
                parent->setVisible(false);
            }
            return;
        } else if (!parent->isVisible()) {
            parent->setVisible(true);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    auto avatarManager = DependencyManager::get<AvatarManager>();
    // we need to take one avatar out so we don't include ourselves
    STAT_UPDATE(avatarCount, avatarManager->size() - 1);
    STAT_UPDATE(updatedAvatarCount, avatarManager->getNumAvatarsUpdated());
    STAT_UPDATE(notUpdatedAvatarCount, avatarManager->getNumAvatarsNotUpdated());
    STAT_UPDATE(serverCount, (int)nodeList->size());
    STAT_UPDATE_FLOAT(renderrate, qApp->getRenderLoopRate(), 0.1f);
    if (qApp->getActiveDisplayPlugin()) {
        auto displayPlugin = qApp->getActiveDisplayPlugin();
        auto stats = displayPlugin->getHardwareStats();
        STAT_UPDATE(appdropped, stats["app_dropped_frame_count"].toInt());
        STAT_UPDATE(longrenders, stats["long_render_count"].toInt());
        STAT_UPDATE(longsubmits, stats["long_submit_count"].toInt());
        STAT_UPDATE(longframes, stats["long_frame_count"].toInt());
        STAT_UPDATE_FLOAT(presentrate, displayPlugin->presentRate(), 0.1f);
        STAT_UPDATE_FLOAT(presentnewrate, displayPlugin->newFramePresentRate(), 0.1f);
        STAT_UPDATE_FLOAT(presentdroprate, displayPlugin->droppedFrameRate(), 0.1f);
        STAT_UPDATE_FLOAT(stutterrate, displayPlugin->stutterRate(), 0.1f);
    } else {
        STAT_UPDATE(appdropped, -1);
        STAT_UPDATE(longrenders, -1);
        STAT_UPDATE(longsubmits, -1);
        STAT_UPDATE(presentrate, -1);
        STAT_UPDATE(presentnewrate, -1);
        STAT_UPDATE(presentdroprate, -1);
    }
    STAT_UPDATE(gameLoopRate, (int)qApp->getGameLoopRate());

    auto bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    STAT_UPDATE(packetInCount, (int)bandwidthRecorder->getCachedTotalAverageInputPacketsPerSecond());
    STAT_UPDATE(packetOutCount, (int)bandwidthRecorder->getCachedTotalAverageOutputPacketsPerSecond());
    STAT_UPDATE_FLOAT(mbpsIn, (float)bandwidthRecorder->getCachedTotalAverageInputKilobitsPerSecond() / 1000.0f, 0.01f);
    STAT_UPDATE_FLOAT(mbpsOut, (float)bandwidthRecorder->getCachedTotalAverageOutputKilobitsPerSecond() / 1000.0f, 0.01f);

    STAT_UPDATE_FLOAT(assetMbpsIn, (float)bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AssetServer) / 1000.0f, 0.01f);
    STAT_UPDATE_FLOAT(assetMbpsOut, (float)bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AssetServer) / 1000.0f, 0.01f);

    // Second column: ping
    SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
    SharedNodePointer avatarMixerNode = nodeList->soloNodeOfType(NodeType::AvatarMixer);
    SharedNodePointer assetServerNode = nodeList->soloNodeOfType(NodeType::AssetServer);
    SharedNodePointer messageMixerNode = nodeList->soloNodeOfType(NodeType::MessagesMixer);
    STAT_UPDATE(audioPing, audioMixerNode ? audioMixerNode->getPingMs() : -1); 
    const int mixerLossRate = (int)roundf(_audioStats->data()->getMixerStream()->lossRateWindow() * 100.0f);
    const int clientLossRate = (int)roundf(_audioStats->data()->getClientStream()->lossRateWindow() * 100.0f);
    const int largestLossRate = mixerLossRate > clientLossRate ? mixerLossRate : clientLossRate;
    STAT_UPDATE(audioPacketLoss, audioMixerNode ? largestLossRate : -1);
    STAT_UPDATE(avatarPing, avatarMixerNode ? avatarMixerNode->getPingMs() : -1);
    STAT_UPDATE(assetPing, assetServerNode ? assetServerNode->getPingMs() : -1);
    STAT_UPDATE(messagePing, messageMixerNode ? messageMixerNode->getPingMs() : -1);

    //// Now handle entity servers, since there could be more than one, we average their ping times
    int totalPingOctree = 0;
    int octreeServerCount = 0;
    int pingOctreeMax = 0;
    int totalEntityKbps = 0;
    nodeList->eachNode([&](const SharedNodePointer& node) {
        // TODO: this should also support entities
        if (node->getType() == NodeType::EntityServer) {
            totalPingOctree += node->getPingMs();
            totalEntityKbps += node->getInboundBandwidth();
            octreeServerCount++;
            if (pingOctreeMax < node->getPingMs()) {
                pingOctreeMax = node->getPingMs();
            }
        }
    });

    // update the entities ping with the average for all connected entity servers
    STAT_UPDATE(entitiesPing, octreeServerCount ? totalPingOctree / octreeServerCount : -1);

    // Third column, avatar stats
    auto myAvatar = avatarManager->getMyAvatar();
    glm::vec3 avatarPos = myAvatar->getWorldPosition();
    STAT_UPDATE(position, QVector3D(avatarPos.x, avatarPos.y, avatarPos.z));
    STAT_UPDATE_FLOAT(speed, glm::length(myAvatar->getWorldVelocity()), 0.01f);
    STAT_UPDATE_FLOAT(yaw, myAvatar->getBodyYaw(), 0.1f);
    if (_expanded || force) {
        SharedNodePointer avatarMixer = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        if (avatarMixer) {
            STAT_UPDATE(avatarMixerInKbps, (int)roundf(bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AvatarMixer)));
            STAT_UPDATE(avatarMixerInPps, (int)roundf(bandwidthRecorder->getAverageInputPacketsPerSecond(NodeType::AvatarMixer)));
            STAT_UPDATE(avatarMixerOutKbps, (int)roundf(bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AvatarMixer)));
            STAT_UPDATE(avatarMixerOutPps, (int)roundf(bandwidthRecorder->getAverageOutputPacketsPerSecond(NodeType::AvatarMixer)));
        } else {
            STAT_UPDATE(avatarMixerInKbps, -1);
            STAT_UPDATE(avatarMixerInPps, -1);
            STAT_UPDATE(avatarMixerOutKbps, -1);
            STAT_UPDATE(avatarMixerOutPps, -1);
        }
        STAT_UPDATE_FLOAT(myAvatarSendRate, avatarManager->getMyAvatarSendRate(), 0.1f);

        SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
        auto audioClient = DependencyManager::get<AudioClient>();
        if (audioMixerNode || force) {
            STAT_UPDATE(audioMixerKbps, (int)roundf(
                bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AudioMixer) +
                bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AudioMixer)));
            STAT_UPDATE(audioMixerPps, (int)roundf(
                bandwidthRecorder->getAverageInputPacketsPerSecond(NodeType::AudioMixer) +
                bandwidthRecorder->getAverageOutputPacketsPerSecond(NodeType::AudioMixer)));

            STAT_UPDATE(audioMixerInKbps, (int)roundf(bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AudioMixer)));
            STAT_UPDATE(audioMixerInPps, (int)roundf(bandwidthRecorder->getAverageInputPacketsPerSecond(NodeType::AudioMixer)));
            STAT_UPDATE(audioMixerOutKbps, (int)roundf(bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AudioMixer)));
            STAT_UPDATE(audioMixerOutPps, (int)roundf(bandwidthRecorder->getAverageOutputPacketsPerSecond(NodeType::AudioMixer)));
            STAT_UPDATE(audioAudioInboundPPS, (int)audioClient->getAudioInboundPPS());
            STAT_UPDATE(audioSilentInboundPPS, (int)audioClient->getSilentInboundPPS());
            STAT_UPDATE(audioOutboundPPS, (int)audioClient->getAudioOutboundPPS());
            STAT_UPDATE(audioSilentOutboundPPS, (int)audioClient->getSilentOutboundPPS());
        } else {
            STAT_UPDATE(audioMixerKbps, -1);
            STAT_UPDATE(audioMixerPps, -1);
            STAT_UPDATE(audioMixerInKbps, -1);
            STAT_UPDATE(audioMixerInPps, -1);
            STAT_UPDATE(audioMixerOutKbps, -1);
            STAT_UPDATE(audioMixerOutPps, -1);
            STAT_UPDATE(audioOutboundPPS, -1);
            STAT_UPDATE(audioSilentOutboundPPS, -1);
            STAT_UPDATE(audioAudioInboundPPS, -1);
            STAT_UPDATE(audioSilentInboundPPS, -1);
        }
        STAT_UPDATE(audioCodec, audioClient->getSelectedAudioFormat());
        STAT_UPDATE(audioNoiseGate, audioClient->getNoiseGateOpen() ? "Open" : "Closed");

        STAT_UPDATE(entityPacketsInKbps, octreeServerCount ? totalEntityKbps / octreeServerCount : -1);

        auto loadingRequests = ResourceCache::getLoadingRequests();
        STAT_UPDATE(downloads, loadingRequests.size());
        STAT_UPDATE(downloadLimit, ResourceCache::getRequestLimit())
        STAT_UPDATE(downloadsPending, ResourceCache::getPendingRequestCount());
        STAT_UPDATE(processing, DependencyManager::get<StatTracker>()->getStat("Processing").toInt());
        STAT_UPDATE(processingPending, DependencyManager::get<StatTracker>()->getStat("PendingProcessing").toInt());
        

        // See if the active download urls have changed
        bool shouldUpdateUrls = _downloads != _downloadUrls.size();
        if (!shouldUpdateUrls) {
            for (int i = 0; i < _downloads; i++) {
                if (loadingRequests[i]->getURL().toString() != _downloadUrls[i]) {
                    shouldUpdateUrls = true;
                    break;
                }
            }
        }
        // If the urls have changed, update the list
        if (shouldUpdateUrls) {
            _downloadUrls.clear();
            foreach (const auto& resource, loadingRequests) {
                _downloadUrls << resource->getURL().toString();
            }
            emit downloadUrlsChanged();
        }
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
    NodeToOctreeSceneStats* octreeServerSceneStats = qApp->getOcteeSceneStats();
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
            if (stats.isFullScene()) {
                sendingModeStream << "F";
            }
            else {
                sendingModeStream << "p";
            }
        }

        // calculate server node totals
        totalNodes += stats.getTotalElements();
        if (_expanded) {
            totalInternal += stats.getTotalInternal();
            totalLeaves += stats.getTotalLeaves();
        }
    }
    if (_expanded || force) {
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

    auto gpuContext = qApp->getGPUContext();

    // Update Frame timing (in ms)
    STAT_UPDATE(gpuFrameTime, (float)gpuContext->getFrameTimerGPUAverage());
    STAT_UPDATE(batchFrameTime, (float)gpuContext->getFrameTimerBatchAverage());
    auto config = qApp->getRenderEngine()->getConfiguration().get();
    STAT_UPDATE(engineFrameTime, (float) config->getCPURunTime());
    STAT_UPDATE(avatarSimulationTime, (float)avatarManager->getAvatarSimulationTime());
    

    STAT_UPDATE(gpuBuffers, (int)gpu::Context::getBufferGPUCount());
    STAT_UPDATE(gpuBufferMemory, (int)BYTES_TO_MB(gpu::Context::getBufferGPUMemSize()));
    STAT_UPDATE(gpuTextures, (int)gpu::Context::getTextureGPUCount());

    STAT_UPDATE(glContextSwapchainMemory, (int)BYTES_TO_MB(gl::Context::getSwapchainMemoryUsage()));

    STAT_UPDATE(qmlTextureMemory, (int)BYTES_TO_MB(OffscreenQmlSurface::getUsedTextureMemory()));
    STAT_UPDATE(texturePendingTransfers, (int)BYTES_TO_MB(gpu::Context::getTexturePendingGPUTransferMemSize()));
    STAT_UPDATE(gpuTextureMemory, (int)BYTES_TO_MB(gpu::Context::getTextureGPUMemSize()));
    STAT_UPDATE(gpuTextureResidentMemory, (int)BYTES_TO_MB(gpu::Context::getTextureResidentGPUMemSize()));
    STAT_UPDATE(gpuTextureFramebufferMemory, (int)BYTES_TO_MB(gpu::Context::getTextureFramebufferGPUMemSize()));
    STAT_UPDATE(gpuTextureResourceMemory, (int)BYTES_TO_MB(gpu::Context::getTextureResourceGPUMemSize()));
    STAT_UPDATE(gpuTextureResourcePopulatedMemory, (int)BYTES_TO_MB(gpu::Context::getTextureResourcePopulatedGPUMemSize()));
    STAT_UPDATE(gpuTextureExternalMemory, (int)BYTES_TO_MB(gpu::Context::getTextureExternalGPUMemSize()));
    STAT_UPDATE(gpuTextureMemoryPressureState, getTextureMemoryPressureModeString());
    STAT_UPDATE(gpuFreeMemory, (int)BYTES_TO_MB(gpu::Context::getFreeGPUMemSize()));
    STAT_UPDATE(rectifiedTextureCount, (int)RECTIFIED_TEXTURE_COUNT.load());
    STAT_UPDATE(decimatedTextureCount, (int)DECIMATED_TEXTURE_COUNT.load());

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
        _recentMaxPackets = (int)voxelPacketsToProcess;
    }

    // Server Octree Elements
    STAT_UPDATE(serverElements, (int)totalNodes);
    STAT_UPDATE(localElements, (int)OctreeElement::getNodeCount());

    if (_expanded || force) {
        STAT_UPDATE(serverInternal, (int)totalInternal);
        STAT_UPDATE(serverLeaves, (int)totalLeaves);
        // Local Voxels
        STAT_UPDATE(localInternal, (int)OctreeElement::getInternalNodeCount());
        STAT_UPDATE(localLeaves, (int)OctreeElement::getLeafNodeCount());
        // LOD Details
        STAT_UPDATE(lodStatus, "You can see " + DependencyManager::get<LODManager>()->getLODFeedbackText());
    }


    bool performanceTimerShouldBeActive = Menu::getInstance()->isOptionChecked(MenuOption::Stats) && _expanded;
    if (performanceTimerShouldBeActive != PerformanceTimer::isActive()) {
        PerformanceTimer::setActive(performanceTimerShouldBeActive);
    }
    if (performanceTimerShouldBeActive) {
        PerformanceTimer::tallyAllTimerRecords(); // do this even if we're not displaying them, so they don't stack up
    }

    if (performanceTimerShouldBeActive &&
        Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails)) {
        if (!_showTimingDetails) {
            _showTimingDetails = true;
            emit timingExpandedChanged();
        }

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
                arg(QString(qPrintable(functionName)), -80, noBreakingSpace).
                arg((float)record.getMovingAverage() / (float)USECS_PER_MSEC, 8, 'f', 3, noBreakingSpace).
                arg((int)record.getCount(), 6, 10, noBreakingSpace);
            linesDisplayed++;
            if (onlyDisplayTopTen && linesDisplayed == 10) {
                break;
            }
        }
        _timingStats = perfLines;
        emit timingStatsChanged();
    } else if (_showTimingDetails) {
        _showTimingDetails = false;
        emit timingExpandedChanged();
    }

    if (_expanded && performanceTimerShouldBeActive) {
        if (!_showGameUpdateStats) {
            _showGameUpdateStats = true;
        }
        class SortableStat {
        public:
            SortableStat(QString a, float p) : message(a), priority(p) {}
            QString message;
            float priority;
            bool operator<(const SortableStat& other) const { return priority < other.priority; }
        };

        const QMap<QString, PerformanceTimerRecord>& allRecords = PerformanceTimer::getAllTimerRecords();
        std::priority_queue<SortableStat> idleUpdateStats;
        auto itr = allRecords.find("/idle/update");
        if (itr != allRecords.end()) {
            float dt = (float)itr.value().getMovingAverage() / (float)USECS_PER_MSEC;
            _gameUpdateStats = QString("/idle/update = %1 ms").arg(dt);

            QVector<QString> categories = { "devices", "physics", "otherAvatars", "MyAvatar", "misc" };
            for (int32_t j = 0; j < categories.size(); ++j) {
                QString recordKey = "/idle/update/" + categories[j];
                itr = allRecords.find(recordKey);
                if (itr != allRecords.end()) {
                    float dt = (float)itr.value().getMovingAverage() / (float)USECS_PER_MSEC;
                    QString message = QString("\n    %1 = %2").arg(categories[j]).arg(dt);
                    idleUpdateStats.push(SortableStat(message, dt));
                }
            }
            while (!idleUpdateStats.empty()) {
                SortableStat stat = idleUpdateStats.top();
                _gameUpdateStats += stat.message;
                idleUpdateStats.pop();
            }
            emit gameUpdateStatsChanged();
        } else if (_gameUpdateStats != "") {
            _gameUpdateStats = "";
            emit gameUpdateStatsChanged();
        }
    } else if (_showGameUpdateStats) {
        _showGameUpdateStats = false;
        _gameUpdateStats = "";
        emit gameUpdateStatsChanged();
    }
}

void Stats::setRenderDetails(const render::RenderDetails& details) {
    STAT_UPDATE(triangles, details._trianglesRendered);
    STAT_UPDATE(materialSwitches, details._materialSwitches);
    if (_expanded) {
        STAT_UPDATE(itemConsidered, details._item._considered);
        STAT_UPDATE(itemOutOfView, details._item._outOfView);
        STAT_UPDATE(itemTooSmall, details._item._tooSmall);
        STAT_UPDATE(itemRendered, details._item._rendered);
        STAT_UPDATE(shadowConsidered, details._shadow._considered);
        STAT_UPDATE(shadowOutOfView, details._shadow._outOfView);
        STAT_UPDATE(shadowTooSmall, details._shadow._tooSmall);
        STAT_UPDATE(shadowRendered, details._shadow._rendered);
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
