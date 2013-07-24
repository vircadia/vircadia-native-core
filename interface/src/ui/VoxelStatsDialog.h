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
#include <QLabel>

#include <VoxelSceneStats.h>

class VoxelStatsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    VoxelStatsDialog(QWidget* parent, VoxelSceneStats* model);

signals:
    void closed();

public slots:
    void reject();

protected:
    // State <- data model held by BandwidthMeter
    void paintEvent(QPaintEvent*);

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private:
    QLabel*             _labels[VoxelSceneStats::ITEM_COUNT];
    VoxelSceneStats*    _model;
};

#endif /* defined(__interface__VoxelStatsDialog__) */

