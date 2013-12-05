//
//  VoxelServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServer__
#define __voxel_server__VoxelServer__

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <ThreadedAssignment.h>
#include <EnvironmentData.h>

#include <OctreeServer.h>


#include "VoxelServerConsts.h"

/// Handles assignments of type VoxelServer - sending voxels to various clients.
class VoxelServer : public OctreeServer {
public:                
    VoxelServer(const unsigned char* dataBuffer, int numBytes);
    ~VoxelServer();

    bool wantSendEnvironments() const { return _sendEnvironments; }
    bool getSendMinimalEnvironment() const { return _sendMinimalEnvironment; }
    EnvironmentData* getEnvironmentData(int i) { return &_environmentData[i]; }
    int getEnvironmentDataCount() const { return sizeof(_environmentData)/sizeof(EnvironmentData); }

    // Subclasses must implement these methods    
    virtual OctreeQueryNode* createOctreeQueryNode(Node* newNode);
    virtual Octree* createTree();
    virtual unsigned char getMyNodeType() const { return NODE_TYPE_VOXEL_SERVER; }
    virtual PACKET_TYPE getMyQueryMessageType() const { return PACKET_TYPE_VOXEL_QUERY; }
    virtual const char* getMyServerName() const { return VOXEL_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const { return VOXEL_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const { return LOCAL_VOXELS_PERSIST_FILE; }
    
    // subclass may implement these method
    virtual void beforeRun();
    virtual bool hasSpecialPacketToSend();
    virtual int sendSpecialPacket(Node* node);


private:
    bool _sendEnvironments;
    bool _sendMinimalEnvironment;
    EnvironmentData _environmentData[3];
    unsigned char _tempOutputBuffer[MAX_PACKET_SIZE];
};

#endif // __voxel_server__VoxelServer__
