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

#include <sstream>

#include <stdlib.h>

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

#include "Stats.h"
#include "BandwidthRecorder.h"
#include "InterfaceConfig.h"
#include "Menu.h"
#include "Util.h"
#include "SequenceNumberStats.h"

using namespace std;

const int STATS_PELS_PER_LINE = 16;
const int STATS_PELS_INITIALOFFSET = 12;

const int STATS_GENERAL_MIN_WIDTH = 165; 
const int STATS_PING_MIN_WIDTH = 190;
const int STATS_GEO_MIN_WIDTH = 240;
const int STATS_OCTREE_MIN_WIDTH = 410;

Stats* Stats::getInstance() {
    static Stats stats;
    return &stats;
}

Stats::Stats():
        _expanded(false),
        _recentMaxPackets(0),
        _resetRecentMaxPacketsSoon(true),
        _generalStatsWidth(STATS_GENERAL_MIN_WIDTH),
        _pingStatsWidth(STATS_PING_MIN_WIDTH),
        _geoStatsWidth(STATS_GEO_MIN_WIDTH),
        _octreeStatsWidth(STATS_OCTREE_MIN_WIDTH),
        _lastHorizontalOffset(0)
{
    auto canvasSize = Application::getInstance()->getCanvasSize();
    resetWidth(canvasSize.x, 0);
}

void Stats::toggleExpanded() {
    _expanded = !_expanded;
}

// called on mouse click release
// check for clicks over stats  in order to expand or contract them
void Stats::checkClick(int mouseX, int mouseY, int mouseDragStartedX, int mouseDragStartedY, int horizontalOffset) {
    auto canvasSize = Application::getInstance()->getCanvasSize();

    if (0 != glm::compMax(glm::abs(glm::ivec2(mouseX - mouseDragStartedX, mouseY - mouseDragStartedY)))) {
        // not worried about dragging on stats
        return;
    }

    int statsHeight = 0, 
        statsWidth = 0, 
        statsX = 0, 
        statsY = 0, 
        lines = 0;

    statsX = horizontalOffset;

    // top-left stats click
    lines = _expanded ? 5 : 3;
    statsHeight = lines * STATS_PELS_PER_LINE + 10;
    if (mouseX > statsX && mouseX < statsX + _generalStatsWidth && mouseY > statsY && mouseY < statsY + statsHeight) {
        toggleExpanded();
        return;
    }
    statsX += _generalStatsWidth;

    // ping stats click
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        lines = _expanded ? 4 : 3;
        statsHeight = lines * STATS_PELS_PER_LINE + 10;
        if (mouseX > statsX && mouseX < statsX + _pingStatsWidth && mouseY > statsY && mouseY < statsY + statsHeight) {
            toggleExpanded();
            return;
        }
        statsX += _pingStatsWidth;
    }

    // geo stats panel click
    lines = _expanded ? 4 : 3;
    statsHeight = lines * STATS_PELS_PER_LINE + 10;
    if (mouseX > statsX && mouseX < statsX + _geoStatsWidth  && mouseY > statsY && mouseY < statsY + statsHeight) {
        toggleExpanded();
        return;
    }
    statsX += _geoStatsWidth;

    // top-right stats click
    lines = _expanded ? 11 : 3;
    statsHeight = lines * STATS_PELS_PER_LINE + 10;
    statsWidth = canvasSize.x - statsX;
    if (mouseX > statsX && mouseX < statsX + statsWidth  && mouseY > statsY && mouseY < statsY + statsHeight) {
        toggleExpanded();
        return;
    }
}

void Stats::resetWidth(int width, int horizontalOffset) {
    auto canvasSize = Application::getInstance()->getCanvasSize();
    int extraSpace = canvasSize.x - horizontalOffset - 2
                   - STATS_GENERAL_MIN_WIDTH
                   - (Menu::getInstance()->isOptionChecked(MenuOption::TestPing) ? STATS_PING_MIN_WIDTH -1 : 0)
                   - STATS_GEO_MIN_WIDTH
                   - STATS_OCTREE_MIN_WIDTH;

    int panels = 4;

    _generalStatsWidth = STATS_GENERAL_MIN_WIDTH;
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        _pingStatsWidth = STATS_PING_MIN_WIDTH;
    } else {
        _pingStatsWidth = 0;
        panels = 3;
    }
    _geoStatsWidth = STATS_GEO_MIN_WIDTH;
    _octreeStatsWidth = STATS_OCTREE_MIN_WIDTH;

    if (extraSpace > panels) {
        _generalStatsWidth += (int) extraSpace / panels;
        if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
            _pingStatsWidth += (int) extraSpace / panels;
        }
        _geoStatsWidth += (int) extraSpace / panels;
        _octreeStatsWidth += canvasSize.x -
            (_generalStatsWidth + _pingStatsWidth + _geoStatsWidth + 3);
    }
}


