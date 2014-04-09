//
//  Stats.h
//  interface/src/ui
//
//  Created by Lucas Crisman on 22/03/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef _hifi_Stats_h
#define _hifi_Stats_h

#include <QObject>

#include <NodeList.h>

class Stats: public QObject {
    Q_OBJECT

public:
    static Stats* getInstance();

    Stats();

    static void drawBackground(unsigned int rgba, int x, int y, int width, int height);

    void toggleExpanded();
    void checkClick(int mouseX, int mouseY, int mouseDragStartedX, int mouseDragStartedY, int horizontalOffset);
    void resetWidth(int width, int horizontalOffset);
    void display(const float* color, int horizontalOffset, float fps, int packetsPerSecond, int bytesPerSecond, int voxelPacketsToProcess);
private:
    static Stats* _sharedInstance;

    bool _expanded;

    int _recentMaxPackets; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon;

    int _generalStatsWidth;
    int _pingStatsWidth;
    int _geoStatsWidth;
    int _voxelStatsWidth;

    int _lastHorizontalOffset;
};

#endif
