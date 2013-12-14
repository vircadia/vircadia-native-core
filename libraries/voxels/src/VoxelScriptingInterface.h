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

#include <JurisdictionListener.h>
#include <OctreeScriptingInterface.h>
#include "VoxelEditPacketSender.h"

/// handles scripting of voxel commands from JS passed to assigned clients
class VoxelScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    VoxelEditPacketSender* getVoxelPacketSender() { return (VoxelEditPacketSender*)getPacketSender(); }

    virtual NODE_TYPE getServerNodeType() const { return NODE_TYPE_VOXEL_SERVER; }
    virtual OctreeEditPacketSender* createPacketSender() { return new VoxelEditPacketSender(); }

public slots:
    /// queues the creation of a voxel which will be sent by calling process on the PacketSender
    /// \param x the x-coordinate of the voxel (in VS space)
    /// \param y the y-coordinate of the voxel (in VS space)
    /// \param z the z-coordinate of the voxel (in VS space)
    /// \param scale the scale of the voxel (in VS space)
    /// \param red the R value for RGB color of voxel
    /// \param green the G value for RGB color of voxel
    /// \param blue the B value for RGB color of voxel
    void queueVoxelAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);
    
    /// queues the destructive creation of a voxel which will be sent by calling process on the PacketSender
    /// \param x the x-coordinate of the voxel (in VS space)
    /// \param y the y-coordinate of the voxel (in VS space)
    /// \param z the z-coordinate of the voxel (in VS space)
    /// \param scale the scale of the voxel (in VS space)
    /// \param red the R value for RGB color of voxel
    /// \param green the G value for RGB color of voxel
    /// \param blue the B value for RGB color of voxel
    void queueDestructiveVoxelAdd(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);
    
    /// queues the deletion of a voxel, sent by calling process on the PacketSender
    /// \param x the x-coordinate of the voxel (in VS space)
    /// \param y the y-coordinate of the voxel (in VS space)
    /// \param z the z-coordinate of the voxel (in VS space)
    /// \param scale the scale of the voxel (in VS space)
    void queueVoxelDelete(float x, float y, float z, float scale);

private:
    void queueVoxelAdd(PACKET_TYPE addPacketType, VoxelDetail& addVoxelDetails);
};

#endif /* defined(__hifi__VoxelScriptingInterface__) */