// translucent background box that makes stats more readable
void Stats::drawBackground(unsigned int rgba, int x, int y, int width, int height) {
    glm::vec4 color(((rgba >> 24) & 0xff) / 255.0f,
                      ((rgba >> 16) & 0xff) / 255.0f,
                      ((rgba >> 8) & 0xff)  / 255.0f,
                      (rgba & 0xff) / 255.0f);

    DependencyManager::get<GeometryCache>()->renderQuad(x, y, width, height, color);
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

// display expanded or contracted stats
void Stats::display(
        const float* color, 
        int horizontalOffset, 
        float fps, 
        int inPacketsPerSecond,
        int outPacketsPerSecond,
        int inKbitsPerSecond,
        int outKbitsPerSecond,
        int voxelPacketsToProcess) 
{
    auto canvasSize = Application::getInstance()->getCanvasSize();

    unsigned int backgroundColor = 0x33333399;
    int verticalOffset = 0, lines = 0;
    float scale = 0.10f;
    float rotation = 0.0f;
    int font = 2;

    QLocale locale(QLocale::English);
    std::stringstream octreeStats;

    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();

    if (_lastHorizontalOffset != horizontalOffset) {
        resetWidth(canvasSize.x, horizontalOffset);
        _lastHorizontalOffset = horizontalOffset;
    }

    glPointSize(1.0f);

    // we need to take one avatar out so we don't include ourselves
    int totalAvatars = DependencyManager::get<AvatarManager>()->size() - 1;
    int totalServers = DependencyManager::get<NodeList>()->size();

    lines = 5;
    int columnOneWidth = _generalStatsWidth;

    PerformanceTimer::tallyAllTimerRecords(); // do this even if we're not displaying them, so they don't stack up
    
    if (_expanded && Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails)) {

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
    
    drawBackground(backgroundColor, horizontalOffset, 0, columnOneWidth, (lines + 1) * STATS_PELS_PER_LINE);
    horizontalOffset += 5;
    
    int columnOneHorizontalOffset = horizontalOffset;

    QString serverNodes = QString("Servers: %1").arg(totalServers);
    QString avatarNodes = QString("Avatars: %1").arg(totalAvatars);
    QString framesPerSecond = QString("Framerate: %1 FPS").arg(fps, 3, 'f', 0);
 
    verticalOffset = STATS_PELS_INITIALOFFSET; // first one is offset by less than a line
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, serverNodes.toUtf8().constData(), color);
    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, avatarNodes.toUtf8().constData(), color);
    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, framesPerSecond.toUtf8().constData(), color);

    QString packetsPerSecondString = QString("Packets In/Out: %1/%2").arg(inPacketsPerSecond).arg(outPacketsPerSecond);
    QString averageMegabitsPerSecond = QString("Mbps In/Out: %1/%2").
        arg((float)inKbitsPerSecond * 1.0f / 1000.0f).
        arg((float)outKbitsPerSecond * 1.0f / 1000.0f);

    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, packetsPerSecondString.toUtf8().constData(), color);
    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, averageMegabitsPerSecond.toUtf8().constData(), color);

    
    // TODO: the display of these timing details should all be moved to JavaScript
    if (_expanded && Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails)) {
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


    verticalOffset = STATS_PELS_INITIALOFFSET;
    horizontalOffset = _lastHorizontalOffset + _generalStatsWidth + 1;

    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        int pingAudio = -1, pingAvatar = -1, pingVoxel = -1, pingOctreeMax = -1;

        auto nodeList = DependencyManager::get<NodeList>();
        SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
        SharedNodePointer avatarMixerNode = nodeList->soloNodeOfType(NodeType::AvatarMixer);

        pingAudio = audioMixerNode ? audioMixerNode->getPingMs() : -1;
        pingAvatar = avatarMixerNode ? avatarMixerNode->getPingMs() : -1;

        // Now handle voxel servers, since there could be more than one, we average their ping times
        unsigned long totalPingOctree = 0;
        int octreeServerCount = 0;
        
        nodeList->eachNode([&totalPingOctree, &pingOctreeMax, &octreeServerCount](const SharedNodePointer& node){
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

        lines = _expanded ? 4 : 3;
        
        // only draw our background if column one didn't draw a wide background
        if (columnOneWidth == _generalStatsWidth) {
            drawBackground(backgroundColor, horizontalOffset, 0, _pingStatsWidth, (lines + 1) * STATS_PELS_PER_LINE);
        }
        horizontalOffset += 5;

        
        QString audioPing;
        if (pingAudio >= 0) {
            audioPing = QString("Audio ping: %1").arg(pingAudio);
        } else {
            audioPing = QString("Audio ping: --");
        }

        QString avatarPing;
        if (pingAvatar >= 0) {
            avatarPing = QString("Avatar ping: %1").arg(pingAvatar);
        } else {
            avatarPing = QString("Avatar ping: --");
        }

        QString voxelAvgPing;
        if (pingVoxel >= 0) {
            voxelAvgPing = QString("Entities avg ping: %1").arg(pingVoxel);
        } else {
            voxelAvgPing = QString("Entities avg ping: --");
        }

        drawText(horizontalOffset, verticalOffset, scale, rotation, font, audioPing.toUtf8().constData(), color);
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, avatarPing.toUtf8().constData(), color);
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, voxelAvgPing.toUtf8().constData(), color);

        if (_expanded) {
            QString voxelMaxPing;
            if (pingVoxel >= 0) {  // Average is only meaningful if pingVoxel is valid.
                voxelMaxPing = QString("Voxel max ping: %1").arg(pingOctreeMax);
            } else {
                voxelMaxPing = QString("Voxel max ping: --");
            }

            verticalOffset += STATS_PELS_PER_LINE;
            drawText(horizontalOffset, verticalOffset, scale, rotation, font, voxelMaxPing.toUtf8().constData(), color);
        }

        verticalOffset = STATS_PELS_INITIALOFFSET;
        horizontalOffset = _lastHorizontalOffset + _generalStatsWidth + _pingStatsWidth + 2;
    }
    
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    glm::vec3 avatarPos = myAvatar->getPosition();

    lines = _expanded ? 7 : 3;

    if (columnOneWidth == _generalStatsWidth) {
        drawBackground(backgroundColor, horizontalOffset, 0, _geoStatsWidth, (lines + 1) * STATS_PELS_PER_LINE);
    }
    horizontalOffset += 5;

    QString avatarPosition = QString("Position: %1, %2, %3").
        arg(avatarPos.x, -1, 'f', 1).
        arg(avatarPos.y, -1, 'f', 1).
        arg(avatarPos.z, -1, 'f', 1);
    QString avatarVelocity = QString("Velocity: %1").arg(glm::length(myAvatar->getVelocity()), -1, 'f', 1);
    QString avatarBodyYaw = QString("Yaw: %1").arg(myAvatar->getBodyYaw(), -1, 'f', 1);
    QString avatarMixerStats;

    drawText(horizontalOffset, verticalOffset, scale, rotation, font, avatarPosition.toUtf8().constData(), color);
    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, avatarVelocity.toUtf8().constData(), color);
    verticalOffset += STATS_PELS_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, avatarBodyYaw.toUtf8().constData(), color);

    if (_expanded) {
        SharedNodePointer avatarMixer = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::AvatarMixer);
        if (avatarMixer) {
            avatarMixerStats = QString("Avatar Mixer: %1 kbps, %2 pps").
                arg(roundf(bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AudioMixer) +
                           bandwidthRecorder->getAverageOutputKilobitsPerSecond(NodeType::AudioMixer))).
                arg(roundf(bandwidthRecorder->getAverageInputPacketsPerSecond(NodeType::AudioMixer) +
                           bandwidthRecorder->getAverageOutputPacketsPerSecond(NodeType::AudioMixer)));
        } else {
            avatarMixerStats = QString("No Avatar Mixer");
        }

        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, avatarMixerStats.toUtf8().constData(), color);
        
        stringstream downloads;
        downloads << "Downloads: ";
        foreach (Resource* resource, ResourceCache::getLoadingRequests()) {
            downloads << (int)(resource->getProgress() * 100.0f) << "% ";
        }
        downloads << "(" << ResourceCache::getPendingRequestCount() << " pending)";
        
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, downloads.str().c_str(), color);
    }

    verticalOffset = STATS_PELS_INITIALOFFSET;
    horizontalOffset = _lastHorizontalOffset + _generalStatsWidth + _pingStatsWidth + _geoStatsWidth + 3;

    lines = _expanded ? 10 : 2;

    drawBackground(backgroundColor, horizontalOffset, 0, canvasSize.x - horizontalOffset,
        (lines + 1) * STATS_PELS_PER_LINE);
    horizontalOffset += 5;

    // Model/Entity render details
    EntityTreeRenderer* entities = Application::getInstance()->getEntities();
    octreeStats.str("");
    octreeStats << "Entity Items rendered: " << entities->getItemsRendered() 
                << " / Out of view:" << entities->getItemsOutOfView()
                << " / Too small:" << entities->getItemsTooSmall();
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);

    if (_expanded) {
        octreeStats.str("");
        octreeStats << "  Meshes rendered: " << entities->getMeshesRendered() 
                    << " / Out of view:" << entities->getMeshesOutOfView()
                    << " / Too small:" << entities->getMeshesTooSmall();
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);

        octreeStats.str("");
        octreeStats << "  Triangles: " << entities->getTrianglesRendered() 
                    << " / Quads:" << entities->getQuadsRendered()
                    << " / Material Switches:" << entities->getMaterialSwitches();
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);

        octreeStats.str("");
        octreeStats << "  Mesh Parts Rendered Opaque: " << entities->getOpaqueMeshPartsRendered() 
                    << " / Translucent:" << entities->getTranslucentMeshPartsRendered();
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
    }

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
    if (_expanded) {
        octreeStats.str("");
        QString displayLODDetails = DependencyManager::get<LODManager>()->getLODFeedbackText();
        octreeStats << "LOD: You can see " << qPrintable(displayLODDetails.trimmed());
        verticalOffset += STATS_PELS_PER_LINE;
        drawText(horizontalOffset, verticalOffset, scale, rotation, font, (char*)octreeStats.str().c_str(), color);
    }
}

