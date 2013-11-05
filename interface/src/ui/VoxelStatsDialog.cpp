//
//  VoxelStatsDialog.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QFormLayout>
#include <QDialogButtonBox>

#include <QPalette>
#include <QColor>

#include <VoxelSceneStats.h>

#include "Application.h"

#include "ui/VoxelStatsDialog.h"

VoxelStatsDialog::VoxelStatsDialog(QWidget* parent, NodeToVoxelSceneStats* model) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint),
    _model(model) {
    
    _statCount = 0;

    for (int i = 0; i < MAX_STATS; i++) {
        _labels[i] = NULL;
    }

    this->setWindowTitle("Voxel Statistics");

    // Create layouter
    _form = new QFormLayout();
    this->QDialog::setLayout(_form);

    // Setup stat items
    _serverVoxels = AddStatItem("Voxels on Servers", GREENISH);
    _localVoxels = AddStatItem("Local Voxels", YELLOWISH);
    _localVoxelsMemory = AddStatItem("Voxels Memory", GREYISH);
    _voxelsRendered = AddStatItem("Voxels Rendered", GREENISH);
    _sendingMode = AddStatItem("Sending Mode", YELLOWISH);

     VoxelSceneStats temp;
     for (int i = 0; i < VoxelSceneStats::ITEM_COUNT; i++) {
         VoxelSceneStats::Item item = (VoxelSceneStats::Item)(i);
         VoxelSceneStats::ItemInfo& itemInfo = temp.getItemInfo(item);
         AddStatItem(itemInfo.caption, itemInfo.colorRGBA);
     }
}

int VoxelStatsDialog::AddStatItem(const char* caption, unsigned colorRGBA) {
    char strBuf[64];
    const int STATS_LABEL_WIDTH = 550;
    
    _statCount++; // increment our current stat count

    QLabel* label = _labels[_statCount] = new QLabel();  

    // Set foreground color to 62.5% brightness of the meter (otherwise will be hard to read on the bright background)
    QPalette palette = label->palette();
    
    // This goofiness came from the bandwidth meter code, it basically stores a color in an unsigned and extracts it
    unsigned rgb = colorRGBA >> 8;
    const unsigned colorpart1 = 0xfefefeu;
    const unsigned colorpart2 = 0xf8f8f8;
    rgb = ((rgb & colorpart1) >> 1) + ((rgb & colorpart2) >> 3);
    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
    label->setPalette(palette);
    
    label->setFixedWidth(STATS_LABEL_WIDTH);

    snprintf(strBuf, sizeof(strBuf), " %s:", caption);
    _form->addRow(strBuf, label);
    
    return _statCount;
}

VoxelStatsDialog::~VoxelStatsDialog() {
    for (int i = 0; i < _statCount; i++) {
        delete _labels[i];
    }
}

