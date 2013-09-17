//
//  VoxelScriptingInterface.h
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelScriptingInterface__
#define __hifi__VoxelScriptingInterface__

#include <QtCore/QObject>

#include <VoxelEditPacketSender.h>

class VoxelScriptingInterface : public QObject {
    Q_OBJECT
public:
    VoxelEditPacketSender* getVoxelPacketSender() { return &_voxelPacketSender; }
public slots:
    void queueVoxelAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);
private:
    VoxelEditPacketSender _voxelPacketSender;
};

#endif /* defined(__hifi__VoxelScriptingInterface__) */
