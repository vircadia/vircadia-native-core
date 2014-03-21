//
//  Stats.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QObject>

#include <NodeList.h>

//#include "Menu.h"

class Stats: public QObject {
    Q_OBJECT

public:
    static Stats* getInstance();

    Stats();

    static void drawBackground(unsigned int rgba, int x, int y, int width, int height);

    void toggleExpanded();
    void checkClick(int mouseX, int mouseY, int mouseDragStartedX, int mouseDragStartedY, int horizontalOffset);
    void resetWidthOnResizeGL(int width);
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
};