void VoxelStatsDialog::paintEvent(QPaintEvent* event) {

    // Update labels

    VoxelSystem* voxels = Application::getInstance()->getVoxels();
    QLabel* label;
    QLocale locale(QLocale::English);
    std::stringstream statsValue;
    statsValue.precision(4);

    // Voxels Rendered    
    label = _labels[_voxelsRendered];
    statsValue << "Max: " << voxels->getMaxVoxels() / 1000.f << "K " << 
        "Rendered: " << voxels->getVoxelsRendered() / 1000.f << "K " <<
        "Written: " << voxels->getVoxelsWritten() / 1000.f << "K " <<
        "Abandoned: " << voxels->getAbandonedVoxels() / 1000.f << "K " <<
        "Updated: " << voxels->getVoxelsUpdated() / 1000.f << "K ";
    label->setText(statsValue.str().c_str());

    // Voxels Memory Usage
    label = _labels[_localVoxelsMemory];
    statsValue.str("");
    statsValue << 
        "Nodes RAM: " << VoxelNode::getTotalMemoryUsage() / 1000000.f << "MB "
        "Geometry RAM: " << voxels->getVoxelMemoryUsageRAM() / 1000000.f << "MB " <<
        "VBO: " << voxels->getVoxelMemoryUsageVBO() / 1000000.f << "MB ";
    if (voxels->hasVoxelMemoryUsageGPU()) {
        statsValue << "GPU: " << voxels->getVoxelMemoryUsageGPU() / 1000000.f << "MB ";
    }
    label->setText(statsValue.str().c_str());

    // Local Voxels
    label = _labels[_localVoxels];
    unsigned long localTotal = VoxelNode::getNodeCount();
    unsigned long localInternal = VoxelNode::getInternalNodeCount();
    unsigned long localLeaves = VoxelNode::getLeafNodeCount();
    QString localTotalString = locale.toString((uint)localTotal); // consider adding: .rightJustified(10, ' ');
    QString localInternalString = locale.toString((uint)localInternal);
    QString localLeavesString = locale.toString((uint)localLeaves);

    statsValue.str("");
    statsValue << 
        "Total: " << localTotalString.toLocal8Bit().constData() << " / " <<
        "Internal: " << localInternalString.toLocal8Bit().constData() << " / " <<
        "Leaves: " << localLeavesString.toLocal8Bit().constData() << "";
    label->setText(statsValue.str().c_str());

    // iterate all the current voxel stats, and list their sending modes, total their voxels, etc...
    std::stringstream sendingMode("");

    int serverCount = 0;
    unsigned long totalNodes = 0;
    unsigned long totalInternal = 0;
    unsigned long totalLeaves = 0;
    NodeToVoxelSceneStats* sceneStats = Application::getInstance()->getVoxelSceneStats();
    for(NodeToVoxelSceneStatsIterator i = sceneStats->begin(); i != sceneStats->end(); i++) {
        //const QUuid& uuid = i->first;
        VoxelSceneStats& stats = i->second;
        serverCount++;

        // calculate server node totals
        totalNodes += stats.getTotalVoxels();
        totalInternal += stats.getTotalInternal();
        totalLeaves += stats.getTotalLeaves();
    
        // Sending mode
        if (serverCount > 1) {
            sendingMode << ",";
        }
        if (stats.isMoving()) {
            sendingMode << "M";
        } else {
            sendingMode << "S";
        }
    }
    sendingMode << " - " << serverCount << " servers";
    label = _labels[_sendingMode];
    label->setText(sendingMode.str().c_str());
    
    // Server Voxels
    QString serversTotalString = locale.toString((uint)totalNodes); // consider adding: .rightJustified(10, ' ');
    QString serversInternalString = locale.toString((uint)totalInternal);
    QString serversLeavesString = locale.toString((uint)totalLeaves);
    label = _labels[_serverVoxels];
    statsValue.str("");
    statsValue << 
        "Total: " << serversTotalString.toLocal8Bit().constData() << " / " <<
        "Internal: " << serversInternalString.toLocal8Bit().constData() << " / " <<
        "Leaves: " << serversLeavesString.toLocal8Bit().constData() << "";
    label->setText(statsValue.str().c_str());

    /**
    voxelStats.str("");
    int voxelPacketsToProcess = _voxelProcessor.packetsToProcessCount();
    QString packetsString = locale.toString((int)voxelPacketsToProcess);
    QString maxString = locale.toString((int)_recentMaxPackets);
    voxelStats << "Voxel Packets to Process: " << packetsString.toLocal8Bit().constData() 
                << " [Recent Max: " << maxString.toLocal8Bit().constData() << "]";
    **/


    this->QDialog::paintEvent(event);
    //this->setFixedSize(this->width(), this->height());
}

void VoxelStatsDialog::reject() {
    // Just regularly close upon ESC
    this->QDialog::close();
}

void VoxelStatsDialog::closeEvent(QCloseEvent* event) {
    this->QDialog::closeEvent(event);
    emit closed();
}


