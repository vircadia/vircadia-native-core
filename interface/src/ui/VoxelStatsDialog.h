//
//  VoxelStatsDialog.h
//  interface
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelStatsDialog__
#define __hifi__VoxelStatsDialog__

#include <QDialog>
#include <QFormLayout>
#include <QLabel>

#include <VoxelSceneStats.h>

#define MAX_STATS 40

class VoxelStatsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    VoxelStatsDialog(QWidget* parent, NodeToVoxelSceneStats* model);
    ~VoxelStatsDialog();

signals:
    void closed();

public slots:
    void reject();

protected:
    // State <- data model held by BandwidthMeter
    void paintEvent(QPaintEvent*);

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

    int AddStatItem(const char* caption, unsigned colorRGBA);

private:
    QFormLayout* _form;
    QLabel* _labels[MAX_STATS];
    NodeToVoxelSceneStats* _model;
    int _statCount;
    
    int _sendingMode;
    int _serverVoxels;
    int _localVoxels;
    int _localVoxelsMemory;
    int _voxelsRendered;
};

#endif /* defined(__interface__VoxelStatsDialog__) */

