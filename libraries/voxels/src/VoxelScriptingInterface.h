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
#include "VoxelEditPacketSender.h"

/// handles scripting of voxel commands from JS passed to assigned clients
class VoxelScriptingInterface : public QObject {
    Q_OBJECT
public:
    VoxelScriptingInterface();
    
    VoxelEditPacketSender* getVoxelPacketSender() { return &_voxelPacketSender; }
    JurisdictionListener* getJurisdictionListener() { return &_jurisdictionListener; }
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

    /// Set the desired max packet size in bytes that should be created
    void setMaxPacketSize(int maxPacketSize) { return _voxelPacketSender.setMaxPacketSize(maxPacketSize); }

    /// returns the current desired max packet size in bytes that will be created
    int getMaxPacketSize() const { return _voxelPacketSender.getMaxPacketSize(); }

    /// set the max packets per second send rate
    void setPacketsPerSecond(int packetsPerSecond) { return _voxelPacketSender.setPacketsPerSecond(packetsPerSecond); }

    /// get the max packets per second send rate
    int getPacketsPerSecond() const  { return _voxelPacketSender.getPacketsPerSecond(); }

    /// does a voxel server exist to send to
    bool voxelServersExist() const { return _voxelPacketSender.voxelServersExist(); }

    /// are there packets waiting in the send queue to be sent
    bool hasPacketsToSend() const { return _voxelPacketSender.hasPacketsToSend(); }

    /// how many packets are there in the send queue waiting to be sent
    int packetsToSendCount() const { return _voxelPacketSender.packetsToSendCount(); }

    /// returns the packets per second send rate of this object over its lifetime
    float getLifetimePPS() const { return _voxelPacketSender.getLifetimePPS(); }

    /// returns the bytes per second send rate of this object over its lifetime
    float getLifetimeBPS() const { return _voxelPacketSender.getLifetimeBPS(); }
    
    /// returns the packets per second queued rate of this object over its lifetime
    float getLifetimePPSQueued() const  { return _voxelPacketSender.getLifetimePPSQueued(); }

    /// returns the bytes per second queued rate of this object over its lifetime
    float getLifetimeBPSQueued() const { return _voxelPacketSender.getLifetimeBPSQueued(); }

    /// returns lifetime of this object from first packet sent to now in usecs
    long long unsigned int getLifetimeInUsecs() const { return _voxelPacketSender.getLifetimeInUsecs(); }

    /// returns lifetime of this object from first packet sent to now in usecs
    float getLifetimeInSeconds() const { return _voxelPacketSender.getLifetimeInSeconds(); }

    /// returns the total packets sent by this object over its lifetime
    long long unsigned int getLifetimePacketsSent() const { return _voxelPacketSender.getLifetimePacketsSent(); }

    /// returns the total bytes sent by this object over its lifetime
    long long unsigned int getLifetimeBytesSent() const { return _voxelPacketSender.getLifetimeBytesSent(); }

    /// returns the total packets queued by this object over its lifetime
    long long unsigned int getLifetimePacketsQueued() const { return _voxelPacketSender.getLifetimePacketsQueued(); }

    /// returns the total bytes queued by this object over its lifetime
    long long unsigned int getLifetimeBytesQueued() const { return _voxelPacketSender.getLifetimeBytesQueued(); }

private:
    /// attached VoxelEditPacketSender that handles queuing and sending of packets to VS
    VoxelEditPacketSender _voxelPacketSender;
    JurisdictionListener _jurisdictionListener;
    
    void queueVoxelAdd(PACKET_TYPE addPacketType, VoxelDetail& addVoxelDetails);
};

#endif /* defined(__hifi__VoxelScriptingInterface__) */
